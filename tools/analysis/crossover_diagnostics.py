#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import math
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any

try:
    import numpy as np
except ImportError:  # pragma: no cover - handled in main.
    np = None  # type: ignore[assignment]

try:
    from tqdm import tqdm
except ImportError:  # pragma: no cover - progress is optional.
    tqdm = None  # type: ignore[assignment]


SCRIPT_DIR = Path(__file__).resolve().parent
TOOLS_DIR = SCRIPT_DIR.parent
DEFAULT_INPUT = TOOLS_DIR / "out" / "experiments"
DEFAULT_ALGORITHM = "algo"
DEFAULT_GENERATION_STEP = 10
DEFAULT_TIME_SCALE_MULTIPLIER = 1.0
SPECIMEN_LIST_KEYS = ("population", "specimens", "paretoFront")


@dataclass(frozen=True)
class GenerationArrays:
    directions: np.ndarray
    throttles: np.ndarray
    init_times: np.ndarray
    end_times: np.ndarray
    durations: np.ndarray
    lengths: np.ndarray


@dataclass(frozen=True)
class GenerationStats:
    path: Path
    generation: int
    source: str
    specimen_count: int
    pair_count: int
    position_means: tuple[float | None, ...]
    position_counts: tuple[int, ...]

    @property
    def overall_mean(self) -> float | None:
        weighted_sum = sum(
            mean * count
            for mean, count in zip(self.position_means, self.position_counts)
            if mean is not None
        )
        total_count = sum(self.position_counts)
        return weighted_sum / total_count if total_count > 0 else None


class NullProgress:
    def update(self, _: int = 1) -> None:
        pass

    def close(self) -> None:
        pass


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Compute per-chromosome-position mean maneuver similarity using the "
            "same alignment and maneuver similarity model as AlignedSimilarityCrossover."
        )
    )
    parser.add_argument(
        "input",
        nargs="?",
        type=Path,
        default=DEFAULT_INPUT,
        help=(
            "Experiment JSON file or a directory with *.json files. "
            f"Default: {DEFAULT_INPUT}"
        ),
    )
    parser.add_argument(
        "--algorithm",
        default=DEFAULT_ALGORITHM,
        help=(
            "When input is a directory, only include files named *_<algorithm>_*.json. "
            "Use 'all' to include all JSON files. Default: algo."
        ),
    )
    parser.add_argument(
        "--generation",
        type=int,
        action="append",
        help=(
            "Generation to include. Can be passed multiple times. "
            "When set, --generation-step is ignored."
        ),
    )
    parser.add_argument(
        "--generation-step",
        type=int,
        default=DEFAULT_GENERATION_STEP,
        help=(
            "Analyze every Nth generation when --generation is not set. "
            f"Default: {DEFAULT_GENERATION_STEP}."
        ),
    )
    parser.add_argument(
        "--time-scale-multiplier",
        type=float,
        default=DEFAULT_TIME_SCALE_MULTIPLIER,
        help=(
            "AlignedSimilarityCrossover timeScaleMultiplier. "
            f"Default: {DEFAULT_TIME_SCALE_MULTIPLIER}."
        ),
    )
    parser.add_argument(
        "--include-self",
        action="store_true",
        help="Also evaluate specimen paired with itself. Default: only distinct pairs.",
    )
    parser.add_argument(
        "--no-progress",
        action="store_true",
        help="Disable tqdm progress output.",
    )
    return parser.parse_args()


def discover_input_files(input_path: Path, algorithm: str) -> list[Path]:
    if input_path.is_file():
        return [input_path]

    if input_path.is_dir():
        pattern = "*.json" if algorithm == "all" else f"*_{algorithm}_*.json"
        files = sorted(path for path in input_path.glob(pattern) if path.is_file())
        if files:
            return files
        raise ValueError(f"no matching JSON files found in {input_path}")

    raise ValueError(f"input path does not exist: {input_path}")


def clamp01(values: np.ndarray) -> np.ndarray:
    return np.clip(values, 0.0, 1.0)


