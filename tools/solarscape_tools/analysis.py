from __future__ import annotations

import json
import math
from pathlib import Path

import numpy as np
import pandas as pd

from solarscape_tools.experiments import (
    ALGORITHM_ORDER,
    ExperimentFile,
    algorithm_label,
    discover_experiment_files,
    experiment_sort_key,
    group_experiment_files,
    validate_run_groups,
)
from solarscape_tools.pareto import CRITERIA, final_pareto_front, load_pareto_json


FITNESS_COLUMNS = [
    "minimumDistance",
    "minimumDistanceTime",
    "fuelUsed",
    "fuelConstraintViolation",
]
SUMMARY_COLUMNS = [
    "scenario",
    "algorithm",
    "runs",
    "aggregation",
    "selected_run",
    "final_front_size",
    "final_feasible",
    "final_feasible_rate",
    "final_min_distance",
    "final_min_fuel_used",
    "final_best_time",
    "best_so_far_distance_mean",
    "best_so_far_distance_std",
    "best_so_far_distance_best",
    "best_so_far_generation_mean",
    "time_at_best_distance_mean",
    "fuel_used_at_best_distance_mean",
    "best_so_far_fuel_used_mean",
    "earliest_time_mean",
]
BEST_SOLUTION_COLUMNS = [
    "scenario",
    "algorithm",
    "metric",
    "metric_goal",
    "metric_value",
    "run",
    "generation",
    "specimen",
    "source_file",
    "feasible",
    "minimumDistance",
    "minimumDistanceTime",
    "fuelUsed",
    "fuelConstraintViolation",
    "maneuvers",
]


def normalize_scenario_filter(values: list[str] | None) -> set[str] | None:
    if values is None:
        return None

    return {
        value.lower() if value.lower().startswith("scenario") else f"scenario{value.lower()}"
        for value in values
    }


def normalize_filter(values: list[str] | None) -> set[str] | None:
    if values is None:
        return None
    return {value.lower() for value in values}


def parse_criteria(criteria_keys: list[str], include_constraint: bool) -> list[str]:
    keys = list(dict.fromkeys(criteria_keys))
    if include_constraint and "fuelConstraintViolation" not in keys:
        keys.append("fuelConstraintViolation")

    unknown = [
        key
        for key in keys
        if key not in CRITERIA
    ]
    if unknown:
        valid = ", ".join(sorted(CRITERIA))
        raise ValueError(f"Unknown criterion '{unknown[0]}'. Valid criteria: {valid}")

    return keys


def discover_and_validate_experiments(
    input_dir: Path,
    scenarios: set[str] | None,
    algorithms: set[str] | None,
    expected_runs: int,
    allow_incomplete: bool,
) -> list[ExperimentFile]:
    experiments = discover_experiment_files(
        input_dir=input_dir,
        scenarios=scenarios,
        algorithms=algorithms,
    )
    if not experiments:
        raise ValueError(f"no experiment JSON files found in {input_dir}")

    warnings = validate_run_groups(
        groups=group_experiment_files(experiments),
        expected_runs=expected_runs,
        allow_incomplete=allow_incomplete,
    )
    for warning in warnings:
        print(f"warning: {warning}")

    return experiments


def load_experiment_frame(experiments: list[ExperimentFile]) -> pd.DataFrame:
    rows = []

    for experiment in sorted(experiments, key=experiment_sort_key):
        data = load_pareto_json(experiment.path)
        for generation, front in generation_fronts(data, experiment.path):
            front_size = len(front)
            for specimen_index, specimen in enumerate(front):
                if not isinstance(specimen, dict):
                    continue

                fitness = specimen.get("fitness")
                if not isinstance(fitness, dict):
                    continue

                row = {
                    "scenario": experiment.scenario,
                    "algorithm": experiment.algorithm,
                    "run": experiment.run,
                    "generation": generation,
                    "specimen": specimen_index,
                    "front_size": front_size,
                    "source_file": experiment.path.name,
                    "maneuvers": json.dumps(
                        specimen.get("maneuvers", []),
                        separators=(",", ":"),
                    ),
                }
                for column in FITNESS_COLUMNS:
                    row[column] = numeric_or_nan(fitness.get(column))
                rows.append(row)

    frame = pd.DataFrame.from_records(rows)
    if frame.empty:
        return pd.DataFrame(
            columns=[
                "scenario",
                "algorithm",
                "run",
                "generation",
                "specimen",
                "front_size",
                "source_file",
                "maneuvers",
                *FITNESS_COLUMNS,
                "fuelViolation",
                "feasible",
            ]
        )

    frame["fuelViolation"] = frame["fuelConstraintViolation"].clip(lower=0)
    frame["feasible"] = frame["fuelViolation"].le(0.0)
    return frame


