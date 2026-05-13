#!/usr/bin/env bash
set -euo pipefail
make clean
make grade
printf '[M5] static build and audit passed.\n'
