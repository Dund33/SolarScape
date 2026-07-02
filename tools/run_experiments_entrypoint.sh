#!/bin/sh
set -e

if [ "$#" -eq 0 ]; then
    set -- --runs 5
fi

has_jobs=0
for arg in "$@"; do
    case "$arg" in
        -j|--jobs|--jobs=*)
            has_jobs=1
            ;;
    esac
done

if [ "$has_jobs" -eq 0 ]; then
    set -- "$@" --jobs "$(nproc)"
fi

exec python3 /opt/solarscape/tools/run_experiments.py \
    --executables-dir /opt/solarscape/bin \
    --scenarios-dir /opt/solarscape/scenarios \
    --output-dir /data/experiments \
    "$@"
