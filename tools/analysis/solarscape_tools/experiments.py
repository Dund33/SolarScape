from __future__ import annotations

import re
import sys
from dataclasses import dataclass
from pathlib import Path


ALGORITHM_NAMES = {
    "SolarScape": "algo",
    "SolarScapeNSGAII": "nsgaii",
    "SolarScapeNSGAIII": "nsgaiii",
    "SolarScapeMOEAD": "moead",
}

ALGORITHM_ORDER = {
    "algo": 0,
    "nsgaii": 1,
    "nsgaiii": 2,
    "moead": 3,
}

ALGORITHM_LABELS = {
    "algo": "Proposed algorithm",
    "nsgaii": "NSGA-II",
    "nsgaiii": "NSGA-III",
    "moead": "MOEA/D",
}

SCENARIO_RE = re.compile(r"scenario(\d+)", re.IGNORECASE)
RUN_FILE_RE = re.compile(
    r"^(?P<scenario>scenario[^_]*)_(?P<algorithm>.+)_run(?P<run>\d+)\.json$",
    re.IGNORECASE,
)


@dataclass(frozen=True)
class ExperimentFile:
    scenario: str
    algorithm: str
    run: int
    path: Path


def executable_base_name(path: Path) -> str:
    return path.stem if path.suffix.lower() == ".exe" else path.name


def algorithm_name(path: Path) -> str:
    base_name = executable_base_name(path)

    if base_name in ALGORITHM_NAMES:
        return ALGORITHM_NAMES[base_name]

    suffix = base_name.removeprefix("SolarScape")
    raw_name = suffix if suffix else base_name
    return re.sub(r"[^a-zA-Z0-9]+", "-", raw_name).strip("-").lower()


def algorithm_label(algorithm: str) -> str:
    return ALGORITHM_LABELS.get(algorithm, algorithm)


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


def scenario_sort_key(scenario: str) -> tuple[int, int | str]:
    suffix = scenario.removeprefix("scenario")
    if suffix.isdigit():
        return 0, int(suffix)
    return 1, scenario


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
    mutation_probability: float | None = None,
) -> str:
    scenario_id = scenario_number(scenario)
    algorithm = algorithm_name(executable)
    run_width = max(2, len(str(run_count)))
    mutation_part = (
        f"_mp{mutation_probability_tag(mutation_probability)}"
        if mutation_probability is not None
        else ""
    )

    return f"scenario{scenario_id}_{algorithm}{mutation_part}_run{run_index:0{run_width}d}.json"


def mutation_probability_tag(value: float) -> str:
    return f"{value:.12g}".replace("-", "m").replace(".", "p")


def experiment_sort_key(
    experiment: ExperimentFile,
) -> tuple[tuple[int, int | str], int, str, int]:
    return (
        scenario_sort_key(experiment.scenario),
        ALGORITHM_ORDER.get(experiment.algorithm, 100),
        experiment.algorithm,
        experiment.run,
    )


def discover_experiment_files(
    input_dir: Path,
    scenarios: set[str] | None = None,
    algorithms: set[str] | None = None,
) -> list[ExperimentFile]:
    experiments: list[ExperimentFile] = []

    for path in input_dir.glob("*.json"):
        match = RUN_FILE_RE.match(path.name)
        if match is None:
            continue

        scenario = match.group("scenario").lower()
        algorithm = match.group("algorithm").lower()
        run = int(match.group("run"))

        if scenarios is not None and scenario not in scenarios:
            continue
        if algorithms is not None and algorithm not in algorithms:
            continue

        experiments.append(
            ExperimentFile(
                scenario=scenario,
                algorithm=algorithm,
                run=run,
                path=path,
            )
        )

    return sorted(experiments, key=experiment_sort_key)


def group_experiment_files(
    experiments: list[ExperimentFile],
) -> dict[tuple[str, str], list[ExperimentFile]]:
    groups: dict[tuple[str, str], list[ExperimentFile]] = {}

    for experiment in experiments:
        groups.setdefault((experiment.scenario, experiment.algorithm), []).append(
            experiment
        )

    for group in groups.values():
        group.sort(key=lambda experiment: experiment.run)

    return groups


def normalize_filter(values: list[str] | None) -> set[str] | None:
    if values is None:
        return None
    return {value.lower() for value in values}


def validate_run_groups(
    groups: dict[tuple[str, str], list[ExperimentFile]],
    expected_runs: int,
    allow_incomplete: bool,
) -> list[str]:
    if expected_runs <= 0:
        raise ValueError("--expected-runs must be greater than zero.")

    incomplete = [
        (scenario, algorithm, len(experiments))
        for (scenario, algorithm), experiments in groups.items()
        if len(experiments) < expected_runs
    ]
    if incomplete and not allow_incomplete:
        lines = [
            f"{scenario}/{algorithm}: {count} run(s)"
            for scenario, algorithm, count in incomplete
        ]
        raise ValueError(
            "Missing expected run files. Use --allow-incomplete to continue anyway: "
            + "; ".join(lines)
        )

    warnings = []
    for (scenario, algorithm), experiments in sorted(groups.items()):
        if len(experiments) != expected_runs:
            warnings.append(
                f"{scenario}/{algorithm} has {len(experiments)} run(s), "
                f"expected {expected_runs}; using all available runs"
            )

    return warnings
