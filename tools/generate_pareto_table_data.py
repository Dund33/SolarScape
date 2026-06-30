#!/usr/bin/env python3

from __future__ import annotations

import argparse
import csv
import math
import sys
from dataclasses import dataclass
from pathlib import Path

from solarscape_tools.experiments import (
    ALGORITHM_ORDER,
    ExperimentFile,
    algorithm_label,
    discover_experiment_files,
    experiment_sort_key,
    group_experiment_files,
    normalize_filter,
    scenario_sort_key,
    validate_run_groups,
)
from solarscape_tools.pareto import (
    feasible_fitnesses,
    final_pareto_front,
    fitnesses_from_front,
    fuel_violation,
    load_pareto_json,
    numeric_value,
)


SCRIPT_DIR = Path(__file__).resolve().parent
DEFAULT_INPUT_DIR = SCRIPT_DIR / "out" / "experiments"
DEFAULT_OUTPUT_PATH = SCRIPT_DIR / "out" / "pareto_front_summary.csv"


@dataclass(frozen=True)
class RunSummary:
    scenario: str
    algorithm: str
    run: int
    final_front_size: float
    final_feasible: float
    final_feasible_rate: float
    final_min_distance: float
    final_best_fuel: float
    final_best_time: float
    best_so_far_distance: float
    best_so_far_generation: float
    time_at_best_distance: float
    fuel_at_best_distance: float
    best_so_far_fuel: float
    earliest_time: float


@dataclass(frozen=True)
class AggregatedSummary:
    scenario: str
    algorithm: str
    run_count: int
    aggregation: str
    selected_run: int | None
    final_front_size: float
    final_feasible: float
    final_feasible_rate: float
    final_min_distance: float
    final_best_fuel: float
    final_best_time: float
    best_so_far_distance: float
    best_so_far_distance_std: float
    best_so_far_distance_best: float
    best_so_far_generation: float
    time_at_best_distance: float
    fuel_at_best_distance: float
    best_so_far_fuel: float
    earliest_time: float


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Generate thesis-ready CSV/Markdown data summarizing SolarScape "
            "trajectory optimization experiments."
        )
    )
    parser.add_argument(
        "-i",
        "--input-dir",
        type=Path,
        default=DEFAULT_INPUT_DIR,
        help=f"Directory with scenario_algorithm_runXX.json files. Default: {DEFAULT_INPUT_DIR}",
    )
    parser.add_argument(
        "--scenario",
        action="append",
        help=(
            "Scenario to include, for example scenario1 or 1. "
            "Can be passed multiple times."
        ),
    )
    parser.add_argument(
        "--algorithm",
        action="append",
        help=(
            "Algorithm to include, for example algo, nsgaii, or moead. "
            "Can be passed multiple times."
        ),
    )
    parser.add_argument(
        "--aggregation",
        choices=("mean", "best", "best-run"),
        default="mean",
        help=(
            "How to combine multiple runs. mean reports average metrics and "
            "sample std for best-so-far distance; best uses the best observed "
            "value per metric; best-run reports the run with the smallest "
            "best-so-far distance. Default: mean."
        ),
    )
    parser.add_argument(
        "--constraint-mode",
        choices=("feasible-only", "prefer-feasible", "ignore"),
        default="feasible-only",
        help=(
            "How objective columns handle infeasible specimens. feasible-only "
            "uses only specimens with fuelConstraintViolation <= tolerance; "
            "prefer-feasible falls back to the least violating specimen when "
            "none are feasible; ignore uses all specimens. Default: feasible-only."
        ),
    )
    parser.add_argument(
        "--fuel-tolerance",
        type=float,
        default=0.0,
        help="Maximum fuelConstraintViolation treated as feasible. Default: 0.",
    )
    parser.add_argument(
        "--expected-runs",
        type=int,
        default=5,
        help="Minimum run count expected in each scenario/algorithm group. Default: 5.",
    )
    parser.add_argument(
        "--allow-incomplete",
        action="store_true",
        help="Generate output even when a group has fewer than --expected-runs files.",
    )
    parser.add_argument(
        "-o",
        "--output",
        type=Path,
        default=DEFAULT_OUTPUT_PATH,
        help=f"Output data file. Default: {DEFAULT_OUTPUT_PATH}",
    )
    parser.add_argument(
        "--format",
        choices=("csv", "markdown"),
        default="csv",
        help="Output data format. Default: csv.",
    )
    parser.add_argument(
        "--precision",
        type=int,
        default=3,
        help="Decimal/significant precision for objective values. Default: 3.",
    )
    return parser.parse_args()


