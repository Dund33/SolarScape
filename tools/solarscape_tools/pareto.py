from __future__ import annotations

import json
import math
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class Criterion:
    key: str
    label: str
    axis_label: str
    goal: str

    def oriented_value(self, value: float) -> float:
        if self.goal == "min":
            return value
        if self.goal == "max":
            return -value
        raise ValueError(f"Unsupported criterion goal: {self.goal}")


CRITERIA = {
    "minimumDistance": Criterion(
        key="minimumDistance",
        label="Minimum distance",
        axis_label="minimum distance [m]",
        goal="min",
    ),
    "minimumDistanceTime": Criterion(
        key="minimumDistanceTime",
        label="Time of minimum distance",
        axis_label="time of minimum distance [s]",
        goal="min",
    ),
    "fuelUsed": Criterion(
        key="fuelUsed",
        label="Fuel used",
        axis_label="fuel used [kg]",
        goal="min",
    ),
    "fuelConstraintViolation": Criterion(
        key="fuelConstraintViolation",
        label="Fuel constraint violation",
        axis_label="fuel constraint violation [kg]",
        goal="min",
    ),
}

DEFAULT_CRITERIA = (
    "minimumDistance",
    "minimumDistanceTime",
    "fuelUsed",
)


def load_pareto_json(path: Path) -> dict:
    with path.open("r", encoding="utf-8") as handle:
        data = json.load(handle)

    if not isinstance(data, dict):
        raise ValueError(f"Expected a JSON object in {path}")

    return data


def final_pareto_front(data: dict, path: Path | None = None) -> list[dict]:
    generations = data.get("generations")
    if isinstance(generations, list):
        if not generations:
            raise ValueError(f"Missing final generation in {display_path(path)}")

        final_generation = generations[-1]
        if not isinstance(final_generation, dict):
            raise ValueError(f"Invalid final generation in {display_path(path)}")

        pareto_front = final_generation.get("paretoFront", [])
    else:
        pareto_front = data.get("paretoFront", [])

    if not isinstance(pareto_front, list):
        raise ValueError(f"Missing paretoFront list in {display_path(path)}")

    return [
        specimen
        for specimen in pareto_front
        if isinstance(specimen, dict)
    ]


def display_path(path: Path | None) -> str:
    return str(path) if path is not None else "JSON data"


def fitnesses_from_front(pareto_front: list[dict]) -> list[dict]:
    fitnesses = [
        specimen.get("fitness", {})
        for specimen in pareto_front
        if isinstance(specimen, dict)
    ]

    return [
        fitness
        for fitness in fitnesses
        if isinstance(fitness, dict)
    ]


def numeric_value(fitness: dict, key: str) -> float | None:
    try:
        value = float(fitness[key])
    except (KeyError, TypeError, ValueError):
        return None

    if not math.isfinite(value):
        return None
    return value


def required_numeric_value(fitness: dict, key: str) -> float:
    value = numeric_value(fitness, key)
    if value is None:
        return math.inf
    return value


def fuel_violation(fitness: dict) -> float:
    value = numeric_value(fitness, "fuelConstraintViolation")
    if value is None:
        return math.inf
    return max(0.0, value)


def is_feasible_fitness(fitness: dict, fuel_tolerance: float) -> bool:
    return fuel_violation(fitness) <= fuel_tolerance


def feasible_fitnesses(
    fitnesses: list[dict],
    fuel_tolerance: float,
) -> list[dict]:
    return [
        fitness
        for fitness in fitnesses
        if is_feasible_fitness(fitness, fuel_tolerance)
    ]


def best_objective_value(fitnesses: list[dict], criterion: Criterion) -> float:
    values = [
        numeric_value(fitness, criterion.key)
        for fitness in fitnesses
    ]
    numeric_values = [
        value
        for value in values
        if value is not None
    ]

    if not numeric_values:
        return math.nan
    if criterion.goal == "min":
        return min(numeric_values)
    if criterion.goal == "max":
        return max(numeric_values)
    raise ValueError(f"Unsupported criterion goal: {criterion.goal}")


def best_generation_value(
    pareto_front: list[dict],
    criterion: Criterion,
    constraint_mode: str,
    fuel_tolerance: float,
) -> float:
    fitnesses = [
        fitness
        for fitness in fitnesses_from_front(pareto_front)
        if numeric_value(fitness, criterion.key) is not None
    ]

    if not fitnesses:
        return math.nan

    if criterion.key == "fuelConstraintViolation" or constraint_mode == "ignore":
        return best_objective_value(fitnesses, criterion)

    feasible = feasible_fitnesses(fitnesses, fuel_tolerance)

    if feasible:
        return best_objective_value(feasible, criterion)

    if constraint_mode == "feasible-only":
        return math.nan

    if constraint_mode != "prefer-feasible":
        raise ValueError(f"Unsupported constraint mode: {constraint_mode}")

    best_fitness = min(
        fitnesses,
        key=lambda fitness: (
            fuel_violation(fitness),
            criterion.oriented_value(required_numeric_value(fitness, criterion.key)),
        ),
    )
    value = numeric_value(best_fitness, criterion.key)
    if value is None:
        return math.nan
    return value
