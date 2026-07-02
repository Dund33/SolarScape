#!/usr/bin/env python3

from __future__ import annotations

import argparse
import math
import re
from pathlib import Path

import matplotlib.pyplot as plt
import pandas as pd
import seaborn as sns

from solarscape_tools.analysis import (
    aggregate_feasibility_series,
    aggregate_quality_series,
    best_values_by_generation,
    discover_and_validate_experiments,
    feasibility_series_frame,
    final_pareto_points_frame,
    final_solution_summary_frame,
    load_experiment_frame,
    normalize_filter,
    normalize_scenario_filter,
    parse_criteria,
)
from solarscape_tools.experiments import ALGORITHM_ORDER, algorithm_label
from solarscape_tools.pareto import CRITERIA, DEFAULT_CRITERIA


SCRIPT_DIR = Path(__file__).resolve().parent
DEFAULT_INPUT_DIR = SCRIPT_DIR / "out" / "experiments"
DEFAULT_OUTPUT_DIR = SCRIPT_DIR / "out" / "quality_plots"
DEFAULT_DATA_OUTPUT = DEFAULT_OUTPUT_DIR / "quality_series.csv"

SERIES_LABELS = {
    "best-so-far": "best-so-far",
    "current-front": "current Pareto front",
}
UNCERTAINTY_LABELS = {
    "none": "mean",
    "std": "mean +/- sample std",
    "sem": "mean +/- SEM",
    "ci95": "mean +/- 95% CI",
}
ALGORITHM_PALETTE = {
    "Proposed algorithm": "#1f77b4",
    "NSGA-II": "#ff7f0e",
    "MOEA/D": "#2ca02c",
}
AXIS_LABELS = {
    "minimumDistance": "Minimum distance [m]",
    "fuelUsed": "Fuel used [kg]",
    "minimumDistanceTime": "Time at minimum distance [s]",
    "fuelConstraintViolation": "Fuel constraint violation [kg]",
}
METRIC_LABELS = {
    "minimumDistance": "Minimum distance",
    "fuelUsed": "Fuel used",
    "minimumDistanceTime": "Time at minimum distance",
    "fuelConstraintViolation": "Fuel constraint violation",
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Generate thesis-ready quality plots from SolarScape experiment JSONs. "
            "The default output compares Proposed algorithm, NSGA-II, and MOEA/D "
            "with convergence, final-run distribution, feasibility, and final "
            "Pareto-front figures."
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
        "--output-dir",
        type=Path,
        default=DEFAULT_OUTPUT_DIR,
        help=f"Directory where plots will be written. Default: {DEFAULT_OUTPUT_DIR}",
    )
    parser.add_argument(
        "--data-output",
        type=Path,
        default=DEFAULT_DATA_OUTPUT,
        help=f"CSV file with the plotted convergence series. Default: {DEFAULT_DATA_OUTPUT}",
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
        default=["minimumDistance"],
        help=(
            "Fitness criteria to plot as convergence curves. Default: minimumDistance. "
            "Useful alternatives: "
            + ", ".join(DEFAULT_CRITERIA)
            + "."
        ),
    )
    parser.add_argument(
        "--include-constraint",
        action="store_true",
        help="Also plot fuelConstraintViolation as a convergence criterion.",
    )
    parser.add_argument(
        "--constraint-mode",
        choices=("prefer-feasible", "feasible-only", "ignore"),
        default="prefer-feasible",
        help=(
            "How to choose one value from a Pareto front. prefer-feasible uses "
            "feasible specimens when available and otherwise falls back to the "
            "smallest fuel violation. Default: prefer-feasible."
        ),
    )
    parser.add_argument(
        "--fuel-tolerance",
        type=float,
        default=0.0,
        help="Maximum fuelConstraintViolation treated as feasible. Default: 0.",
    )
    parser.add_argument(
        "--uncertainty",
        choices=("none", "std", "sem", "ci95"),
        default="std",
        help="Uncertainty band computed across runs. Default: std.",
    )
    parser.add_argument(
        "--series",
        choices=("best-so-far", "current-front"),
        default="best-so-far",
        help=(
            "Series to plot. best-so-far shows convergence and keeps the best "
            "value found up to each generation; current-front reproduces the "
            "value from the current generation Pareto front. Default: best-so-far."
        ),
    )
    parser.add_argument(
        "--plot-mode",
        choices=("comparison", "separate"),
        default="comparison",
        help=(
            "comparison overlays algorithms for each scenario/criterion; "
            "separate creates one convergence plot per scenario/algorithm/criterion. "
            "Default: comparison."
        ),
    )
    parser.add_argument(
        "--log-y",
        action="store_true",
        help="Use a logarithmic y-axis when all plotted values are positive.",
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
        help="Generate plots even when a group has fewer than --expected-runs files.",
    )
    parser.add_argument(
        "--format",
        choices=("pdf", "png", "svg"),
        default="pdf",
        help="Output image format. Default: pdf.",
    )
    parser.add_argument(
        "--dpi",
        type=int,
        default=150,
        help="DPI used for raster formats. Default: 150.",
    )
    return parser.parse_args()