def normalize_scenario_filter(values: list[str] | None) -> set[str] | None:
    normalized = normalize_filter(values)
    if normalized is None:
        return None

    return {
        value if value.startswith("scenario") else f"scenario{value}"
        for value in normalized
    }


def generation_fronts(data: dict, path: Path) -> list[tuple[int, list[dict]]]:
    generations = data.get("generations")
    if isinstance(generations, list):
        result = []
        for fallback_generation, generation in enumerate(generations):
            if not isinstance(generation, dict):
                continue
            generation_number = int(
                generation.get("generation", fallback_generation))
            pareto_front = generation.get("paretoFront", [])
            if isinstance(pareto_front, list):
                result.append((generation_number, pareto_front))

        if not result:
            raise ValueError(f"Missing generation Pareto fronts in {path}")

        return result

    return [(0, final_pareto_front(data, path))]


def candidate_fitnesses(
    fitnesses: list[dict],
    constraint_mode: str,
    fuel_tolerance: float,
) -> list[dict]:
    if constraint_mode == "ignore":
        return fitnesses

    feasible = feasible_fitnesses(fitnesses, fuel_tolerance)
    if feasible:
        return feasible

    if constraint_mode == "feasible-only":
        return []

    if constraint_mode != "prefer-feasible":
        raise ValueError(f"Unsupported constraint mode: {constraint_mode}")

    finite_violations = [
        fuel_violation(fitness)
        for fitness in fitnesses
        if math.isfinite(fuel_violation(fitness))
    ]
    if not finite_violations:
        return []

    minimum_violation = min(finite_violations)
    return [
        fitness
        for fitness in fitnesses
        if fuel_violation(fitness) == minimum_violation
    ]


def best_distance_fitness(fitnesses: list[dict]) -> dict | None:
    candidates = [
        fitness
        for fitness in fitnesses
        if numeric_value(fitness, "minimumDistance") is not None
    ]
    if not candidates:
        return None

    return min(
        candidates,
        key=lambda fitness: (
            numeric_or_default(fitness, "minimumDistance", math.inf),
            numeric_or_default(fitness, "minimumDistanceTime", math.inf),
            -numeric_or_default(
                fitness,
                "minimumDistanceFuelMass",
                -math.inf),
            fuel_violation(fitness),
        ),
    )


def numeric_or_default(
    fitness: dict,
    key: str,
    default: float,
) -> float:
    value = numeric_value(fitness, key)
    return default if value is None else value


def numeric_or_nan(
    fitness: dict,
    key: str,
) -> float:
    value = numeric_value(fitness, key)
    return math.nan if value is None else value


def best_fuel_value(fitnesses: list[dict]) -> float:
    values = [
        numeric_value(fitness, "minimumDistanceFuelMass")
        for fitness in fitnesses
    ]
    finite = [
        value
        for value in values
        if value is not None
    ]
    if not finite:
        return math.nan
    return max(finite)


def earliest_time_value(fitnesses: list[dict]) -> float:
    values = [
        numeric_value(fitness, "minimumDistanceTime")
        for fitness in fitnesses
    ]
    finite = [
        value
        for value in values
        if value is not None
    ]
    if not finite:
        return math.nan
    return min(finite)


