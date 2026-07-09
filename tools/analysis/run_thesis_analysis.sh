#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd -- "$SCRIPT_DIR/../.." && pwd)"

if [[ -n "${PYTHON:-}" ]]; then
    PYTHON_BIN="$PYTHON"
elif [[ -x "$REPO_ROOT/.venv/bin/python" ]]; then
    PYTHON_BIN="$REPO_ROOT/.venv/bin/python"
else
    PYTHON_BIN="python3"
fi

cd "$REPO_ROOT"

echo "Using Python: $PYTHON_BIN"
echo "Building experiment cache..."
"$PYTHON_BIN" tools/analysis/build_experiment_cache.py

echo "Exporting thesis data..."
"$PYTHON_BIN" tools/analysis/export_final_metrics.py
"$PYTHON_BIN" tools/analysis/export_algorithm_summary.py
"$PYTHON_BIN" tools/analysis/export_final_pareto_points.py
"$PYTHON_BIN" tools/analysis/export_best_solutions.py
if [[ "${EXPORT_CONVERGENCE_RUNS:-0}" == "1" ]]; then
    "$PYTHON_BIN" tools/analysis/export_convergence_runs.py
fi
"$PYTHON_BIN" tools/analysis/export_convergence_summary.py
"$PYTHON_BIN" tools/analysis/export_feasibility_summary.py

echo "Generating thesis plots..."
"$PYTHON_BIN" tools/analysis/plot_final_metric_distribution.py
"$PYTHON_BIN" tools/analysis/plot_convergence.py
"$PYTHON_BIN" tools/analysis/plot_feasibility.py
"$PYTHON_BIN" tools/analysis/plot_pareto_front_size.py
"$PYTHON_BIN" tools/analysis/plot_final_pareto.py

echo "Done. Cache: tools/out/thesis_cache/experiment_frame.parquet"
echo "Done. Data: tools/out/thesis_data"
echo "Done. Plots: tools/out/thesis_plots"