def direction_similarity(lhs: np.ndarray, rhs: np.ndarray) -> np.ndarray:
    lhs_lengths = np.linalg.norm(lhs, axis=-1)
    rhs_lengths = np.linalg.norm(rhs, axis=-1)
    both_zero = (lhs_lengths <= 0.0) & (rhs_lengths <= 0.0)
    one_zero = (lhs_lengths <= 0.0) | (rhs_lengths <= 0.0)
    valid = ~one_zero

    similarity = np.zeros(lhs_lengths.shape, dtype=np.float64)
    similarity[both_zero] = 1.0

    if np.any(valid):
        dot_products = np.sum(lhs[valid] * rhs[valid], axis=-1)
        cosines = np.clip(dot_products / (lhs_lengths[valid] * rhs_lengths[valid]), -1.0, 1.0)
        similarity[valid] = (cosines + 1.0) * 0.5

    return similarity


def maneuver_similarity(
    lhs_directions: np.ndarray,
    rhs_directions: np.ndarray,
    lhs_throttles: np.ndarray,
    rhs_throttles: np.ndarray,
    lhs_init_times: np.ndarray,
    rhs_init_times: np.ndarray,
    lhs_end_times: np.ndarray,
    rhs_end_times: np.ndarray,
    lhs_durations: np.ndarray,
    rhs_durations: np.ndarray,
    time_scale_multiplier: float,
) -> np.ndarray:
    throttle_similarity = 1.0 - np.abs(clamp01(lhs_throttles) - clamp01(rhs_throttles))
    directions = direction_similarity(lhs_directions, rhs_directions)
    time_scale = np.maximum.reduce((np.ones_like(lhs_durations), lhs_durations, rhs_durations)) * time_scale_multiplier
    init_similarity = 1.0 / (1.0 + np.abs(lhs_init_times - rhs_init_times) / time_scale)
    end_similarity = 1.0 / (1.0 + np.abs(lhs_end_times - rhs_end_times) / time_scale)

    return throttle_similarity * directions * init_similarity * end_similarity


def log_similarity_sum(similarities: np.ndarray) -> np.ndarray:
    with np.errstate(divide="ignore"):
        logs = np.where(similarities > 0.0, np.log(similarities), -np.inf)
    return np.sum(logs, axis=1)


def generation_number(generation: dict[str, Any], fallback: int) -> int:
    try:
        return int(generation.get("generation", fallback))
    except (TypeError, ValueError):
        return fallback


def should_analyze_generation(generation_id: int, requested_generations: set[int] | None, generation_step: int) -> bool:
    if requested_generations is not None:
        return generation_id in requested_generations
    return generation_id % generation_step == 0


def specimen_maneuvers(specimen: Any) -> list[dict[str, Any]] | None:
    if not isinstance(specimen, dict):
        return None

    maneuvers = specimen.get("maneuvers")
    if not isinstance(maneuvers, list):
        return None

    return [maneuver for maneuver in maneuvers if isinstance(maneuver, dict)]


def generation_specimens(generation: dict[str, Any]) -> tuple[str, list[list[dict[str, Any]]]]:
    for key in SPECIMEN_LIST_KEYS:
        specimens = generation.get(key)
        if not isinstance(specimens, list):
            continue

        maneuvers = [
            specimen
            for specimen in (specimen_maneuvers(item) for item in specimens)
            if specimen is not None
        ]
        return key, maneuvers

    direct_specimen = specimen_maneuvers(generation)
    if direct_specimen is not None:
        return "generation.maneuvers", [direct_specimen]

    return "none", []


def maneuver_direction(maneuver: dict[str, Any]) -> tuple[float, float, float]:
    direction = maneuver.get("thrustDirection", {})
    if not isinstance(direction, dict):
        return 0.0, 0.0, 0.0

    return (
        float(direction.get("x", 0.0)),
        float(direction.get("y", 0.0)),
        float(direction.get("z", 0.0)),
    )