def summarize_experiment(
    experiment: ExperimentFile,
    constraint_mode: str,
    fuel_tolerance: float,
) -> RunSummary:
    data = load_pareto_json(experiment.path)
    fronts = generation_fronts(data, experiment.path)
    final_generation, final_front = fronts[-1]
    del final_generation

    final_fitnesses = fitnesses_from_front(final_front)
    final_feasible = feasible_fitnesses(final_fitnesses, fuel_tolerance)
    final_candidates = candidate_fitnesses(
        final_fitnesses,
        constraint_mode,
        fuel_tolerance,
    )
    final_best_distance = best_distance_fitness(final_candidates)

    best_distance = math.nan
    best_distance_generation = math.nan
    time_at_best_distance = math.nan
    fuel_at_best_distance = math.nan
    best_fuel = math.nan
    earliest_time = math.nan

    for generation, pareto_front in fronts:
        fitnesses = fitnesses_from_front(pareto_front)
        candidates = candidate_fitnesses(
            fitnesses,
            constraint_mode,
            fuel_tolerance,
        )

        generation_best_distance = best_distance_fitness(candidates)
        if generation_best_distance is not None:
            distance = numeric_value(
                generation_best_distance,
                "minimumDistance")
            if distance is not None and (
                not math.isfinite(best_distance) or
                distance < best_distance
            ):
                best_distance = distance
                best_distance_generation = float(generation)
                time_at_best_distance = numeric_or_nan(
                    generation_best_distance,
                    "minimumDistanceTime")
                fuel_at_best_distance = numeric_or_nan(
                    generation_best_distance,
                    "minimumDistanceFuelMass")

        generation_best_fuel = best_fuel_value(candidates)
        if (
            math.isfinite(generation_best_fuel) and
            (
                not math.isfinite(best_fuel) or
                generation_best_fuel > best_fuel
            )
        ):
            best_fuel = generation_best_fuel

        generation_earliest_time = earliest_time_value(candidates)
        if (
            math.isfinite(generation_earliest_time) and
            (
                not math.isfinite(earliest_time) or
                generation_earliest_time < earliest_time
            )
        ):
            earliest_time = generation_earliest_time

    final_front_size = float(len(final_front))
    final_feasible_count = float(len(final_feasible))
    final_feasible_rate = (
        final_feasible_count / final_front_size
        if final_front_size > 0.0
        else math.nan
    )

    return RunSummary(
        scenario=experiment.scenario,
        algorithm=experiment.algorithm,
        run=experiment.run,
        final_front_size=final_front_size,
        final_feasible=final_feasible_count,
        final_feasible_rate=final_feasible_rate,
        final_min_distance=(
            numeric_value(final_best_distance, "minimumDistance")
            if final_best_distance is not None
            else math.nan
        ),
        final_best_fuel=best_fuel_value(final_candidates),
        final_best_time=earliest_time_value(final_candidates),
        best_so_far_distance=best_distance,
        best_so_far_generation=best_distance_generation,
        time_at_best_distance=time_at_best_distance,
        fuel_at_best_distance=fuel_at_best_distance,
        best_so_far_fuel=best_fuel,
        earliest_time=earliest_time,
    )


def finite_values(values: list[float]) -> list[float]:
    return [
        value
        for value in values
        if math.isfinite(value)
    ]


def finite_mean(values: list[float]) -> float:
    finite = finite_values(values)
    if not finite:
        return math.nan
    return sum(finite) / len(finite)


def finite_std(values: list[float]) -> float:
    finite = finite_values(values)
    if len(finite) <= 1:
        return 0.0 if finite else math.nan

    mean = sum(finite) / len(finite)
    variance = sum((value - mean) ** 2 for value in finite) / (len(finite) - 1)
    return math.sqrt(variance)


def finite_min(values: list[float]) -> float:
    finite = finite_values(values)
    if not finite:
        return math.nan
    return min(finite)


def finite_max(values: list[float]) -> float:
    finite = finite_values(values)
    if not finite:
        return math.nan
    return max(finite)


def values_for(
    summaries: list[RunSummary],
    field_name: str,
) -> list[float]:
    return [
        float(getattr(summary, field_name))
        for summary in summaries
    ]


def best_run_key(summary: RunSummary) -> tuple[float, float, float, float, float, int]:
    has_distance = 0.0 if math.isfinite(summary.best_so_far_distance) else 1.0
    distance = (
        summary.best_so_far_distance
        if math.isfinite(summary.best_so_far_distance)
        else math.inf
    )
    fuel = (
        summary.fuel_at_best_distance
        if math.isfinite(summary.fuel_at_best_distance)
        else -math.inf
    )
    time = (
        summary.time_at_best_distance
        if math.isfinite(summary.time_at_best_distance)
        else math.inf
    )

    return (
        has_distance,
        distance,
        -fuel,
        time,
        -summary.final_feasible_rate,
        summary.run,
    )


