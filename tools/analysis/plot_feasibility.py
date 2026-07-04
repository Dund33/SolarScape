#!/usr/bin/env python3

from __future__ import annotations

import argparse
import sys
from pathlib import Path

import matplotlib.pyplot as plt
import seaborn as sns

from solarscape_tools.analysis import (
    aggregate_feasibility_series,
    discover_and_validate_experiments,
    feasibility_series_frame,
    load_experiment_frame,
    normalize_filter,
    normalize_scenario_filter,
)
from solarscape_tools.display import add_display_columns, scenario_title
from solarscape_tools.plotting import (
    algorithm_hue_order,
    format_legend,
    palette_for,
    pdf_path,
    save_figure,
)


SCRIPT_DIR = Path(__file__).resolve().parent
TOOLS_DIR = SCRIPT_DIR.parent
DEFAULT_INPUT_DIR = TOOLS_DIR / "out" / "experiments"
DEFAULT_OUTPUT_DIR = TOOLS_DIR / "out" / "thesis_plots" / "feasibility"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate PDF feasible-fraction curves comparing algorithms."
    )
    parser.add_argument("-i", "--input-dir", type=Path, default=DEFAULT_INPUT_DIR)
    parser.add_argument("-o", "--output-dir", type=Path, default=DEFAULT_OUTPUT_DIR)
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
    parser.add_argument("--dpi", type=int, default=150)
    return parser.parse_args()


def plot_group(group, output_path: Path, args: argparse.Namespace) -> None:
    group = add_display_columns(group).sort_values(["algorithm", "generation"])
    if group.empty:
        return

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
                row_group["lower"].clip(lower=0.0, upper=1.0).to_numpy(),
                row_group["upper"].clip(lower=0.0, upper=1.0).to_numpy(),
                color=palette[label],
                alpha=0.14,
                linewidth=0,
            )

    ax.set_xlabel("Generation")
    ax.set_ylabel("Feasible fraction")
    ax.set_ylim(-0.02, 1.02)
    ax.set_title(f"{scenario_title(group['scenario'].iloc[0])}: feasibility")
    format_legend(ax)
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
        feasibility = feasibility_series_frame(frame, args.fuel_tolerance)
        feasibility_summary = aggregate_feasibility_series(feasibility, args.uncertainty)
    except ValueError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2

    written = 0
    for scenario, group in feasibility_summary.groupby("scenario", sort=True):
        output_path = pdf_path(args.output_dir, scenario, "feasibility")
        plot_group(group, output_path, args)
        print(f"saved {output_path}")
        written += 1

    print(f"generated {written} plot(s)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
