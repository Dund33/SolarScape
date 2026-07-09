#!/usr/bin/env python3

from __future__ import annotations

import sys
from pathlib import Path
from types import SimpleNamespace

import matplotlib.pyplot as plt
import seaborn as sns
from joblib import Parallel, delayed

from solarscape_tools.analysis import aggregate_feasibility_series, feasibility_series_frame, load_experiment_frame_cache
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
FRAME_CACHE_PATH = TOOLS_DIR / "out" / "thesis_cache" / "experiment_frame.parquet"
OUTPUT_DIR = TOOLS_DIR / "out" / "thesis_plots" / "feasibility"
FEASIBILITY_LABELS = {
    "fuel": "Fuel-feasible Pareto-front fraction [-]",
    "target-window": "Target-window feasible Pareto-front fraction [-]",
    "mission": "Mission-feasible Pareto-front fraction [-]",
}
CONFIG = SimpleNamespace(
    feasibility_mode="mission",
    fuel_tolerance=0.0,
    target_tolerance=0.0,
    uncertainty="iqr",
    output_dir=OUTPUT_DIR,
    dpi=300,
    jobs=12,
)


def plot_group(group, output_path: Path, args: SimpleNamespace) -> None:
    group = add_display_columns(group).sort_values(["algorithm", "generation"])
    if group.empty:
        return

    fig, ax = plt.subplots(figsize=(7.6, 5.0))
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
        linewidth=2.0,
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
    ax.set_ylabel(FEASIBILITY_LABELS[args.feasibility_mode])
    ax.set_ylim(-0.02, 1.02)
    ax.set_title(
        f"{scenario_title(group['scenario'].iloc[0])}: "
        f"{args.feasibility_mode} feasibility"
    )
    format_legend(ax)
    ax.grid(axis="x", visible=False)
    ax.grid(axis="y", color="0.86", linewidth=0.8)
    save_figure(fig, output_path, args.dpi)


def main() -> int:
    args = CONFIG
    sns.set_theme(style="whitegrid", context="notebook", font_scale=1.15)

    try:
        frame = load_experiment_frame_cache(FRAME_CACHE_PATH)
        feasibility = feasibility_series_frame(
            frame,
            args.fuel_tolerance,
            args.feasibility_mode,
            args.target_tolerance,
        )
        feasibility_summary = aggregate_feasibility_series(feasibility, args.uncertainty)
    except ValueError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2

    tasks = []
    for scenario, group in feasibility_summary.groupby("scenario", sort=True):
        output_path = pdf_path(args.output_dir, scenario, "feasibility")
        tasks.append((group.copy(), output_path))

    Parallel(n_jobs=args.jobs)(
        delayed(plot_group)(group, output_path, args)
        for group, output_path in tasks
    )
    for _, output_path in tasks:
        print(f"saved {output_path}")

    print(f"generated {len(tasks)} plot(s)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