def aggregate_summaries(
    summaries: list[RunSummary],
    aggregation: str,
) -> AggregatedSummary:
    if not summaries:
        raise ValueError("Cannot aggregate an empty summary list.")

    scenario = summaries[0].scenario
    algorithm = summaries[0].algorithm

    if aggregation == "best-run":
        selected = min(summaries, key=best_run_key)
        return AggregatedSummary(
            scenario=scenario,
            algorithm=algorithm,
            run_count=len(summaries),
            aggregation=aggregation,
            selected_run=selected.run,
            final_front_size=selected.final_front_size,
            final_feasible=selected.final_feasible,
            final_feasible_rate=selected.final_feasible_rate,
            final_min_distance=selected.final_min_distance,
            final_best_fuel=selected.final_best_fuel,
            final_best_time=selected.final_best_time,
            best_so_far_distance=selected.best_so_far_distance,
            best_so_far_distance_std=0.0,
            best_so_far_distance_best=selected.best_so_far_distance,
            best_so_far_generation=selected.best_so_far_generation,
            time_at_best_distance=selected.time_at_best_distance,
            fuel_at_best_distance=selected.fuel_at_best_distance,
            best_so_far_fuel=selected.best_so_far_fuel,
            earliest_time=selected.earliest_time,
        )

    if aggregation == "best":
        return AggregatedSummary(
            scenario=scenario,
            algorithm=algorithm,
            run_count=len(summaries),
            aggregation=aggregation,
            selected_run=None,
            final_front_size=finite_max(values_for(summaries, "final_front_size")),
            final_feasible=finite_max(values_for(summaries, "final_feasible")),
            final_feasible_rate=finite_max(values_for(summaries, "final_feasible_rate")),
            final_min_distance=finite_min(values_for(summaries, "final_min_distance")),
            final_best_fuel=finite_max(values_for(summaries, "final_best_fuel")),
            final_best_time=finite_min(values_for(summaries, "final_best_time")),
            best_so_far_distance=finite_min(
                values_for(summaries, "best_so_far_distance")),
            best_so_far_distance_std=0.0,
            best_so_far_distance_best=finite_min(
                values_for(summaries, "best_so_far_distance")),
            best_so_far_generation=finite_min(
                values_for(summaries, "best_so_far_generation")),
            time_at_best_distance=finite_min(
                values_for(summaries, "time_at_best_distance")),
            fuel_at_best_distance=finite_max(
                values_for(summaries, "fuel_at_best_distance")),
            best_so_far_fuel=finite_max(
                values_for(summaries, "best_so_far_fuel")),
            earliest_time=finite_min(values_for(summaries, "earliest_time")),
        )

    if aggregation != "mean":
        raise ValueError(f"Unsupported aggregation: {aggregation}")

    best_distances = values_for(summaries, "best_so_far_distance")
    return AggregatedSummary(
        scenario=scenario,
        algorithm=algorithm,
        run_count=len(summaries),
        aggregation=aggregation,
        selected_run=None,
        final_front_size=finite_mean(values_for(summaries, "final_front_size")),
        final_feasible=finite_mean(values_for(summaries, "final_feasible")),
        final_feasible_rate=finite_mean(values_for(summaries, "final_feasible_rate")),
        final_min_distance=finite_mean(values_for(summaries, "final_min_distance")),
        final_best_fuel=finite_mean(values_for(summaries, "final_best_fuel")),
        final_best_time=finite_mean(values_for(summaries, "final_best_time")),
        best_so_far_distance=finite_mean(best_distances),
        best_so_far_distance_std=finite_std(best_distances),
        best_so_far_distance_best=finite_min(best_distances),
        best_so_far_generation=finite_mean(
            values_for(summaries, "best_so_far_generation")),
        time_at_best_distance=finite_mean(
            values_for(summaries, "time_at_best_distance")),
        fuel_at_best_distance=finite_mean(
            values_for(summaries, "fuel_at_best_distance")),
        best_so_far_fuel=finite_mean(values_for(summaries, "best_so_far_fuel")),
        earliest_time=finite_mean(values_for(summaries, "earliest_time")),
    )


def format_count(value: float) -> str:
    if not math.isfinite(value):
        return ""
    rounded = round(value)
    if abs(value - rounded) < 1e-9:
        return str(int(rounded))
    return f"{value:.1f}"


def trim_fixed(value: str) -> str:
    value = value.rstrip("0").rstrip(".")
    if value == "-0":
        return "0"
    return value


def format_metric(value: float, precision: int) -> str:
    if not math.isfinite(value):
        return ""

    abs_value = abs(value)
    if abs_value != 0.0 and (abs_value >= 10000.0 or abs_value < 0.001):
        return f"{value:.{precision}e}"

    return trim_fixed(f"{value:.{precision}f}")


def scenario_title(scenario: str) -> str:
    suffix = scenario.removeprefix("scenario")
    if suffix:
        return f"Scenario {suffix}"
    return scenario


def summary_sort_key(
    summary: AggregatedSummary,
) -> tuple[tuple[int, int | str], int, str]:
    return (
        scenario_sort_key(summary.scenario),
        ALGORITHM_ORDER.get(summary.algorithm, 100),
        summary.algorithm,
    )


