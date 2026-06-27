#!/usr/bin/env python3

import argparse
import re
import subprocess
import sys
from pathlib import Path


ALGORITHM_NAMES = {
    "SolarScape": "algo",
    "SolarScapeNSGAII": "nsgaii",
    "SolarScapeMOEAD": "moead",
}

ALGORITHM_ORDER = {
    "algo": 0,
    "nsgaii": 1,
    "moead": 2,
}

SCENARIO_RE = re.compile(r"scenario(\d+)", re.IGNORECASE)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Run every SolarScape* executable for every scenario file "
            "and store Pareto front JSON outputs."
        )
    )
    parser.add_argument(
        "--executables-dir",
        required=True,
        type=Path,
        help="Directory containing SolarScape* executables.",
    )
    parser.add_argument(
        "--scenarios-dir",
        required=True,
        type=Path,
        help="Directory containing scenario*.yml or scenario*.yaml files.",
    )
    parser.add_argument(
        "--output-dir",
        required=True,
        type=Path,
        help="Directory where JSON results will be written.",
    )
    parser.add_argument(
        "-n",
        "--runs",
        required=True,
        type=int,
        help="Number of repetitions for every algorithm/scenario pair.",
    )
    parser.add_argument(
        "--timeout-seconds",
        type=float,
        default=None,
        help="Optional timeout for a single run.",
    )
    parser.add_argument(
        "--keep-going",
        action="store_true",
        help="Continue remaining runs after a failed process.",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print commands without running them.",
    )
    return parser.parse_args()


def executable_base_name(path: Path) -> str:
    return path.stem if path.suffix.lower() == ".exe" else path.name


def algorithm_name(path: Path) -> str:
    base_name = executable_base_name(path)

    if base_name in ALGORITHM_NAMES:
        return ALGORITHM_NAMES[base_name]

    suffix = base_name.removeprefix("SolarScape")
    raw_name = suffix if suffix else base_name
    return re.sub(r"[^a-zA-Z0-9]+", "-", raw_name).strip("-").lower()


def is_candidate_executable(path: Path) -> bool:
    if not path.is_file():
        return False

    base_name = executable_base_name(path)
    if not base_name.startswith("SolarScape"):
        return False

    if sys.platform == "win32":
        return path.suffix.lower() == ".exe"

    return path.stat().st_mode & 0o111 != 0


def find_executables(executables_dir: Path) -> list[Path]:
    executables = [
        path
        for path in executables_dir.glob("SolarScape*")
        if is_candidate_executable(path)
    ]

    return sorted(
        executables,
        key=lambda path: (
            ALGORITHM_ORDER.get(algorithm_name(path), 100),
            path.name,
        ),
    )


def scenario_number(path: Path) -> str:
    match = SCENARIO_RE.search(path.stem)
    return match.group(1) if match else path.stem


def find_scenarios(scenarios_dir: Path) -> list[Path]:
    scenarios = [
        path
        for pattern in ("scenario*.yml", "scenario*.yaml")
        for path in scenarios_dir.glob(pattern)
        if path.is_file()
    ]

    def sort_key(path: Path) -> tuple[int, int | str, str]:
        number = scenario_number(path)

        if number.isdigit():
            return 0, int(number), path.name

        return 1, number, path.name

    return sorted(scenarios, key=sort_key)


def output_file_name(
    scenario: Path,
    executable: Path,
    run_index: int,
    run_count: int,
) -> str:
    scenario_id = scenario_number(scenario)
    algorithm = algorithm_name(executable)
    run_width = max(2, len(str(run_count)))

    return f"scenario{scenario_id}_{algorithm}_run{run_index:0{run_width}d}.json"


def run_experiment(
    executable: Path,
    scenario: Path,
    output_file: Path,
    timeout_seconds: float | None,
    dry_run: bool,
) -> subprocess.CompletedProcess[str] | None:
    command = [
        str(executable),
        "--config",
        str(scenario),
        "--output",
        str(output_file),
    ]

    print(" ".join(command))

    if dry_run:
        return None

    return subprocess.run(
        command,
        cwd=executable.parent,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        timeout=timeout_seconds,
        check=False,
    )


def validate_args(args: argparse.Namespace) -> None:
    if args.runs <= 0:
        raise ValueError("--runs must be greater than zero.")

    if not args.executables_dir.is_dir():
        raise ValueError(
            f"Executables directory does not exist: {args.executables_dir}"
        )

    if not args.scenarios_dir.is_dir():
        raise ValueError(f"Scenarios directory does not exist: {args.scenarios_dir}")


def main() -> int:
    args = parse_args()

    try:
        validate_args(args)
    except ValueError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2

    executables_dir = args.executables_dir.resolve()
    scenarios_dir = args.scenarios_dir.resolve()
    output_dir = args.output_dir.resolve()

    executables = find_executables(executables_dir)
    scenarios = find_scenarios(scenarios_dir)

    if not executables:
        print(
            f"error: no SolarScape* executables found in {executables_dir}",
            file=sys.stderr,
        )
        return 2

    if not scenarios:
        print(
            f"error: no scenario*.yml or scenario*.yaml files found in {scenarios_dir}",
            file=sys.stderr,
        )
        return 2

    output_dir.mkdir(parents=True, exist_ok=True)

    total_runs = len(executables) * len(scenarios) * args.runs
    failed_runs = 0
    current_run = 0

    for scenario in scenarios:
        for executable in executables:
            for run_index in range(1, args.runs + 1):
                current_run += 1
                output_file = output_dir / output_file_name(
                    scenario,
                    executable,
                    run_index,
                    args.runs,
                )

                print(f"[{current_run}/{total_runs}] -> {output_file.name}")

                try:
                    completed = run_experiment(
                        executable.resolve(),
                        scenario.resolve(),
                        output_file,
                        args.timeout_seconds,
                        args.dry_run,
                    )
                except subprocess.TimeoutExpired as error:
                    failed_runs += 1
                    print(
                        f"timeout after {error.timeout} seconds: {executable.name} "
                        f"on {scenario.name}",
                        file=sys.stderr,
                    )
                    if not args.keep_going:
                        return 1
                    continue

                if completed is None:
                    continue

                if completed.stdout:
                    print(completed.stdout, end="")

                if completed.returncode != 0:
                    failed_runs += 1
                    print(
                        f"failed with exit code {completed.returncode}: "
                        f"{executable.name} on {scenario.name}",
                        file=sys.stderr,
                    )
                    if completed.stderr:
                        print(completed.stderr, file=sys.stderr, end="")
                    if not args.keep_going:
                        return completed.returncode
                elif completed.stderr:
                    print(completed.stderr, file=sys.stderr, end="")

    if failed_runs > 0:
        print(f"finished with {failed_runs} failed run(s)", file=sys.stderr)
        return 1

    print(f"finished {total_runs} run(s)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
