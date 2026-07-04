#!/usr/bin/env python3

from __future__ import annotations

import argparse
import sys
from pathlib import Path

from solarscape_tools.analysis import (
    best_solutions_by_metric,
    discover_and_validate_experiments,
    load_experiment_frame,
    normalize_filter,
    normalize_scenario_filter,
    parse_criteria,
)


SCRIPT_DIR = Path(__file__).resolve().parent
TOOLS_DIR = SCRIPT_DIR.parent
DEFAULT_INPUT_DIR = TOOLS_DIR / "out" / "experiments"
DEFAULT_OUTPUT_PATH = TOOLS_DIR / "out" / "thesis_data" / "best_solutions.csv"
DEFAULT_BEST_CRITERIA = (
    "targetWindowViolation",
    "fuelUsed",
    "minimumDistanceTime",
    "fuelConstraintViolation",
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Export the best solution found for each metric, scenario, and algorithm."
        )
    )
    parser.add_argument("-i", "--input-dir", type=Path, default=DEFAULT_INPUT_DIR)
    parser.add_argument("-o", "--output", type=Path, default=DEFAULT_OUTPUT_PATH)
    parser.add_argument("--scenario", action="append")
    parser.add_argument("--algorithm", action="append")
    parser.add_argument(
        "--criteria",
        nargs="+",
        default=list(DEFAULT_BEST_CRITERIA),
        help="Metrics to select best solutions for. Default: "
        + ", ".join(DEFAULT_BEST_CRITERIA)
        + ".",
    )
    parser.add_argument(
        "--constraint-mode",
        choices=("prefer-feasible", "feasible-only", "ignore"),
        default="feasible-only",
        help=(
            "How objective metrics handle infeasible specimens. This does not "
            "filter the fuelConstraintViolation metric itself. Default: feasible-only."
        ),
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
        criteria = parse_criteria(args.criteria, include_constraint=False)
        experiments = discover_and_validate_experiments(
            input_dir=args.input_dir,
            scenarios=normalize_scenario_filter(args.scenario),
            algorithms=normalize_filter(args.algorithm),
            expected_runs=args.expected_runs,
            allow_incomplete=args.allow_incomplete,
        )
        frame = load_experiment_frame(experiments)
        best_solutions = best_solutions_by_metric(
            frame=frame,
            criteria=criteria,
            constraint_mode=args.constraint_mode,
            fuel_tolerance=args.fuel_tolerance,
        )
    except ValueError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2

    best_solutions["constraint_mode"] = args.constraint_mode
    best_solutions["fuel_tolerance"] = args.fuel_tolerance
    args.output.parent.mkdir(parents=True, exist_ok=True)
    best_solutions.to_csv(args.output, index=False)
    print(f"saved {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
