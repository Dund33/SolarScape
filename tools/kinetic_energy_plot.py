import argparse

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd


def compute_energies(
    csv_path,
    probe_mass,
    central_mass,
    gravitational_constant,
    time_col="time",
    x_col="x",
    y_col="y",
    z_col="z",
    vx_col="vx",
    vy_col="vy",
    vz_col="vz",
    center_x=0.0,
    center_y=0.0,
    center_z=0.0,
):
    df = pd.read_csv(csv_path)

    required_columns = [time_col, x_col, y_col, z_col, vx_col, vy_col, vz_col]
    for col in required_columns:
        if col not in df.columns:
            raise ValueError(f"Missing required column: {col}")

    time = df[time_col].to_numpy(dtype=float)
    x = df[x_col].to_numpy(dtype=float)
    y = df[y_col].to_numpy(dtype=float)
    z = df[z_col].to_numpy(dtype=float)
    vx = df[vx_col].to_numpy(dtype=float)
    vy = df[vy_col].to_numpy(dtype=float)
    vz = df[vz_col].to_numpy(dtype=float)

    sort_idx = np.argsort(time)
    time = time[sort_idx]
    x = x[sort_idx]
    y = y[sort_idx]
    z = z[sort_idx]
    vx = vx[sort_idx]
    vy = vy[sort_idx]
    vz = vz[sort_idx]

    speed_squared = vx**2 + vy**2 + vz**2
    kinetic_energy = 0.5 * probe_mass * speed_squared

    dx = x - center_x
    dy = y - center_y
    dz = z - center_z
    distance = np.sqrt(dx**2 + dy**2 + dz**2)
    if np.any(distance == 0.0):
        raise ValueError("Cannot compute potential energy for distance equal to zero.")

    potential_energy = -gravitational_constant * probe_mass * central_mass / distance
    total_energy = kinetic_energy + potential_energy

    return time, kinetic_energy, potential_energy, total_energy


def plot_energies(time, kinetic_energy, potential_energy, total_energy, output_path):
    fig, ax = plt.subplots(figsize=(10, 6))
    ax.plot(
        time,
        kinetic_energy,
        color="tab:blue",
        linewidth=1.2,
        linestyle="-",
        label="Kinetic energy",
    )
    ax.plot(
        time,
        potential_energy,
        color="tab:red",
        linewidth=1.2,
        linestyle="-",
        label="Potential energy",
    )
    ax.plot(
        time,
        total_energy,
        color="tab:green",
        linewidth=1.2,
        linestyle="--",
        label="Total energy",
    )
    ax.set_xlabel("time [s]")
    ax.set_ylabel("energy [J]")
    ax.set_title("Probe energy over time")
    ax.grid(True, alpha=0.3)
    ax.legend()
    fig.tight_layout()
    fig.savefig(output_path)
    plt.close(fig)


def main():
    parser = argparse.ArgumentParser(
        description="Compute probe energies from validation CSV and save a PDF plot."
    )

    parser.add_argument("csv_path", help="Path to the CSV file.")
    parser.add_argument(
        "-o",
        "--output",
        default="kinetic_energy.pdf",
        help="Output PDF path.",
    )
    parser.add_argument(
        "--mass",
        type=float,
        default=1.0,
        help="Probe mass used for energy calculations.",
    )
    parser.add_argument(
        "--central-mass",
        type=float,
        default=1.0e18,
        help="Mass of the central body used for potential energy.",
    )
    parser.add_argument(
        "--gravitational-constant",
        type=float,
        default=0.000000000066743,
        help="Gravitational constant used for potential energy.",
    )

    parser.add_argument("--time-col", default="time", help="Name of the time column.")
    parser.add_argument("--x-col", default="x", help="Name of the x position column.")
    parser.add_argument("--y-col", default="y", help="Name of the y position column.")
    parser.add_argument("--z-col", default="z", help="Name of the z position column.")
    parser.add_argument("--vx-col", default="vx", help="Name of the x velocity column.")
    parser.add_argument("--vy-col", default="vy", help="Name of the y velocity column.")
    parser.add_argument("--vz-col", default="vz", help="Name of the z velocity column.")
    parser.add_argument(
        "--center-x",
        type=float,
        default=0.0,
        help="X coordinate of the central body.",
    )
    parser.add_argument(
        "--center-y",
        type=float,
        default=0.0,
        help="Y coordinate of the central body.",
    )
    parser.add_argument(
        "--center-z",
        type=float,
        default=0.0,
        help="Z coordinate of the central body.",
    )

    args = parser.parse_args()

    time, kinetic_energy, potential_energy, total_energy = compute_energies(
        csv_path=args.csv_path,
        probe_mass=args.mass,
        central_mass=args.central_mass,
        gravitational_constant=args.gravitational_constant,
        time_col=args.time_col,
        x_col=args.x_col,
        y_col=args.y_col,
        z_col=args.z_col,
        vx_col=args.vx_col,
        vy_col=args.vy_col,
        vz_col=args.vz_col,
        center_x=args.center_x,
        center_y=args.center_y,
        center_z=args.center_z,
    )

    plot_energies(
        time=time,
        kinetic_energy=kinetic_energy,
        potential_energy=potential_energy,
        total_energy=total_energy,
        output_path=args.output,
    )

    print(f"Saved energy plot to {args.output}")


if __name__ == "__main__":
    main()
