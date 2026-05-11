#!/usr/bin/env bash
set -Eeuo pipefail
ISO="${1:-build/mcsos.iso}"
LOG="${2:-build/m3_serial.log}"
TIMEOUT_SEC="${MCSOS_QEMU_TIMEOUT:-8}"
fail() { echo "FAIL: $*" >&2; exit 1; }
test -f "$ISO" || fail "ISO tidak ditemukan: $ISO"
command -v qemu-system-x86_64 >/dev/null 2>&1 || fail "qemu-system-x86_64 tidak ditemukan"
OVMF_CODE=""
for f in /usr/share/OVMF/OVMF_CODE_4M.fd /usr/share/OVMF/OVMF_CODE.fd /usr/share/ovmf/OVMF.fd /usr/share/qemu/OVMF.fd; do
    [ -f "$f" ] && { OVMF_CODE="$f"; break; }
done
OVMF_VARS=""
for f in /usr/share/OVMF/OVMF_VARS_4M.fd /usr/share/OVMF/OVMF_VARS.fd /usr/share/ovmf/OVMF_VARS.fd; do
    [ -f "$f" ] && { OVMF_VARS="$f"; break; }
done
[ -n "$OVMF_CODE" ] || fail "OVMF_CODE tidak ditemukan"
[ -n "$OVMF_VARS" ] || fail "OVMF_VARS tidak ditemukan"
cp "$OVMF_VARS" build/m3_OVMF_VARS.fd
mkdir -p "$(dirname "$LOG")"
rm -f "$LOG"
timeout "$TIMEOUT_SEC" qemu-system-x86_64 \
    -machine q35 -m 256M -smp 1 -cpu qemu64 \
    -drive if=pflash,format=raw,readonly=on,file="$OVMF_CODE" \
    -drive if=pflash,format=raw,file=build/m3_OVMF_VARS.fd \
    -cdrom "$ISO" -boot d \
    -serial file:"$LOG" \
    -display none -no-reboot -no-shutdown || true
cat "$LOG"
grep -q 'MCSOS 260502 M3 kernel entered' "$LOG" || fail "log boot M3 tidak ditemukan"
grep -q '\[M3\] selftest: basic invariants passed' "$LOG" || fail "selftest M3 tidak lulus"
grep -q '\[M3\] ready for QEMU smoke test and GDB audit' "$LOG" || fail "ready string tidak ditemukan"
echo "PASS: QEMU smoke test M3 selesai"

chmod +x tools/scripts/m3_qemu_run.sh
