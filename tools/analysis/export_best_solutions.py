#!/usr/bin/env python3

from __future__ import annotations

import sys
from pathlib import Path

from solarscape_tools.analysis import (
    best_solutions_by_metric,
    load_experiment_frame_cache,
    parse_criteria,
)


SCRIPT_DIR = Path(__file__).resolve().parent
TOOLS_DIR = SCRIPT_DIR.parent
FRAME_CACHE_PATH = TOOLS_DIR / "out" / "thesis_cache" / "experiment_frame.parquet"
OUTPUT_PATH = TOOLS_DIR / "out" / "thesis_data" / "best_solutions.csv"
DEFAULT_BEST_CRITERIA = (
    "targetWindowViolation",
    "fuelUsed",
    "minimumDistanceTime",
    "fuelConstraintViolation",
)
CONSTRAINT_MODE = "feasible-only"
FUEL_TOLERANCE = 0.0


def main() -> int:
    try:
        criteria = parse_criteria(list(DEFAULT_BEST_CRITERIA), include_constraint=False)
        frame = load_experiment_frame_cache(FRAME_CACHE_PATH)
        best_solutions = best_solutions_by_metric(
            frame=frame,
            criteria=criteria,
            constraint_mode=CONSTRAINT_MODE,
            fuel_tolerance=FUEL_TOLERANCE,
        )
    except ValueError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2

    best_solutions["constraint_mode"] = CONSTRAINT_MODE
    best_solutions["fuel_tolerance"] = FUEL_TOLERANCE
    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    best_solutions.to_csv(OUTPUT_PATH, index=False)
    print(f"saved {OUTPUT_PATH}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
