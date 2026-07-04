from __future__ import annotations

import math
from pathlib import Path

import matplotlib.pyplot as plt
import pandas as pd

from solarscape_tools.display import safe_file_name
from solarscape_tools.experiments import ALGORITHM_ORDER, algorithm_label


ALGORITHM_PALETTE = {
    "Proposed algorithm": "#1f77b4",
    "NSGA-II": "#ff7f0e",
    "MOEA/D": "#2ca02c",
}


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


def apply_y_axis_scale(ax, values: pd.DataFrame, log_y: bool) -> None:
    finite = finite_values(values)
    if not finite:
        return
    if log_y and min(finite) > 0.0:
        ax.set_yscale("log")
    elif min(finite) >= 0.0:
        ax.set_ylim(bottom=0.0)


def apply_x_axis_scale(ax, values: pd.DataFrame, log_x: bool) -> None:
    finite = finite_values(values)
    if log_x and finite and min(finite) > 0.0:
        ax.set_xscale("log")


def finite_values(values: pd.DataFrame) -> list[float]:
    return [
        float(value)
        for value in values.to_numpy().ravel()
        if pd.notna(value) and math.isfinite(float(value))
    ]


def style_numeric_axis(ax) -> None:
    if ax.get_xscale() != "log":
        try:
            ax.ticklabel_format(axis="x", style="sci", scilimits=(-3, 4))
        except (AttributeError, ValueError):
            pass
    if ax.get_yscale() != "log":
        try:
            ax.ticklabel_format(axis="y", style="sci", scilimits=(-3, 4))
        except (AttributeError, ValueError):
            pass


def format_legend(ax) -> None:
    legend = ax.get_legend()
    if legend is not None:
        legend.set_title("")


def save_figure(fig, output_path: Path, dpi: int) -> None:
    output_path.parent.mkdir(parents=True, exist_ok=True)
    fig.tight_layout()
    fig.savefig(output_path, dpi=dpi)
    plt.close(fig)


def pdf_path(output_dir: Path, *parts: str) -> Path:
    return output_dir / f"{'_'.join(safe_file_name(part) for part in parts)}.pdf"
