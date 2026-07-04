#!/usr/bin/env python3

from __future__ import annotations

import argparse
import sys
from pathlib import Path

from solarscape_tools.analysis import (
    best_values_by_generation,
    discover_and_validate_experiments,
    load_experiment_frame,
    normalize_filter,
    normalize_scenario_filter,
    parse_criteria,
)
from solarscape_tools.display import add_display_columns
from solarscape_tools.pareto import DEFAULT_CRITERIA


SCRIPT_DIR = Path(__file__).resolve().parent
TOOLS_DIR = SCRIPT_DIR.parent
DEFAULT_INPUT_DIR = TOOLS_DIR / "out" / "experiments"
DEFAULT_OUTPUT_PATH = TOOLS_DIR / "out" / "thesis_data" / "convergence_runs.csv"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Export per-run, per-generation convergence values from SolarScape "
            "experiment JSON files."
        )
    )
    parser.add_argument(
        "-i",
        "--input-dir",
        type=Path,
        default=DEFAULT_INPUT_DIR,
        help=f"Directory with scenario_algorithm_runXX.json files. Default: {DEFAULT_INPUT_DIR}",
    )
    parser.add_argument(
        "-o",
        "--output",
        type=Path,
        default=DEFAULT_OUTPUT_PATH,
        help=f"Output CSV file. Default: {DEFAULT_OUTPUT_PATH}",
    )
    parser.add_argument(
        "--scenario",
        action="append",
        help="Scenario to include, for example scenario1 or 1. Can be passed multiple times.",
    )
    parser.add_argument(
        "--algorithm",
        action="append",
        help="Algorithm to include, for example algo, nsgaii, or moead. Can be passed multiple times.",
    )
    parser.add_argument(
        "--criteria",
        nargs="+",
        default=list(DEFAULT_CRITERIA),
        help="Criteria to export. Default: " + ", ".join(DEFAULT_CRITERIA) + ".",
    )
    parser.add_argument(
        "--include-constraint",
        action="store_true",
        help="Also export fuelConstraintViolation.",
    )
    parser.add_argument(
        "--constraint-mode",
        choices=("prefer-feasible", "feasible-only", "ignore"),
        default="prefer-feasible",
        help="How objective values are selected from each Pareto front. Default: prefer-feasible.",
    )
    parser.add_argument(
        "--fuel-tolerance",
        type=float,
        default=0.0,
        help="Maximum fuelConstraintViolation treated as feasible. Default: 0.",
    )
    parser.add_argument(
        "--series",
        choices=("best-so-far", "current-front"),
        default="best-so-far",
        help="Export cumulative best-so-far values or current-front values. Default: best-so-far.",
    )
    parser.add_argument(
        "--expected-runs",
        type=int,
        default=5,
        help="Minimum run count expected in each scenario/algorithm group. Default: 5.",
    )
    parser.add_argument(
        "--allow-incomplete",
        action="store_true",
        help="Export data even when a group has fewer than --expected-runs files.",
    )
    return parser.parse_args()


def ordered_output(frame, args: argparse.Namespace):
    output = frame.copy()
    output["series"] = args.series
    output["constraint_mode"] = args.constraint_mode
    output["fuel_tolerance"] = args.fuel_tolerance
    output = add_display_columns(output)
    columns = [
        "scenario",
        "scenario_label",
        "algorithm",
        "algorithm_label",
        "run",
        "generation",
        "criterion",
        "criterion_label",
        "value",
        "series",
        "constraint_mode",
        "fuel_tolerance",
    ]
    return output[[column for column in columns if column in output.columns]]


def write_csv(frame, output_path: Path) -> None:
    output_path.parent.mkdir(parents=True, exist_ok=True)
    frame.to_csv(output_path, index=False)


def main() -> int:
    args = parse_args()

    if args.fuel_tolerance < 0.0:
        print("error: --fuel-tolerance cannot be negative", file=sys.stderr)
        return 2

    try:
        criteria = parse_criteria(args.criteria, args.include_constraint)
        experiments = discover_and_validate_experiments(
            input_dir=args.input_dir,
            scenarios=normalize_scenario_filter(args.scenario),
            algorithms=normalize_filter(args.algorithm),
            expected_runs=args.expected_runs,
            allow_incomplete=args.allow_incomplete,
        )
        frame = load_experiment_frame(experiments)
        values = best_values_by_generation(
            frame=frame,
            criteria=criteria,
            constraint_mode=args.constraint_mode,
            fuel_tolerance=args.fuel_tolerance,
            series_mode=args.series,
        )
    except ValueError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2

    output = ordered_output(values, args)
    write_csv(output, args.output)
    print(f"saved {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
