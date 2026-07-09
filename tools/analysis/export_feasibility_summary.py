#!/usr/bin/env python3

from __future__ import annotations

import sys
from pathlib import Path

from solarscape_tools.analysis import (
    aggregate_feasibility_series,
    feasibility_series_frame,
    load_experiment_frame_cache,
)
from solarscape_tools.display import add_display_columns


SCRIPT_DIR = Path(__file__).resolve().parent
TOOLS_DIR = SCRIPT_DIR.parent
FRAME_CACHE_PATH = TOOLS_DIR / "out" / "thesis_cache" / "experiment_frame.parquet"
OUTPUT_PATH = TOOLS_DIR / "out" / "thesis_data" / "feasibility_summary.csv"
FEASIBILITY_MODE = "mission"
FUEL_TOLERANCE = 0.0
TARGET_TOLERANCE = 0.0
UNCERTAINTY = "iqr"


def main() -> int:
    try:
        frame = load_experiment_frame_cache(FRAME_CACHE_PATH)
        feasibility = feasibility_series_frame(
            frame,
            FUEL_TOLERANCE,
            FEASIBILITY_MODE,
            TARGET_TOLERANCE,
        )
        summary = aggregate_feasibility_series(feasibility, UNCERTAINTY)
    except ValueError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2

    output = add_display_columns(summary)
    output = output.rename(
        columns={
            "mean": "feasibility_rate_mean",
            "std": "feasibility_rate_std",
            "median": "feasibility_rate_median",
            "q1": "feasibility_rate_q1",
            "q3": "feasibility_rate_q3",
            "lower": "feasibility_rate_lower",
            "upper": "feasibility_rate_upper",
        }
    )
    output["uncertainty"] = UNCERTAINTY
    output["feasibility_mode"] = FEASIBILITY_MODE
    output["fuel_tolerance"] = FUEL_TOLERANCE
    output["target_tolerance"] = TARGET_TOLERANCE
    columns = [
        "scenario",
        "scenario_label",
        "algorithm",
        "algorithm_label",
        "generation",
        "feasibility_rate_mean",
        "feasibility_rate_std",
        "feasibility_rate_median",
        "feasibility_rate_q1",
        "feasibility_rate_q3",
        "feasibility_rate_lower",
        "feasibility_rate_upper",
        "finite_run_count",
        "run_count",
        "uncertainty",
        "feasibility_mode",
        "fuel_tolerance",
        "target_tolerance",
    ]
    output = output[[column for column in columns if column in output.columns]]
    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    output.to_csv(OUTPUT_PATH, index=False)
    print(f"saved {OUTPUT_PATH}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
