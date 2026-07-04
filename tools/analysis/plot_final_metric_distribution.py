#!/usr/bin/env python3

from __future__ import annotations

import argparse
import sys
from pathlib import Path

import matplotlib.pyplot as plt
import seaborn as sns

from solarscape_tools.analysis import (
    discover_and_validate_experiments,
    final_solution_summary_frame,
    load_experiment_frame,
    normalize_filter,
    normalize_scenario_filter,
)
from solarscape_tools.display import add_display_columns, axis_label, metric_label, scenario_title
from solarscape_tools.plotting import (
    algorithm_hue_order,
    apply_y_axis_scale,
    palette_for,
    pdf_path,
    save_figure,
    style_numeric_axis,
)


SCRIPT_DIR = Path(__file__).resolve().parent
TOOLS_DIR = SCRIPT_DIR.parent
DEFAULT_INPUT_DIR = TOOLS_DIR / "out" / "experiments"
DEFAULT_OUTPUT_DIR = TOOLS_DIR / "out" / "thesis_plots" / "final_metric_distribution"
RUN_METRICS = (
    "final_front_size",
    "final_feasible_rate",
    "final_min_distance",
    "final_min_fuel_used",
    "final_best_time",
    "best_so_far_distance",
    "best_so_far_generation",
    "time_at_best_distance",
    "fuel_used_at_best_distance",
    "best_so_far_fuel_used",
    "earliest_time",
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate a PDF boxplot for one final per-run metric."
    )
    parser.add_argument("-i", "--input-dir", type=Path, default=DEFAULT_INPUT_DIR)
    parser.add_argument("-o", "--output-dir", type=Path, default=DEFAULT_OUTPUT_DIR)
    parser.add_argument("--scenario", action="append")
    parser.add_argument("--algorithm", action="append")
    parser.add_argument(
        "--metric",
        choices=RUN_METRICS,
        default="best_so_far_distance",
        help="Metric shown on the y-axis. Default: best_so_far_distance.",
    )
    parser.add_argument(
        "--constraint-mode",
        choices=("prefer-feasible", "feasible-only", "ignore"),
        default="prefer-feasible",
    )
    parser.add_argument("--fuel-tolerance", type=float, default=0.0)
    parser.add_argument("--log-y", action="store_true")
    parser.add_argument("--expected-runs", type=int, default=5)
    parser.add_argument("--allow-incomplete", action="store_true")
    parser.add_argument("--dpi", type=int, default=150)
    return parser.parse_args()


def plot_group(group, output_path: Path, args: argparse.Namespace) -> None:
    group = add_display_columns(group).dropna(subset=[args.metric])
    if group.empty:
        return

    fig, ax = plt.subplots(figsize=(6.4, 4.0))
    order = algorithm_hue_order(group)
    palette = palette_for(group)

    sns.boxplot(
        data=group,
        x="algorithm_label",
        y=args.metric,
        order=order,
        palette=palette,
        showfliers=False,
        width=0.55,
        ax=ax,
    )
    sns.stripplot(
        data=group,
        x="algorithm_label",
        y=args.metric,
        order=order,
        palette=palette,
        jitter=0.16,
        size=4.0,
        alpha=0.75,
        linewidth=0.4,
        edgecolor="white",
        ax=ax,
    )

    ax.set_xlabel("")
    ax.set_ylabel(axis_label(args.metric))
    ax.set_title(f"{scenario_title(group['scenario'].iloc[0])}: {metric_label(args.metric)}")
    apply_y_axis_scale(ax, group[[args.metric]], args.log_y)
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
        metrics = final_solution_summary_frame(
            frame=frame,
            constraint_mode=args.constraint_mode,
            fuel_tolerance=args.fuel_tolerance,
        )
    except ValueError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2

    written = 0
    for scenario, group in metrics.groupby("scenario", sort=True):
        output_path = pdf_path(args.output_dir, scenario, args.metric, "distribution")
        plot_group(group, output_path, args)
        print(f"saved {output_path}")
        written += 1

    print(f"generated {written} plot(s)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
