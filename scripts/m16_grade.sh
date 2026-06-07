#!/usr/bin/env bash
set -euo pipefail
PASS=0
FAIL=0

check() {
  local desc="$1"
  local result="$2"
  if [ "$result" = "ok" ]; then
    echo "PASS: $desc"
    PASS=$((PASS+1))
  else
    echo "FAIL: $desc"
    FAIL=$((FAIL+1))
  fi
}

echo "== M16 Grade Check =="
echo ""

# 1. Source file ada
[ -f kernel/fs/mcsfs1j/m16_mcsfs_journal.c ] && check "source MCSFS1J ada" "ok" || check "source MCSFS1J ada" "fail"

# 2. Makefile test ada
[ -f tests/m16/Makefile ] && check "Makefile test ada" "ok" || check "Makefile test ada" "fail"

# 3. Host unit test pass
cd tests/m16
make clean host > /dev/null 2>&1 && check "host unit test PASS" "ok" || check "host unit test PASS" "fail"
cd ../..

# 4. Freestanding object ada
cd tests/m16
make freestanding > /dev/null 2>&1 && check "freestanding compile OK" "ok" || check "freestanding compile OK" "fail"
cd ../..

# 5. nm_undefined kosong
[ -f evidence/m16/nm_undefined.txt ] && [ ! -s evidence/m16/nm_undefined.txt ] && check "nm_undefined kosong" "ok" || check "nm_undefined kosong" "fail"

# 6. readelf ELF64
[ -f evidence/m16/readelf_header.txt ] && grep -q "ELF64" evidence/m16/readelf_header.txt && check "readelf ELF64" "ok" || check "readelf ELF64" "fail"

# 7. readelf x86-64
grep -q "Advanced Micro Devices X86-64" evidence/m16/readelf_header.txt && check "readelf x86-64" "ok" || check "readelf x86-64" "fail"

# 8. sha256sum ada dan tidak kosong
[ -f evidence/m16/sha256sum.txt ] && [ -s evidence/m16/sha256sum.txt ] && check "sha256sum tersimpan" "ok" || check "sha256sum tersimpan" "fail"

# 9. QEMU serial log ada
[ -f logs/m16/qemu_serial.log ] && [ -s logs/m16/qemu_serial.log ] && check "QEMU serial log ada" "ok" || check "QEMU serial log ada" "fail"

# 10. Preflight log ada
[ -f logs/m16/preflight.log ] && check "preflight log ada" "ok" || check "preflight log ada" "fail"

echo ""
echo "== Hasil: $PASS PASS, $FAIL FAIL =="
if [ $FAIL -eq 0 ]; then
  echo "STATUS: M16 SIAP UJI"
else
  echo "STATUS: ADA ITEM BELUM SELESAI"
fi
