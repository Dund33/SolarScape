#!/usr/bin/env bash

python3 analysis/run_experiments.py \
  --executables-dir ../build-wsl \
  --scenarios-dir .. \
  --output-dir out/experiments \
  --runs 5 \
  --jobs 12
