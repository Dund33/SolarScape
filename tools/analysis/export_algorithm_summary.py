#!/usr/bin/env python3

from __future__ import annotations

import argparse
import sys
from pathlib import Path

from solarscape_tools.analysis import (
    aggregate_summary,
    discover_and_validate_experiments,
    final_solution_summary_frame,
    load_experiment_frame,
    normalize_filter,
    normalize_scenario_filter,
)


SCRIPT_DIR = Path(__file__).resolve().parent
TOOLS_DIR = SCRIPT_DIR.parent
DEFAULT_INPUT_DIR = TOOLS_DIR / "out" / "experiments"
DEFAULT_OUTPUT_DIR = TOOLS_DIR / "out" / "thesis_data"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Export one aggregated performance row per scenario and algorithm."
        )
    )
    parser.add_argument("-i", "--input-dir", type=Path, default=DEFAULT_INPUT_DIR)
    parser.add_argument(
        "-o",
        "--output",
        type=Path,
        help=(
            "Output CSV file. Default: "
            f"{DEFAULT_OUTPUT_DIR / 'algorithm_summary_<aggregation>.csv'}"
        ),
    )
    parser.add_argument("--scenario", action="append")
    parser.add_argument("--algorithm", action="append")
    parser.add_argument(
        "--aggregation",
        choices=("mean", "best", "best-run"),
        default="mean",
        help="Aggregation over runs. Default: mean.",
    )
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
    output_path = (
        args.output
        if args.output is not None
        else DEFAULT_OUTPUT_DIR / f"algorithm_summary_{args.aggregation}.csv"
    )

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
        run_metrics = final_solution_summary_frame(
            frame=frame,
            constraint_mode=args.constraint_mode,
            fuel_tolerance=args.fuel_tolerance,
        )
        summary = aggregate_summary(run_metrics, args.aggregation)
    except ValueError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2

    summary["constraint_mode"] = args.constraint_mode
    summary["fuel_tolerance"] = args.fuel_tolerance
    output_path.parent.mkdir(parents=True, exist_ok=True)
    summary.to_csv(output_path, index=False)
    print(f"saved {output_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
