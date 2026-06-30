#!/usr/bin/env python3

import argparse
import concurrent.futures
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path

from solarscape_tools.experiments import (
    ALGORITHM_ORDER,
    algorithm_label,
    algorithm_name,
    find_executables,
    find_scenarios,
    output_file_name,
)


@dataclass(frozen=True)
class Experiment:
    index: int
    total: int
    algorithm: str
    executable: Path
    scenario: Path
    output_file: Path


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
        "-j",
        "--jobs",
        type=int,
        default=1,
        help="Number of MOEA/D experiment processes to run in parallel.",
    )
    parser.add_argument(
        "--keep-going",
        action="store_true",
        help="Continue remaining runs after a failed process.",
    )
    parser.add_argument(
        "--force",
        action="store_true",
        help="Re-run experiments even when their output JSON files already exist.",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print commands without running them.",
    )
    return parser.parse_args()


def run_experiment(
    experiment: Experiment,
    timeout_seconds: float | None,
    dry_run: bool,
) -> subprocess.CompletedProcess[str] | None:
    command = [
        str(experiment.executable),
        "--config",
        str(experiment.scenario),
        "--output",
        str(experiment.output_file),
    ]

    print(f"[{experiment.index}/{experiment.total}] -> {experiment.output_file.name}")
    print(" ".join(command))

    if dry_run:
        return None

    return subprocess.run(
        command,
        cwd=experiment.executable.parent,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        timeout=timeout_seconds,
        check=False,
    )


def validate_args(args: argparse.Namespace) -> None:
    if args.runs <= 0:
        raise ValueError("--runs must be greater than zero.")

    if args.jobs <= 0:
        raise ValueError("--jobs must be greater than zero.")

    if not args.executables_dir.is_dir():
        raise ValueError(
            f"Executables directory does not exist: {args.executables_dir}"
        )

    if not args.scenarios_dir.is_dir():
        raise ValueError(f"Scenarios directory does not exist: {args.scenarios_dir}")


def build_experiments(
    executables: list[Path],
    scenarios: list[Path],
    output_dir: Path,
    run_count: int,
    force: bool,
) -> tuple[list[Experiment], int]:
    total_runs = len(executables) * len(scenarios) * run_count
    experiments: list[Experiment] = []
    current_run = 0

    for executable in executables:
        algorithm = algorithm_name(executable)
        for scenario in scenarios:
            for run_index in range(1, run_count + 1):
                current_run += 1
                output_file = output_dir / output_file_name(
                    scenario,
                    executable,
                    run_index,
                    run_count,
                )

                if output_file.exists() and not force:
                    print(
                        f"[{current_run}/{total_runs}] skipping existing "
                        f"output: {output_file.name}"
                    )
                    continue

                experiments.append(
                    Experiment(
                        current_run,
                        total_runs,
                        algorithm,
                        executable.resolve(),
                        scenario.resolve(),
                        output_file,
                    )
                )

    return experiments, total_runs


def print_completed_output(
    experiment: Experiment,
    completed: subprocess.CompletedProcess[str] | None,
) -> None:
    print(f"[{experiment.index}/{experiment.total}] <- {experiment.output_file.name}")

    if completed is None:
        return

    if completed.stdout:
        print(completed.stdout, end="")

    if completed.stderr:
        print(completed.stderr, file=sys.stderr, end="")


def handle_failed_run(
    experiment: Experiment,
    completed: subprocess.CompletedProcess[str] | None,
) -> int | None:
    if completed is None or completed.returncode == 0:
        return None

    print(
        f"failed with exit code {completed.returncode}: "
        f"{experiment.executable.name} on {experiment.scenario.name}",
        file=sys.stderr,
    )
    return completed.returncode


def run_experiments_sequentially(
    experiments: list[Experiment],
    timeout_seconds: float | None,
    dry_run: bool,
    keep_going: bool,
) -> tuple[int, int]:
    failed_runs = 0

    for experiment in experiments:
        try:
            completed = run_experiment(
                experiment,
                timeout_seconds,
                dry_run)
        except subprocess.TimeoutExpired as error:
            failed_runs += 1
            print(
                f"timeout after {error.timeout} seconds: "
                f"{experiment.executable.name} on {experiment.scenario.name}",
                file=sys.stderr,
            )
            if not keep_going:
                return 1, failed_runs
            continue

        print_completed_output(
            experiment,
            completed)

        failed_exit_code = handle_failed_run(
            experiment,
            completed)

        if failed_exit_code is not None:
            failed_runs += 1
            if not keep_going:
                return failed_exit_code, failed_runs

    return 0, failed_runs


def run_experiments_in_parallel(
    experiments: list[Experiment],
    jobs: int,
    timeout_seconds: float | None,
    dry_run: bool,
    keep_going: bool,
) -> tuple[int, int]:
    failed_runs = 0
    worker_count = min(
        jobs,
        len(experiments))

    with concurrent.futures.ThreadPoolExecutor(max_workers=worker_count) as executor:
        futures = {
            executor.submit(
                run_experiment,
                experiment,
                timeout_seconds,
                dry_run,
            ): experiment
            for experiment in experiments
        }

        for future in concurrent.futures.as_completed(futures):
            experiment = futures[future]

            try:
                completed = future.result()
            except subprocess.TimeoutExpired as error:
                failed_runs += 1
                print(
                    f"timeout after {error.timeout} seconds: "
                    f"{experiment.executable.name} on {experiment.scenario.name}",
                    file=sys.stderr,
                )
                if not keep_going:
                    executor.shutdown(cancel_futures=True)
                    return 1, failed_runs
                continue

            print_completed_output(
                experiment,
                completed)

            failed_exit_code = handle_failed_run(
                experiment,
                completed)

            if failed_exit_code is not None:
                failed_runs += 1
                if not keep_going:
                    executor.shutdown(cancel_futures=True)
                    return failed_exit_code, failed_runs

    return 0, failed_runs


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

    experiments, total_runs = build_experiments(
        executables,
        scenarios,
        output_dir,
        args.runs,
        args.force,
    )

    if not experiments:
        print(f"nothing to run; {total_runs} output file(s) already exist")
        return 0

    failed_runs = 0
    print(f"running {len(experiments)} of {total_runs} experiment(s)")

    ordered_algorithms = sorted(
        {experiment.algorithm for experiment in experiments},
        key=lambda algorithm: ALGORITHM_ORDER.get(algorithm, 100))

    for algorithm in ordered_algorithms:
        algorithm_experiments = [
            experiment
            for experiment in experiments
            if experiment.algorithm == algorithm
        ]

        if not algorithm_experiments:
            continue

        if algorithm == "moead":
            jobs = min(
                args.jobs,
                len(algorithm_experiments))
            print(
                f"running {len(algorithm_experiments)} MOEA/D experiment(s) "
                f"with {jobs} worker process(es)"
            )
            exit_code, algorithm_failed_runs = run_experiments_in_parallel(
                algorithm_experiments,
                args.jobs,
                args.timeout_seconds,
                args.dry_run,
                args.keep_going)
        else:
            label = algorithm_label(algorithm)
            print(
                f"running {len(algorithm_experiments)} {label} "
                "experiment(s) sequentially"
            )
            exit_code, algorithm_failed_runs = run_experiments_sequentially(
                algorithm_experiments,
                args.timeout_seconds,
                args.dry_run,
                args.keep_going)

        failed_runs += algorithm_failed_runs

        if exit_code != 0 and not args.keep_going:
            return exit_code

    if failed_runs > 0:
        print(f"finished with {failed_runs} failed run(s)", file=sys.stderr)
        return 1

    print(f"finished {len(experiments)} run(s)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