def build_generation_arrays(specimens: list[list[dict[str, Any]]]) -> GenerationArrays:
    specimen_count = len(specimens)
    lengths = np.array([len(specimen) for specimen in specimens], dtype=np.int64)
    max_length = int(np.max(lengths)) if specimen_count > 0 else 0

    directions = np.zeros((specimen_count, max_length, 3), dtype=np.float64)
    throttles = np.zeros((specimen_count, max_length), dtype=np.float64)
    init_times = np.zeros((specimen_count, max_length), dtype=np.float64)
    end_times = np.zeros((specimen_count, max_length), dtype=np.float64)
    durations = np.zeros((specimen_count, max_length), dtype=np.float64)

    for specimen_index, specimen in enumerate(specimens):
        previous_end_time = 0.0
        for maneuver_index, maneuver in enumerate(specimen):
            init_delay = float(maneuver.get("initDelay", 0.0))
            duration = float(maneuver.get("duration", 0.0))
            init_time = previous_end_time + init_delay
            end_time = init_time + duration

            directions[specimen_index, maneuver_index] = maneuver_direction(maneuver)
            throttles[specimen_index, maneuver_index] = float(maneuver.get("throttleValue", 0.0))
            init_times[specimen_index, maneuver_index] = init_time
            end_times[specimen_index, maneuver_index] = end_time
            durations[specimen_index, maneuver_index] = duration
            previous_end_time = end_time

    return GenerationArrays(
        directions=directions,
        throttles=throttles,
        init_times=init_times,
        end_times=end_times,
        durations=durations,
        lengths=lengths,
    )


def pair_indices(specimen_count: int, include_self: bool) -> tuple[np.ndarray, np.ndarray]:
    diagonal_offset = 0 if include_self else 1
    return np.triu_indices(specimen_count, k=diagonal_offset)


def similarity_for_pairs(
    arrays: GenerationArrays,
    lhs_indices: np.ndarray,
    rhs_indices: np.ndarray,
    lhs_begin: int,
    rhs_begin: int,
    length: int,
    time_scale_multiplier: float,
) -> np.ndarray:
    lhs_slice = slice(lhs_begin, lhs_begin + length)
    rhs_slice = slice(rhs_begin, rhs_begin + length)

    return maneuver_similarity(
        arrays.directions[lhs_indices, lhs_slice],
        arrays.directions[rhs_indices, rhs_slice],
        arrays.throttles[lhs_indices, lhs_slice],
        arrays.throttles[rhs_indices, rhs_slice],
        arrays.init_times[lhs_indices, lhs_slice],
        arrays.init_times[rhs_indices, rhs_slice],
        arrays.end_times[lhs_indices, lhs_slice],
        arrays.end_times[rhs_indices, rhs_slice],
        arrays.durations[lhs_indices, lhs_slice],
        arrays.durations[rhs_indices, rhs_slice],
        time_scale_multiplier,
    )


def add_position_similarities(
    position_sums: np.ndarray,
    position_counts: np.ndarray,
    similarities: np.ndarray,
) -> None:
    if similarities.size == 0:
        return

    length = similarities.shape[1]
    position_sums[:length] += np.sum(similarities, axis=0)
    position_counts[:length] += similarities.shape[0]


