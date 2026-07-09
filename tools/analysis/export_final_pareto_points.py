#!/usr/bin/env python3

from __future__ import annotations

import sys
from pathlib import Path

from solarscape_tools.analysis import (
    final_pareto_points_frame,
    load_experiment_frame_cache,
)
from solarscape_tools.display import add_display_columns


SCRIPT_DIR = Path(__file__).resolve().parent
TOOLS_DIR = SCRIPT_DIR.parent
FRAME_CACHE_PATH = TOOLS_DIR / "out" / "thesis_cache" / "experiment_frame.parquet"
OUTPUT_PATH = TOOLS_DIR / "out" / "thesis_data" / "final_pareto_points.csv"
FUEL_TOLERANCE = 0.0
FEASIBLE_ONLY = False


def main() -> int:
    try:
        frame = load_experiment_frame_cache(FRAME_CACHE_PATH)
        pareto_points = final_pareto_points_frame(
            frame=frame,
            fuel_tolerance=FUEL_TOLERANCE,
            feasible_only=FEASIBLE_ONLY,
        )
    except ValueError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2

    output = add_display_columns(pareto_points)
    output["fuel_tolerance"] = FUEL_TOLERANCE
    output["feasible_only"] = FEASIBLE_ONLY
    first_columns = [
        "scenario",
        "scenario_label",
        "algorithm",
        "algorithm_label",
        "run",
        "generation",
        "specimen",
    ]
    remaining = [column for column in output.columns if column not in first_columns]
    output = output[first_columns + remaining]
    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    output.to_csv(OUTPUT_PATH, index=False)
    print(f"saved {OUTPUT_PATH}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