def generation_fronts(data: dict, path: Path) -> list[tuple[int, list[dict]]]:
    generations = data.get("generations")
    if isinstance(generations, list):
        result = []
        for fallback_generation, generation in enumerate(generations):
            if not isinstance(generation, dict):
                continue
            pareto_front = generation.get("paretoFront", [])
            if isinstance(pareto_front, list):
                result.append((
                    int(generation.get("generation", fallback_generation)),
                    pareto_front,
                ))

        if not result:
            raise ValueError(f"Missing generation Pareto fronts in {path}")

        return result

    return [(0, final_pareto_front(data, path))]


def numeric_or_nan(value) -> float:
    try:
        result = float(value)
    except (TypeError, ValueError):
        return math.nan
    return result if math.isfinite(result) else math.nan


def candidate_frame(
    frame: pd.DataFrame,
    constraint_mode: str,
    fuel_tolerance: float,
    group_columns: list[str],
) -> pd.DataFrame:
    if constraint_mode == "ignore":
        return frame.copy()

    feasible = frame[frame["fuelViolation"].le(fuel_tolerance)]
    if constraint_mode == "feasible-only":
        return feasible.copy()

    if constraint_mode != "prefer-feasible":
        raise ValueError(f"Unsupported constraint mode: {constraint_mode}")

    feasible_keys = feasible[group_columns].drop_duplicates()
    feasible_part = feasible.copy()
    infeasible_part = frame.merge(
        feasible_keys,
        on=group_columns,
        how="left",
        indicator=True,
    )
    infeasible_part = infeasible_part[infeasible_part["_merge"] == "left_only"].drop(
        columns="_merge"
    )
    if infeasible_part.empty:
        return feasible_part.copy()

    min_violation = infeasible_part.groupby(group_columns)["fuelViolation"].transform("min")
    fallback = infeasible_part[infeasible_part["fuelViolation"].eq(min_violation)]
    return pd.concat([feasible_part, fallback], ignore_index=True)


def best_values_by_generation(
    frame: pd.DataFrame,
    criteria: list[str],
    constraint_mode: str,
    fuel_tolerance: float,
    series_mode: str,
) -> pd.DataFrame:
    candidates = candidate_frame(
        frame=frame,
        constraint_mode=constraint_mode,
        fuel_tolerance=fuel_tolerance,
        group_columns=["scenario", "algorithm", "run", "generation"],
    )

    pieces = []
    group_columns = ["scenario", "algorithm", "run", "generation"]
    for criterion in criteria:
        criterion_data = candidates.dropna(subset=[criterion])
        if criterion_data.empty:
            continue

        grouped = criterion_data.groupby(group_columns, as_index=False)
        values = (
            grouped[criterion].max()
            if CRITERIA[criterion].goal == "max"
            else grouped[criterion].min()
        )
        values = values.rename(columns={criterion: "value"})
        values["criterion"] = criterion
        pieces.append(values)

    if not pieces:
        return pd.DataFrame(columns=[*group_columns, "criterion", "value"])

    result = pd.concat(pieces, ignore_index=True)
    result = result.sort_values([*group_columns, "criterion"])
    if series_mode == "current-front":
        return result

    if series_mode != "best-so-far":
        raise ValueError(f"Unsupported series mode: {series_mode}")

    return add_best_so_far(result)


