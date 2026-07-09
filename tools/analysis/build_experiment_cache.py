#!/usr/bin/env python3

from __future__ import annotations

import sys
from pathlib import Path

from solarscape_tools.analysis import (
    discover_and_validate_experiments,
    load_experiment_frame,
    write_experiment_frame_cache,
)


SCRIPT_DIR = Path(__file__).resolve().parent
TOOLS_DIR = SCRIPT_DIR.parent
INPUT_DIR = TOOLS_DIR / "out" / "experiments"
OUTPUT_PATH = TOOLS_DIR / "out" / "thesis_cache" / "experiment_frame.parquet"
EXPECTED_RUNS = 10
ALLOW_INCOMPLETE = True


def main() -> int:
    try:
        experiments = discover_and_validate_experiments(
            input_dir=INPUT_DIR,
            expected_runs=EXPECTED_RUNS,
            allow_incomplete=ALLOW_INCOMPLETE,
        )
        frame = load_experiment_frame(experiments)
        write_experiment_frame_cache(frame, OUTPUT_PATH)
    except ValueError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2

    print(f"saved {OUTPUT_PATH}")
    print(f"cached {len(frame)} Pareto-front point row(s)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
