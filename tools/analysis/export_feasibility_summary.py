#!/usr/bin/env python3

from __future__ import annotations

import argparse
import sys
from pathlib import Path

from solarscape_tools.analysis import (
    aggregate_feasibility_series,
    discover_and_validate_experiments,
    feasibility_series_frame,
    load_experiment_frame,
    normalize_filter,
    normalize_scenario_filter,
)
from solarscape_tools.display import add_display_columns


SCRIPT_DIR = Path(__file__).resolve().parent
TOOLS_DIR = SCRIPT_DIR.parent
DEFAULT_INPUT_DIR = TOOLS_DIR / "out" / "experiments"
DEFAULT_OUTPUT_PATH = TOOLS_DIR / "out" / "thesis_data" / "feasibility_summary.csv"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Export mean feasible fraction over generations."
    )
    parser.add_argument("-i", "--input-dir", type=Path, default=DEFAULT_INPUT_DIR)
    parser.add_argument("-o", "--output", type=Path, default=DEFAULT_OUTPUT_PATH)
    parser.add_argument("--scenario", action="append")
    parser.add_argument("--algorithm", action="append")
    parser.add_argument("--fuel-tolerance", type=float, default=0.0)
    parser.add_argument(
        "--uncertainty",
        choices=("none", "std", "sem", "ci95"),
        default="std",
    )
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
        feasibility = feasibility_series_frame(frame, args.fuel_tolerance)
        summary = aggregate_feasibility_series(feasibility, args.uncertainty)
    except ValueError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2

    output = add_display_columns(summary)
    output = output.rename(
        columns={
            "mean": "feasibility_rate_mean",
            "std": "feasibility_rate_std",
            "lower": "feasibility_rate_lower",
            "upper": "feasibility_rate_upper",
        }
    )
    output["uncertainty"] = args.uncertainty
    output["fuel_tolerance"] = args.fuel_tolerance
    columns = [
        "scenario",
        "scenario_label",
        "algorithm",
        "algorithm_label",
        "generation",
        "feasibility_rate_mean",
        "feasibility_rate_std",
        "feasibility_rate_lower",
        "feasibility_rate_upper",
        "finite_run_count",
        "run_count",
        "uncertainty",
        "fuel_tolerance",
    ]
    output = output[[column for column in columns if column in output.columns]]
    args.output.parent.mkdir(parents=True, exist_ok=True)
    output.to_csv(args.output, index=False)
    print(f"saved {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
