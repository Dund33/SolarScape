#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import math
import statistics
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Iterable


SCRIPT_DIR = Path(__file__).resolve().parent
TOOLS_DIR = SCRIPT_DIR.parent
DEFAULT_INPUT = TOOLS_DIR / "out" / "experiments"
DEFAULT_MIN_REGION_SIMILARITY = 0.01
DEFAULT_TIME_SCALE_MULTIPLIER = 1.0
SPECIMEN_LIST_KEYS = ("population", "specimens", "paretoFront")


@dataclass(frozen=True)
class ManeuverTime:
    init_time: float
    end_time: float


@dataclass(frozen=True)
class GenerationStats:
    path: Path
    generation: int
    source: str
    specimen_count: int
    pair_count: int
    lengths: tuple[float, ...]
    shorter_lengths: tuple[int, ...]
    mean: float | None
    minimum: float | None
    maximum: float | None
    stddev: float | None
    shorter_mean: float | None
    shorter_minimum: float | None
    shorter_maximum: float | None
    shorter_stddev: float | None


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Compute normalized AlignedSimilarityCrossover exchange-region length "
            "statistics for all pairwise crosses in experiment JSON generations."
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
        "--generation",
        type=int,
        action="append",
        help="Generation to include. Can be passed multiple times. Default: all.",
    )
    parser.add_argument(
        "--min-region-similarity",
        type=float,
        default=DEFAULT_MIN_REGION_SIMILARITY,
        help=(
            "AlignedSimilarityCrossover minRegionSimilarity. "
            f"Default: {DEFAULT_MIN_REGION_SIMILARITY}."
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
        "--sample-stddev",
        action="store_true",
        help="Use sample standard deviation. Default: population standard deviation.",
    )
    return parser.parse_args()


def discover_input_files(input_path: Path) -> list[Path]:
    if input_path.is_file():
        return [input_path]

    if input_path.is_dir():
        files = sorted(path for path in input_path.glob("*.json") if path.is_file())
        if files:
            return files
        raise ValueError(f"no JSON files found in {input_path}")

    raise ValueError(f"input path does not exist: {input_path}")


def clamp(value: float, minimum: float, maximum: float) -> float:
    return max(minimum, min(maximum, value))


def absolute_maneuver_times(maneuvers: list[dict[str, Any]]) -> list[ManeuverTime]:
    times: list[ManeuverTime] = []
    previous_end_time = 0.0

    for maneuver in maneuvers:
        init_time = previous_end_time + float(maneuver.get("initDelay", 0.0))
        end_time = init_time + float(maneuver.get("duration", 0.0))
        times.append(ManeuverTime(init_time=init_time, end_time=end_time))
        previous_end_time = end_time

    return times


def direction_vector(maneuver: dict[str, Any]) -> tuple[float, float, float]:
    direction = maneuver.get("thrustDirection", {})
    if not isinstance(direction, dict):
        return 0.0, 0.0, 0.0

    return (
        float(direction.get("x", 0.0)),
        float(direction.get("y", 0.0)),
        float(direction.get("z", 0.0)),
    )


def direction_similarity(lhs: dict[str, Any], rhs: dict[str, Any]) -> float:
    lhs_x, lhs_y, lhs_z = direction_vector(lhs)
    rhs_x, rhs_y, rhs_z = direction_vector(rhs)

    lhs_length = math.sqrt(lhs_x * lhs_x + lhs_y * lhs_y + lhs_z * lhs_z)
    rhs_length = math.sqrt(rhs_x * rhs_x + rhs_y * rhs_y + rhs_z * rhs_z)

    if lhs_length <= 0.0 and rhs_length <= 0.0:
        return 1.0
    if lhs_length <= 0.0 or rhs_length <= 0.0:
        return 0.0

    cosine = clamp(
        (lhs_x * rhs_x + lhs_y * rhs_y + lhs_z * rhs_z)
        / (lhs_length * rhs_length),
        -1.0,
        1.0,
    )
    return (cosine + 1.0) * 0.5


def log_similarity(similarity: float) -> float:
    if similarity <= 0.0:
        return -math.inf
    return math.log(similarity)


def log_time_similarity(lhs: float, rhs: float, scale: float) -> float:
    return -math.log1p(abs(lhs - rhs) / scale)


def maneuver_log_similarity(
    lhs: dict[str, Any],
    rhs: dict[str, Any],
    lhs_time: ManeuverTime,
    rhs_time: ManeuverTime,
    time_scale_multiplier: float,
) -> float:
    throttle_similarity = 1.0 - abs(
        clamp(float(lhs.get("throttleValue", 0.0)), 0.0, 1.0)
        - clamp(float(rhs.get("throttleValue", 0.0)), 0.0, 1.0)
    )
    time_scale = (
        max(
            1.0,
            float(lhs.get("duration", 0.0)),
            float(rhs.get("duration", 0.0)),
        )
        * time_scale_multiplier
    )

    return (
        log_similarity(throttle_similarity)
        + log_similarity(direction_similarity(lhs, rhs))
        + log_time_similarity(lhs_time.init_time, rhs_time.init_time, time_scale)
        + log_time_similarity(lhs_time.end_time, rhs_time.end_time, time_scale)
    )


def alignment_log_similarity_sum(
    shorter: list[dict[str, Any]],
    longer: list[dict[str, Any]],
    shorter_times: list[ManeuverTime],
    longer_times: list[ManeuverTime],
    longer_begin: int,
    time_scale_multiplier: float,
) -> float:
    return sum(
        maneuver_log_similarity(
            shorter[index],
            longer[longer_begin + index],
            shorter_times[index],
            longer_times[longer_begin + index],
            time_scale_multiplier,
        )
        for index in range(len(shorter))
    )


def best_alignment_begin(
    shorter: list[dict[str, Any]],
    longer: list[dict[str, Any]],
    shorter_times: list[ManeuverTime],
    longer_times: list[ManeuverTime],
    time_scale_multiplier: float,
) -> int:
    best_begin = 0
    best_score = -math.inf

    for longer_begin in range(len(longer) - len(shorter) + 1):
        score = alignment_log_similarity_sum(
            shorter,
            longer,
            shorter_times,
            longer_times,
            longer_begin,
            time_scale_multiplier,
        )
        if score > best_score:
            best_begin = longer_begin
            best_score = score

    return best_begin


def exchange_region_length(
    parent1: list[dict[str, Any]],
    parent2: list[dict[str, Any]],
    min_region_log_similarity: float,
    time_scale_multiplier: float,
) -> int:
    if not parent1 or not parent2:
        return 0

    parent1_times = absolute_maneuver_times(parent1)
    parent2_times = absolute_maneuver_times(parent2)

    if len(parent1) <= len(parent2):
        shorter = parent1
        longer = parent2
        shorter_times = parent1_times
        longer_times = parent2_times
    else:
        shorter = parent2
        longer = parent1
        shorter_times = parent2_times
        longer_times = parent1_times

    longer_begin = best_alignment_begin(
        shorter,
        longer,
        shorter_times,
        longer_times,
        time_scale_multiplier,
    )

    cumulative_log_similarity = 0.0
    length = 0
    for index in range(len(shorter)):
        next_log_similarity = cumulative_log_similarity + maneuver_log_similarity(
            shorter[index],
            longer[longer_begin + index],
            shorter_times[index],
            longer_times[longer_begin + index],
            time_scale_multiplier,
        )
        if next_log_similarity < min_region_log_similarity:
            break

        cumulative_log_similarity = next_log_similarity
        length += 1

    return length


def generation_number(generation: dict[str, Any], fallback: int) -> int:
    try:
        return int(generation.get("generation", fallback))
    except (TypeError, ValueError):
        return fallback


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


def pair_metrics(
    specimens: list[list[dict[str, Any]]],
    min_region_log_similarity: float,
    time_scale_multiplier: float,
    include_self: bool,
) -> Iterable[tuple[float, int]]:
    start_offset = 0 if include_self else 1

    for left_index, left in enumerate(specimens):
        for right_index in range(left_index + start_offset, len(specimens)):
            right = specimens[right_index]
            shorter_length = min(len(left), len(right))
            if shorter_length == 0:
                yield 0.0, 0
                continue

            normalized_length = exchange_region_length(
                left,
                right,
                min_region_log_similarity,
                time_scale_multiplier,
            ) / shorter_length
            yield normalized_length, shorter_length


def summarize_lengths(
    lengths: list[float] | tuple[float, ...],
    sample_stddev: bool,
) -> tuple[float | None, float | None, float | None, float | None]:
    if not lengths:
        return None, None, None, None

    if sample_stddev:
        stddev = statistics.stdev(lengths) if len(lengths) > 1 else 0.0
    else:
        stddev = statistics.pstdev(lengths)

    return (
        statistics.fmean(lengths),
        min(lengths),
        max(lengths),
        stddev,
    )


def analyze_file(
    path: Path,
    requested_generations: set[int] | None,
    min_region_log_similarity: float,
    time_scale_multiplier: float,
    include_self: bool,
    sample_stddev: bool,
) -> list[GenerationStats]:
    with path.open("r", encoding="utf-8") as input_file:
        data = json.load(input_file)

    generations = data.get("generations")
    if not isinstance(generations, list):
        raise ValueError(f"missing generations list in {path}")

    results: list[GenerationStats] = []
    for fallback_generation, generation in enumerate(generations):
        if not isinstance(generation, dict):
            continue

        generation_id = generation_number(generation, fallback_generation)
        if requested_generations is not None and generation_id not in requested_generations:
            continue

        source, specimens = generation_specimens(generation)
        metrics = tuple(
            pair_metrics(
                specimens,
                min_region_log_similarity,
                time_scale_multiplier,
                include_self,
            )
        )
        lengths = tuple(length for length, _ in metrics)
        shorter_lengths = tuple(shorter_length for _, shorter_length in metrics)
        mean, minimum, maximum, stddev = summarize_lengths(lengths, sample_stddev)
        (
            shorter_mean,
            shorter_minimum,
            shorter_maximum,
            shorter_stddev,
        ) = summarize_lengths(shorter_lengths, sample_stddev)
        results.append(
            GenerationStats(
                path=path,
                generation=generation_id,
                source=source,
                specimen_count=len(specimens),
                pair_count=len(lengths),
                lengths=lengths,
                shorter_lengths=shorter_lengths,
                mean=mean,
                minimum=minimum,
                maximum=maximum,
                stddev=stddev,
                shorter_mean=shorter_mean,
                shorter_minimum=shorter_minimum,
                shorter_maximum=shorter_maximum,
                shorter_stddev=shorter_stddev,
            )
        )

    return results


def format_optional_float(value: float | None) -> str:
    if value is None:
        return "n/a"
    return f"{value:.6g}"


def print_stats(stats: list[GenerationStats]) -> None:
    header = (
        "file",
        "generation",
        "source",
        "specimens",
        "pairs",
        "mean",
        "min",
        "max",
        "stddev",
        "shorter_mean",
        "shorter_min",
        "shorter_max",
        "shorter_stddev",
    )
    rows = [
        (
            stat.path.name,
            str(stat.generation),
            stat.source,
            str(stat.specimen_count),
            str(stat.pair_count),
            format_optional_float(stat.mean),
            format_optional_float(stat.minimum),
            format_optional_float(stat.maximum),
            format_optional_float(stat.stddev),
            format_optional_float(stat.shorter_mean),
            format_optional_float(stat.shorter_minimum),
            format_optional_float(stat.shorter_maximum),
            format_optional_float(stat.shorter_stddev),
        )
        for stat in stats
    ]

    widths = [
        max(len(row[column]) for row in (header, *rows))
        for column in range(len(header))
    ]

    print("  ".join(value.ljust(widths[index]) for index, value in enumerate(header)))
    print("  ".join("-" * width for width in widths))
    for row in rows:
        print("  ".join(value.ljust(widths[index]) for index, value in enumerate(row)))


def print_total(stats: list[GenerationStats], sample_stddev: bool) -> None:
    lengths = [length for stat in stats for length in stat.lengths]
    shorter_lengths = [
        shorter_length
        for stat in stats
        for shorter_length in stat.shorter_lengths
    ]
    mean, minimum, maximum, stddev = summarize_lengths(lengths, sample_stddev)
    (
        shorter_mean,
        shorter_minimum,
        shorter_maximum,
        shorter_stddev,
    ) = summarize_lengths(shorter_lengths, sample_stddev)
    total_generations = len(stats)
    total_pairs = sum(stat.pair_count for stat in stats)
    usable_generations = sum(1 for stat in stats if stat.pair_count > 0)
    stddev_kind = "sample" if sample_stddev else "population"

    print()
    print(f"generations: {total_generations}")
    print(f"generations with pairs: {usable_generations}")
    print(f"pairs: {total_pairs}")
    print(f"overall mean: {format_optional_float(mean)}")
    print(f"overall min: {format_optional_float(minimum)}")
    print(f"overall max: {format_optional_float(maximum)}")
    print(f"overall stddev: {format_optional_float(stddev)}")
    print(f"overall shorter mean: {format_optional_float(shorter_mean)}")
    print(f"overall shorter min: {format_optional_float(shorter_minimum)}")
    print(f"overall shorter max: {format_optional_float(shorter_maximum)}")
    print(f"overall shorter stddev: {format_optional_float(shorter_stddev)}")
    print(f"stddev: {stddev_kind}")


def main() -> int:
    args = parse_args()

    if args.min_region_similarity <= 0.0 or args.min_region_similarity > 1.0:
        print("error: --min-region-similarity must be in range (0, 1]", file=sys.stderr)
        return 2

    if args.time_scale_multiplier <= 0.0:
        print("error: --time-scale-multiplier must be greater than zero", file=sys.stderr)
        return 2

    requested_generations = set(args.generation) if args.generation is not None else None
    min_region_log_similarity = math.log(args.min_region_similarity)

    try:
        paths = discover_input_files(args.input)
        stats = [
            stat
            for path in paths
            for stat in analyze_file(
                path=path,
                requested_generations=requested_generations,
                min_region_log_similarity=min_region_log_similarity,
                time_scale_multiplier=args.time_scale_multiplier,
                include_self=args.include_self,
                sample_stddev=args.sample_stddev,
            )
        ]
    except (OSError, json.JSONDecodeError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2

    if not stats:
        print("error: no matching generations found", file=sys.stderr)
        return 2

    print_stats(stats)
    print_total(stats, args.sample_stddev)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