def add_display_columns(frame: pd.DataFrame) -> pd.DataFrame:
    result = frame.copy()
    if "scenario" in result:
        result["scenario_label"] = result["scenario"].map(scenario_title)
    if "algorithm" in result:
        result["algorithm_label"] = result["algorithm"].map(algorithm_label)
    if "criterion" in result:
        result["criterion_label"] = result["criterion"].map(metric_label)
    return result


def metric_label(criterion: str) -> str:
    if criterion in METRIC_LABELS:
        return METRIC_LABELS[criterion]
    return CRITERIA[criterion].label


def axis_label(criterion: str) -> str:
    if criterion in AXIS_LABELS:
        return AXIS_LABELS[criterion]
    return CRITERIA[criterion].axis_label


def scenario_title(scenario: str) -> str:
    suffix = scenario.removeprefix("scenario")
    return f"Scenario {suffix}" if suffix else scenario


def algorithm_hue_order(frame: pd.DataFrame) -> list[str]:
    algorithms = sorted(
        frame["algorithm"].dropna().unique(),
        key=lambda value: (ALGORITHM_ORDER.get(value, 100), value),
    )
    return [algorithm_label(algorithm) for algorithm in algorithms]


def palette_for(frame: pd.DataFrame) -> dict[str, str]:
    return {
        label: ALGORITHM_PALETTE.get(label, "0.35")
        for label in algorithm_hue_order(frame)
    }


def plot_convergence_group(
    group: pd.DataFrame,
    output_path: Path,
    uncertainty: str,
    series_mode: str,
    log_y: bool,
    dpi: int,
) -> None:
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

    if uncertainty != "none":
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
        f"{scenario_title(first['scenario'])}: {metric_label(first['criterion'])} "
        f"convergence ({SERIES_LABELS[series_mode]})"
    )
    format_legend(ax)
    apply_y_axis_scale(ax, group[["lower", "mean", "upper"]], log_y)
    style_numeric_axis(ax)
    save_figure(fig, output_path, dpi)


