#!/usr/bin/env bash
set -euo pipefail

required_files=(
  build/kernel.elf
  build/kernel.map
  build/inspect/readelf-header.txt
  build/inspect/readelf-program-headers.txt
  build/inspect/objdump-disassembly.txt
  build/inspect/nm-symbols.txt
  build/mcsos.iso
  build/mcsos.iso.sha256
  build/qemu-serial.log
)

for f in "${required_files[@]}"; do
  if [ ! -s "$f" ]; then
    echo "ERROR: artefak tidak ada atau kosong: $f" >&2
    exit 1
  fi
  echo "OK artifact: $f"
done

grep -q 'Class:.*ELF64' build/inspect/readelf-header.txt
grep -q 'Machine:.*Advanced Micro Devices X86-64' build/inspect/readelf-header.txt
grep -q 'Entry point address:.*0xffffffff80000000' build/inspect/readelf-header.txt
grep -q 'MCSOS 260502 M3 kernel entered' build/qemu-serial.log
grep -q '\[M3\] selftest: basic invariants passed' build/qemu-serial.log
grep -q '\[M3\] ready for QEMU smoke test and GDB audit' build/qemu-serial.log

echo "OK: M2 local grading checks passed"
chmod +x tools/scripts/grade_m2.sh
