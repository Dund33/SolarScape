#!/usr/bin/env python3

from __future__ import annotations

import sys
from pathlib import Path

from solarscape_tools.analysis import (
    aggregate_quality_series,
    best_values_by_generation,
    load_experiment_frame_cache,
    parse_criteria,
)
from solarscape_tools.display import add_display_columns


SCRIPT_DIR = Path(__file__).resolve().parent
TOOLS_DIR = SCRIPT_DIR.parent
FRAME_CACHE_PATH = TOOLS_DIR / "out" / "thesis_cache" / "experiment_frame.parquet"
OUTPUT_PATH = TOOLS_DIR / "out" / "thesis_data" / "convergence_summary.csv"
CRITERIA = ("targetWindowViolation",)
CONSTRAINT_MODE = "prefer-feasible"
FUEL_TOLERANCE = 0.0
SERIES = "best-so-far"
UNCERTAINTY = "iqr"


def ordered_output(frame):
    output = frame.copy()
    output["series"] = SERIES
    output["uncertainty"] = UNCERTAINTY
    output["constraint_mode"] = CONSTRAINT_MODE
    output["fuel_tolerance"] = FUEL_TOLERANCE
    output = add_display_columns(output)
    columns = [
        "scenario",
        "scenario_label",
        "algorithm",
        "algorithm_label",
        "criterion",
        "criterion_label",
        "generation",
        "mean",
        "std",
        "median",
        "q1",
        "q3",
        "lower",
        "upper",
        "finite_run_count",
        "run_count",
        "series",
        "uncertainty",
        "constraint_mode",
        "fuel_tolerance",
    ]
    return output[[column for column in columns if column in output.columns]]


def main() -> int:
    try:
        criteria = parse_criteria(list(CRITERIA), include_constraint=False)
        frame = load_experiment_frame_cache(FRAME_CACHE_PATH)
        values = best_values_by_generation(
            frame=frame,
            criteria=criteria,
            constraint_mode=CONSTRAINT_MODE,
            fuel_tolerance=FUEL_TOLERANCE,
            series_mode=SERIES,
        )
        summary = aggregate_quality_series(values, UNCERTAINTY)
    except ValueError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2

    output = ordered_output(summary)
    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    output.to_csv(OUTPUT_PATH, index=False)
    print(f"saved {OUTPUT_PATH}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