def analyze_pair_group(
    arrays: GenerationArrays,
    shorter_indices: np.ndarray,
    longer_indices: np.ndarray,
    shorter_length: int,
    longer_length: int,
    time_scale_multiplier: float,
    position_sums: np.ndarray,
    position_counts: np.ndarray,
) -> None:
    if shorter_length == 0 or shorter_indices.size == 0:
        return

    if shorter_length == longer_length:
        similarities = similarity_for_pairs(
            arrays,
            shorter_indices,
            longer_indices,
            lhs_begin=0,
            rhs_begin=0,
            length=shorter_length,
            time_scale_multiplier=time_scale_multiplier,
        )
        add_position_similarities(position_sums, position_counts, similarities)
        return

    best_begins = np.zeros(shorter_indices.shape, dtype=np.int64)
    best_scores = np.full(shorter_indices.shape, -np.inf, dtype=np.float64)

    for longer_begin in range(longer_length - shorter_length + 1):
        similarities = similarity_for_pairs(
            arrays,
            shorter_indices,
            longer_indices,
            lhs_begin=0,
            rhs_begin=longer_begin,
            length=shorter_length,
            time_scale_multiplier=time_scale_multiplier,
        )
        scores = log_similarity_sum(similarities)
        improved = scores > best_scores
        best_scores[improved] = scores[improved]
        best_begins[improved] = longer_begin

    for longer_begin in np.unique(best_begins):
        selected = best_begins == longer_begin
        similarities = similarity_for_pairs(
            arrays,
            shorter_indices[selected],
            longer_indices[selected],
            lhs_begin=0,
            rhs_begin=int(longer_begin),
            length=shorter_length,
            time_scale_multiplier=time_scale_multiplier,
        )
        add_position_similarities(position_sums, position_counts, similarities)


def position_similarity_stats(
    specimens: list[list[dict[str, Any]]],
    time_scale_multiplier: float,
    include_self: bool,
) -> tuple[int, tuple[float | None, ...], tuple[int, ...]]:
    arrays = build_generation_arrays(specimens)
    specimen_count = len(specimens)
    max_length = int(np.max(arrays.lengths)) if specimen_count > 0 else 0
    left_indices, right_indices = pair_indices(specimen_count, include_self)
    pair_count = int(left_indices.size)

    position_sums = np.zeros(max_length, dtype=np.float64)
    position_counts = np.zeros(max_length, dtype=np.int64)

    if pair_count == 0:
        return 0, tuple(None for _ in range(max_length)), tuple(0 for _ in range(max_length))

    left_lengths = arrays.lengths[left_indices]
    right_lengths = arrays.lengths[right_indices]
    left_is_shorter = left_lengths <= right_lengths
    shorter_indices = np.where(left_is_shorter, left_indices, right_indices)
    longer_indices = np.where(left_is_shorter, right_indices, left_indices)
    shorter_lengths = np.minimum(left_lengths, right_lengths)
    longer_lengths = np.maximum(left_lengths, right_lengths)

    group_keys = np.stack((shorter_lengths, longer_lengths), axis=1)
    for shorter_length, longer_length in np.unique(group_keys, axis=0):
        selected = (shorter_lengths == shorter_length) & (longer_lengths == longer_length)
        analyze_pair_group(
            arrays=arrays,
            shorter_indices=shorter_indices[selected],
            longer_indices=longer_indices[selected],
            shorter_length=int(shorter_length),
            longer_length=int(longer_length),
            time_scale_multiplier=time_scale_multiplier,
            position_sums=position_sums,
            position_counts=position_counts,
        )

    with np.errstate(divide="ignore", invalid="ignore"):
        means = np.divide(
            position_sums,
            position_counts,
            out=np.full_like(position_sums, np.nan, dtype=np.float64),
            where=position_counts > 0,
        )

    return (
        pair_count,
        tuple(None if math.isnan(value) else float(value) for value in means),
        tuple(int(count) for count in position_counts),
    )


def json_generations(data: dict[str, Any], path: Path) -> list[Any]:
    generations = data.get("generations")
    if isinstance(generations, list):
        return generations

    pareto_front_history = data.get("paretoFrontHistory")
    if isinstance(pareto_front_history, list):
        return pareto_front_history

    raise ValueError(f"missing generations list in {path}")


