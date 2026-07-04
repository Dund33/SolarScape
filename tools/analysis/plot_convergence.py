#!/usr/bin/env python3

from __future__ import annotations

import argparse
import sys
from pathlib import Path

import matplotlib.pyplot as plt
import seaborn as sns

from solarscape_tools.analysis import (
    aggregate_quality_series,
    best_values_by_generation,
    discover_and_validate_experiments,
    load_experiment_frame,
    normalize_filter,
    normalize_scenario_filter,
    parse_criteria,
)
from solarscape_tools.display import (
    add_display_columns,
    axis_label,
    metric_label,
    scenario_title,
)
from solarscape_tools.pareto import DEFAULT_CRITERIA
from solarscape_tools.plotting import (
    algorithm_hue_order,
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
DEFAULT_OUTPUT_DIR = TOOLS_DIR / "out" / "thesis_plots" / "convergence"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate PDF convergence plots comparing algorithms."
    )
    parser.add_argument("-i", "--input-dir", type=Path, default=DEFAULT_INPUT_DIR)
    parser.add_argument("-o", "--output-dir", type=Path, default=DEFAULT_OUTPUT_DIR)
    parser.add_argument("--scenario", action="append")
    parser.add_argument("--algorithm", action="append")
    parser.add_argument(
        "--criteria",
        nargs="+",
        default=list(DEFAULT_CRITERIA),
        help="Criteria to plot. Default: " + ", ".join(DEFAULT_CRITERIA) + ".",
    )
    parser.add_argument("--include-constraint", action="store_true")
    parser.add_argument(
        "--constraint-mode",
        choices=("prefer-feasible", "feasible-only", "ignore"),
        default="prefer-feasible",
    )
    parser.add_argument("--fuel-tolerance", type=float, default=0.0)
    parser.add_argument(
        "--series",
        choices=("best-so-far", "current-front"),
        default="best-so-far",
    )
    parser.add_argument(
        "--uncertainty",
        choices=("none", "std", "sem", "ci95"),
        default="std",
    )
    parser.add_argument("--log-y", action="store_true")
    parser.add_argument("--expected-runs", type=int, default=5)
    parser.add_argument("--allow-incomplete", action="store_true")
    parser.add_argument("--dpi", type=int, default=150)
    return parser.parse_args()


def plot_group(group, output_path: Path, args: argparse.Namespace) -> None:
    group = add_display_columns(group).sort_values(["algorithm", "generation"])
    if group.empty:
        return

    first = group.iloc[0]
    fig, ax = plt.subplots(figsize=(6.6, 4.0))
    hue_order = algorithm_hue_order(group)
    palette = palette_for(group)

    sns.lineplot(
        data=group,
        x="generation",
        y="mean",
        hue="algorithm_label",
        hue_order=hue_order,
        palette=palette,
        estimator=None,
        linewidth=1.8,
        ax=ax,
    )

    if args.uncertainty != "none":
        for _, row_group in group.groupby("algorithm", sort=False):
            row_group = row_group.sort_values("generation")
            label = row_group["algorithm_label"].iloc[0]
            ax.fill_between(
                row_group["generation"].to_numpy(),
                row_group["lower"].to_numpy(),
                row_group["upper"].to_numpy(),
                color=palette[label],
                alpha=0.14,
                linewidth=0,
            )

    ax.set_xlabel("Generation")
    ax.set_ylabel(axis_label(first["criterion"]))
    ax.set_title(
        f"{scenario_title(first['scenario'])}: "
        f"{metric_label(first['criterion'])} convergence"
    )
    format_legend(ax)
    apply_y_axis_scale(ax, group[["lower", "mean", "upper"]], args.log_y)
    style_numeric_axis(ax)
    save_figure(fig, output_path, args.dpi)


def main() -> int:
    args = parse_args()
    sns.set_theme(style="whitegrid", context="paper")

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
        quality_series = aggregate_quality_series(values, args.uncertainty)
    except ValueError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2

    written = 0
    for (scenario, criterion), group in quality_series.groupby(
        ["scenario", "criterion"],
        sort=True,
    ):
        parts = [scenario, criterion, "convergence"]
        if args.series != "best-so-far":
            parts.insert(2, args.series)
        output_path = pdf_path(args.output_dir, *parts)
        plot_group(group, output_path, args)
        print(f"saved {output_path}")
        written += 1

    print(f"generated {written} plot(s)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
