#!/usr/bin/env python3

from __future__ import annotations

import sys
from pathlib import Path
from types import SimpleNamespace

import matplotlib.pyplot as plt
import numpy as np
import seaborn as sns
from joblib import Parallel, delayed
from matplotlib.ticker import MaxNLocator

from solarscape_tools.analysis import (
    final_pareto_points_frame,
    load_experiment_frame_cache,
)
from solarscape_tools.display import add_display_columns, axis_label, scenario_title
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
FRAME_CACHE_PATH = TOOLS_DIR / "out" / "thesis_cache" / "experiment_frame.parquet"
OUTPUT_DIR = TOOLS_DIR / "out" / "thesis_plots" / "final_pareto"
CONFIG = SimpleNamespace(
    x="targetWindowViolation",
    y="fuelUsed",
    fuel_tolerance=0.0,
    feasible_only=False,
    mission_feasible_only=True,
    target_tolerance=0.0,
    kind="fuel-histogram",
    hist_bins="auto",
    log_x=False,
    log_y=False,
    output_dir=OUTPUT_DIR,
    dpi=300,
    jobs=12,
)


def plot_group(group, output_path: Path, args: SimpleNamespace) -> None:
    group = add_display_columns(group)
    if args.kind == "fuel-distribution":
        plot_fuel_distribution_group(group, output_path, args)
        return
    if args.kind == "fuel-histogram":
        plot_fuel_histogram_group(group, output_path, args)
        return

    group = group.dropna(subset=[args.x, args.y])
    if group.empty:
        return

    fig, ax = plt.subplots(figsize=(7.6, 5.0))
    hue_order = algorithm_hue_order(group)
    palette = palette_for(group)

    sns.scatterplot(
        data=group,
        x=args.x,
        y=args.y,
        hue="algorithm_label",
        hue_order=hue_order,
        palette=palette,
        alpha=0.72,
        s=42,
        linewidth=0.35,
        edgecolor="white",
        ax=ax,
    )

    ax.set_xlabel(axis_label(args.x))
    ax.set_ylabel(axis_label(args.y))
    ax.set_title(f"{scenario_title(group['scenario'].iloc[0])}: final Pareto front")
    format_legend(ax)
    apply_x_axis_scale(ax, group[[args.x]], args.log_x)
    apply_y_axis_scale(ax, group[[args.y]], args.log_y)
    style_numeric_axis(ax)
    ax.grid(axis="x", color="0.88", linewidth=0.8)
    ax.grid(axis="y", color="0.88", linewidth=0.8)
    save_figure(fig, output_path, args.dpi)


def mission_feasible_points(group, args: SimpleNamespace):
    group = group.dropna(subset=["fuelUsed"])
    if not args.mission_feasible_only:
        return group

    return group[
        group["fuelViolation"].le(args.fuel_tolerance)
        & group["targetWindowViolation"].le(args.target_tolerance)
    ].copy()


def plot_fuel_distribution_group(group, output_path: Path, args: SimpleNamespace) -> None:
    original_group = group.copy()
    group = mission_feasible_points(group, args)

    fig, ax = plt.subplots(figsize=(7.6, 5.0))
    hue_order = algorithm_hue_order(original_group)
    palette = palette_for(original_group)

    if group.empty:
        ax.text(
            0.5,
            0.5,
            "No mission-feasible final Pareto front points",
            ha="center",
            va="center",
            transform=ax.transAxes,
        )
        ax.set_xticks(range(len(hue_order)))
        ax.set_xticklabels(hue_order)
    else:
        sns.boxplot(
            data=group,
            x="algorithm_label",
            hue="algorithm_label",
            y="fuelUsed",
            order=hue_order,
            hue_order=hue_order,
            palette=palette,
            legend=False,
            showfliers=False,
            showmeans=True,
            width=0.55,
            linewidth=1.3,
            meanprops={
                "marker": "D",
                "markerfacecolor": "white",
                "markeredgecolor": "black",
                "markersize": 4.5,
            },
            medianprops={"color": "black", "linewidth": 1.4},
            whiskerprops={"linewidth": 1.1},
            capprops={"linewidth": 1.1},
            ax=ax,
        )
        sns.stripplot(
            data=group,
            x="algorithm_label",
            y="fuelUsed",
            order=hue_order,
            jitter=0.18,
            color="0.12",
            size=2.4,
            alpha=0.24,
            linewidth=0.25,
            edgecolor="white",
            ax=ax,
        )

        counts = group.groupby("algorithm_label", sort=False)["fuelUsed"].count().reindex(hue_order).fillna(0).astype(int)
        ax.set_xticks(range(len(hue_order)))
        ax.set_xticklabels([f"{label}\n(n={counts[label]})" for label in hue_order])

    ax.set_xlabel("")
    ax.set_ylabel(axis_label("fuelUsed"))
    title_suffix = "mission-feasible final Pareto front points" if args.mission_feasible_only else "final Pareto front points"
    ax.set_title(f"{scenario_title(original_group['scenario'].iloc[0])}: fuel used in {title_suffix}")
    apply_y_axis_scale(ax, group[["fuelUsed"]] if not group.empty else original_group[["fuelUsed"]], args.log_y)
    style_numeric_axis(ax)
    ax.grid(axis="x", visible=False)
    ax.grid(axis="y", color="0.86", linewidth=0.8)
    save_figure(fig, output_path, args.dpi)


