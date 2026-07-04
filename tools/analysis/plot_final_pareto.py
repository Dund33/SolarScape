#!/usr/bin/env python3

from __future__ import annotations

import argparse
import sys
from pathlib import Path

import matplotlib.pyplot as plt
import seaborn as sns

from solarscape_tools.analysis import (
    discover_and_validate_experiments,
    final_pareto_points_frame,
    load_experiment_frame,
    normalize_filter,
    normalize_scenario_filter,
)
from solarscape_tools.display import add_display_columns, axis_label, scenario_title
from solarscape_tools.pareto import CRITERIA
from solarscape_tools.plotting import (
    algorithm_hue_order,
    apply_x_axis_scale,
    apply_y_axis_scale,
    format_legend,
    palette_for,
    pdf_path,
    save_figure,
    style_numeric_axis,
)


SCRIPT_DIR = Path(__file__).resolve().parent
TOOLS_DIR = SCRIPT_DIR.parent
DEFAULT_INPUT_DIR = TOOLS_DIR / "out" / "experiments"
DEFAULT_OUTPUT_DIR = TOOLS_DIR / "out" / "thesis_plots" / "final_pareto"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate PDF scatter plots of final Pareto fronts."
    )
    parser.add_argument("-i", "--input-dir", type=Path, default=DEFAULT_INPUT_DIR)
    parser.add_argument("-o", "--output-dir", type=Path, default=DEFAULT_OUTPUT_DIR)
    parser.add_argument("--scenario", action="append")
    parser.add_argument("--algorithm", action="append")
    parser.add_argument(
        "--x",
        choices=tuple(CRITERIA),
        default="targetWindowViolation",
        help="Criterion shown on x-axis. Default: targetWindowViolation.",
    )
    parser.add_argument(
        "--y",
        choices=tuple(CRITERIA),
        default="fuelUsed",
        help="Criterion shown on y-axis. Default: fuelUsed.",
    )
    parser.add_argument("--fuel-tolerance", type=float, default=0.0)
    parser.add_argument("--feasible-only", action="store_true")
    parser.add_argument("--log-x", action="store_true")
    parser.add_argument("--log-y", action="store_true")
    parser.add_argument("--expected-runs", type=int, default=5)
    parser.add_argument("--allow-incomplete", action="store_true")
    parser.add_argument("--dpi", type=int, default=150)
    return parser.parse_args()


def plot_group(group, output_path: Path, args: argparse.Namespace) -> None:
    group = add_display_columns(group).dropna(subset=[args.x, args.y])
    if group.empty:
        return

    fig, ax = plt.subplots(figsize=(6.4, 4.0))
    hue_order = algorithm_hue_order(group)
    palette = palette_for(group)

    sns.scatterplot(
        data=group,
        x=args.x,
        y=args.y,
        hue="algorithm_label",
        hue_order=hue_order,
        palette=palette,
        alpha=0.62,
        s=22,
        linewidth=0,
        ax=ax,
    )

    ax.set_xlabel(axis_label(args.x))
    ax.set_ylabel(axis_label(args.y))
    ax.set_title(f"{scenario_title(group['scenario'].iloc[0])}: final Pareto front")
    format_legend(ax)
    apply_x_axis_scale(ax, group[[args.x]], args.log_x)
    apply_y_axis_scale(ax, group[[args.y]], args.log_y)
    style_numeric_axis(ax)
    save_figure(fig, output_path, args.dpi)


def main() -> int:
    args = parse_args()
    sns.set_theme(style="whitegrid", context="paper")

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
        points = final_pareto_points_frame(
            frame=frame,
            fuel_tolerance=args.fuel_tolerance,
            feasible_only=args.feasible_only,
        )
    except ValueError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2

    written = 0
    for scenario, group in points.groupby("scenario", sort=True):
        output_path = pdf_path(args.output_dir, scenario, args.x, args.y, "pareto")
        plot_group(group, output_path, args)
        print(f"saved {output_path}")
        written += 1

    print(f"generated {written} plot(s)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
