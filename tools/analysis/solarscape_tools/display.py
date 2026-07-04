from __future__ import annotations

import re

from solarscape_tools.experiments import algorithm_label
from solarscape_tools.pareto import CRITERIA


AXIS_LABELS = {
    "minimumDistance": "Minimum distance [m]",
    "targetWindowViolation": "Target window violation [m]",
    "fuelUsed": "Fuel used [kg]",
    "minimumDistanceTime": "Time at minimum distance [s]",
    "fuelConstraintViolation": "Fuel constraint violation [kg]",
}

METRIC_LABELS = {
    "minimumDistance": "Minimum distance",
    "targetWindowViolation": "Target window violation",
    "fuelUsed": "Fuel used",
    "minimumDistanceTime": "Time at minimum distance",
    "fuelConstraintViolation": "Fuel constraint violation",
}

RUN_METRIC_LABELS = {
    "final_front_size": "Final Pareto front size",
    "final_feasible": "Final feasible specimens",
    "final_feasible_rate": "Final feasible fraction",
    "final_min_distance": "Final minimum distance",
    "final_min_fuel_used": "Final minimum fuel used",
    "final_best_time": "Final earliest minimum-distance time",
    "best_so_far_distance": "Best-so-far minimum distance",
    "best_so_far_generation": "Generation of best distance",
    "time_at_best_distance": "Time at best distance",
    "fuel_used_at_best_distance": "Fuel at best distance",
    "best_so_far_fuel_used": "Best-so-far fuel used",
    "earliest_time": "Best-so-far earliest time",
}

RUN_METRIC_AXIS_LABELS = {
    "final_front_size": "Specimens",
    "final_feasible": "Specimens",
    "final_feasible_rate": "Feasible fraction",
    "final_min_distance": "Minimum distance [m]",
    "final_min_fuel_used": "Fuel used [kg]",
    "final_best_time": "Time at minimum distance [s]",
    "best_so_far_distance": "Minimum distance [m]",
    "best_so_far_generation": "Generation",
    "time_at_best_distance": "Time at minimum distance [s]",
    "fuel_used_at_best_distance": "Fuel used [kg]",
    "best_so_far_fuel_used": "Fuel used [kg]",
    "earliest_time": "Time at minimum distance [s]",
}


def scenario_title(scenario: str) -> str:
    suffix = scenario.removeprefix("scenario")
    return f"Scenario {suffix}" if suffix else scenario


def metric_label(metric: str) -> str:
    if metric in METRIC_LABELS:
        return METRIC_LABELS[metric]
    if metric in RUN_METRIC_LABELS:
        return RUN_METRIC_LABELS[metric]
    return CRITERIA[metric].label


def axis_label(metric: str) -> str:
    if metric in AXIS_LABELS:
        return AXIS_LABELS[metric]
    if metric in RUN_METRIC_AXIS_LABELS:
        return RUN_METRIC_AXIS_LABELS[metric]
    return CRITERIA[metric].axis_label


def add_display_columns(frame):
    result = frame.copy()
    if "scenario" in result:
        result["scenario_label"] = result["scenario"].map(scenario_title)
    if "algorithm" in result:
        result["algorithm_label"] = result["algorithm"].map(algorithm_label)
    if "criterion" in result:
        result["criterion_label"] = result["criterion"].map(metric_label)
    if "metric" in result:
        result["metric_label"] = result["metric"].map(metric_label)
    return result


def safe_file_name(value: str) -> str:
    return re.sub(r"[^a-zA-Z0-9_.-]+", "_", value).strip("_")
