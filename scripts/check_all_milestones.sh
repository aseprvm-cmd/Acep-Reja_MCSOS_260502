#!/usr/bin/env bash
echo "== Rekap Milestone M0-M16 =="
echo ""

milestones=(
  "M0:master:M0 baseline"
  "M1:m1:M1 toolchain"
  "M2:m2:M2 bootable kernel"
  "M3:praktikum/m3-panic-debug-audit:M3 panic/debug"
  "M4:m4-idt-exception-path:M4 IDT/trap"
  "M5:praktikum/m5-timer-irq:M5 timer/IRQ"
  "M6:m6-pmm:M6 PMM allocator"
  "M7:m7-vmm:M7 VMM page table"
  "M8:praktikum-m8-kernel-heap:M8 kernel heap"
  "M9:a9-kernel-thread-scheduler:M9 scheduler"
  "M10:praktikum/m10-syscall-abi:M10 syscall ABI"
  "M11:praktikum-m11-elf-user-loader:M11 ELF loader"
  "M12:praktikum/m12-sync:M12 locking"
  "M13:praktikum-m13-vfs-ramfs:M13 VFS/RAMFS"
  "M14:praktikum-m14-block-device:M14 block device"
  "M15:praktikum-m15-mcsfs1:M15 MCSFS1"
  "M16:praktikum-m16-journal-recovery:M16 MCSFS1J journal"
)

PASS=0
FAIL=0
for entry in "${milestones[@]}"; do
  IFS=':' read -r label branch desc <<< "$entry"
  if git show-ref --verify --quiet "refs/heads/$branch" 2>/dev/null || \
     git show-ref --verify --quiet "refs/tags/$branch" 2>/dev/null || \
     git log --oneline | grep -qi "$label"; then
    echo "PASS: $label — $desc"
    PASS=$((PASS+1))
  else
    echo "FAIL: $label — $desc (branch/tag tidak ditemukan)"
    FAIL=$((FAIL+1))
  fi
done

echo ""
echo "== Hasil: $PASS PASS, $FAIL FAIL dari 17 milestone =="
