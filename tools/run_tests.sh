#!/usr/bin/env bash

python3 analysis/run_experiments.py \
  --executables-dir ../build-wsl \
  --scenarios-dir .. \
  --output-dir out/experiments \
  --runs 10 \
  --mutation-probabilities 0.0067 0.033 0.1
