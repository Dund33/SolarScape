import argparse
import numpy as np
import pandas as pd


def parabolic_minimum_time(t0, r0, t1, r1, t2, r2):
    """
    Estimates the time of the local minimum using a parabola fitted
    through three neighboring points: (t0, r0), (t1, r1), (t2, r2).
    """
    coeffs = np.polyfit([t0, t1, t2], [r0, r1, r2], deg=2)
    a, b, _ = coeffs

    if abs(a) < 1e-15:
        return t1

    return -b / (2 * a)


def find_periapsis_times(time, radius):
    """
    Finds local minima of radius(t), interpreted as periapsis passages.
    """
    periapsis_times = []

    for i in range(1, len(radius) - 1):
        if radius[i] < radius[i - 1] and radius[i] < radius[i + 1]:
            t_min = parabolic_minimum_time(
                time[i - 1], radius[i - 1],
                time[i], radius[i],
                time[i + 1], radius[i + 1],
            )
            periapsis_times.append(t_min)

    return np.array(periapsis_times)


def select_body(df, body_id, body_id_col):
    if body_id_col not in df.columns:
        return df

    if body_id is None:
        body_id = int(df[body_id_col].max())

    body_df = df[df[body_id_col] == body_id]
    if body_df.empty:
        available_ids = sorted(df[body_id_col].unique())
        raise ValueError(
            f"bodyId={body_id} was not found. Available bodyId values: {available_ids}"
        )

    return body_df


def compute_orbital_period(
    csv_path,
    body_id=None,
    body_id_col="bodyId",
    time_col="time",
    x_col="x",
    y_col="y",
    z_col="z",
):
    df = pd.read_csv(csv_path)

    required_columns = [time_col, x_col, y_col, z_col]
    for col in required_columns:
        if col not in df.columns:
            raise ValueError(f"Missing required column: {col}")

    df = select_body(df, body_id, body_id_col)

    time = df[time_col].to_numpy(dtype=float)
    x = df[x_col].to_numpy(dtype=float)
    y = df[y_col].to_numpy(dtype=float)
    z = df[z_col].to_numpy(dtype=float)

    sort_idx = np.argsort(time)
    time = time[sort_idx]
    x = x[sort_idx]
    y = y[sort_idx]
    z = z[sort_idx]

    radius = np.sqrt(x**2 + y**2 + z**2)

    periapsis_times = find_periapsis_times(time, radius)

    if len(periapsis_times) < 2:
        raise RuntimeError(
            "Could not detect at least two periapsis passages. "
            "The CSV may contain less than one full orbit or the sampling may be too sparse."
        )

    periods = np.diff(periapsis_times)

    return {
        "periapsis_times": periapsis_times,
        "periods": periods,
        "mean_period": np.mean(periods),
        "std_period": np.std(periods),
    }


def main():
    parser = argparse.ArgumentParser(
        description="Compute orbital period from a CSV file containing orbital positions over time."
    )

    parser.add_argument("csv_path", help="Path to the CSV file.")

    parser.add_argument(
        "--body-id",
        type=int,
        default=None,
        help="bodyId to analyze. Defaults to the highest bodyId in the CSV.",
    )
    parser.add_argument("--body-id-col", default="bodyId", help="Name of the body id column.")
    parser.add_argument("--time-col", default="time", help="Name of the time column.")
    parser.add_argument("--x-col", default="x", help="Name of the x position column.")
    parser.add_argument("--y-col", default="y", help="Name of the y position column.")
    parser.add_argument("--z-col", default="z", help="Name of the z position column.")

    args = parser.parse_args()

    result = compute_orbital_period(
        csv_path=args.csv_path,
        body_id=args.body_id,
        body_id_col=args.body_id_col,
        time_col=args.time_col,
        x_col=args.x_col,
        y_col=args.y_col,
        z_col=args.z_col,
    )

    print("Detected periapsis times:")
    for t in result["periapsis_times"]:
        print(f"  {t:.6f}")

    print("\nDetected orbital periods:")
    for T in result["periods"]:
        print(f"  {T:.6f}")

    print("\nEstimated orbital period:")
    print(f"  mean = {result['mean_period']:.6f}")
    print(f"  std  = {result['std_period']:.6f}")


if __name__ == "__main__":
    main()
