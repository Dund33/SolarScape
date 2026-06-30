#!/usr/bin/env python3

from __future__ import annotations

import math
from dataclasses import dataclass

from solarscape_tools.experiments import (
    ExperimentFile,
    algorithm_label,
    discover_experiment_files,
    experiment_sort_key,
    group_experiment_files,
)
from solarscape_tools.pareto import (
    CRITERIA,
    DEFAULT_CRITERIA,
    Criterion,
    best_generation_value,
    load_pareto_json,
)


@dataclass(frozen=True)
class RunSeries:
    scenario: str
    algorithm: str
    run: int
    generations: tuple[int, ...]
    values_by_criterion: dict[str, tuple[float, ...]]


@dataclass(frozen=True)
class AggregatedSeries:
    scenario: str
    algorithm: str
    criterion: Criterion
    generations: tuple[int, ...]
    mean: tuple[float, ...]
    lower: tuple[float, ...]
    upper: tuple[float, ...]
    counts: tuple[int, ...]
    run_count: int


def parse_criteria(criteria_keys: list[str], include_constraint: bool) -> list[Criterion]:
    keys = []
    seen = set()
    for key in criteria_keys:
        if key in seen:
            continue
        keys.append(key)
        seen.add(key)

    if include_constraint and "fuelConstraintViolation" not in keys:
        keys.append("fuelConstraintViolation")

    criteria = []
    for key in keys:
        if key not in CRITERIA:
            valid = ", ".join(sorted(CRITERIA))
            raise ValueError(f"Unknown criterion '{key}'. Valid criteria: {valid}")
        criteria.append(CRITERIA[key])

    return criteria


def load_run_series(
    experiment: ExperimentFile,
    criteria: list[Criterion],
    constraint_mode: str,
    fuel_tolerance: float,
    series_mode: str = "current-front",
) -> RunSeries:
    data = load_pareto_json(experiment.path)

    generations = data.get("generations")
    if not isinstance(generations, list):
        raise ValueError(f"Missing generations list in {experiment.path}")

    generation_numbers = []
    values_by_criterion = {criterion.key: [] for criterion in criteria}

    for fallback_generation, generation in enumerate(generations):
        generation_numbers.append(
            int(generation.get("generation", fallback_generation))
        )
        pareto_front = generation.get("paretoFront", [])

        for criterion in criteria:
            values_by_criterion[criterion.key].append(
                best_generation_value(
                    pareto_front=pareto_front,
                    criterion=criterion,
                    constraint_mode=constraint_mode,
                    fuel_tolerance=fuel_tolerance,
                )
            )

    if series_mode == "best-so-far":
        values_by_criterion = {
            criterion.key: cumulative_best_values(
                values_by_criterion[criterion.key],
                criterion)
            for criterion in criteria
        }
    elif series_mode != "current-front":
        raise ValueError(f"Unsupported series mode: {series_mode}")

    return RunSeries(
        scenario=experiment.scenario,
        algorithm=experiment.algorithm,
        run=experiment.run,
        generations=tuple(generation_numbers),
        values_by_criterion={
            criterion: tuple(values)
            for criterion, values in values_by_criterion.items()
        },
    )


def cumulative_best_values(
    values: list[float],
    criterion: Criterion,
) -> list[float]:
    best_value = math.nan
    result = []

    for value in values:
        if math.isfinite(value):
            if (
                not math.isfinite(best_value) or
                criterion.oriented_value(value) <
                criterion.oriented_value(best_value)
            ):
                best_value = value

        result.append(best_value)

    return result


def aggregate_group(
    runs: list[RunSeries],
    criterion: Criterion,
    uncertainty: str,
) -> AggregatedSeries:
    if not runs:
        raise ValueError("Cannot aggregate an empty run list.")

    generation_numbers = sorted(
        {
            generation
            for run in runs
            for generation in run.generations
        }
    )

    mean_values = []
    lower_values = []
    upper_values = []
    counts = []
    value_maps = [
        dict(zip(run.generations, run.values_by_criterion[criterion.key]))
        for run in runs
    ]

    for generation in generation_numbers:
        generation_values = []

        for value_by_generation in value_maps:
            value = value_by_generation.get(generation, math.nan)
            if math.isfinite(value):
                generation_values.append(value)

        count = len(generation_values)
        counts.append(count)

        if count == 0:
            mean_values.append(math.nan)
            lower_values.append(math.nan)
            upper_values.append(math.nan)
            continue

        mean = sum(generation_values) / count
        spread = uncertainty_spread(generation_values, uncertainty)

        mean_values.append(mean)
        lower_values.append(mean - spread)
        upper_values.append(mean + spread)

    first_run = runs[0]
    return AggregatedSeries(
        scenario=first_run.scenario,
        algorithm=first_run.algorithm,
        criterion=criterion,
        generations=tuple(generation_numbers),
        mean=tuple(mean_values),
        lower=tuple(lower_values),
        upper=tuple(upper_values),
        counts=tuple(counts),
        run_count=len(runs),
    )


def uncertainty_spread(values: list[float], uncertainty: str) -> float:
    if uncertainty == "none" or len(values) <= 1:
        return 0.0

    mean = sum(values) / len(values)
    variance = sum((value - mean) ** 2 for value in values) / (len(values) - 1)
    sample_std = math.sqrt(variance)

    if uncertainty == "std":
        return sample_std
    if uncertainty == "sem":
        return sample_std / math.sqrt(len(values))
    if uncertainty == "ci95":
        return t_critical_95(len(values) - 1) * sample_std / math.sqrt(len(values))

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