def plot_final_distribution(
    group: pd.DataFrame,
    output_path: Path,
    log_y: bool,
    dpi: int,
) -> None:
    group = add_display_columns(group).dropna(subset=["best_so_far_distance"])
    if group.empty:
        return

    fig, ax = plt.subplots(figsize=(6.4, 4.0))
    order = algorithm_hue_order(group)
    palette = palette_for(group)

    sns.boxplot(
        data=group,
        x="algorithm_label",
        y="best_so_far_distance",
        order=order,
        palette=palette,
        showfliers=False,
        width=0.55,
        ax=ax,
    )
    sns.stripplot(
        data=group,
        x="algorithm_label",
        y="best_so_far_distance",
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
    ax.set_ylabel("Minimum distance [m]")
    ax.set_title(f"{scenario_title(group['scenario'].iloc[0])}: final run spread")
    apply_y_axis_scale(ax, group[["best_so_far_distance"]], log_y)
    style_numeric_axis(ax)
    save_figure(fig, output_path, dpi)


def plot_feasibility_group(
    group: pd.DataFrame,
    output_path: Path,
    uncertainty: str,
    dpi: int,
) -> None:
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

    if uncertainty != "none":
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
    ax.set_title(f"{scenario_title(group['scenario'].iloc[0])}: feasibility rate")
    format_legend(ax)
    save_figure(fig, output_path, dpi)


def plot_final_pareto(
    group: pd.DataFrame,
    output_path: Path,
    log_y: bool,
    dpi: int,
) -> None:
    group = add_display_columns(group).dropna(
        subset=["minimumDistance", "fuelUsed"]
    )
    if group.empty:
        return

    plot_data = group
    fig, ax = plt.subplots(figsize=(6.4, 4.0))
    hue_order = algorithm_hue_order(plot_data)
    palette = palette_for(plot_data)

    sns.scatterplot(
        data=plot_data,
        x="minimumDistance",
        y="fuelUsed",
        hue="algorithm_label",
        hue_order=hue_order,
        palette=palette,
        alpha=0.62,
        s=22,
        linewidth=0,
        ax=ax,
    )

    ax.set_xlabel("Minimum distance [m]")
    ax.set_ylabel("Fuel used [kg]")
    ax.set_title(f"{scenario_title(group['scenario'].iloc[0])}: final Pareto front")
    format_legend(ax)
    apply_y_axis_scale(ax, plot_data[["fuelUsed"]], log_y)
    style_numeric_axis(ax)
    save_figure(fig, output_path, dpi)


def apply_y_axis_scale(ax, values: pd.DataFrame, log_y: bool) -> None:
    finite = [
        float(value)
        for value in values.to_numpy().ravel()
        if pd.notna(value) and math.isfinite(float(value))
    ]
    if not finite:
        return
    if log_y and min(finite) > 0.0:
        ax.set_yscale("log")
    elif min(finite) >= 0.0:
        ax.set_ylim(bottom=0.0)


def style_numeric_axis(ax) -> None:
    if ax.get_yscale() != "log":
        ax.ticklabel_format(axis="y", style="sci", scilimits=(-3, 4))


def format_legend(ax) -> None:
    legend = ax.get_legend()
    if legend is not None:
        legend.set_title("")


def save_figure(fig, output_path: Path, dpi: int) -> None:
    output_path.parent.mkdir(parents=True, exist_ok=True)
    fig.tight_layout()
    fig.savefig(output_path, dpi=dpi)
    plt.close(fig)


def convergence_output_path(
    output_dir: Path,
    scenario: str,
    criterion: str,
    series_mode: str,
    output_format: str,
    algorithm: str | None = None,
) -> Path:
    parts = [scenario]
    if algorithm is not None:
        parts.append(algorithm)
    parts.append(criterion)
    if series_mode != "best-so-far":
        parts.append(series_mode)
    parts.append("convergence")
    if algorithm is None:
        parts.append("comparison")
    return output_dir / f"{'_'.join(safe_file_name(part) for part in parts)}.{output_format}"


def scenario_output_path(
    output_dir: Path,
    scenario: str,
    suffix: str,
    output_format: str,
) -> Path:
    return output_dir / f"{safe_file_name(scenario)}_{suffix}.{output_format}"


def safe_file_name(value: str) -> str:
    return re.sub(r"[^a-zA-Z0-9_.-]+", "_", value).strip("_")


def write_csv(frame: pd.DataFrame, output_path: Path) -> None:
    output_path.parent.mkdir(parents=True, exist_ok=True)
    add_display_columns(frame).to_csv(output_path, index=False)
    print(f"saved {output_path}")


def write_quality_series_csv(
    frame: pd.DataFrame,
    output_path: Path,
    uncertainty: str,
    series_mode: str,
) -> None:
    output = frame.copy()
    output.insert(3, "series", series_mode)
    output.insert(4, "uncertainty", uncertainty)
    write_csv(output, output_path)


def main() -> int:
    args = parse_args()
    sns.set_theme(style="whitegrid", context="paper")

    if args.fuel_tolerance < 0.0:
        print("error: --fuel-tolerance cannot be negative")
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
        final_summary = final_solution_summary_frame(
            frame=frame,
            constraint_mode=args.constraint_mode,
            fuel_tolerance=args.fuel_tolerance,
        )
        feasibility = feasibility_series_frame(frame, args.fuel_tolerance)
        feasibility_series = aggregate_feasibility_series(feasibility, args.uncertainty)
        pareto_points = final_pareto_points_frame(frame, args.fuel_tolerance)
    except ValueError as error:
        print(f"error: {error}")
        return 2

    args.output_dir.mkdir(parents=True, exist_ok=True)
    write_quality_series_csv(
        quality_series,
        args.data_output,
        uncertainty=args.uncertainty,
        series_mode=args.series,
    )
    write_csv(final_summary, args.output_dir / "final_solution_summary.csv")
    write_csv(feasibility_series, args.output_dir / "feasibility_series.csv")
    write_csv(pareto_points, args.output_dir / "final_pareto_points.csv")

    written = []
    convergence_groups = ["scenario", "criterion"]
    if args.plot_mode == "separate":
        convergence_groups.append("algorithm")

    for group_key, group in quality_series.groupby(convergence_groups, sort=True):
        if args.plot_mode == "separate":
            scenario, criterion, algorithm = group_key
        else:
            scenario, criterion = group_key
            algorithm = None

        output_path = convergence_output_path(
            output_dir=args.output_dir,
            scenario=scenario,
            criterion=criterion,
            algorithm=algorithm,
            series_mode=args.series,
            output_format=args.format,
        )
        plot_convergence_group(
            group=group,
            output_path=output_path,
            uncertainty=args.uncertainty,
            series_mode=args.series,
            log_y=args.log_y,
            dpi=args.dpi,
        )
        written.append(output_path)
        print(f"saved {output_path}")

    for scenario, group in final_summary.groupby("scenario", sort=True):
        output_path = scenario_output_path(
            args.output_dir,
            scenario,
            "final_minimumDistance_distribution",
            args.format,
        )
        plot_final_distribution(group, output_path, args.log_y, args.dpi)
        written.append(output_path)
        print(f"saved {output_path}")

    for scenario, group in feasibility_series.groupby("scenario", sort=True):
        output_path = scenario_output_path(
            args.output_dir,
            scenario,
            "feasibility_rate",
            args.format,
        )
        plot_feasibility_group(group, output_path, args.uncertainty, args.dpi)
        written.append(output_path)
        print(f"saved {output_path}")

    for scenario, group in pareto_points.groupby("scenario", sort=True):
        output_path = scenario_output_path(
            args.output_dir,
            scenario,
            "final_pareto_distance_fuel",
            args.format,
        )
        plot_final_pareto(group, output_path, args.log_y, args.dpi)
        written.append(output_path)
        print(f"saved {output_path}")

    print(f"generated {len(written)} plot(s)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
