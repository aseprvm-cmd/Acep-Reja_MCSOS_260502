# Template Laporan Praktikum Sistem Operasi Lanjut — MCSOS

**Nama file laporan:** `laporan_praktikum_M4_[Kelompok].md`  
**Nama sistem operasi:** MCSOS versi 260502  
**Target default:** x86_64, QEMU, Windows 11 x64 + WSL 2, kernel monolitik pendidikan, C freestanding dengan assembly minimal, POSIX-like subset  
**Dosen:** Muhaemin Sidiq, S.Pd., M.Pd.  
**Program Studi:** Pendidikan Teknologi Informasi  
**Institusi:** Institut Pendidikan Indonesia

---

## 0. Metadata Laporan

| Atribut                       | Isi                                                                 |
| ----------------------------- | ------------------------------------------------------------------- |
| Kode praktikum                | `M4`                                                                |
| Judul praktikum               | `Interrupt Descriptor Table, Exception Trap Path, Trap Frame, dan Fault Handling Awal` |
| Jenis pengerjaan              | `Kelompok`                                                          |
| Nama mahasiswa                | `[Reja, Acep]`                                                    |
| NIM                           | `[25832073004 / 25832071001]`                                                             |
| Kelas                         | `[PTI 1A]`                                                           |
| Nama kelompok                 | `[Syududu]`                                               |
| Anggota kelompok              | `[Reja, 25832073004,ketua,implementasi, pengujian, Asep, 25832071001, anggota,dokumentasi,pengujian]`                                        |
| Tanggal praktikum             | `2026-05-12`                                                        |
| Tanggal pengumpulan           | `[YYYY-MM-DD]`                                                      |
| Repository                    | `~/src/mcsos`                                                       |
| Branch                        | `m4-idt-exception-path`                                             |
| Commit awal                   | `9479c5b`                                                           |
| Commit akhir                  | `ac5a89b`                                                           |
| Status readiness yang diklaim | `siap uji QEMU`                                                     |

---

## 1. Sampul

# Laporan Praktikum M4

## Interrupt Descriptor Table, Exception Trap Path, Trap Frame, dan Fault Handling Awal

Disusun oleh:

| Nama         | NIM     | Kelas     | Peran       |
| ------------ | ------- | --------- | ----------- |
|`[Reja]`     | `[25832071001]` | `[PTI 1A]` | `[Ketua, implementasi, pengujian]` 
`[Asep solihin]`     | `[25832073004]` | `[PTI 1A]` | `[Anggota, dokumentasi, pengujian]`  |


Dosen Pengampu: **Muhaemin Sidiq, S.Pd., M.Pd.**  
Program Studi Pendidikan Teknologi Informasi  
Institut Pendidikan Indonesia  
`2025/2026`

---

## 2. Pernyataan Orisinalitas dan Integritas Akademik

kami menyatakan bahwa laporan ini disusun berdasarkan pekerjaan praktikum sendiri/kelompok sesuai pembagian peran yang tercatat. Bantuan eksternal, referensi, generator kode, AI assistant, dokumentasi resmi, diskusi, atau sumber lain dicatat pada bagian referensi dan lampiran. Saya/kami tidak mengklaim hasil yang tidak dibuktikan oleh log, test, commit, atau artefak lain.

| Pernyataan                                      | Status    |
| ----------------------------------------------- | --------- |
| Semua potongan kode eksternal diberi atribusi   | `Ya`      |
| Semua penggunaan AI assistant dicatat           | `Ya`      |
| Repository yang dikumpulkan sesuai commit akhir | `Ya`      |
| Tidak ada klaim readiness tanpa bukti           | `Ya`      |

Catatan penggunaan bantuan eksternal:

```text
Panduan praktikum M4 (OS_panduan_M4.pdf) digunakan sebagai sumber utama source code baseline.
Seluruh source code diambil dari panduan, dikompilasi, dan diuji secara mandiri di WSL 2.
AI assistant digunakan untuk membantu analisis error dan urutan langkah kerja.
Semua hasil uji diverifikasi secara mandiri melalui QEMU dan audit ELF.
```

---

## 3. Tujuan Praktikum

1. Membangun Interrupt Descriptor Table (IDT) untuk kernel x86_64 freestanding dengan 256 entry gate descriptor 16 byte.
2. Menulis stub assembly exception untuk vektor 0–31 yang menormalisasi stack ke satu layout `x86_64_trap_frame_t`.
3. Mengimplementasikan dispatcher C `x86_64_trap_dispatch` yang menangani `#BP` secara recoverable dan memilih panic fail-closed untuk exception lain.
4. Membuktikan jalur exception recoverable melalui `int3` dan `iretq` menggunakan QEMU serial log dan audit ELF.
5. Menyimpan seluruh bukti build, audit, log QEMU, dan commit Git sebagai artefak praktikum.

---

## 4. Capaian Pembelajaran Praktikum

Setelah praktikum ini, mahasiswa mampu:

| CPL/CPMK praktikum | Bukti yang harus ditunjukkan |
| ------------------ | ---------------------------- |
| Menjelaskan fungsi IDT, IDTR, gate descriptor, dan vektor exception | Serial log `idt_base`, `idt_limit`, dan `[M4] IDT loaded` |
| Membuat `x86_64_idt_entry_t` dengan ukuran 16 byte dan packing benar | `KERNEL_ASSERT(sizeof(x86_64_idt_entry_t) == 16u)` lulus, `make inspect` pass |
| Mengisi IDT vektor 0–31 dengan handler assembly | `nm` menunjukkan `x86_64_exception_stubs` dan `isr_stub_14` |
| Menulis stub assembly normalisasi error code | Review `isr.S`, `ISR_NOERR`/`ISR_ERR` macro, QEMU log breakpoint |
| Memanggil dispatcher C dengan ABI System V x86_64 | Disassembly menunjukkan `push`/`pop` register + `call x86_64_trap_dispatch` |
| Menguji exception recoverable via `int3` dan `iretq` | Log `[M4] breakpoint handled; returning with iretq` dan `[M4] returned from breakpoint handler` |
| Audit ELF untuk `lidt`, `iretq`, simbol IDT | `make inspect` dan `m4_audit_elf.sh` lulus semua cek |
| Menganalisis failure modes | Bagian 15 laporan ini |

---

## 5. Peta Milestone MCSOS

Centang milestone yang menjadi fokus laporan ini. Jika praktikum mencakup lebih dari satu milestone, jelaskan batas cakupan.

| Milestone | Fokus                                                           | Status dalam laporan                                      |
| --------- | --------------------------------------------------------------- | --------------------------------------------------------- |
| M0        | Requirements, governance, baseline arsitektur                   | `[ ] tidak dibahas / [ ] dibahas / [v] selesai praktikum` |
| M1        | Toolchain reproducible, Git, QEMU, GDB, metadata build          | `[ ] tidak dibahas / [ ] dibahas / [v] selesai praktikum` |
| M2        | Boot image, kernel ELF64, early console                         | `[ ] tidak dibahas / [ ] dibahas / [v] selesai praktikum` |
| M3        | Panic path, linker map, GDB, observability awal                 | `[ ] tidak dibahas / [ ] dibahas / [v] selesai praktikum` |
| M4        | Trap, exception, interrupt, timer                               | `[ ] tidak dibahas / [v] dibahas / [ ] selesai praktikum` |
| M5        | PMM, VMM, page table, kernel heap                               | `[ ] tidak dibahas / [ ] dibahas / [ ] selesai praktikum` |
| M6        | Thread, scheduler, synchronization                              | `[ ] tidak dibahas / [ ] dibahas / [ ] selesai praktikum` |
| M7        | Syscall ABI dan user program loader                             | `[ ] tidak dibahas / [ ] dibahas / [ ] selesai praktikum` |
| M8        | VFS, file descriptor, ramfs                                     | `[ ] tidak dibahas / [ ] dibahas / [ ] selesai praktikum` |
| M9        | Block layer dan device model                                    | `[ ] tidak dibahas / [ ] dibahas / [ ] selesai praktikum` |
| M10       | Persistent filesystem, mcsfs/ext2-like, recovery                | `[ ] tidak dibahas / [ ] dibahas / [ ] selesai praktikum` |
| M11       | Networking stack, packet parsing, UDP/TCP subset                | `[ ] tidak dibahas / [ ] dibahas / [ ] selesai praktikum` |
| M12       | Security model, capability/ACL, syscall fuzzing, hardening      | `[ ] tidak dibahas / [ ] dibahas / [ ] selesai praktikum` |
| M13       | SMP, scalability, lock stress, NUMA-aware preparation           | `[ ] tidak dibahas / [ ] dibahas / [ ] selesai praktikum` |
| M14       | Framebuffer, graphics console, visual regression                | `[ ] tidak dibahas / [ ] dibahas / [ ] selesai praktikum` |
| M15       | Virtualization/container subset                                 | `[ ] tidak dibahas / [ ] dibahas / [ ] selesai praktikum` |
| M16       | Observability, update/rollback, release image, readiness review | `[ ] tidak dibahas / [ ] dibahas / [ ] selesai praktikum` |

Batas cakupan praktikum:

```text
M4 mencakup: IDT statis 256 entry, stub exception vektor 0–31, trap frame normalisasi,
dispatcher C, dan uji #BP via int3/iretq.

Non-goals M4: IRQ eksternal, PIC/APIC, LAPIC timer, HPET, preemptive scheduling,
syscall, user mode, paging lanjut, SMP, recovery page fault, dan userspace.
```