def best_solutions_by_metric(
    frame: pd.DataFrame,
    criteria: list[str],
    constraint_mode: str,
    fuel_tolerance: float,
) -> pd.DataFrame:
    pieces = []

    for criterion in criteria:
        candidates = metric_candidates(
            frame=frame,
            criterion=criterion,
            constraint_mode=constraint_mode,
            fuel_tolerance=fuel_tolerance,
        ).dropna(subset=[criterion])

        if candidates.empty:
            continue

        ascending = CRITERIA[criterion].goal == "min"
        sorted_candidates = candidates.sort_values(
            [
                "scenario",
                "algorithm",
                criterion,
                "fuelViolation",
                "minimumDistance",
                "minimumDistanceTime",
                "fuelUsed",
                "run",
                "generation",
                "specimen",
            ],
            ascending=[
                True,
                True,
                ascending,
                True,
                True,
                True,
                True,
                True,
                True,
                True,
            ],
        )
        best = sorted_candidates.drop_duplicates(["scenario", "algorithm"]).copy()
        best["metric"] = criterion
        best["metric_goal"] = CRITERIA[criterion].goal
        best["metric_value"] = best[criterion]
        pieces.append(best)

    if not pieces:
        return pd.DataFrame(columns=BEST_SOLUTION_COLUMNS)

    result = pd.concat(pieces, ignore_index=True)
    result["algorithm_order"] = result["algorithm"].map(ALGORITHM_ORDER).fillna(100)
    result["scenario_order"] = result["scenario"].map(scenario_order_value)
    result["metric_order"] = result["metric"].map(
        {criterion: index for index, criterion in enumerate(criteria)}
    )
    result = result.sort_values(
        ["scenario_order", "algorithm_order", "algorithm", "metric_order"]
    )
    result["algorithm"] = result["algorithm"].map(algorithm_label)
    result["scenario"] = result["scenario"].map(scenario_title)
    return result[BEST_SOLUTION_COLUMNS]


def metric_candidates(
    frame: pd.DataFrame,
    criterion: str,
    constraint_mode: str,
    fuel_tolerance: float,
) -> pd.DataFrame:
    if criterion == "fuelConstraintViolation":
        return frame.copy()

    return candidate_frame(
        frame=frame,
        constraint_mode=constraint_mode,
        fuel_tolerance=fuel_tolerance,
        group_columns=["scenario", "algorithm", "run", "generation"],
    )


def add_best_so_far(frame: pd.DataFrame) -> pd.DataFrame:
    pieces = []
    for criterion, data in frame.groupby("criterion", sort=False):
        data = data.sort_values(["scenario", "algorithm", "run", "generation"]).copy()
        group = data.groupby(["scenario", "algorithm", "run"], sort=False)["value"]
        if CRITERIA[criterion].goal == "max":
            data["value"] = group.cummax()
        else:
            data["value"] = group.cummin()
        pieces.append(data)

    return pd.concat(pieces, ignore_index=True)


def aggregate_quality_series(
    values: pd.DataFrame,
    uncertainty: str,
) -> pd.DataFrame:
    return aggregate_value_series(
        values=values,
        value_column="value",
        group_columns=["scenario", "algorithm", "criterion", "generation"],
        run_group_columns=["scenario", "algorithm", "criterion"],
        uncertainty=uncertainty,
    )


def aggregate_value_series(
    values: pd.DataFrame,
    value_column: str,
    group_columns: list[str],
    run_group_columns: list[str],
    uncertainty: str,
) -> pd.DataFrame:
    output_columns = [
        *group_columns,
        "mean",
        "std",
        "finite_run_count",
        "lower",
        "upper",
        "run_count",
    ]
    if values.empty:
        return pd.DataFrame(columns=output_columns)

    grouped = values.groupby(group_columns, as_index=False)[value_column]
    result = grouped.agg(mean="mean", std="std", finite_run_count="count")
    result["std"] = result["std"].fillna(0.0)
    result["spread"] = uncertainty_spread(result, uncertainty)
    result["lower"] = result["mean"] - result["spread"]
    result["upper"] = result["mean"] + result["spread"]
    result["run_count"] = result.groupby(run_group_columns)["finite_run_count"].transform("max")
    return result.drop(columns="spread")[output_columns]


def uncertainty_spread(frame: pd.DataFrame, uncertainty: str) -> pd.Series:
    if uncertainty == "none":
        return pd.Series(0.0, index=frame.index)
    if uncertainty == "std":
        return frame["std"]
    if uncertainty == "sem":
        return frame["std"] / np.sqrt(frame["finite_run_count"])
    if uncertainty == "ci95":
        critical_values = frame["finite_run_count"].astype(int).sub(1).map(t_critical_95)
        return critical_values * frame["std"] / np.sqrt(frame["finite_run_count"])
    raise ValueError(f"Unsupported uncertainty mode: {uncertainty}")


def t_critical_95(degrees_of_freedom: int) -> float:
    values = {
        1: 12.706,
        2: 4.303,
        3: 3.182,
        4: 2.776,
        5: 2.571,
        6: 2.447,
        7: 2.365,
        8: 2.306,
        9: 2.262,
        10: 2.228,
        11: 2.201,
        12: 2.179,
        13: 2.160,
        14: 2.145,
        15: 2.131,
        16: 2.120,
        17: 2.110,
        18: 2.101,
        19: 2.093,
        20: 2.086,
        21: 2.080,
        22: 2.074,
        23: 2.069,
        24: 2.064,
        25: 2.060,
        26: 2.056,
        27: 2.052,
        28: 2.048,
        29: 2.045,
        30: 2.042,
    }
    return values.get(degrees_of_freedom, 1.960)