def summary_row(
    summary: AggregatedSummary,
    precision: int,
) -> list[str | int]:
    return [
        scenario_title(summary.scenario),
        algorithm_label(summary.algorithm),
        summary.run_count,
        summary.aggregation,
        "" if summary.selected_run is None else summary.selected_run,
        format_count(summary.final_front_size),
        format_count(summary.final_feasible),
        format_metric(summary.final_feasible_rate, precision),
        format_metric(summary.final_min_distance, precision),
        format_metric(summary.final_best_fuel, precision),
        format_metric(summary.final_best_time, precision),
        format_metric(summary.best_so_far_distance, precision),
        format_metric(summary.best_so_far_distance_std, precision),
        format_metric(summary.best_so_far_distance_best, precision),
        format_metric(summary.best_so_far_generation, precision),
        format_metric(summary.time_at_best_distance, precision),
        format_metric(summary.fuel_at_best_distance, precision),
        format_metric(summary.best_so_far_fuel, precision),
        format_metric(summary.earliest_time, precision),
    ]


def output_headers() -> list[str]:
    return [
        "scenario",
        "algorithm",
        "runs",
        "aggregation",
        "selected_run",
        "final_front_size",
        "final_feasible",
        "final_feasible_rate",
        "final_min_distance",
        "final_best_fuel",
        "final_best_time",
        "best_so_far_distance_mean",
        "best_so_far_distance_std",
        "best_so_far_distance_best",
        "best_so_far_generation_mean",
        "time_at_best_distance_mean",
        "fuel_at_best_distance_mean",
        "best_so_far_fuel_mean",
        "earliest_time_mean",
    ]


def emit_csv(
    summaries: list[AggregatedSummary],
    precision: int,
    output_path: Path,
) -> None:
    output_path.parent.mkdir(parents=True, exist_ok=True)

    with output_path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.writer(handle)
        writer.writerow(output_headers())
        for summary in sorted(summaries, key=summary_sort_key):
            writer.writerow(summary_row(summary, precision))


def emit_markdown(
    summaries: list[AggregatedSummary],
    precision: int,
    output_path: Path,
) -> None:
    output_path.parent.mkdir(parents=True, exist_ok=True)
    headers = output_headers()
    lines = [
        "| " + " | ".join(headers) + " |",
        "| " + " | ".join("---" for _ in headers) + " |",
    ]
    for summary in sorted(summaries, key=summary_sort_key):
        lines.append(
            "| "
            + " | ".join(str(value) for value in summary_row(summary, precision))
            + " |"
        )

    output_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def emit_output(
    summaries: list[AggregatedSummary],
    output_format: str,
    precision: int,
    output_path: Path,
) -> None:
    if output_format == "csv":
        emit_csv(summaries, precision, output_path)
    elif output_format == "markdown":
        emit_markdown(summaries, precision, output_path)
    else:
        raise ValueError(f"Unsupported output format: {output_format}")


def main() -> int:
    args = parse_args()

    if args.fuel_tolerance < 0.0:
        print("error: --fuel-tolerance cannot be negative", file=sys.stderr)
        return 2
    if args.precision < 0:
        print("error: --precision cannot be negative", file=sys.stderr)
        return 2

    experiments = discover_experiment_files(
        input_dir=args.input_dir,
        scenarios=normalize_scenario_filter(args.scenario),
        algorithms=normalize_filter(args.algorithm),
    )
    if not experiments:
        print(f"error: no experiment JSON files found in {args.input_dir}", file=sys.stderr)
        return 2

    groups = group_experiment_files(experiments)

    try:
        warnings = validate_run_groups(
            groups=groups,
            expected_runs=args.expected_runs,
            allow_incomplete=args.allow_incomplete,
        )
    except ValueError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2

    for warning in warnings:
        print(f"warning: {warning}", file=sys.stderr)

    summaries = []
    for group_key in sorted(groups, key=lambda key: experiment_sort_key(groups[key][0])):
        run_summaries = [
            summarize_experiment(
                experiment=experiment,
                constraint_mode=args.constraint_mode,
                fuel_tolerance=args.fuel_tolerance,
            )
            for experiment in groups[group_key]
        ]
        summaries.append(
            aggregate_summaries(
                summaries=run_summaries,
                aggregation=args.aggregation,
            )
        )

    emit_output(
        summaries=summaries,
        output_format=args.format,
        precision=args.precision,
        output_path=args.output,
    )
    print(f"saved {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
