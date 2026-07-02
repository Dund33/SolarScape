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
from solarscape_tools.pareto import CRITERIA


SCRIPT_DIR = Path(__file__).resolve().parent
DEFAULT_INPUT_DIR = SCRIPT_DIR / "out" / "experiments"
DEFAULT_OUTPUT_PATH = SCRIPT_DIR / "out" / "pareto_front_summary.csv"
DEFAULT_CRITERIA = (
    "targetWindowViolation",
    "fuelUsed",
    "minimumDistanceTime",
    "fuelConstraintViolation",
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Generate a CSV/Markdown list of the best SolarScape solutions "
            "for each metric, algorithm, and scenario."
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
        help=(
            "Metrics to select best solutions for. Default: "
            + ", ".join(DEFAULT_CRITERIA)
            + "."
        ),
    )
    parser.add_argument(
        "--constraint-mode",
        choices=("feasible-only", "prefer-feasible", "ignore"),
        default="feasible-only",
        help=(
            "How objective metrics handle infeasible specimens. This does not "
            "filter the fuelConstraintViolation metric itself. Default: feasible-only."
        ),
    )
    parser.add_argument(
        "--fuel-tolerance",
        type=float,
        default=0.0,
        help="Maximum fuelConstraintViolation treated as feasible. Default: 0.",
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
        help="Generate output even when a group has fewer than --expected-runs files.",
    )
    parser.add_argument(
        "-o",
        "--output",
        type=Path,
        default=DEFAULT_OUTPUT_PATH,
        help=f"Output data file. Default: {DEFAULT_OUTPUT_PATH}",
    )
    parser.add_argument(
        "--format",
        choices=("csv", "markdown"),
        default="csv",
        help="Output data format. Default: csv.",
    )
    return parser.parse_args()


def write_output(frame, output_path: Path, output_format: str) -> None:
    output_path.parent.mkdir(parents=True, exist_ok=True)
    if output_format == "csv":
        frame.to_csv(output_path, index=False)
    elif output_format == "markdown":
        lines = [
            "| " + " | ".join(frame.columns) + " |",
            "| " + " | ".join("---" for _ in frame.columns) + " |",
        ]
        for row in frame.itertuples(index=False, name=None):
            lines.append("| " + " | ".join(str(value) for value in row) + " |")
        output_path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    else:
        raise ValueError(f"Unsupported output format: {output_format}")


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
        write_output(best_solutions, args.output, args.format)
    except ValueError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2
    except KeyError as error:
        valid = ", ".join(sorted(CRITERIA))
        print(f"error: unknown metric {error}. Valid metrics: {valid}", file=sys.stderr)
        return 2

    print(f"saved {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