def run_summary_frame(
    frame: pd.DataFrame,
    constraint_mode: str,
    fuel_tolerance: float,
) -> pd.DataFrame:
    final = final_generation_frame(frame)
    final_candidates = candidate_frame(
        final,
        constraint_mode,
        fuel_tolerance,
        ["scenario", "algorithm", "run"],
    )
    all_candidates = candidate_frame(
        frame,
        constraint_mode,
        fuel_tolerance,
        ["scenario", "algorithm", "run", "generation"],
    )

    final_stats = final_group_stats(final, final_candidates, fuel_tolerance)
    best_stats = best_so_far_stats(all_candidates)
    return final_stats.merge(
        best_stats,
        on=["scenario", "algorithm", "run"],
        how="outer",
    )


def final_solution_summary_frame(
    frame: pd.DataFrame,
    constraint_mode: str,
    fuel_tolerance: float,
) -> pd.DataFrame:
    summary = run_summary_frame(
        frame=frame,
        constraint_mode=constraint_mode,
        fuel_tolerance=fuel_tolerance,
    )
    if summary.empty:
        return summary

    result = summary.copy()
    result["scenario_order"] = result["scenario"].map(scenario_order_value)
    result["algorithm_order"] = result["algorithm"].map(ALGORITHM_ORDER).fillna(100)
    return result.sort_values(["scenario_order", "algorithm_order", "run"]).drop(
        columns=["scenario_order", "algorithm_order"]
    )


def feasibility_series_frame(
    frame: pd.DataFrame,
    fuel_tolerance: float,
) -> pd.DataFrame:
    group_columns = ["scenario", "algorithm", "run", "generation"]
    output_columns = [*group_columns, "front_size", "feasible", "feasibility_rate"]
    if frame.empty:
        return pd.DataFrame(columns=output_columns)

    data = frame.copy()
    data["feasible_at_tolerance"] = data["fuelViolation"].le(fuel_tolerance)
    result = data.groupby(group_columns, as_index=False).agg(
        front_size=("specimen", "count"),
        feasible=("feasible_at_tolerance", "sum"),
    )
    result["feasibility_rate"] = result["feasible"] / result["front_size"]
    result["scenario_order"] = result["scenario"].map(scenario_order_value)
    result["algorithm_order"] = result["algorithm"].map(ALGORITHM_ORDER).fillna(100)
    return result.sort_values(
        ["scenario_order", "algorithm_order", "run", "generation"]
    ).drop(columns=["scenario_order", "algorithm_order"])[output_columns]


def aggregate_feasibility_series(
    feasibility: pd.DataFrame,
    uncertainty: str,
) -> pd.DataFrame:
    return aggregate_value_series(
        values=feasibility,
        value_column="feasibility_rate",
        group_columns=["scenario", "algorithm", "generation"],
        run_group_columns=["scenario", "algorithm"],
        uncertainty=uncertainty,
    )


def final_pareto_points_frame(
    frame: pd.DataFrame,
    fuel_tolerance: float,
    feasible_only: bool = False,
) -> pd.DataFrame:
    columns = [
        "scenario",
        "algorithm",
        "run",
        "generation",
        "specimen",
        "source_file",
        "front_size",
        "feasible",
        "minimumDistance",
        "fuelUsed",
        "minimumDistanceTime",
        "fuelConstraintViolation",
        "fuelViolation",
        "maneuvers",
    ]
    if frame.empty:
        return pd.DataFrame(columns=columns)

    result = final_generation_frame(frame)
    result["feasible"] = result["fuelViolation"].le(fuel_tolerance)
    if feasible_only:
        result = result[result["feasible"]].copy()

    result["scenario_order"] = result["scenario"].map(scenario_order_value)
    result["algorithm_order"] = result["algorithm"].map(ALGORITHM_ORDER).fillna(100)
    result = result.sort_values(
        ["scenario_order", "algorithm_order", "run", "generation", "specimen"]
    )
    return result.drop(columns=["scenario_order", "algorithm_order"])[columns]


