#!/usr/bin/env python3

from __future__ import annotations

import sys
from pathlib import Path
from types import SimpleNamespace

import matplotlib.pyplot as plt
import seaborn as sns
from joblib import Parallel, delayed

from solarscape_tools.analysis import (
    aggregate_quality_series,
    best_values_by_generation,
    load_experiment_frame_cache,
    parse_criteria,
)
from solarscape_tools.display import (
    add_display_columns,
    axis_label,
    metric_label,
    scenario_title,
)
from solarscape_tools.plotting import algorithm_hue_order, apply_y_axis_scale, palette_for, pdf_path, save_figure, style_numeric_axis


SCRIPT_DIR = Path(__file__).resolve().parent
TOOLS_DIR = SCRIPT_DIR.parent
FRAME_CACHE_PATH = TOOLS_DIR / "out" / "thesis_cache" / "experiment_frame.parquet"
OUTPUT_DIR = TOOLS_DIR / "out" / "thesis_plots" / "convergence"
LOG_Y_FLOOR = 1.0
AUTO_LOG_CRITERIA = {"targetWindowViolation"}
CONFIG = SimpleNamespace(
    criteria=("targetWindowViolation",),
    include_constraint=False,
    constraint_mode="prefer-feasible",
    fuel_tolerance=0.0,
    series="best-so-far",
    uncertainty="iqr",
    log_y=True,
    output_dir=OUTPUT_DIR,
    dpi=300,
    jobs=12,
)


def central_column(args: SimpleNamespace) -> str:
    return "median" if args.uncertainty == "iqr" else "mean"


def use_log_y(args: SimpleNamespace, criterion: str) -> bool:
    return args.log_y or criterion in AUTO_LOG_CRITERIA


def plot_value_columns(group, y_column: str, log_y: bool):
    result = group.copy()
    result["central_plot"] = result[y_column]
    result["lower_plot"] = result["lower"]
    result["upper_plot"] = result["upper"]
    if log_y:
        for column in ("central_plot", "lower_plot", "upper_plot"):
            result[column] = result[column].clip(lower=LOG_Y_FLOOR)
    return result


def convergence_legend_kwargs(scenario: str, criterion: str, hue_count: int) -> dict:
    if criterion == "targetWindowViolation" and scenario == "scenario1":
        return {
            "loc": "upper center",
            "bbox_to_anchor": (0.5, -0.22),
            "ncol": hue_count,
        }
    if criterion == "targetWindowViolation" and scenario == "scenario2":
        return {
            "loc": "lower left",
            "bbox_to_anchor": (0.03, 0.04),
            "ncol": 1,
        }
    if criterion == "targetWindowViolation" and scenario == "scenario3":
        return {
            "loc": "upper right",
            "bbox_to_anchor": (0.97, 0.97),
            "ncol": 1,
        }
    return {
        "loc": "lower center",
        "bbox_to_anchor": (0.5, 1.02),
        "ncol": hue_count,
    }


def plot_group(group, output_path: Path, args: SimpleNamespace) -> None:
    group = add_display_columns(group).sort_values(["algorithm", "generation"])
    if group.empty:
        return

    first = group.iloc[0]
    fig, ax = plt.subplots(figsize=(7.6, 5.0))
    hue_order = algorithm_hue_order(group)
    palette = palette_for(group)
    y_column = central_column(args)
    log_y = use_log_y(args, first["criterion"])
    group = plot_value_columns(group, y_column, log_y)

    sns.lineplot(
        data=group,
        x="generation",
        y="central_plot",
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
                row_group["lower_plot"].to_numpy(),
                row_group["upper_plot"].to_numpy(),
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
    legend_kwargs = convergence_legend_kwargs(first["scenario"], first["criterion"], len(hue_order))
    ax.legend(
        title="",
        frameon=True,
        borderaxespad=0.0,
        **legend_kwargs,
    )
    apply_y_axis_scale(ax, group[["lower_plot", "central_plot", "upper_plot"]], log_y)
    style_numeric_axis(ax)
    ax.grid(axis="x", visible=False)
    ax.grid(axis="y", color="0.86", linewidth=0.8)
    save_figure(fig, output_path, args.dpi, bbox_inches="tight")


def main() -> int:
    args = CONFIG
    sns.set_theme(style="whitegrid", context="notebook", font_scale=1.15)

    try:
        criteria = parse_criteria(args.criteria, args.include_constraint)
        frame = load_experiment_frame_cache(FRAME_CACHE_PATH)
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

    tasks = []
    for (scenario, criterion), group in quality_series.groupby(
        ["scenario", "criterion"],
        sort=True,
    ):
        parts = [scenario, criterion, "convergence"]
        if args.series != "best-so-far":
            parts.insert(2, args.series)
        output_path = pdf_path(args.output_dir, *parts)
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
