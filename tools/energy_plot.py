import argparse


def parse_mass_value(line):
    return float(line.split(":", 1)[1].strip())


def load_masses_from_config(config_path):
    bodies_masses = []
    target_body_mass = None
    probe_empty_mass = None
    probe_fuel_mass = None
    section = None

    with open(config_path, "r", encoding="utf-8") as config_file:
        for raw_line in config_file:
            line = raw_line.split("#", 1)[0].rstrip()
            if not line.strip():
                continue

            if not line.startswith(" "):
                section = line.rstrip(":")
                continue

            stripped = line.strip()
            if section == "bodies" and stripped.startswith("mass:"):
                bodies_masses.append(parse_mass_value(stripped))
            elif section == "targetBody" and stripped.startswith("mass:"):
                target_body_mass = parse_mass_value(stripped)
            elif section == "probe" and stripped.startswith("emptyMass:"):
                probe_empty_mass = parse_mass_value(stripped)
            elif section == "probe" and stripped.startswith("fuelMass:"):
                probe_fuel_mass = parse_mass_value(stripped)

    missing = []
    if target_body_mass is None:
        missing.append("targetBody.mass")
    if probe_empty_mass is None:
        missing.append("probe.emptyMass")
    if probe_fuel_mass is None:
        missing.append("probe.fuelMass")
    if missing:
        raise ValueError(
            f"Missing required mass values in config: {', '.join(missing)}"
        )

    return bodies_masses + [target_body_mass, probe_empty_mass + probe_fuel_mass]


def parse_masses(value):
    masses = [float(mass.strip()) for mass in value.split(",") if mass.strip()]
    if not masses:
        raise ValueError("Mass list cannot be empty.")
    return masses


def resolve_masses(args):
    if args.masses is not None:
        return parse_masses(args.masses)

    if args.config_yaml is not None:
        return load_masses_from_config(args.config_yaml)

    raise ValueError("Provide masses with --masses or --config-yaml.")


def validate_body_ids(body_ids, masses, np):
    if body_ids.size == 0:
        raise ValueError("CSV does not contain any rows.")

    if np.any(body_ids < 0):
        raise ValueError("bodyId values must be non-negative.")

    max_body_id = int(body_ids.max())
    if max_body_id >= len(masses):
        raise ValueError(
            f"CSV contains bodyId={max_body_id}, but only {len(masses)} masses were provided."
        )


def compute_potential_energy(positions, masses, gravitational_constant, np):
    potential_energy = 0.0

    for i in range(len(masses)):
        if masses[i] == 0.0:
            continue

        for j in range(i + 1, len(masses)):
            if masses[j] == 0.0:
                continue

            distance = np.linalg.norm(positions[i] - positions[j])
            if distance == 0.0:
                raise ValueError(
                    "Cannot compute potential energy for non-zero masses at the same position."
                )

            potential_energy -= gravitational_constant * masses[i] * masses[j] / distance

    return potential_energy


def compute_energies(
    csv_path,
    masses,
    gravitational_constant,
    body_id_col="bodyId",
    time_col="time",
    x_col="x",
    y_col="y",
    z_col="z",
    vx_col="vx",
    vy_col="vy",
    vz_col="vz",
):
    import numpy as np
    import pandas as pd

    df = pd.read_csv(csv_path)

    required_columns = [
        body_id_col,
        time_col,
        x_col,
        y_col,
        z_col,
        vx_col,
        vy_col,
        vz_col,
    ]
    for col in required_columns:
        if col not in df.columns:
            raise ValueError(f"Missing required column: {col}")

    masses = np.asarray(masses, dtype=float)
    if np.any(masses < 0.0):
        raise ValueError("Mass values must be non-negative.")

    body_ids = df[body_id_col].to_numpy(dtype=int)
    validate_body_ids(body_ids, masses, np)

    df = df.copy()
    df["_mass"] = masses[body_ids]
    df["_speed_squared"] = (
        df[vx_col].to_numpy(dtype=float) ** 2
        + df[vy_col].to_numpy(dtype=float) ** 2
        + df[vz_col].to_numpy(dtype=float) ** 2
    )
    df["_kinetic_energy"] = 0.5 * df["_mass"] * df["_speed_squared"]

    times = []
    kinetic_energies = []
    potential_energies = []

    for time, group in df.groupby(time_col, sort=True):
        group_body_ids = group[body_id_col].to_numpy(dtype=int)
        if len(group_body_ids) != len(set(group_body_ids)):
            raise ValueError(f"Duplicate bodyId values for time={time}.")

        ordered = group.sort_values(body_id_col)
        ordered_body_ids = ordered[body_id_col].to_numpy(dtype=int)
        ordered_masses = masses[ordered_body_ids]
        positions = ordered[[x_col, y_col, z_col]].to_numpy(dtype=float)

        times.append(float(time))
        kinetic_energies.append(float(ordered["_kinetic_energy"].sum()))
        potential_energies.append(
            compute_potential_energy(
                positions,
                ordered_masses,
                gravitational_constant,
                np,
            )
        )

    kinetic_energies = np.asarray(kinetic_energies)
    potential_energies = np.asarray(potential_energies)

    return (
        np.asarray(times),
        kinetic_energies,
        potential_energies,
        kinetic_energies + potential_energies,
    )


def plot_energies(time, kinetic_energy, potential_energy, total_energy, output_path):
    import matplotlib.pyplot as plt

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
    ax.set_title("System energy over time")
    ax.grid(True, alpha=0.3)
    ax.legend()
    fig.tight_layout()
    fig.savefig(output_path)
    plt.close(fig)


def main():
    parser = argparse.ArgumentParser(
        description="Compute system kinetic, potential, and total energy from a validation CSV."
    )

    parser.add_argument("csv_path", help="Path to the CSV file.")
    parser.add_argument(
        "-o",
        "--output",
        default="energy.pdf",
        help="Output PDF path.",
    )
    parser.add_argument(
        "--masses",
        help="Comma-separated masses indexed by bodyId, for example: 1e18,1e15,0,1",
    )
    parser.add_argument(
        "--config-yaml",
        help="Validation YAML used to read masses in bodyId order.",
    )
    parser.add_argument(
        "--gravitational-constant",
        type=float,
        default=0.000000000066743,
        help="Gravitational constant used for potential energy.",
    )

    parser.add_argument("--body-id-col", default="bodyId", help="Name of the body id column.")
    parser.add_argument("--time-col", default="time", help="Name of the time column.")
    parser.add_argument("--x-col", default="x", help="Name of the x position column.")
    parser.add_argument("--y-col", default="y", help="Name of the y position column.")
    parser.add_argument("--z-col", default="z", help="Name of the z position column.")
    parser.add_argument("--vx-col", default="vx", help="Name of the x velocity column.")
    parser.add_argument("--vy-col", default="vy", help="Name of the y velocity column.")
    parser.add_argument("--vz-col", default="vz", help="Name of the z velocity column.")

    args = parser.parse_args()

    masses = resolve_masses(args)

    time, kinetic_energy, potential_energy, total_energy = compute_energies(
        csv_path=args.csv_path,
        masses=masses,
        gravitational_constant=args.gravitational_constant,
        body_id_col=args.body_id_col,
        time_col=args.time_col,
        x_col=args.x_col,
        y_col=args.y_col,
        z_col=args.z_col,
        vx_col=args.vx_col,
        vy_col=args.vy_col,
        vz_col=args.vz_col,
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