def final_generation_frame(frame: pd.DataFrame) -> pd.DataFrame:
    max_generation = frame.groupby(["scenario", "algorithm", "run"])["generation"].transform("max")
    return frame[frame["generation"].eq(max_generation)].copy()


def final_group_stats(
    final: pd.DataFrame,
    candidates: pd.DataFrame,
    fuel_tolerance: float,
) -> pd.DataFrame:
    group_columns = ["scenario", "algorithm", "run"]
    stats = final.groupby(group_columns, as_index=False).agg(
        final_front_size=("specimen", "count"),
        final_feasible=("fuelViolation", lambda values: values.le(fuel_tolerance).sum()),
    )
    stats["final_feasible_rate"] = stats["final_feasible"] / stats["final_front_size"]

    candidate_stats = candidates.groupby(group_columns, as_index=False).agg(
        final_min_distance=("minimumDistance", "min"),
        final_min_fuel_used=("fuelUsed", "min"),
        final_best_time=("minimumDistanceTime", "min"),
    )
    return stats.merge(candidate_stats, on=group_columns, how="left")


def best_so_far_stats(candidates: pd.DataFrame) -> pd.DataFrame:
    group_columns = ["scenario", "algorithm", "run"]
    distance_candidates = candidates.dropna(subset=["minimumDistance"])
    distance_candidates = distance_candidates.sort_values(
        [
            *group_columns,
            "minimumDistance",
            "minimumDistanceTime",
            "fuelUsed",
            "fuelViolation",
        ],
        ascending=[True, True, True, True, True, True, True],
    )
    best_distance = distance_candidates.drop_duplicates(group_columns)
    best_distance = best_distance[
        [
            *group_columns,
            "minimumDistance",
            "generation",
            "minimumDistanceTime",
            "fuelUsed",
        ]
    ].rename(
        columns={
            "minimumDistance": "best_so_far_distance",
            "generation": "best_so_far_generation",
            "minimumDistanceTime": "time_at_best_distance",
            "fuelUsed": "fuel_used_at_best_distance",
        }
    )

    other = candidates.groupby(group_columns, as_index=False).agg(
        best_so_far_fuel_used=("fuelUsed", "min"),
        earliest_time=("minimumDistanceTime", "min"),
    )
    return best_distance.merge(other, on=group_columns, how="outer")


def aggregate_summary(
    run_summary: pd.DataFrame,
    aggregation: str,
) -> pd.DataFrame:
    if aggregation == "mean":
        return aggregate_summary_mean(run_summary, aggregation)
    if aggregation == "best":
        return aggregate_summary_best(run_summary, aggregation)
    if aggregation == "best-run":
        return aggregate_summary_best_run(run_summary, aggregation)
    raise ValueError(f"Unsupported aggregation: {aggregation}")


def aggregate_summary_mean(
    run_summary: pd.DataFrame,
    aggregation: str,
) -> pd.DataFrame:
    group_columns = ["scenario", "algorithm"]
    result = run_summary.groupby(group_columns, as_index=False).agg(
        runs=("run", "count"),
        final_front_size=("final_front_size", "mean"),
        final_feasible=("final_feasible", "mean"),
        final_feasible_rate=("final_feasible_rate", "mean"),
        final_min_distance=("final_min_distance", "mean"),
        final_min_fuel_used=("final_min_fuel_used", "mean"),
        final_best_time=("final_best_time", "mean"),
        best_so_far_distance_mean=("best_so_far_distance", "mean"),
        best_so_far_distance_std=("best_so_far_distance", sample_std),
        best_so_far_distance_best=("best_so_far_distance", "min"),
        best_so_far_generation_mean=("best_so_far_generation", "mean"),
        time_at_best_distance_mean=("time_at_best_distance", "mean"),
        fuel_used_at_best_distance_mean=("fuel_used_at_best_distance", "mean"),
        best_so_far_fuel_used_mean=("best_so_far_fuel_used", "mean"),
        earliest_time_mean=("earliest_time", "mean"),
    )
    result.insert(3, "aggregation", aggregation)
    result.insert(4, "selected_run", pd.NA)
    return order_summary(result)


