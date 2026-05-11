#!/usr/bin/env bash
set -Eeuo pipefail
ISO="${1:-build/mcsos.iso}"
test -f "$ISO" || { echo "FAIL: ISO tidak ditemukan: $ISO" >&2; exit 1; }
OVMF_CODE=""
for f in /usr/share/OVMF/OVMF_CODE_4M.fd /usr/share/OVMF/OVMF_CODE.fd /usr/share/ovmf/OVMF.fd; do
    [ -f "$f" ] && { OVMF_CODE="$f"; break; }
done
OVMF_VARS=""
for f in /usr/share/OVMF/OVMF_VARS_4M.fd /usr/share/OVMF/OVMF_VARS.fd /usr/share/ovmf/OVMF_VARS.fd; do
    [ -f "$f" ] && { OVMF_VARS="$f"; break; }
done
[ -n "$OVMF_CODE" ] || { echo "FAIL: OVMF_CODE tidak ditemukan" >&2; exit 1; }
[ -n "$OVMF_VARS" ] || { echo "FAIL: OVMF_VARS tidak ditemukan" >&2; exit 1; }
cp "$OVMF_VARS" build/m3_debug_OVMF_VARS.fd
# shellcheck disable=SC2093
exec qemu-system-x86_64 \
    -machine q35 -m 256M -smp 1 -cpu qemu64 \
    -drive if=pflash,format=raw,readonly=on,file="$OVMF_CODE" \
    -drive if=pflash,format=raw,file=build/m3_debug_OVMF_VARS.fd \
    -cdrom "$ISO" -boot d \
    -serial mon:stdio \
    -display none -no-reboot -no-shutdown \
    -s -S

chmod +x tools/scripts/m3_qemu_debug.sh