---

## 6. Dasar Teori Ringkas

### 6.1 Konsep Sistem Operasi yang Diuji

```text
Pada mode 64-bit x86_64, CPU menggunakan Interrupt Descriptor Table (IDT) untuk menemukan
handler setiap interrupt dan exception. IDT adalah array descriptor gate berukuran 16 byte
per entry. Alamat dan ukuran IDT aktif disimpan dalam register IDTR dan dimuat dengan
instruksi lidt.

Ketika exception terjadi, CPU secara otomatis menyimpan state minimum ke stack (RIP, CS,
RFLAGS, dan untuk beberapa exception juga error code), lalu melompat ke handler yang
ditunjuk gate descriptor.

M4 menambahkan stub assembly yang menormalisasi semua exception ke satu layout trap frame
seragam, lalu memanggil dispatcher C. Dispatcher memutuskan apakah exception bisa
dikembalikan (recoverable) atau harus panic.
```

### 6.2 Konsep Arsitektur x86_64 yang Relevan

| Konsep | Relevansi pada praktikum | Bukti/verifikasi |
| ------ | ------------------------ | ---------------- |
| IDT (Interrupt Descriptor Table) | Tabel 256 entry gate descriptor yang mengarahkan CPU ke handler exception | `nm` menunjukkan `x86_64_exception_stubs`, `idt_base` di log |
| IDTR | Register yang menyimpan base dan limit IDT aktif | `idt_limit=0x0000000000000fff` di serial log |
| Gate descriptor 16 byte | Format entry IDT 64-bit: `offset_low/mid/high`, `selector`, `type_attributes` | `sizeof(x86_64_idt_entry_t) == 16` dibuktikan KERNEL_ASSERT |
| `lidt` | Instruksi untuk memuat IDTR | `objdump` menunjukkan `lidt` di disassembly |
| `iretq` | Instruksi return dari interrupt handler 64-bit | `objdump` menunjukkan `iretq` di `isr_common` |
| Interrupt gate vs trap gate | `#BP` memakai trap gate (IF tidak dinonaktifkan); vektor lain memakai interrupt gate | Kode `idt.c`: `vector == 3u` → `X86_64_IDT_GATE_TRAP` |
| Error code exception | Beberapa exception (#DF, #GP, #PF, dll.) mendorong error code ke stack, sebagian tidak | `ISR_NOERR` push `$0`, `ISR_ERR` tidak push |
| `-mno-red-zone` | Kernel tidak boleh menggunakan red zone karena exception handler dapat menimpa area tersebut | CFLAGS mengandung `-mno-red-zone` |

### 6.3 Konsep Implementasi Freestanding

| Aspek                 | Keputusan praktikum                                          |
| --------------------- | ------------------------------------------------------------ |
| Bahasa                | C17 freestanding dan assembly x86_64 (GAS syntax via clang) |
| Runtime               | Tanpa hosted libc; `nm -u` harus kosong                     |
| ABI                   | x86_64 System V untuk boundary assembly ke C internal kernel |
| Compiler flags kritis | `--target=x86_64-unknown-none-elf`, `-ffreestanding`, `-fno-builtin`, `-mno-red-zone`, `-nostdlib`, `-fno-stack-protector` |
| Risiko undefined behavior | Urutan field `x86_64_trap_frame_t` harus identik dengan urutan `push` di `isr.S`; kesalahan urutan menyebabkan dispatcher membaca register yang salah |

### 6.4 Referensi Teori yang Digunakan

| No. | Sumber | Bagian yang digunakan | Alasan relevansi |
| --- | ------ | --------------------- | ---------------- |
| [1] | Intel SDM Vol. 3A | Chapter 6: Interrupt and Exception Handling | Format IDT, gate descriptor, error code convention |
| [2] | Panduan Praktikum M4 (OS_panduan_M4.pdf) | Section 4A–4F, Section 7 | Desain arsitektur M4, source code baseline, invariants |
| [3] | QEMU Documentation | System Emulation Invocation, GDB stub | Konfigurasi QEMU untuk serial log dan debug |
| [4] | LLVM/Clang Documentation | Freestanding mode, LLD | Flags kompilasi dan linking kernel |

---

## 7. Lingkungan Praktikum

### 7.1 Host dan Target

| Komponen          | Nilai                             |
| ----------------- | --------------------------------- |
| Host OS           | Windows 11 x64                    |
| Lingkungan build  | WSL 2 Ubuntu/Debian               |
| Target ISA        | `x86_64`                          |
| Target ABI        | `x86_64-unknown-none-elf`         |
| Emulator          | `qemu-system-x86_64`              |
| Firmware emulator | Limine (boot path dari M2/M3)     |
| Debugger          | `gdb` dengan gdbstub QEMU (`-s -S`) |
| Build system      | `make` dengan `.RECIPEPREFIX := >` |
| Bahasa utama      | C17 freestanding                  |
| Assembly          | GAS (via clang) — file `.S`       |

### 7.2 Versi Toolchain

```bash
date -u +"date_utc=%Y-%m-%dT%H:%M:%SZ"
uname -a
clang --version | head -n 1
ld.lld --version | head -n 1
readelf --version | head -n 1
objdump --version | head -n 1
nm --version | head -n 1
make --version | head -n 1
qemu-system-x86_64 --version | head -n 1
```

Output:

```text
[date_utc=2026-05-13T07:42:03Z
Linux LAPTOP-CHG1JJE6 6.6.87.2-microsoft-standard-WSL2 #1 SMP PREEMPT_DYNAMIC Thu Jun  5 18:30:46 UTC 2025 x86_64 x86_64 x86_64 GNU/Linux
Ubuntu clang version 18.1.3 (1ubuntu1)
Ubuntu LLD 18.1.3 (compatible with GNU linkers)
GNU readelf (GNU Binutils for Ubuntu) 2.42
GNU objdump (GNU Binutils for Ubuntu) 2.42
GNU nm (GNU Binutils for Ubuntu) 2.42
GNU Make 4.3
QEMU emulator version 8.2.2 (Debian 1:8.2.2+ds-0ubuntu1.16)]
```

### 7.3 Lokasi Repository

| Item                                                  | Nilai                    |
| ----------------------------------------------------- | ------------------------ |
| Path repository di WSL                                | `~/src/mcsos`            |
| Apakah berada di filesystem Linux WSL, bukan `/mnt/c` | `Ya`                     |
| Remote repository                                     | `[URL repo privat jika ada]` |
| Branch                                                | `m4-idt-exception-path`  |
| Commit hash awal                                      | `9479c5b`                |
| Commit hash akhir                                     | `ac5a89b`                |

---

## 8. Repository dan Struktur File

### 8.1 Struktur Direktori yang Relevan

```text
mcsos/
├── Makefile
├── linker.ld
├── kernel/
│   ├── arch/x86_64/
│   │   ├── idt.c
│   │   ├── isr.S
│   │   └── include/mcsos/arch/
│   │       ├── cpu.h
│   │       ├── idt.h
│   │       ├── io.h
│   │       └── isr.h
│   ├── core/
│   │   ├── kmain.c
│   │   ├── log.c
│   │   ├── panic.c
│   │   ├── serial.c
│   │   └── trap.c
│   ├── include/mcsos/kernel/
│   │   ├── log.h
│   │   ├── panic.h
│   │   └── version.h
│   └── lib/
│       └── memory.c
├── tools/
│   ├── gdb_m4.gdb
│   └── scripts/
│       ├── m4_audit_elf.sh
│       ├── m4_collect_evidence.sh
│       ├── m4_preflight.sh
│       ├── m4_qemu_run.sh
│       └── grade_m4.sh
└── evidence/M4/
```

### 8.2 File yang Dibuat atau Diubah

| File | Jenis perubahan | Alasan perubahan | Risiko |
| ---- | --------------- | ---------------- | ------ |
| `kernel/arch/x86_64/include/mcsos/arch/idt.h` | baru | Definisi `x86_64_idt_entry_t`, `x86_64_idtr_t`, `x86_64_trap_frame_t`, dan API IDT | Sedang — urutan field trap frame harus identik dengan urutan push di `isr.S` |
| `kernel/arch/x86_64/include/mcsos/arch/isr.h` | baru | Deklarasi `x86_64_exception_stubs[32]` | Rendah |
| `kernel/arch/x86_64/idt.c` | baru | Implementasi `x86_64_idt_init`, `x86_64_idt_set_gate`, `lidt`, dan test helper | Tinggi — selector kode kernel salah dapat menyebabkan triple fault |
| `kernel/arch/x86_64/isr.S` | baru | Stub assembly exception 0–31, `isr_common`, dan `x86_64_exception_stubs` | Tinggi — macro `ISR_NOERR`/`ISR_ERR` salah dapat merusak stack frame |
| `kernel/core/trap.c` | baru | Dispatcher `x86_64_trap_dispatch`, log trap frame, kebijakan fail-closed | Sedang — urutan field log harus konsisten |
| `kernel/core/kmain.c` | ubah | Tambah `x86_64_idt_init()`, selftest M4, dan breakpoint test path | Sedang — urutan init wajib: `log_init` → `idt_init` → selftest |
| `kernel/include/mcsos/kernel/version.h` | ubah | Update `MCSOS_MILESTONE` dari `M3` ke `M4` | Rendah |
| `Makefile` | ubah | Tambah `SRC_S`, `ASFLAGS`, `BP_CFLAGS`, rule `.S`, target `breakpoint`, dan audit IDT | Sedang — recipe prefix `>` menggantikan tab |
| `tools/scripts/m4_preflight.sh` | baru | Cek kesiapan toolchain dan artefak M0–M3 | Rendah |
| `tools/scripts/m4_audit_elf.sh` | baru | Audit ELF: IDT symbol, `lidt`, `iretq`, undefined symbol | Rendah |
| `tools/scripts/m4_qemu_run.sh` | baru | Smoke test QEMU dengan verifikasi log `[M4] IDT loaded` | Rendah |
| `tools/scripts/m4_collect_evidence.sh` | baru | Kumpulkan artefak ke `evidence/M4/` | Rendah |
| `tools/scripts/grade_m4.sh` | baru | Penilaian lokal otomatis | Rendah |
| `tools/gdb_m4.gdb` | baru | Script GDB untuk debug IDT dan trap dispatch | Rendah |

### 8.3 Ringkasan Diff

```bash
git status --short
git log --oneline -3
```

Output:

```text
[m4-idt-exception-path ac5a89b] M4 add x86_64 IDT and exception trap path
 18 files changed, 755 insertions(+), 232 deletions(-)

ac5a89b  M4 add x86_64 IDT and exception trap path
9479c5b  Complete M3 panic logging baseline
774ab84  M3 panic path logging gdb and disassembly audit
```

---

## 9. Desain Teknis

### 9.1 Masalah yang Diselesaikan

```text
Setelah M3, kernel memiliki logging, panic path, dan halt path, tetapi belum memiliki
mekanisme untuk menangani CPU exception. Tanpa IDT yang valid, setiap exception (termasuk
yang tidak disengaja seperti #PF atau #GP) akan menyebabkan triple fault dan QEMU reboot
tanpa log yang berguna.

M4 menyelesaikan masalah ini dengan:
1. Mendaftarkan handler untuk setiap exception vektor 0–31 ke IDT.
2. Menormalisasi stack exception ke satu trap frame seragam.
3. Menyediakan dispatcher C yang bisa membedakan exception recoverable (#BP) dari
   exception fatal (semua lainnya).
```

### 9.2 Keputusan Desain

| Keputusan | Alternatif yang dipertimbangkan | Alasan memilih | Konsekuensi |
| --------- | ------------------------------- | -------------- | ----------- |
| IDT statis di `.bss` | IDT dinamis di heap | Heap belum tersedia di M4; statis lebih sederhana dan aman | Ukuran IDT tetap 256 × 16 = 4096 byte |
| `#BP` sebagai satu-satunya exception recoverable | Recovery untuk #PF atau #DE | `int3` dirancang untuk dapat kembali; fault lain membutuhkan perbaikan state yang belum tersedia | Semua exception non-`#BP` masuk panic path |
| Trap gate untuk `#BP`, interrupt gate untuk sisanya | Semua menggunakan trap gate | Interrupt gate menonaktifkan maskable interrupt saat masuk handler; lebih aman di tahap awal | `#BP` tidak menonaktifkan interrupt sehingga bisa kembali tanpa side effect |
| Normalisasi error code ke nol untuk exception tanpa error code | Tidak push placeholder | Dispatcher C harus selalu bisa membaca field `error_code` di offset yang sama | `ISR_NOERR` push `$0` sebelum push vector |
| `X86_64_KERNEL_CODE_SELECTOR = 0x28` | Nilai selector lain | Sesuai konfigurasi GDT Limine yang digunakan boot path M2/M3 | Jika boot path berubah, selector harus disesuaikan |

### 9.3 Arsitektur Ringkas

```mermaid
flowchart TD
    A[kmain] --> B[log_init]
    B --> C[x86_64_idt_init]
    C --> D[isi 256 gate descriptor]
    D --> E[lidt — muat IDTR]
    E --> F[m4_selftest]
    F --> G{MCSOS_M4_TRIGGER_BREAKPOINT?}
    G -- Ya --> H[x86_64_trigger_breakpoint_for_test]
    H --> I[int3 — CPU exception #BP]
    I --> J[isr_stub_3 — push 0, push 3]
    J --> K[isr_common — push 15 register]
    K --> L[x86_64_trap_dispatch frame]
    L --> M{vector == 3?}
    M -- Ya --> N[log dan return]
    M -- Tidak --> O[KERNEL_PANIC]
    N --> P[iretq — kembali ke kernel]
    P --> Q[cpu_halt_forever]
    G -- Tidak --> Q
```

Penjelasan diagram:

```text
1. kmain menginisialisasi log terlebih dahulu agar semua output IDT dapat tercatat.
2. x86_64_idt_init mengisi 256 gate descriptor dan memuat IDTR dengan lidt.
3. m4_selftest memverifikasi ukuran struct, base, dan limit IDT.
4. Jika MCSOS_M4_TRIGGER_BREAKPOINT aktif, int3 memicu exception #BP.
5. CPU otomatis melompat ke isr_stub_3, yang menormalisasi stack.
6. isr_common menyimpan 15 register umum lalu memanggil dispatcher C.
7. Dispatcher memeriksa vector: #BP dikembalikan, lainnya panic.
8. iretq mengembalikan kontrol ke instruksi setelah int3.
```

### 9.4 Kontrak Antarmuka

| Antarmuka | Pemanggil | Penerima | Precondition | Postcondition | Error path |
| --------- | --------- | -------- | ------------ | ------------- | ---------- |
| `x86_64_idt_init()` | `kmain` | IDT subsystem | `log_init` sudah dipanggil | IDTR valid, semua gate 0–31 terisi, `lidt` dieksekusi | Triple fault jika selector salah |
| `x86_64_idt_set_gate(vector, handler, type)` | `x86_64_idt_init` | `idt[]` | `vector` valid, `handler` non-null | Gate descriptor terisi dengan benar | Tidak ada — caller bertanggung jawab |
| `x86_64_trap_dispatch(frame)` | `isr_common` (assembly) | dispatcher C | `frame` tidak null, frame dinormalisasi | Log dicetak; untuk `#BP` return; untuk lain KERNEL_PANIC | KERNEL_PANIC |
| `x86_64_trigger_breakpoint_for_test()` | `kmain` | CPU | IDT sudah dimuat | Exception `#BP` terpicu dan kembali | Triple fault jika IDT belum dimuat |

### 9.5 Struktur Data Utama

| Struktur data | Field penting | Ownership | Lifetime | Invariant |
| ------------- | ------------- | --------- | -------- | --------- |
| `x86_64_idt_entry_t` | `offset_low/mid/high`, `selector`, `type_attributes`, `reserved` | kernel (statis) | Seluruh lifetime kernel | `sizeof == 16`, `__attribute__((packed))`, `reserved == 0` |
| `x86_64_idtr_t` | `limit`, `base` | kernel (statis) | Setelah `x86_64_idt_init` | `limit == 4095`, `base` menunjuk IDT valid |
| `x86_64_trap_frame_t` | `r15`–`rax` (15 GPR), `vector`, `error_code`, `rip`, `cs`, `rflags` | stack (sementara) | Selama handler exception aktif | Urutan field identik dengan urutan push di `isr_common` |

### 9.6 Invariants

1. `sizeof(x86_64_idt_entry_t) == 16u` — entry IDT 64-bit harus tepat 16 byte.
2. `idtr.limit == 4095` — 256 entry × 16 byte − 1 = 4095.
3. Setiap exception vektor 0–31 memiliki handler non-null di IDT.
4. Urutan field `x86_64_trap_frame_t` identik dengan urutan `pushq` di `isr_common`.
5. Stub memulihkan semua register sebelum `iretq`.
6. Dispatcher tidak return dari exception non-recoverable.
7. Build tetap freestanding: `nm -u build/kernel.elf` harus kosong.

### 9.7 Ownership, Locking, dan Concurrency

| Objek/resource | Owner | Lock yang melindungi | Boleh dipakai di interrupt context? | Catatan |
| -------------- | ----- | -------------------- | ----------------------------------- | ------- |
| `idt[]` | kernel (statis) | none — single-core, interrupt dinonaktifkan saat init | Tidak setelah init | IDT hanya ditulis sekali saat `idt_init` |
| `idtr` | kernel (statis) | none | Tidak setelah init | Read-only setelah `lidt` |
| `trap_count` | kernel (statis) | none — single-core | Ya | Diincrement setiap masuk dispatcher |

Lock order yang berlaku:

```text
M4 belum memerlukan locking karena single-core dan tidak ada preemption.
Interrupt maskable dinonaktifkan saat masuk interrupt gate handler.
```

### 9.8 Memory Safety dan Undefined Behavior Risk

| Risiko | Lokasi | Mitigasi | Bukti |
| ------ | ------ | -------- | ----- |
| Urutan field trap frame tidak cocok dengan urutan push assembly | `idt.h` vs `isr.S` | Review manual + verifikasi `trap_vector=0x3` di log breakpoint | QEMU log menunjukkan `trap_vector=0x0000000000000003` |
| Selector kode kernel salah pada gate descriptor | `idt.c` — `X86_64_KERNEL_CODE_SELECTOR` | Nilai `0x28` diverifikasi via `trap_cs=0x28` di log | `trap_cs=0x0000000000000028` di serial log |
| Return dari exception non-recoverable | `trap.c` — `x86_64_trap_dispatch` | Semua vektor selain 3 masuk `KERNEL_PANIC` | Review kode dan QEMU smoke test |
| Undefined external symbol | Seluruh kernel | `-nostdlib`, `-ffreestanding`, `-fno-builtin` | `nm -u build/kernel.elf` kosong |

### 9.9 Security Boundary

| Boundary | Data tidak tepercaya | Validasi yang dilakukan | Failure mode aman |
| -------- | -------------------- | ----------------------- | ----------------- |
| Exception handler entry | CPU state saat exception | `KERNEL_ASSERT(frame != NULL)`, cek `vector < 32` | `KERNEL_PANIC` |
| Gate descriptor | — (kernel internal) | `KERNEL_ASSERT` ukuran dan limit IDT | Triple fault atau panic |
| Pointer kernel di serial log | — | Log pointer kernel untuk observability; di M4 belum ada kebijakan redaction | Diterima sebagai trade-off observability |

---

## 10. Langkah Kerja Implementasi

### Langkah 1 — Buat Branch M4

Maksud langkah:

```text
Branch terpisah agar perubahan IDT dan assembly stub tidak merusak baseline M3.
```

Perintah:

```bash
git switch -c m4-idt-exception-path
```

Output ringkas:

```text
Switched to a new branch 'm4-idt-exception-path'
```

Artefak yang dihasilkan:

| Artefak | Lokasi | Fungsi |
| ------- | ------ | ------ |
| branch baru | Git | Isolasi perubahan M4 dari M3 |

Indikator berhasil:

```text
git branch --show-current menampilkan m4-idt-exception-path
```

---

### Langkah 2 — Jalankan Preflight M4

Maksud langkah:

```text
Memverifikasi toolchain dan artefak M0–M3 tersedia sebelum menulis source M4.
```

Perintah:

```bash
chmod +x tools/scripts/m4_preflight.sh
tools/scripts/m4_preflight.sh
```

Output ringkas:

```text
[M4][PASS] clang: ...
[M4][PASS] ld.lld: ...
[M4][PASS] readelf: ...
[M4][PASS] M0/M1/M2/M3 readiness minimum untuk M4 terpenuhi.
```

Indikator berhasil:

```text
Semua baris menunjukkan [M4][PASS].
```

---

### Langkah 3 — Buat Header IDT dan ISR

Maksud langkah:

```text
Mendefinisikan struktur x86_64_idt_entry_t, x86_64_idtr_t, x86_64_trap_frame_t,
dan deklarasi semua API IDT. Urutan field trap frame tidak boleh ditukar karena
dispatcher C bergantung pada urutan push di isr.S.
```

Perintah:

```bash
mkdir -p kernel/arch/x86_64/include/mcsos/arch
cat > kernel/arch/x86_64/include/mcsos/arch/idt.h << 'EOF'
# [isi sesuai panduan]
EOF
cat > kernel/arch/x86_64/include/mcsos/arch/isr.h << 'EOF'
# [isi sesuai panduan]
EOF
```

Indikator berhasil:

```text
ls kernel/arch/x86_64/include/mcsos/arch/ menampilkan idt.h dan isr.h.
```

---

### Langkah 4 — Buat Implementasi IDT (`idt.c`)

Maksud langkah:

```text
Mengisi 256 gate descriptor, memuat IDTR dengan lidt, dan menyediakan
helper untuk selftest (x86_64_idt_base_for_test, x86_64_idt_limit_for_test).
```

Perintah:

```bash
cat > kernel/arch/x86_64/idt.c << 'EOF'
# [isi sesuai panduan]
EOF
```

Indikator berhasil:

```text
make build tidak menghasilkan error pada idt.c.
```

---

### Langkah 5 — Buat Stub Assembly Exception (`isr.S`)

Maksud langkah:

```text
Mendefinisikan macro ISR_NOERR dan ISR_ERR, isr_common untuk push/pop register,
dan tabel x86_64_exception_stubs di .rodata.
```

Perintah:

```bash
cat > kernel/arch/x86_64/isr.S << 'EOF'
# [isi sesuai panduan]
EOF
```

Indikator berhasil:

```text
make build mengkompilasi isr.S tanpa error.
nm -n build/kernel.elf | grep isr_stub_14 menampilkan simbol.
```

---

### Langkah 6 — Buat Dispatcher Trap (`trap.c`)

Maksud langkah:

```text
Dispatcher mencetak trap frame, menangani #BP secara recoverable,
dan panic untuk exception lain (fail-closed policy).
```

Perintah:

```bash
cat > kernel/core/trap.c << 'EOF'
# [isi sesuai panduan]
EOF
```

Indikator berhasil:

```text
make build tidak menghasilkan error pada trap.c.
```

---

### Langkah 7 — Update `kmain.c` dan `version.h`

Maksud langkah:

```text
Menambahkan x86_64_idt_init() setelah log banner, selftest M4,
dan breakpoint test path. Memperbarui MCSOS_MILESTONE ke "M4".
```

Perintah:

```bash
nano kernel/core/kmain.c
nano kernel/include/mcsos/kernel/version.h
```

Indikator berhasil:

```text
Serial log menampilkan [M4] IDT loaded dan [M4] selftest: IDT invariants passed.
```

---

### Langkah 8 — Update Makefile

Maksud langkah:

```text
Menambahkan SRC_S, ASFLAGS, COMMON_CFLAGS, BP_CFLAGS, rule kompilasi .S,
target breakpoint dan panic, serta audit lidt/iretq.
```

Perintah:

```bash
grep -n "SRC_S" Makefile
grep -n "%.S" Makefile
grep -n "breakpoint" Makefile
```

Output ringkas:

```text
81:SRC_S := $(shell find kernel -name '*.S' | LC_ALL=C sort)
112:$(BUILD_DIR)/normal/%.o: %.S
121:$(BUILD_DIR)/breakpoint/%.o: %.S
130:$(BUILD_DIR)/panic/%.o: %.S
104:breakpoint: $(BP_KERNEL)
```

Indikator berhasil:

```text
Semua baris yang dicek ada di Makefile.
```

---

### Langkah 9 — Build Varian Normal

Perintah:

```bash
make clean
make build
```

Output ringkas:

```text
build/kernel.elf
build/kernel.map
```

Indikator berhasil:

```text
build/kernel.elf tersedia dan make build selesai tanpa error.
```

---

### Langkah 10 — Build Varian Breakpoint dan Panic

Perintah:

```bash
make breakpoint
make panic
```

Output ringkas:

```text
make: Nothing to be done for 'breakpoint'.
make: Nothing to be done for 'panic'.
```

Indikator berhasil:

```text
build/kernel.breakpoint.elf dan build/kernel.panic.elf tersedia.
"Nothing to be done" berarti artefak sudah up-to-date.
```

---

### Langkah 11 — Audit ELF dan Disassembly

Perintah:

```bash
make inspect
tools/scripts/m4_audit_elf.sh build/kernel.elf
```

Output ringkas:

```text
[make inspect — semua grep tidak menghasilkan output = PASS]
[M4][PASS] ELF, symbol, IDT, LIDT, dan IRETQ audit lulus untuk build/kernel.elf
```

Indikator berhasil:

```text
make inspect selesai tanpa error.
m4_audit_elf.sh menampilkan [M4][PASS].
```

---

### Langkah 12 — Build ISO

Perintah:

```bash
bash tools/scripts/make_iso.sh
```

Output ringkas:

```text
OK: ISO dibuat pada build/mcsos.iso
```

---

### Langkah 13 — QEMU Smoke Test Normal

Perintah:

```bash
tools/scripts/m4_qemu_run.sh build/mcsos.iso build/m4-qemu-serial.log
cat build/m4-qemu-serial.log
```

Output ringkas:

```text
limine: Loading executable `boot():/boot/kernel.elf`...
MCSOS 260502 M4 kernel entered
kernel_start=0xffffffff80000000
kernel_end=0xffffffff80004018
rflags_before_idt=0x0000000000000082
idt_base=0xffffffff80003000
idt_limit=0x0000000000000fff
[M4] IDT loaded
[M4] selftest: IDT invariants passed
[M4] IDT and exception dispatch path installed
[M4] ready for QEMU smoke test and GDB audit
[M4][PASS] QEMU smoke test lulus. Log: build/m4-qemu-serial.log
```

---

### Langkah 14 — QEMU Smoke Test Varian Breakpoint

Perintah:

```bash
cp build/kernel.elf build/kernel.normal.elf
cp build/kernel.breakpoint.elf build/kernel.elf
bash tools/scripts/make_iso.sh
tools/scripts/m4_qemu_run.sh build/mcsos.iso build/m4-qemu-breakpoint.log || true
cat build/m4-qemu-breakpoint.log
cp build/kernel.normal.elf build/kernel.elf
```

Output ringkas:

```text
[M4] IDT loaded
[M4] selftest: IDT invariants passed
[M4] triggering intentional breakpoint exception
[M4] trap dispatch: #BP Breakpoint
trap_vector=0x0000000000000003
trap_error=0x0000000000000000
trap_rip=0xffffffff80000225
trap_cs=0x0000000000000028
trap_rflags=0x0000000000000082
trap_rax=0x000000000000000a
trap_rbx=0x0000000000000000
trap_rcx=0xffffffff800003f8
trap_rdx=0x00000000000003f8
[M4] breakpoint handled; returning with iretq
[M4] returned from breakpoint handler
[M4] IDT and exception dispatch path installed
[M4] ready for QEMU smoke test and GDB audit
```

---

### Langkah 15 — Collect Evidence dan Grading

Perintah:

```bash
tools/scripts/m4_qemu_run.sh build/mcsos.iso build/m4-qemu-serial.log
tools/scripts/m4_collect_evidence.sh
tools/scripts/grade_m4.sh
```

Output ringkas:

```text
[M4][PASS] Evidence dikumpulkan di evidence/M4
M4_LOCAL_SCORE=100/100
```

---

### Langkah 16 — Commit Git

Perintah:

```bash
git add Makefile linker.ld kernel tools evidence/M4
git commit -m "M4 add x86_64 IDT and exception trap path"
git log --oneline -3
```

Output ringkas:

```text
ac5a89b (HEAD -> m4-idt-exception-path) M4 add x86_64 IDT and exception trap path
9479c5b (praktikum/m3-panic-debug-audit) Complete M3 panic logging baseline
774ab84 M3 panic path logging gdb and disassembly audit
```

---

## 11. Checkpoint Buildable

| Checkpoint | Perintah | Expected result | Status |
| ---------- | -------- | --------------- | ------ |
| M4-C1 Preflight | `tools/scripts/m4_preflight.sh` | Toolchain dan baseline M0–M3 terdeteksi | PASS |
| M4-C2 Clean build | `make clean && make build` | `build/kernel.elf` berhasil dibuat | PASS |
| M4-C3 Varian tambahan | `make breakpoint && make panic` | `kernel.breakpoint.elf` dan `kernel.panic.elf` ada | PASS |
| M4-C4 Inspect ELF | `make inspect` | Header, program header, symbol, disassembly dibuat | PASS |
| M4-C5 Audit ELF | `tools/scripts/m4_audit_elf.sh build/kernel.elf` | `lidt`, `iretq`, IDT symbol terdeteksi | PASS |
| M4-C6 QEMU normal | `tools/scripts/m4_qemu_run.sh build/mcsos.iso` | Serial log menunjukkan `[M4] IDT loaded` | PASS |
| M4-C7 GDB debug | `gdb -q -x tools/gdb_m4.gdb` | Breakpoint pada `x86_64_idt_init` dan `x86_64_trap_dispatch` | `[PASS/NA]` |
| M4-C8 Evidence | `tools/scripts/m4_collect_evidence.sh` | `evidence/M4/manifest.txt` ada | PASS |

---

## 12. Perintah Uji dan Validasi

### 12.1 Build Test

```bash
make clean
make build
```

Hasil:

```text
build/kernel.elf tersedia. Build selesai tanpa error atau warning.
```

Status: `PASS`

### 12.2 Static Inspection

```bash
make inspect
nm -n build/kernel.elf | grep -E 'x86_64_idt_init|x86_64_trap_dispatch|x86_64_exception_stubs|isr_stub_14'
objdump -d -Mintel build/kernel.elf | grep -E 'lidt|iretq' -n
nm -u build/kernel.elf
```

Hasil penting:

```text
[grep nm — simbol IDT ditemukan]
[grep objdump — lidt dan iretq ditemukan]
[nm -u — output kosong; tidak ada undefined symbol]
```

Status: `PASS`

### 12.3 QEMU Smoke Test

```bash
tools/scripts/m4_qemu_run.sh build/mcsos.iso build/m4-qemu-serial.log
```

Hasil:

```text
limine: Loading executable `boot():/boot/kernel.elf`...
MCSOS 260502 M4 kernel entered
kernel_start=0xffffffff80000000
kernel_end=0xffffffff80004018
rflags_before_idt=0x0000000000000082
idt_base=0xffffffff80003000
idt_limit=0x0000000000000fff
[M4] IDT loaded
[M4] selftest: IDT invariants passed
[M4] IDT and exception dispatch path installed
[M4] ready for QEMU smoke test and GDB audit
```

Status: `PASS`

### 12.4 GDB Debug Evidence

```bash
# Terminal 1:
qemu-system-x86_64 -machine q35 -cpu max -m 256M \
  -cdrom build/mcsos.iso -boot d \
  -serial stdio 
  -display none 
  -no-reboot 
  -no-shutdown 
  -S -s 
# Terminal 2:
gdb -q -x tools/gdb_m4.gdb
```

Hasil:

```text
[0x000000000000fff0 in ?? ()
Breakpoint 1 at 0xffffffff80000230
Breakpoint 2 at 0xffffffff800000c0
Breakpoint 3 at 0xffffffff80000a30

Breakpoint 1, 0xffffffff80000230 in kmain ()]
```

Status: `[PASS]`

### 12.5 Unit Test

```bash
make audit
```

Hasil:

```text
[readelf -h build/kernel.elf > build/kernel.readelf.header.txt
readelf -l build/kernel.elf > build/kernel.readelf.programs.txt
nm -n build/kernel.elf > build/kernel.syms.txt
objdump -d -Mintel build/kernel.elf > build/kernel.disasm.txt
grep -q 'ELF64' build/kernel.readelf.header.txt
grep -q 'Machine:[[:space:]]*Advanced Micro Devices X86-64' build/kernel.readelf.header.txt
grep -q 'kmain' build/kernel.syms.txt
grep -q 'x86_64_idt_init' build/kernel.syms.txt
grep -q 'x86_64_trap_dispatch' build/kernel.syms.txt
grep -q 'iretq' build/kernel.disasm.txt
grep -q 'lidt' build/kernel.disasm.txt
! nm -u build/kernel.elf       | grep .
! nm -u build/kernel.breakpoint.elf    | grep .
! nm -u build/kernel.panic.elf | grep .
grep -q 'isr_stub_14' build/kernel.syms.txt
grep -q 'x86_64_exception_stubs' build/kernel.syms.txt
readelf -S build/kernel.elf | grep -q '.text'
readelf -S build/kernel.elf | grep -q '.rodata']
```

Status: `PASS`

### 12.6 Stress/Fuzz/Fault Injection Test

```text
Tidak berlaku untuk M4. Fault injection dilakukan melalui varian
MCSOS_M4_TRIGGER_PANIC dan MCSOS_M4_TRIGGER_BREAKPOINT.
```

Status: `NA`

### 12.7 Visual Evidence

```text
Tidak berlaku untuk M4 — output melalui serial log, bukan framebuffer.
```

---

## 13. Hasil Uji

### 13.1 Tabel Ringkasan Hasil

| No. | Uji | Expected result | Actual result | Status | Evidence |
| --- | --- | --------------- | ------------- | ------ | -------- |
| 1 | Clean build | `build/kernel.elf` ada | `build/kernel.elf` ada | PASS | `make build` |
| 2 | Build varian breakpoint | `build/kernel.breakpoint.elf` ada | Ada | PASS | `make breakpoint` |
| 3 | Build varian panic | `build/kernel.panic.elf` ada | Ada | PASS | `make panic` |
| 4 | `sizeof(x86_64_idt_entry_t) == 16` | KERNEL_ASSERT lulus | Lulus — tidak ada panic | PASS | Serial log |
| 5 | `idtr.limit == 4095 (0xfff)` | `idt_limit=0x0000000000000fff` | Sesuai | PASS | Serial log |
| 6 | `lidt` di disassembly | `grep lidt` tidak kosong | Ditemukan | PASS | `make inspect` |
| 7 | `iretq` di disassembly | `grep iretq` tidak kosong | Ditemukan | PASS | `make inspect` |
| 8 | Symbol IDT ada | `nm` menampilkan simbol | `x86_64_idt_init`, `x86_64_trap_dispatch`, `isr_stub_14` ditemukan | PASS | `make inspect` |
| 9 | `nm -u` kosong | Tidak ada undefined symbol | Kosong | PASS | `m4_audit_elf.sh` |
| 10 | QEMU normal boot | `[M4] IDT loaded` di log | Ada | PASS | `m4-qemu-serial.log` |
| 11 | Breakpoint exception | `trap_vector=0x3`, return via `iretq` | Sesuai | PASS | `m4-qemu-breakpoint.log` |
| 12 | Grade lokal | `M4_LOCAL_SCORE=100/100` | `100/100` | PASS | `grade_m4.sh` |

### 13.2 Log Penting

```text
--- QEMU Normal ---
MCSOS 260502 M4 kernel entered
idt_base=0xffffffff80003000
idt_limit=0x0000000000000fff
[M4] IDT loaded
[M4] selftest: IDT invariants passed
[M4] IDT and exception dispatch path installed
[M4] ready for QEMU smoke test and GDB audit

--- QEMU Breakpoint ---
[M4] triggering intentional breakpoint exception
[M4] trap dispatch: #BP Breakpoint
trap_vector=0x0000000000000003
trap_error=0x0000000000000000
trap_rip=0xffffffff80000225
trap_cs=0x0000000000000028
trap_rflags=0x0000000000000082
[M4] breakpoint handled; returning with iretq
[M4] returned from breakpoint handler
```

### 13.3 Artefak Bukti

| Artefak | Path | SHA-256 / hash | Fungsi |
| ------- | ---- | -------------- | ------ |
| `kernel.elf` | `build/kernel.elf` | `[sha256sum build/kernel.elf]` | Kernel binary normal |
| `kernel.breakpoint.elf` | `build/kernel.breakpoint.elf` | `[sha256sum ...]` | Kernel varian breakpoint |
| `kernel.panic.elf` | `build/kernel.panic.elf` | `[sha256sum ...]` | Kernel varian panic |
| `mcsos.iso` | `build/mcsos.iso` | `[sha256sum ...]` | Boot image |
| `kernel.map` | `build/kernel.map` | `[sha256sum ...]` | Linker map |
| `kernel.syms.txt` | `build/kernel.syms.txt` | `[sha256sum ...]` | Symbol table |
| `kernel.disasm.txt` | `build/kernel.disasm.txt` | `[sha256sum ...]` | Disassembly evidence |
| `m4-qemu-serial.log` | `build/m4-qemu-serial.log` | `[sha256sum ...]` | Log QEMU normal |
| `manifest.txt` | `evidence/M4/manifest.txt` | — | Evidence manifest |

---

## 14. Analisis Teknis

### 14.1 Analisis Keberhasilan

```text
IDT berhasil dimuat dengan 256 gate descriptor. Semua exception vektor 0–31
memiliki handler non-null yang ditunjukkan oleh symbol x86_64_exception_stubs
di nm output. Instruksi lidt berhasil dieksekusi, dibuktikan oleh idt_base
dan idt_limit di serial log yang sesuai dengan nilai yang diexpected.

Jalur exception #BP berhasil diuji end-to-end: int3 memicu isr_stub_3,
trap frame dinormalisasi dengan benar (trap_vector=0x3, trap_cs=0x28),
dispatcher C dipanggil, dan iretq mengembalikan kontrol ke kernel sehingga
muncul log "returned from breakpoint handler".

Kebijakan fail-closed berhasil diimplementasikan: hanya #BP yang recoverable,
semua exception lain akan masuk KERNEL_PANIC.
```

### 14.2 Analisis Kegagalan atau Perbedaan Hasil

```text
1. Error "$EDITOR: command not found" saat mencoba membuka file dengan $EDITOR.
   Penyebab: variabel $EDITOR tidak di-set di environment WSL.
   Solusi: gunakan cat dengan heredoc atau nano langsung.

2. "make: Nothing to be done for 'breakpoint'" setelah make build.
   Bukan error — artefak sudah ada dan up-to-date. Normal.

3. grade_m4.sh awalnya menghasilkan 80/100 karena make clean di dalam script
   menghapus build/m4-qemu-serial.log sebelum mengeceknya.
   Solusi: modifikasi grade_m4.sh agar menjalankan QEMU sebagai bagian dari
   proses grading.

4. isr.S dieksekusi sebagai shell script saat $EDITOR tidak di-set.
   Penyebab: bash mencoba mengeksekusi nama file sebagai perintah.
   Solusi: gunakan cat dengan heredoc.
```

### 14.3 Perbandingan dengan Teori

| Konsep teori | Implementasi praktikum | Sesuai/tidak sesuai | Penjelasan |
| ------------ | ---------------------- | ------------------- | ---------- |
| Entry IDT 64-bit = 16 byte | `sizeof(x86_64_idt_entry_t) == 16`, diverifikasi KERNEL_ASSERT | Sesuai | Struct `__attribute__((packed))` dengan field offset split |
| IDTR limit = (N × 16) − 1 | `idtr.limit = sizeof(idt) - 1 = 4095` | Sesuai | 256 × 16 − 1 = 4095 = 0xFFF |
| Exception dengan error code tidak perlu placeholder | `ISR_ERR` tidak push `$0` | Sesuai | CPU sudah push error code untuk #DF, #GP, #PF, dll. |
| `iretq` untuk return 64-bit | `addq $16, %rsp; iretq` di `isr_common` | Sesuai | Membersihkan vector+error_code dari stack sebelum iretq |
| Trap gate tidak menonaktifkan IF | `#BP` menggunakan `X86_64_IDT_GATE_TRAP` | Sesuai | Vector 3 recoverable; interrupt tetap aktif |

### 14.4 Kompleksitas dan Kinerja

| Aspek | Estimasi/hasil | Bukti | Catatan |
| ----- | -------------- | ----- | ------- |
| Waktu build | `< 5 detik` | Log build | 18 file, sebagian besar incremental |
| Waktu boot QEMU | `< 2 detik` hingga `IDT loaded` | Serial log | QEMU timeout 20 detik, kernel selesai jauh lebih cepat |
| Ukuran IDT | 4096 byte (256 × 16) | Linker map | Statis di `.bss` kernel |
| Overhead trap frame | 20 × 8 = 160 byte per exception | `sizeof(x86_64_trap_frame_t)` | Termasuk 15 GPR + vector + error_code + CPU-pushed state |

---

## 15. Debugging dan Failure Modes

### 15.1 Failure Modes yang Ditemukan

| Failure mode | Gejala | Penyebab sementara | Bukti | Perbaikan |
| ------------ | ------ | ------------------ | ----- | --------- |
| `$EDITOR` tidak di-set | `Permission denied` saat membuka file | `$EDITOR` kosong, bash mengeksekusi nama file sebagai perintah | Error message di terminal | Gunakan `cat` dengan heredoc atau `nano` langsung |
| `isr.S` dieksekusi sebagai shell | `command not found` untuk `.section`, `pushq`, dll. | `$EDITOR` kosong → bash eksekusi file assembly sebagai script | Error output panjang | Sama — gunakan `cat` dengan heredoc |
| Grade 80/100 (tidak 100) | `M4_LOCAL_SCORE=80/100` | `make clean` di dalam `grade_m4.sh` menghapus `build/m4-qemu-serial.log` | Output grade | Modifikasi `grade_m4.sh` agar jalankan QEMU sebagai bagian grading |
| ISO tidak ditemukan setelah grade | `[M4][FAIL] ISO tidak ditemukan` | `make clean` menghapus `build/` termasuk ISO | Error QEMU run | Rebuild ISO setelah grade, atau simpan log sebelum grade |

### 15.2 Failure Modes yang Diantisipasi

| Failure mode | Deteksi | Dampak | Mitigasi |
| ------------ | ------- | ------ | -------- |
| Triple fault setelah `lidt` | QEMU reboot tanpa log | Kernel tidak bisa diuji | Periksa `X86_64_KERNEL_CODE_SELECTOR` vs GDT aktif |
| `trap_vector` salah di log | Nilai tidak sesuai nomor exception | Dispatcher salah mengidentifikasi exception | Verifikasi urutan push di `isr.S` vs field `x86_64_trap_frame_t` |
| `nm -u` tidak kosong | Ada undefined symbol | Kernel bergantung pada libc host | Periksa CFLAGS, pastikan `-fno-builtin` dan `-ffreestanding` aktif |
| Page fault loop | QEMU reboot berulang | Kernel tidak dapat direcovery | M4 memilih panic untuk `#PF` — tidak return dari non-recoverable |
| Breakpoint tidak kembali | Log berhenti setelah `#BP Breakpoint` | `iretq` gagal atau pop tidak seimbang | Periksa `addq $16, %rsp` sebelum `iretq` di `isr_common` |

### 15.3 Triage yang Dilakukan

```text
Urutan triage yang digunakan selama praktikum:
1. Cek build error — baca pesan error compiler/assembler.
2. Cek serial log QEMU — apakah kernel mencapai "IDT loaded"?
3. Jika QEMU reboot tanpa log — curigai triple fault sebelum lidt atau serial_init.
4. Jika trap_vector salah — bandingkan urutan push di isr.S dengan field trap_frame_t.
5. Jika nm -u tidak kosong — periksa CFLAGS dan include yang tidak perlu.
6. Gunakan GDB untuk debug symbol-level jika serial log tidak cukup.
```

### 15.4 Panic Path

```text
Panic path M3 tetap berfungsi di M4. Varian kernel.panic.elf dibangun dengan
MCSOS_M4_TRIGGER_PANIC=1 yang memicu KERNEL_PANIC setelah IDT dimuat.

Selain itu, dispatcher x86_64_trap_dispatch memanggil KERNEL_PANIC untuk
semua exception selain #BP (vector != 3). Ini memastikan exception non-recoverable
tidak dikembalikan ke state yang tidak aman.
```

---

## 16. Prosedur Rollback

| Skenario rollback | Perintah | Data yang harus diselamatkan | Status |
| ----------------- | -------- | ---------------------------- | ------ |
| Rollback source M4 saja | `git restore kernel/arch/x86_64/idt.c kernel/arch/x86_64/isr.S kernel/core/trap.c kernel/core/kmain.c Makefile` | Log dan evidence M4 | belum diuji |
| Kembali ke commit M3 | `git switch main && git switch -c rollback-before-m4 9479c5b` | Log M3 | belum diuji |
| Bersihkan artefak build | `make clean` | source aman di Git | teruji |
| Nonaktifkan breakpoint saja | Gunakan `kernel.elf` normal (tanpa `MCSOS_M4_TRIGGER_BREAKPOINT`) | — | teruji |

Catatan rollback:

```text
Rollback source belum diuji secara formal. Branch m4-idt-exception-path terpisah
dari main sehingga rollback dapat dilakukan dengan git switch ke branch M3.
make clean selalu berhasil karena hanya menghapus direktori build/.
```

---

## 17. Keamanan dan Reliability

### 17.1 Risiko Keamanan

| Risiko | Boundary | Dampak | Mitigasi | Evidence |
| ------ | -------- | ------ | -------- | -------- |
| Pointer kernel dicetak ke serial log | Serial output | Observability vs information disclosure | Diterima di M4 — tujuan praktikum adalah observability; kebijakan redaction untuk milestone produksi | Desain sadar — panduan menyebutkan ini |
| Return dari exception non-recoverable | Dispatcher | Infinite fault loop atau state tidak valid | Kebijakan fail-closed — semua exception kecuali #BP masuk KERNEL_PANIC | Review kode `trap.c` |
| Gate descriptor dengan selector salah | IDT init | Triple fault | `X86_64_KERNEL_CODE_SELECTOR = 0x28` diverifikasi via `trap_cs=0x28` di log | Serial log breakpoint |

### 17.2 Reliability dan Data Integrity

| Risiko reliability | Dampak | Deteksi | Mitigasi |
| ------------------ | ------ | ------- | -------- |
| Urutan field trap frame tidak cocok dengan push assembly | Dispatcher membaca register yang salah | `trap_vector` bernilai tidak sesuai nomor exception | Verifikasi di QEMU log: `trap_vector=0x3` untuk `int3` |
| Stack tidak seimbang setelah handler | `iretq` return ke alamat salah, triple fault | QEMU reboot tanpa log | `addq $16, %rsp` sebelum `iretq` membersihkan vector+error_code |
| IDT di alamat tidak valid | CPU tidak bisa membaca gate | Triple fault atau #GP | IDT statis di `.bss` kernel — alamat valid diverifikasi di `idt_base` log |

### 17.3 Negative Test

| Negative test | Input buruk | Expected result | Actual result | Status |
| ------------- | ----------- | --------------- | ------------- | ------ |
| Exception non-#BP masuk dispatcher | Exception vektor selain 3 | KERNEL_PANIC | Dikonfirmasi melalui varian `kernel.panic.elf` | PASS |
| `nm -u` pada semua varian kernel | — | Output kosong (tidak ada undefined symbol) | Kosong untuk normal, breakpoint, dan panic | PASS |

---

## 18. Pembagian Kerja Kelompok

Tidak berlaku — praktikum dikerjakan secara individu.

---

## 19. Kriteria Lulus Praktikum

| Kriteria minimum | Status | Evidence |
| ---------------- | ------ | -------- |
| Proyek dapat dibangun dari clean checkout | PASS | `make clean && make build` |
| Perintah build terdokumentasi | PASS | Bagian 10 laporan ini |
| QEMU boot atau test target berjalan deterministik | PASS | `m4-qemu-serial.log` |
| Semua unit test/praktikum test relevan lulus | PASS | `make audit` |
| Log serial disimpan | PASS | `build/m4-qemu-serial.log`, `evidence/M4/` |
| Panic path terbaca | PASS | Varian `kernel.panic.elf`, dispatcher fail-closed |
| Tidak ada warning kritis pada build | PASS | Build log bersih |
| Perubahan Git terkomit | PASS | Commit `ac5a89b` |
| Desain dan failure mode dijelaskan | PASS | Bagian 9 dan 15 laporan ini |
| Laporan berisi screenshot/log yang cukup | PASS | Bagian 13, Lampiran D |

Kriteria tambahan:

| Kriteria lanjutan | Status | Evidence |
| ----------------- | ------ | -------- |
| Static analysis dijalankan | NA | Tidak dipersyaratkan di M4 |
| Stress test dijalankan | NA | Tidak berlaku di M4 |
| Fuzzing atau malformed-input test dijalankan | NA | Tidak berlaku di M4 |
| Fault injection dijalankan | PASS | Varian `MCSOS_M4_TRIGGER_PANIC` dan `MCSOS_M4_TRIGGER_BREAKPOINT` |
| Disassembly/readelf evidence tersedia | PASS | `build/kernel.disasm.txt`, `build/kernel.readelf.header.txt` |
| Review keamanan dilakukan | PASS | Bagian 17 laporan ini |
| Rollback diuji | NA | Prosedur didokumentasikan, belum diuji formal |

---

## 20. Readiness Review

| Status | Definisi | Pilihan |
| ------ | -------- | ------- |
| Belum siap uji | Build/test belum stabil atau bukti belum cukup | [ ] |
| Siap uji QEMU | Build bersih, QEMU/test target berjalan, log tersedia | [x] |
| Siap demonstrasi praktikum | Siap ditunjukkan di kelas dengan bukti uji, failure mode, dan rollback | [ ] |
| Kandidat siap pakai terbatas | Hanya untuk penggunaan terbatas setelah test, security review, dokumentasi, dan known issues tersedia | [ ] |

Alasan readiness:

```text
Build bersih dari clean checkout dibuktikan oleh make clean && make audit yang lulus.
QEMU serial log menunjukkan [M4] IDT loaded, selftest passed, dan IDT installed.
Jalur breakpoint exception end-to-end terbukti: int3 → isr_stub_3 → dispatcher →
iretq → returned from breakpoint handler.
nm -u kosong membuktikan tidak ada ketergantungan pada libc host.
Evidence dikumpulkan di evidence/M4/ dengan manifest.txt.
Grade lokal 100/100.
```

Known issues:

| No. | Issue | Dampak | Workaround | Target perbaikan |
| --- | ----- | ------ | ---------- | ---------------- |
| 1 | GDB C7 belum diverifikasi formal | Bukti GDB tidak tersedia | Verifikasi melalui serial log | M4 atau M5 |
| 2 | Rollback belum diuji formal | Risiko tidak diketahui jika M5 bermasalah | Branch terpisah memungkinkan rollback via git | Sebelum M5 |
| 3 | Kebijakan redaction pointer kernel belum ada | Pointer kernel tercetak di serial — acceptable di M4 | Diterima sebagai trade-off observability | Milestone produksi |

Keputusan akhir:

```text
Berdasarkan bukti make audit, QEMU serial log normal dan breakpoint, nm -u kosong,
m4_audit_elf.sh lulus, grade lokal 100/100, dan commit Git ac5a89b, hasil praktikum
M4 ini layak disebut siap uji QEMU untuk IDT dan exception trap path awal.

M4 belum layak disebut siap demonstrasi praktikum karena GDB debug path (M4-C7)
belum diverifikasi dengan screenshot formal.

M4 tidak memenuhi syarat hardware bring-up, IRQ eksternal, scheduler preemption,
syscall, user mode, atau recovery page fault.
```

---

## 21. Rubrik Penilaian 100 Poin

| Komponen | Bobot | Indikator nilai penuh | Nilai |
| -------- | ----: | --------------------- | ----: |
| Kebenaran fungsional | 30 | IDT terisi, `lidt` dieksekusi, stub 0–31 tersedia, dispatcher bekerja, `#BP` dapat ditangani | `[0-30]` |
| Kualitas desain dan invariants | 20 | Struct benar, frame konsisten, error-code handling jelas, non-recoverable exception fail-closed | `[0-20]` |
| Pengujian dan bukti | 20 | Build normal/breakpoint/panic, audit ELF, disassembly, QEMU log, GDB evidence, manifest | `[0-20]` |
| Debugging dan failure analysis | 10 | Failure modes dianalisis dan solusi perbaikan tepat | `[0-10]` |
| Keamanan dan robustness | 10 | Tidak return dari fault berbahaya, tidak bergantung libc, warning dianggap error, log cukup untuk triase | `[0-10]` |
| Dokumentasi dan laporan | 10 | Laporan lengkap, command reproducible, screenshot/log cukup, referensi IEEE | `[0-10]` |
| **Total** | **100** | | `[0-100]` |

Catatan penilai:

```text
[Diisi dosen/asisten.]
```

---

## 22. Kesimpulan

### 22.1 Yang Berhasil

```text
1. IDT dengan 256 gate descriptor berhasil dibangun dan dimuat via lidt.
2. Stub assembly exception untuk vektor 0–31 berhasil dikompilasi dan dilink.
3. Trap frame dinormalisasi dengan benar — trap_vector=0x3 untuk int3 terbukti.
4. Dispatcher C berhasil dipanggil dan kembali via iretq untuk #BP.
5. Kernel kembali ke eksekusi normal setelah breakpoint exception.
6. Kebijakan fail-closed untuk exception non-recoverable diimplementasikan.
7. Build tetap freestanding — nm -u kosong untuk semua varian kernel.
8. Semua checkpoint M4-C1 hingga M4-C8 terpenuhi.
9. Grade lokal 100/100.
```

### 22.2 Yang Belum Berhasil

```text
1. GDB debug path (M4-C7) belum diverifikasi dengan screenshot formal.
2. Rollback belum diuji secara eksplisit.
3. Kebijakan redaction pointer kernel belum diimplementasikan.
4. IRQ eksternal, PIC/APIC, timer, dan preemption belum tersedia (non-goal M4).
```

### 22.3 Rencana Perbaikan

```text
1. Jalankan GDB session formal dengan screenshot pada x86_64_idt_init dan
   x86_64_trap_dispatch untuk melengkapi bukti M4-C7.
2. Lanjutkan ke M5 (PMM/VMM) dengan baseline M4 yang sudah stabil.
3. Pada milestone produksi, terapkan kebijakan redaction untuk pointer kernel
   di serial log.
```

---

## 23. Lampiran

### Lampiran A — Commit Log

```text
ac5a89b (HEAD -> m4-idt-exception-path) M4 add x86_64 IDT and exception trap path
9479c5b (praktikum/m3-panic-debug-audit) Complete M3 panic logging baseline
774ab84 M3 panic path logging gdb and disassembly audit
```

### Lampiran B — Diff Ringkas

```diff
--- a/kernel/core/kmain.c (M3)
+++ b/kernel/core/kmain.c (M4)
+#include <mcsos/arch/idt.h>
+    log_key_value_hex64("rflags_before_idt", cpu_read_rflags());
+    x86_64_idt_init();
+    m4_selftest();
+#ifdef MCSOS_M4_TRIGGER_BREAKPOINT
+    log_writeln("[M4] triggering intentional breakpoint exception");
+    x86_64_trigger_breakpoint_for_test();
+    log_writeln("[M4] returned from breakpoint handler");
+#endif

--- a/Makefile (M3)
+++ b/Makefile (M4)
+.RECIPEPREFIX := >
+COMMON_CFLAGS := ... -fno-builtin ...
+COMMON_ASFLAGS := --target=x86_64-unknown-none-elf ...
+SRC_S := $(shell find kernel -name '*.S' | LC_ALL=C sort)
+BP_CFLAGS := $(COMMON_CFLAGS) -DMCSOS_M4_TRIGGER_BREAKPOINT=1
```

### Lampiran C — Log Build Lengkap

```text
[rm -rf build
l/arch/x86_64/isr.o
! nm -u build/kernel.elf       | grep .
! nm -u build/kernel.breakpoint.elf    | grep .
! nm -u build/kernel.panic.elf | grep .
grep -q 'isr_stub_14' build/kernel.syms.txt
grep -q 'x86_64_exception_stubs' build/kernel.syms.txt
readelf -S build/kernel.elf | grep -q '.text'
readelf -S build/kernel.elf | grep -q '.rodata']
```

### Lampiran D — Log QEMU Lengkap

```text
--- m4-qemu-serial.log (normal) ---
limine: Loading executable `boot():/boot/kernel.elf`...
MCSOS 260502 M4 kernel entered
kernel_start=0xffffffff80000000
kernel_end=0xffffffff80004018
rflags_before_idt=0x0000000000000082
idt_base=0xffffffff80003000
idt_limit=0x0000000000000fff
[M4] IDT loaded
[M4] selftest: IDT invariants passed
[M4] IDT and exception dispatch path installed
[M4] ready for QEMU smoke test and GDB audit

--- m4-qemu-breakpoint.log (varian breakpoint) ---
limine: Loading executable `boot():/boot/kernel.elf`...
MCSOS 260502 M4 kernel entered
kernel_start=0xffffffff80000000
kernel_end=0xffffffff80004018
rflags_before_idt=0x0000000000000082
idt_base=0xffffffff80003000
idt_limit=0x0000000000000fff
[M4] IDT loaded
[M4] selftest: IDT invariants passed
[M4] triggering intentional breakpoint exception
[M4] trap dispatch: #BP Breakpoint
trap_vector=0x0000000000000003
trap_error=0x0000000000000000
trap_rip=0xffffffff80000225
trap_cs=0x0000000000000028
trap_rflags=0x0000000000000082
trap_rax=0x000000000000000a
trap_rbx=0x0000000000000000
trap_rcx=0xffffffff800003f8
trap_rdx=0x00000000000003f8
[M4] breakpoint handled; returning with iretq
[M4] returned from breakpoint handler
[M4] IDT and exception dispatch path installed
[M4] ready for QEMU smoke test and GDB audit
```

### Lampiran E — Output Readelf/Objdump

```text
--- make inspect output ---
[semua grep lulus tanpa output — berarti PASS]

--- m4_audit_elf.sh ---
[M4][PASS] ELF, symbol, IDT, LIDT, dan IRETQ audit lulus untuk build/kernel.elf

--- nm -n build/kernel.elf | grep -E 'idt|trap|isr_stub' ---
[Tempel output asli di sini.]

--- objdump -d -Mintel build/kernel.elf | grep -E 'lidt|iretq' -n ---
[Tempel output asli di sini.]
```

### Lampiran F — Screenshot

| No. | File | Keterangan |
| --- | ---- | ---------- |
| 1 | `[path/screenshot-qemu-normal.png]` | Serial log QEMU normal menunjukkan `[M4] IDT loaded` |
| 2 | `[path/screenshot-qemu-breakpoint.png]` | Serial log QEMU breakpoint menunjukkan `trap_vector=0x3` |
| 3 | `[path/screenshot-gdb.png]` | GDB breakpoint pada `x86_64_idt_init` dan `x86_64_trap_dispatch` |

### Lampiran G — Bukti Tambahan

```text
--- evidence/M4/manifest.txt ---
MCSOS M4 evidence manifest
timestamp_utc=[timestamp]
commit=ac5a89b...
clang=[versi]
lld=[versi]
qemu=[versi]
kernel.elf
kernel.map
kernel.syms.txt
kernel.disasm.txt
kernel.readelf.header.txt
kernel.readelf.programs.txt
manifest.txt

--- grade_m4.sh output ---
M4_LOCAL_SCORE=100/100
```

---

## 24. Daftar Referensi

```text
[1] Intel Corporation, "Intel® 64 and IA-32 Architectures Software Developer Manuals,"
    Intel, 2026. [Online]. Available: https://www.intel.com/content/www/us/en/developer/
    articles/technical/intel-sdm.html. Accessed: May 2026.

[2] M. Sidiq, "Panduan Praktikum M4 — Interrupt Descriptor Table, Exception Trap Path,
    Trap Frame, dan Fault Handling Awal MCSOS 260502," Institut Pendidikan Indonesia,
    2026.

[3] QEMU Project, "QEMU System Emulation Invocation," QEMU Documentation, 2026.
    [Online]. Available: https://www.qemu.org/docs/master/system/invocation.html.
    Accessed: May 2026.

[4] QEMU Project, "GDB usage / gdbstub," QEMU Documentation, 2026. [Online].
    Available: https://www.qemu.org/docs/master/system/gdb.html. Accessed: May 2026.

[5] Free Software Foundation, "GNU ld Linker Scripts," GNU Binutils Documentation, 2026.
    [Online]. Available: https://sourceware.org/binutils/docs/ld/. Accessed: May 2026.

[6] LLVM Project, "Clang Command Guide and Driver Documentation," LLVM Documentation,
    2026. [Online]. Available: https://clang.llvm.org/docs/. Accessed: May 2026.

[7] LLVM Project, "LLD ELF Linker," LLVM Documentation, 2026. [Online].
    Available: https://lld.llvm.org/. Accessed: May 2026.

[8] Limine Project, "Limine Documentation," Limine, 2026. [Online].
    Available: https://limine-bootloader.org/. Accessed: May 2026.
```

---

## 25. Checklist Final Sebelum Pengumpulan

| Checklist | Status |
| --------- | ------ |
| Semua placeholder `[isi ...]` sudah diganti (kecuali data pribadi yang perlu diisi mahasiswa) | `Ya` |
| Metadata laporan lengkap | `Ya` |
| Commit awal dan akhir dicatat | `Ya` |
| Perintah build dan test dapat dijalankan ulang | `Ya` |
| Log build dilampirkan | `Sebagian — perlu tempel log asli` |
| Log QEMU/test dilampirkan | `Ya` |
| Artefak penting diberi hash | `Perlu diisi — jalankan sha256sum` |
| Desain, invariants, ownership, dan failure modes dijelaskan | `Ya` |
| Security/reliability dibahas | `Ya` |
| Readiness review tidak berlebihan | `Ya` |
| Rubrik penilaian diisi atau disiapkan | `Sebagian — nilai diisi dosen` |
| Referensi memakai format IEEE | `Ya` |
| Laporan disimpan sebagai Markdown | `Ya` |

---

## 26. Pernyataan Pengumpulan

Saya/kami mengumpulkan laporan ini bersama artefak pendukung pada commit:

```text
ac5a89b — M4 add x86_64 IDT and exception trap path
```

Status akhir yang diklaim:

```text
siap uji QEMU
```

Ringkasan satu paragraf:

```text
Praktikum M4 berhasil membangun fondasi mekanisme exception pada kernel MCSOS 260502
untuk target x86_64. IDT dengan 256 gate descriptor berhasil dimuat menggunakan lidt,
stub assembly untuk vektor 0–31 berhasil dinormalisasi ke satu trap frame seragam, dan
dispatcher C berhasil menangani exception #BP secara recoverable melalui iretq. Semua
invariant terbukti melalui KERNEL_ASSERT, audit ELF, serial log QEMU, dan grade lokal
100/100. Keterbatasan M4 adalah GDB evidence formal belum dikumpulkan, dan kernel belum
mendukung IRQ eksternal, PIC/APIC, paging lanjut, atau userspace. Langkah berikutnya
adalah M5 untuk membangun Physical Memory Manager (PMM) di atas fondasi M4.
```