def histogram_bins(values, requested_bins: str):
    if requested_bins.isdigit():
        return int(requested_bins)
    if values.empty or values.nunique() <= 1:
        return 5
    return np.histogram_bin_edges(values.to_numpy(), bins=requested_bins)


def plot_fuel_histogram_group(group, output_path: Path, args: SimpleNamespace) -> None:
    original_group = group.copy()
    group = mission_feasible_points(group, args)
    hue_order = algorithm_hue_order(original_group)
    palette = palette_for(original_group)

    fig, axes = plt.subplots(1, len(hue_order), figsize=(12.6, 4.8), sharex=True, sharey=True)
    if len(hue_order) == 1:
        axes = [axes]

    bins = histogram_bins(group["fuelUsed"], args.hist_bins)
    max_count = 0
    if not group.empty:
        for label in hue_order:
            values = group[group["algorithm_label"].eq(label)]["fuelUsed"]
            if not values.empty:
                counts, _ = np.histogram(values.to_numpy(), bins=bins)
                max_count = max(max_count, int(counts.max(initial=0)))
    y_limit = max(1, int(np.ceil(max_count * 1.12)))

    for ax, label in zip(axes, hue_order):
        algorithm_group = group[group["algorithm_label"].eq(label)]
        color = palette[label]

        if algorithm_group.empty:
            ax.text(
                0.5,
                0.5,
                "no mission-feasible\nfront points",
                ha="center",
                va="center",
                transform=ax.transAxes,
                fontsize=9.5,
            )
        else:
            sns.histplot(
                data=algorithm_group,
                x="fuelUsed",
                bins=bins,
                color=color,
                edgecolor="white",
                linewidth=0.5,
                alpha=0.82,
                ax=ax,
            )

        ax.set_title(f"{label}\n(n={len(algorithm_group)})", fontsize=10.5)
        ax.set_xlabel("")
        ax.set_ylabel("Points [specimens]")
        ax.set_ylim(0, y_limit)
        ax.yaxis.set_major_locator(MaxNLocator(integer=True))
        ax.tick_params(axis="y", labelleft=True)
        ax.grid(axis="x", color="0.90", linewidth=0.7)
        ax.grid(axis="y", color="0.88", linewidth=0.7)
        style_numeric_axis(ax)

    fig.suptitle(
        f"{scenario_title(original_group['scenario'].iloc[0])}: fuel used in "
        f"{'mission-feasible final Pareto front points' if args.mission_feasible_only else 'final Pareto front points'}",
        fontsize=12,
    )
    fig.supxlabel(axis_label("fuelUsed"), fontsize=10.5)
    save_figure(fig, output_path, args.dpi)


def main() -> int:
    args = CONFIG
    sns.set_theme(style="whitegrid", context="notebook", font_scale=1.15)

    try:
        frame = load_experiment_frame_cache(FRAME_CACHE_PATH)
        points = final_pareto_points_frame(
            frame=frame,
            fuel_tolerance=args.fuel_tolerance,
            feasible_only=args.feasible_only,
        )
    except ValueError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2

    tasks = []
    for scenario, group in points.groupby("scenario", sort=True):
        if args.kind == "fuel-histogram":
            output_path = pdf_path(args.output_dir, scenario, "final_pareto_fuelUsed_histogram")
        elif args.kind == "fuel-distribution":
            output_path = pdf_path(args.output_dir, scenario, "final_pareto_fuelUsed_distribution")
        else:
            output_path = pdf_path(args.output_dir, scenario, args.x, args.y, "pareto")
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
