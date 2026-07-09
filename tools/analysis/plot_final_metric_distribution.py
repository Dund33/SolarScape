#!/usr/bin/env python3

from __future__ import annotations

import sys
from pathlib import Path
from types import SimpleNamespace

import matplotlib.pyplot as plt
import seaborn as sns
from joblib import Parallel, delayed

from solarscape_tools.analysis import (
    final_solution_summary_frame,
    load_experiment_frame_cache,
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
FRAME_CACHE_PATH = TOOLS_DIR / "out" / "thesis_cache" / "experiment_frame.parquet"
OUTPUT_DIR = TOOLS_DIR / "out" / "thesis_plots" / "final_metric_distribution"
DEFAULT_BOXPLOT_METRICS = (
    "best_so_far_distance",
    "final_mission_feasible_rate",
    "fuel_used_at_best_distance",
    "best_so_far_generation",
)
AUTO_LOG_METRICS = {
    "final_front_size",
    "final_min_distance",
    "best_so_far_distance",
}
FRACTION_METRICS = {
    "final_feasible_rate",
    "final_target_feasible_rate",
    "final_mission_feasible_rate",
}
COUNT_DOT_METRICS = {
    "final_front_size",
}
CONFIG = SimpleNamespace(
    metric="all",
    constraint_mode="prefer-feasible",
    fuel_tolerance=0.0,
    log_y=False,
    auto_log_y=True,
    output_dir=OUTPUT_DIR,
    dpi=300,
    jobs=12,
)


def selected_metrics(args: SimpleNamespace) -> tuple[str, ...]:
    if args.metric == "all":
        return DEFAULT_BOXPLOT_METRICS
    return (args.metric,)


def use_log_y(args: SimpleNamespace, metric: str) -> bool:
    return args.log_y or (args.auto_log_y and metric in AUTO_LOG_METRICS)


def plot_group(
    group,
    output_path: Path,
    metric: str,
    args: SimpleNamespace,
) -> None:
    group = add_display_columns(group).dropna(subset=[metric])
    if group.empty:
        return

    fig, ax = plt.subplots(figsize=(7.6, 5.0))
    order = algorithm_hue_order(group)
    palette = palette_for(group)

    if metric in FRACTION_METRICS:
        plot_fraction_group(group, ax, metric, order, palette)
        ax.set_xlabel("")
        ax.set_ylabel(axis_label(metric))
        ax.set_ylim(-0.03, 1.08)
        ax.set_title(f"{scenario_title(group['scenario'].iloc[0])}: {metric_label(metric)}")
        ax.grid(axis="x", visible=False)
        ax.grid(axis="y", color="0.86", linewidth=0.8)
        save_figure(fig, output_path, args.dpi)
        return

    if metric in COUNT_DOT_METRICS:
        plot_count_group(group, ax, metric, order, palette, use_log_y(args, metric))
        ax.set_xlabel("")
        ax.set_ylabel(axis_label(metric))
        ax.set_title(f"{scenario_title(group['scenario'].iloc[0])}: {metric_label(metric)}")
        ax.grid(axis="x", visible=False)
        ax.grid(axis="y", color="0.86", linewidth=0.8)
        save_figure(fig, output_path, args.dpi)
        return

    sns.boxplot(
        data=group,
        x="algorithm_label",
        hue="algorithm_label",
        y=metric,
        order=order,
        hue_order=order,
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
        y=metric,
        order=order,
        jitter=0.16,
        color="0.15",
        size=4.2,
        alpha=0.62,
        linewidth=0.4,
        edgecolor="white",
        ax=ax,
    )

    ax.set_xlabel("")
    ax.set_ylabel(axis_label(metric))
    ax.set_title(f"{scenario_title(group['scenario'].iloc[0])}: {metric_label(metric)}")
    apply_y_axis_scale(ax, group[[metric]], use_log_y(args, metric))
    style_numeric_axis(ax)
    ax.grid(axis="x", visible=False)
    ax.grid(axis="y", color="0.86", linewidth=0.8)
    save_figure(fig, output_path, args.dpi)


def plot_fraction_group(group, ax, metric: str, order: list[str], palette: dict[str, str]) -> None:
    means = group.groupby("algorithm_label", sort=False)[metric].mean().reindex(order)
    positions = list(range(len(order)))
    colors = [palette[label] for label in order]

    ax.bar(
        positions,
        means.fillna(0.0).to_numpy(),
        color=colors,
        edgecolor="0.2",
        linewidth=0.9,
        width=0.58,
        alpha=0.82,
        zorder=2,
    )
    sns.stripplot(
        data=group,
        x="algorithm_label",
        y=metric,
        order=order,
        jitter=0.13,
        color="0.15",
        size=4.4,
        alpha=0.72,
        linewidth=0.4,
        edgecolor="white",
        ax=ax,
        zorder=3,
    )

    counts = group.groupby("algorithm_label", sort=False)[metric].count().reindex(order).fillna(0).astype(int)
    ax.set_xticks(positions)
    ax.set_xticklabels([f"{label}\n(n={counts[label]})" for label in order])

    for position, label in zip(positions, order):
        mean = means[label]
        if mean != mean:
            continue
        y = min(1.04, mean + 0.04)
        ax.text(position, y, f"{mean:.2f}", ha="center", va="bottom", fontsize=9.5)


def plot_count_group(group, ax, metric: str, order: list[str], palette: dict[str, str], log_y: bool) -> None:
    counts = group.groupby("algorithm_label", sort=False)[metric].count().reindex(order).fillna(0).astype(int)
    means = group.groupby("algorithm_label", sort=False)[metric].mean().reindex(order)
    positions = list(range(len(order)))

    sns.stripplot(
        data=group,
        x="algorithm_label",
        y=metric,
        order=order,
        jitter=0.14,
        color="0.18",
        size=4.6,
        alpha=0.68,
        linewidth=0.4,
        edgecolor="white",
        ax=ax,
        zorder=2,
    )

    for position, label in zip(positions, order):
        mean = means[label]
        if mean != mean:
            continue
        ax.scatter(
            [position],
            [mean],
            marker="D",
            s=56,
            facecolors="white",
            edgecolors=palette[label],
            linewidths=1.5,
            zorder=4,
        )
        ax.text(position, mean * 1.10 if log_y else mean + 0.6, f"{mean:.2f}", ha="center", va="bottom", fontsize=9.5)

    ax.set_xticks(positions)
    ax.set_xticklabels([f"{label}\n(n={counts[label]})" for label in order])
    if log_y:
        finite = group[metric].dropna()
        if not finite.empty and finite.min() > 0:
            ax.set_yscale("log")
            ax.set_ylim(max(0.8, finite.min() * 0.75), finite.max() * 1.8)
            return

    apply_y_axis_scale(ax, group[[metric]], log_y)
    style_numeric_axis(ax)


def main() -> int:
    args = CONFIG
    sns.set_theme(style="whitegrid", context="notebook", font_scale=1.15)

    try:
        frame = load_experiment_frame_cache(FRAME_CACHE_PATH)
        metrics = final_solution_summary_frame(
            frame=frame,
            constraint_mode=args.constraint_mode,
            fuel_tolerance=args.fuel_tolerance,
        )
    except ValueError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2

    tasks = []
    for metric in selected_metrics(args):
        for scenario, group in metrics.groupby("scenario", sort=True):
            output_path = pdf_path(args.output_dir, scenario, metric, "distribution")
            tasks.append((group.copy(), output_path, metric))

    Parallel(n_jobs=args.jobs)(
        delayed(plot_group)(group, output_path, metric, args)
        for group, output_path, metric in tasks
    )
    for _, output_path, _ in tasks:
        print(f"saved {output_path}")

    print(f"generated {len(tasks)} plot(s)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
