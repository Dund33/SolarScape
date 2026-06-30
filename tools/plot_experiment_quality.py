#!/usr/bin/env python3

from __future__ import annotations

import argparse
import csv
import math
import re
from collections import defaultdict
from pathlib import Path

import matplotlib.pyplot as plt

from experiment_quality_data import (
    DEFAULT_CRITERIA,
    AggregatedSeries,
    aggregate_group,
    algorithm_label,
    discover_experiment_files,
    experiment_sort_key,
    group_experiment_files,
    load_run_series,
    parse_criteria,
)
from solarscape_tools.experiments import (
    ALGORITHM_ORDER,
    normalize_filter,
    validate_run_groups,
)


SCRIPT_DIR = Path(__file__).resolve().parent
DEFAULT_INPUT_DIR = SCRIPT_DIR / "out" / "experiments"
DEFAULT_OUTPUT_DIR = SCRIPT_DIR / "out" / "quality_plots"
DEFAULT_DATA_OUTPUT = DEFAULT_OUTPUT_DIR / "quality_series.csv"
UNCERTAINTY_LABELS = {
    "none": "mean",
    "std": "mean +/- sample std",
    "sem": "mean +/- SEM",
    "ci95": "mean +/- 95% CI",
}
SERIES_LABELS = {
    "best-so-far": "best-so-far",
    "current-front": "current Pareto front",
}
ALGORITHM_COLORS = {
    "algo": "tab:blue",
    "nsgaii": "tab:orange",
    "moead": "tab:green",
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Plot thesis-ready convergence curves from SolarScape experiment JSONs. "
            "By default each plot compares algorithms in one scenario using "
            "best-so-far values aggregated across runs."
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
        help=f"CSV file with the plotted series data. Default: {DEFAULT_DATA_OUTPUT}",
    )
    parser.add_argument(
        "--scenario",
        action="append",
        help="Scenario to include, for example scenario1. Can be passed multiple times.",
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
            "Fitness criteria to plot. Default: minimumDistance. "
            "Useful alternatives: "
            + ", ".join(DEFAULT_CRITERIA)
            + "."
        ),
    )
    parser.add_argument(
        "--include-constraint",
        action="store_true",
        help="Also plot fuelConstraintViolation.",
    )
    parser.add_argument(
        "--constraint-mode",
        choices=("prefer-feasible", "feasible-only", "ignore"),
        default="prefer-feasible",
        help=(
            "How to choose one value from a Pareto front. "
            "prefer-feasible uses feasible specimens when available and otherwise "
            "falls back to the smallest fuel violation. Default: prefer-feasible."
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
            "Series to plot. best-so-far shows convergence and keeps the "
            "best value found up to each generation; current-front reproduces "
            "the value from the current generation Pareto front. Default: best-so-far."
        ),
    )
    parser.add_argument(
        "--plot-mode",
        choices=("comparison", "separate"),
        default="comparison",
        help=(
            "comparison overlays algorithms for each scenario/criterion; "
            "separate creates one plot per scenario/algorithm/criterion. "
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


def plot_aggregated_series(
    series: AggregatedSeries,
    output_path: Path,
    uncertainty: str,
    dpi: int,
    series_mode: str,
    log_y: bool,
) -> None:
    x = list(series.generations)
    mean = list(series.mean)
    lower = list(series.lower)
    upper = list(series.upper)
    color = ALGORITHM_COLORS.get(series.algorithm, "tab:blue")

    fig, ax = plt.subplots(figsize=(10, 6))
    ax.plot(
        x,
        mean,
        color=color,
        linewidth=1.8,
        label=UNCERTAINTY_LABELS[uncertainty],
    )

    if uncertainty != "none":
        ax.fill_between(
            x,
            lower,
            upper,
            color=color,
            alpha=0.18,
            linewidth=0,
        )

    ax.set_xlabel("generation")
    ax.set_ylabel(series.criterion.axis_label)
    ax.set_title(
        f"{series.scenario} | {algorithm_label(series.algorithm)} | "
        f"{series.criterion.label} | {SERIES_LABELS[series_mode]} "
        f"({series.run_count} runs)"
    )
    ax.grid(True, alpha=0.3)
    ax.legend()
    apply_y_axis_scale(ax, lower, log_y)
    if ax.get_yscale() != "log":
        ax.ticklabel_format(axis="y", style="sci", scilimits=(-3, 4))

    fig.tight_layout()
    fig.savefig(output_path, dpi=dpi)
    plt.close(fig)


def plot_comparison_series(
    series_list: list[AggregatedSeries],
    output_path: Path,
    uncertainty: str,
    dpi: int,
    series_mode: str,
    log_y: bool,
) -> None:
    if not series_list:
        return

    fig, ax = plt.subplots(figsize=(10, 6))
    finite_lower = []

    for series in sorted(
        series_list,
        key=lambda item: (
            ALGORITHM_ORDER.get(item.algorithm, 100),
            item.algorithm,
        ),
    ):
        x = list(series.generations)
        mean = list(series.mean)
        lower = list(series.lower)
        upper = list(series.upper)
        color = ALGORITHM_COLORS.get(series.algorithm, None)
        label = f"{algorithm_label(series.algorithm)} ({series.run_count} runs)"

        ax.plot(
            x,
            mean,
            color=color,
            linewidth=1.8,
            label=label,
        )

        if uncertainty != "none":
            ax.fill_between(
                x,
                lower,
                upper,
                color=color,
                alpha=0.12,
                linewidth=0,
            )

        finite_lower.extend(
            value
            for value in lower
            if math.isfinite(value)
        )

    first_series = series_list[0]
    ax.set_xlabel("generation")
    ax.set_ylabel(first_series.criterion.axis_label)
    ax.set_title(
        f"{first_series.scenario} | {first_series.criterion.label} | "
        f"{SERIES_LABELS[series_mode]}"
    )
    ax.grid(True, alpha=0.3)
    ax.legend()
    apply_y_axis_scale(ax, finite_lower, log_y)
    if ax.get_yscale() != "log":
        ax.ticklabel_format(axis="y", style="sci", scilimits=(-3, 4))

    fig.tight_layout()
    fig.savefig(output_path, dpi=dpi)
    plt.close(fig)


def apply_y_axis_scale(
    ax,
    lower_values: list[float],
    log_y: bool,
) -> None:
    finite_lower = [
        value
        for value in lower_values
        if math.isfinite(value)
    ]
    if not finite_lower:
        return

    if log_y and min(finite_lower) > 0.0:
        ax.set_yscale("log")
        return

    if min(finite_lower) >= 0.0:
        ax.set_ylim(bottom=0.0)


def safe_file_name(value: str) -> str:
    return re.sub(r"[^a-zA-Z0-9_.-]+", "_", value).strip("_")


def output_path_for(
    output_dir: Path,
    series: AggregatedSeries,
    output_format: str,
    series_mode: str,
) -> Path:
    file_name = "_".join(
        safe_file_name(part)
        for part in (
            series.scenario,
            series.algorithm,
            series.criterion.key,
            series_mode,
        )
    )
    return output_dir / f"{file_name}.{output_format}"


def comparison_output_path_for(
    output_dir: Path,
    scenario: str,
    criterion_key: str,
    series_mode: str,
    output_format: str,
) -> Path:
    file_name = "_".join(
        safe_file_name(part)
        for part in (
            scenario,
            criterion_key,
            series_mode,
            "comparison",
        )
    )
    return output_dir / f"{file_name}.{output_format}"


def write_series_csv(
    output_path: Path,
    series_list: list[AggregatedSeries],
    uncertainty: str,
    series_mode: str,
) -> None:
    output_path.parent.mkdir(parents=True, exist_ok=True)

    with output_path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.writer(handle)
        writer.writerow(
            [
                "scenario",
                "algorithm",
                "criterion",
                "series",
                "uncertainty",
                "generation",
                "mean",
                "lower",
                "upper",
                "finite_run_count",
                "run_count",
            ]
        )

        for series in sorted(
            series_list,
            key=lambda item: (
                item.scenario,
                ALGORITHM_ORDER.get(item.algorithm, 100),
                item.algorithm,
                item.criterion.key,
            ),
        ):
            for generation, mean, lower, upper, count in zip(
                series.generations,
                series.mean,
                series.lower,
                series.upper,
                series.counts,
            ):
                writer.writerow(
                    [
                        series.scenario,
                        algorithm_label(series.algorithm),
                        series.criterion.key,
                        series_mode,
                        uncertainty,
                        generation,
                        mean,
                        lower,
                        upper,
                        count,
                        series.run_count,
                    ]
                )


def main() -> int:
    args = parse_args()

    if args.fuel_tolerance < 0.0:
        print("error: --fuel-tolerance cannot be negative")
        return 2

    try:
        criteria = parse_criteria(
            criteria_keys=args.criteria,
            include_constraint=args.include_constraint,
        )
    except ValueError as error:
        print(f"error: {error}")
        return 2

    experiments = discover_experiment_files(
        input_dir=args.input_dir,
        scenarios=normalize_filter(args.scenario),
        algorithms=normalize_filter(args.algorithm),
    )
    if not experiments:
        print(f"error: no experiment JSON files found in {args.input_dir}")
        return 2

    groups = group_experiment_files(experiments)

    try:
        warnings = validate_run_groups(
            groups=groups,
            expected_runs=args.expected_runs,
            allow_incomplete=args.allow_incomplete,
        )
    except ValueError as error:
        print(f"error: {error}")
        return 2
    for warning in warnings:
        print(f"warning: {warning}")

    args.output_dir.mkdir(parents=True, exist_ok=True)

    aggregated_series = []
    for group_key in sorted(groups, key=lambda key: experiment_sort_key(groups[key][0])):
        group_experiments = groups[group_key]
        run_series = [
            load_run_series(
                experiment=experiment,
                criteria=criteria,
                constraint_mode=args.constraint_mode,
                fuel_tolerance=args.fuel_tolerance,
                series_mode=args.series,
            )
            for experiment in group_experiments
        ]

        for criterion in criteria:
            aggregated = aggregate_group(
                runs=run_series,
                criterion=criterion,
                uncertainty=args.uncertainty,
            )
            aggregated_series.append(aggregated)

    write_series_csv(
        output_path=args.data_output,
        series_list=aggregated_series,
        uncertainty=args.uncertainty,
        series_mode=args.series,
    )
    print(f"saved {args.data_output}")

    written = []
    if args.plot_mode == "separate":
        for aggregated in aggregated_series:
            output_path = output_path_for(
                output_dir=args.output_dir,
                series=aggregated,
                output_format=args.format,
                series_mode=args.series,
            )
            plot_aggregated_series(
                series=aggregated,
                output_path=output_path,
                uncertainty=args.uncertainty,
                dpi=args.dpi,
                series_mode=args.series,
                log_y=args.log_y,
            )
            written.append(output_path)
            print(f"saved {output_path}")
    else:
        by_scenario_and_criterion = defaultdict(list)
        for aggregated in aggregated_series:
            by_scenario_and_criterion[
                (aggregated.scenario, aggregated.criterion.key)
            ].append(aggregated)

        for (scenario, criterion_key), scenario_series in sorted(
            by_scenario_and_criterion.items()):
            output_path = comparison_output_path_for(
                output_dir=args.output_dir,
                scenario=scenario,
                criterion_key=criterion_key,
                series_mode=args.series,
                output_format=args.format,
            )
            plot_comparison_series(
                series_list=scenario_series,
                output_path=output_path,
                uncertainty=args.uncertainty,
                dpi=args.dpi,
                series_mode=args.series,
                log_y=args.log_y,
            )
            written.append(output_path)
            print(f"saved {output_path}")

    print(f"generated {len(written)} plot(s)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