def analyze_file(
    path: Path,
    requested_generations: set[int] | None,
    generation_step: int,
    time_scale_multiplier: float,
    include_self: bool,
    progress: Any,
) -> list[GenerationStats]:
    with path.open("r", encoding="utf-8") as input_file:
        data = json.load(input_file)

    generations = json_generations(data, path)
    results: list[GenerationStats] = []

    for fallback_generation, generation in enumerate(generations):
        if not isinstance(generation, dict):
            continue

        generation_id = generation_number(generation, fallback_generation)
        if not should_analyze_generation(generation_id, requested_generations, generation_step):
            continue

        source, specimens = generation_specimens(generation)
        pair_count, position_means, position_counts = position_similarity_stats(
            specimens=specimens,
            time_scale_multiplier=time_scale_multiplier,
            include_self=include_self,
        )
        if pair_count == 0:
            continue

        results.append(
            GenerationStats(
                path=path,
                generation=generation_id,
                source=source,
                specimen_count=len(specimens),
                pair_count=pair_count,
                position_means=position_means,
                position_counts=position_counts,
            )
        )
        progress.update(1)

    return results


def make_progress(disabled: bool) -> Any:
    if disabled or tqdm is None:
        return NullProgress()
    return tqdm(desc="Analyzing generations", unit="generation")


def format_optional_float(value: float | None) -> str:
    if value is None:
        return "n/a"
    return f"{value:.6g}"


def print_stats(stats: list[GenerationStats]) -> None:
    max_position_count = max(len(stat.position_means) for stat in stats)
    header = (
        "file",
        "generation",
        "source",
        "specimens",
        "pairs",
        "overall_mean",
        *(f"pos_{position}" for position in range(max_position_count)),
    )
    rows = []

    for stat in stats:
        position_values = [
            format_optional_float(stat.position_means[position])
            if position < len(stat.position_means)
            else "n/a"
            for position in range(max_position_count)
        ]
        rows.append(
            (
                stat.path.name,
                str(stat.generation),
                stat.source,
                str(stat.specimen_count),
                str(stat.pair_count),
                format_optional_float(stat.overall_mean),
                *position_values,
            )
        )

    widths = [max(len(row[column]) for row in (header, *rows)) for column in range(len(header))]

    print("  ".join(value.ljust(widths[index]) for index, value in enumerate(header)))
    print("  ".join("-" * width for width in widths))
    for row in rows:
        print("  ".join(value.ljust(widths[index]) for index, value in enumerate(row)))


def print_total(stats: list[GenerationStats]) -> None:
    total_pairs = sum(stat.pair_count for stat in stats)
    total_position_counts = sum(sum(stat.position_counts) for stat in stats)
    total_weighted_similarity = sum(
        mean * count
        for stat in stats
        for mean, count in zip(stat.position_means, stat.position_counts)
        if mean is not None
    )
    overall_mean = (
        total_weighted_similarity / total_position_counts
        if total_position_counts > 0
        else None
    )

    print()
    print(f"generations: {len(stats)}")
    print(f"pairs: {total_pairs}")
    print(f"position comparisons: {total_position_counts}")
    print(f"overall mean similarity: {format_optional_float(overall_mean)}")


def main() -> int:
    if np is None:
        print("error: numpy is required for this analysis", file=sys.stderr)
        return 2

    args = parse_args()

    if args.generation_step <= 0:
        print("error: --generation-step must be greater than zero", file=sys.stderr)
        return 2

    if args.time_scale_multiplier <= 0.0:
        print("error: --time-scale-multiplier must be greater than zero", file=sys.stderr)
        return 2

    requested_generations = set(args.generation) if args.generation is not None else None
    progress = make_progress(args.no_progress)

    try:
        paths = discover_input_files(args.input, args.algorithm)
        stats = [
            stat
            for path in paths
            for stat in analyze_file(
                path=path,
                requested_generations=requested_generations,
                generation_step=args.generation_step,
                time_scale_multiplier=args.time_scale_multiplier,
                include_self=args.include_self,
                progress=progress,
            )
        ]
    except (OSError, json.JSONDecodeError, ValueError) as error:
        progress.close()
        print(f"error: {error}", file=sys.stderr)
        return 2

    progress.close()

    if not stats:
        print("error: no matching generations found", file=sys.stderr)
        return 2

    print_stats(stats)
    print_total(stats)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
