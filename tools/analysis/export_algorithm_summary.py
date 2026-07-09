#!/usr/bin/env python3

from __future__ import annotations

import sys
from pathlib import Path

from solarscape_tools.analysis import (
    aggregate_summary,
    final_solution_summary_frame,
    load_experiment_frame_cache,
)


SCRIPT_DIR = Path(__file__).resolve().parent
TOOLS_DIR = SCRIPT_DIR.parent
FRAME_CACHE_PATH = TOOLS_DIR / "out" / "thesis_cache" / "experiment_frame.parquet"
OUTPUT_PATH = TOOLS_DIR / "out" / "thesis_data" / "algorithm_summary_mean.csv"
AGGREGATION = "mean"
CONSTRAINT_MODE = "prefer-feasible"
FUEL_TOLERANCE = 0.0


def main() -> int:
    try:
        frame = load_experiment_frame_cache(FRAME_CACHE_PATH)
        run_metrics = final_solution_summary_frame(
            frame=frame,
            constraint_mode=CONSTRAINT_MODE,
            fuel_tolerance=FUEL_TOLERANCE,
        )
        summary = aggregate_summary(run_metrics, AGGREGATION)
    except ValueError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2

    summary["constraint_mode"] = CONSTRAINT_MODE
    summary["fuel_tolerance"] = FUEL_TOLERANCE
    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    summary.to_csv(OUTPUT_PATH, index=False)
    print(f"saved {OUTPUT_PATH}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
