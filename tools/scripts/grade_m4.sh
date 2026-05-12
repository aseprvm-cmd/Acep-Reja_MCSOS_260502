#!/usr/bin/env bash
set -euo pipefail

score=0

make clean >/dev/null
make audit >/dev/null
score=$((score + 60))

tools/scripts/m4_audit_elf.sh build/kernel.elf >/dev/null
score=$((score + 20))

# rebuild ISO dan jalankan QEMU sebagai bagian dari grading
bash tools/scripts/make_iso.sh >/dev/null 2>&1 || true
tools/scripts/m4_qemu_run.sh build/mcsos.iso build/m4-qemu-serial.log >/dev/null 2>&1 || true

if [[ -f build/m4-qemu-serial.log ]]; then
    grep -q '\[M4\]' build/m4-qemu-serial.log && score=$((score + 10))
fi

[[ -f evidence/M4/manifest.txt ]] && score=$((score + 10))

echo "M4_LOCAL_SCORE=$score/100"