def aggregate_summary_best(
    run_summary: pd.DataFrame,
    aggregation: str,
) -> pd.DataFrame:
    group_columns = ["scenario", "algorithm"]
    result = run_summary.groupby(group_columns, as_index=False).agg(
        runs=("run", "count"),
        final_front_size=("final_front_size", "max"),
        final_feasible=("final_feasible", "max"),
        final_feasible_rate=("final_feasible_rate", "max"),
        final_min_distance=("final_min_distance", "min"),
        final_min_fuel_used=("final_min_fuel_used", "min"),
        final_best_time=("final_best_time", "min"),
        best_so_far_distance_mean=("best_so_far_distance", "min"),
        best_so_far_distance_std=("best_so_far_distance", lambda _: 0.0),
        best_so_far_distance_best=("best_so_far_distance", "min"),
        best_so_far_generation_mean=("best_so_far_generation", "min"),
        time_at_best_distance_mean=("time_at_best_distance", "min"),
        fuel_used_at_best_distance_mean=("fuel_used_at_best_distance", "min"),
        best_so_far_fuel_used_mean=("best_so_far_fuel_used", "min"),
        earliest_time_mean=("earliest_time", "min"),
    )
    result.insert(3, "aggregation", aggregation)
    result.insert(4, "selected_run", pd.NA)
    return order_summary(result)


def aggregate_summary_best_run(
    run_summary: pd.DataFrame,
    aggregation: str,
) -> pd.DataFrame:
    run_counts = run_summary.groupby(
        ["scenario", "algorithm"],
        as_index=False,
    ).agg(runs=("run", "count"))
    ranked = run_summary.sort_values(
        [
            "scenario",
            "algorithm",
            "best_so_far_distance",
            "fuel_used_at_best_distance",
            "time_at_best_distance",
            "final_feasible_rate",
            "run",
        ],
        ascending=[True, True, True, True, True, False, True],
    )
    result = ranked.drop_duplicates(["scenario", "algorithm"]).copy()
    result = result.merge(run_counts, on=["scenario", "algorithm"], how="left")
    result = result.rename(
        columns={
            "run": "selected_run",
            "best_so_far_distance": "best_so_far_distance_mean",
            "best_so_far_generation": "best_so_far_generation_mean",
            "time_at_best_distance": "time_at_best_distance_mean",
            "fuel_used_at_best_distance": "fuel_used_at_best_distance_mean",
            "best_so_far_fuel_used": "best_so_far_fuel_used_mean",
            "earliest_time": "earliest_time_mean",
        }
    )
    result["aggregation"] = aggregation
    result["best_so_far_distance_std"] = 0.0
    result["best_so_far_distance_best"] = result["best_so_far_distance_mean"]
    return order_summary(result)


def sample_std(values: pd.Series) -> float:
    if values.count() <= 1:
        return 0.0 if values.count() == 1 else math.nan
    return values.std(ddof=1)


def order_summary(frame: pd.DataFrame) -> pd.DataFrame:
    result = frame.copy()
    result["algorithm_order"] = result["algorithm"].map(ALGORITHM_ORDER).fillna(100)
    result["scenario_order"] = result["scenario"].map(scenario_order_value)
    result = result.sort_values(["scenario_order", "algorithm_order", "algorithm"])
    result["algorithm"] = result["algorithm"].map(algorithm_label)
    result["scenario"] = result["scenario"].map(scenario_title)
    return result[SUMMARY_COLUMNS]


def scenario_order_value(scenario: str) -> int:
    suffix = scenario.removeprefix("scenario")
    return int(suffix) if suffix.isdigit() else 9999


def scenario_title(scenario: str) -> str:
    suffix = scenario.removeprefix("scenario")
    return f"Scenario {suffix}" if suffix else scenario


def format_summary_for_output(
    frame: pd.DataFrame,
    precision: int,
) -> pd.DataFrame:
    result = frame.copy()
    count_columns = [
        "runs",
        "selected_run",
        "final_front_size",
        "final_feasible",
    ]
    for column in count_columns:
        result[column] = result[column].map(format_count)

    for column in result.columns:
        if column not in {"scenario", "algorithm", "aggregation", *count_columns}:
            result[column] = result[column].map(lambda value: format_metric(value, precision))

    return result


def format_count(value) -> str:
    if pd.isna(value):
        return ""
    value = float(value)
    rounded = round(value)
    if abs(value - rounded) < 1e-9:
        return str(int(rounded))
    return f"{value:.1f}"


def format_metric(value, precision: int) -> str:
    if pd.isna(value):
        return ""
    value = float(value)
    abs_value = abs(value)
    if abs_value != 0.0 and (abs_value >= 10000.0 or abs_value < 0.001):
        return f"{value:.{precision}e}"
    text = f"{value:.{precision}f}".rstrip("0").rstrip(".")
    return "0" if text == "-0" else text
