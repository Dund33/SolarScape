#!/usr/bin/env python3

from __future__ import annotations

import argparse
import sys
from pathlib import Path

from solarscape_tools.analysis import (
    discover_and_validate_experiments,
    final_solution_summary_frame,
    load_experiment_frame,
    normalize_filter,
    normalize_scenario_filter,
)
from solarscape_tools.display import add_display_columns


SCRIPT_DIR = Path(__file__).resolve().parent
TOOLS_DIR = SCRIPT_DIR.parent
DEFAULT_INPUT_DIR = TOOLS_DIR / "out" / "experiments"
DEFAULT_OUTPUT_PATH = TOOLS_DIR / "out" / "thesis_data" / "final_metrics_by_run.csv"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Export final and best-so-far metrics for every run."
    )
    parser.add_argument("-i", "--input-dir", type=Path, default=DEFAULT_INPUT_DIR)
    parser.add_argument("-o", "--output", type=Path, default=DEFAULT_OUTPUT_PATH)
    parser.add_argument("--scenario", action="append")
    parser.add_argument("--algorithm", action="append")
    parser.add_argument(
        "--constraint-mode",
        choices=("prefer-feasible", "feasible-only", "ignore"),
        default="prefer-feasible",
    )
    parser.add_argument("--fuel-tolerance", type=float, default=0.0)
    parser.add_argument("--expected-runs", type=int, default=5)
    parser.add_argument("--allow-incomplete", action="store_true")
    return parser.parse_args()


def main() -> int:
    args = parse_args()

    if args.fuel_tolerance < 0.0:
        print("error: --fuel-tolerance cannot be negative", file=sys.stderr)
        return 2

    try:
        experiments = discover_and_validate_experiments(
            input_dir=args.input_dir,
            scenarios=normalize_scenario_filter(args.scenario),
            algorithms=normalize_filter(args.algorithm),
            expected_runs=args.expected_runs,
            allow_incomplete=args.allow_incomplete,
        )
        frame = load_experiment_frame(experiments)
        metrics = final_solution_summary_frame(
            frame=frame,
            constraint_mode=args.constraint_mode,
            fuel_tolerance=args.fuel_tolerance,
        )
    except ValueError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2

    output = add_display_columns(metrics)
    output["constraint_mode"] = args.constraint_mode
    output["fuel_tolerance"] = args.fuel_tolerance
    first_columns = [
        "scenario",
        "scenario_label",
        "algorithm",
        "algorithm_label",
        "run",
    ]
    remaining = [column for column in output.columns if column not in first_columns]
    output = output[first_columns + remaining]
    args.output.parent.mkdir(parents=True, exist_ok=True)
    output.to_csv(args.output, index=False)
    print(f"saved {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
