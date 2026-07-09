#!/usr/bin/env python3

from __future__ import annotations

import sys
from pathlib import Path

from solarscape_tools.analysis import (
    final_solution_summary_frame,
    load_experiment_frame_cache,
)
from solarscape_tools.display import add_display_columns


SCRIPT_DIR = Path(__file__).resolve().parent
TOOLS_DIR = SCRIPT_DIR.parent
FRAME_CACHE_PATH = TOOLS_DIR / "out" / "thesis_cache" / "experiment_frame.parquet"
OUTPUT_PATH = TOOLS_DIR / "out" / "thesis_data" / "final_metrics_by_run.csv"
CONSTRAINT_MODE = "prefer-feasible"
FUEL_TOLERANCE = 0.0


def main() -> int:
    try:
        frame = load_experiment_frame_cache(FRAME_CACHE_PATH)
        metrics = final_solution_summary_frame(
            frame=frame,
            constraint_mode=CONSTRAINT_MODE,
            fuel_tolerance=FUEL_TOLERANCE,
        )
    except ValueError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2

    output = add_display_columns(metrics)
    output["constraint_mode"] = CONSTRAINT_MODE
    output["fuel_tolerance"] = FUEL_TOLERANCE
    first_columns = [
        "scenario",
        "scenario_label",
        "algorithm",
        "algorithm_label",
        "run",
    ]
    remaining = [column for column in output.columns if column not in first_columns]
    output = output[first_columns + remaining]
    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    output.to_csv(OUTPUT_PATH, index=False)
    print(f"saved {OUTPUT_PATH}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
