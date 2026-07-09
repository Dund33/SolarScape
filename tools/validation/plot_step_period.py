import argparse

import matplotlib.pyplot as plt
import pandas as pd


def load_step_periods(
    csv_path,
    step_col="dt",
    period_col="T_sim",
    std_col="sigma_T_sim",
):
    df = pd.read_csv(csv_path)

    required_columns = [step_col, period_col, std_col]
    for col in required_columns:
        if col not in df.columns:
            raise ValueError(f"Missing required column: {col}")

    df = df[[step_col, period_col, std_col]].copy()
    df[step_col] = pd.to_numeric(df[step_col], errors="raise")
    df[period_col] = pd.to_numeric(df[period_col], errors="raise")
    df[std_col] = pd.to_numeric(df[std_col], errors="raise")
    df = df.sort_values(step_col, ascending=False)

    return df


def plot_boxplot(
    df,
    output_path,
    step_col="dt",
    period_col="T_sim",
    std_col="sigma_T_sim",
):
    theoretical_value = 3040094.051
    label_font_size = 16
    tick_font_size = 13
    legend_font_size = 15
    title_font_size = 18
    offset_font_size = 15

    grouped = list(df.groupby(step_col, sort=False))
    groups = [group[period_col].to_numpy(dtype=float) for _, group in grouped]
    labels = [str(step) for step, _ in grouped]

    fig, ax = plt.subplots(figsize=(10, 6))
    positions = range(1, len(groups) + 1)
    ax.boxplot(groups, labels=labels, positions=positions, showmeans=True)
    ax.axhline(
        theoretical_value,
        color="tab:blue",
        linestyle="--",
        linewidth=1.5,
        label="Theoretical value",
    )

    for position, (_, group) in zip(positions, grouped):
        ax.errorbar(
            [position] * len(group),
            group[period_col].to_numpy(dtype=float),
            yerr=group[std_col].to_numpy(dtype=float),
            fmt="o",
            color="tab:red",
            ecolor="tab:red",
            elinewidth=1.0,
            capsize=4,
            markersize=4,
            label="T ± σT" if position == 1 else None,
        )

    ax.set_xlabel("dt [s]", fontsize=label_font_size)
    ax.set_ylabel("T [s]", fontsize=label_font_size)
    ax.set_title("Simulated period by time step", fontsize=title_font_size)
    ax.tick_params(axis="both", labelsize=tick_font_size)
    ax.yaxis.get_offset_text().set_fontsize(offset_font_size)
    ax.grid(True, axis="y", alpha=0.3)
    ax.legend(fontsize=legend_font_size)
    fig.tight_layout()
    fig.savefig(output_path)
    plt.close(fig)


def main():
    parser = argparse.ArgumentParser(
        description="Create a PDF boxplot from period_validation_step.csv."
    )
    parser.add_argument(
        "csv_path",
        nargs="?",
        default="period_validation_step.csv",
        help="Path to the CSV file.",
    )
    parser.add_argument(
        "-o",
        "--output",
        default="period_validation_step_boxplot.pdf",
        help="Output PDF path.",
    )
    parser.add_argument("--step-col", default="dt", help="Name of the step column.")
    parser.add_argument("--period-col", default="T_sim", help="Name of the period column.")
    parser.add_argument(
        "--std-col",
        default="sigma_T_sim",
        help="Name of the standard deviation column.",
    )

    args = parser.parse_args()

    df = load_step_periods(
        csv_path=args.csv_path,
        step_col=args.step_col,
        period_col=args.period_col,
        std_col=args.std_col,
    )
    plot_boxplot(
        df=df,
        output_path=args.output,
        step_col=args.step_col,
        period_col=args.period_col,
        std_col=args.std_col,
    )

    print(f"Saved boxplot to {args.output}")


if __name__ == "__main__":
    main()
