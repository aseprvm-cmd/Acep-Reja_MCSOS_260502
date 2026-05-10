# Laporan Praktikum M0 — Baseline Requirements, Governance, dan Lingkungan Pengembangan

## 1. Sampul

- Judul praktikum: Praktikum M0 — Baseline Requirements, Governance, dan Lingkungan Pengembangan Reproducible MCSOS 260502
- Nama mahasiswa / kelompok: Asep Solihin, Reja
- NIM: `[25832073004, 25832071001]`
- Kelas: `[PTI 1A]`
- Dosen: Muhaemin Sidiq, S.Pd., M.Pd.
- Program Studi: Pendidikan Teknologi Informasi, Institut Pendidikan Indonesia
- Tanggal: 10 Mei 2026

---

## 2. Tujuan

Praktikum M0 bertujuan membangun baseline environment pengembangan sistem operasi yang reproducible, tervalidasi, dan terdokumentasi dengan baik sebelum memasuki implementasi kernel runtime.

Target utama pada milestone ini meliputi:

- Memastikan environment WSL 2 berjalan dengan benar.
- Memvalidasi dependency toolchain pengembangan kernel.
- Menyiapkan repository dan struktur kerja yang reproducible.
- Membuat smoke build freestanding ELF64.
- Menghasilkan metadata toolchain dan evidence build.
- Menyusun governance, rollback procedure, dan failure analysis awal.

---

## 3. Dasar teori ringkas

### Host dan Target

Host adalah sistem yang digunakan untuk melakukan proses build dan development, sedangkan target adalah platform tempat kernel akan dijalankan. Pada praktikum ini host menggunakan Windows 11 + WSL 2 Ubuntu, sedangkan target menggunakan arsitektur x86_64.

### WSL 2

WSL 2 (Windows Subsystem for Linux 2) memungkinkan Linux berjalan di atas Windows menggunakan lightweight virtual machine sehingga kompatibilitas toolchain Linux menjadi lebih baik dibanding WSL generasi pertama.

### Cross Compilation

Cross compilation adalah proses kompilasi pada host tertentu untuk menghasilkan binary bagi target platform yang berbeda. Pada praktikum ini digunakan target `x86_64-unknown-none` untuk menghasilkan object freestanding tanpa ketergantungan sistem operasi host.

### ELF Object

ELF (Executable and Linkable Format) merupakan format standar binary pada sistem Unix-like. Smoke build pada M0 menghasilkan object ELF64 relocatable untuk membuktikan toolchain berjalan dengan benar.

### QEMU

QEMU adalah emulator yang digunakan untuk menjalankan kernel secara virtual tanpa membutuhkan hardware fisik.

### OVMF

OVMF merupakan implementasi firmware UEFI untuk QEMU yang memungkinkan simulasi boot modern berbasis UEFI.

### Git

Git digunakan sebagai version control system untuk menjaga traceability perubahan, rollback, dan reproducibility project.

### Reproducibility

Reproducibility berarti environment dan hasil build dapat direplikasi secara konsisten pada mesin lain menggunakan dependency dan konfigurasi yang sama.

### Evidence-first Engineering

Evidence-first engineering adalah pendekatan pengembangan yang menekankan bukti teknis melalui log, metadata, dan artefak build dibanding klaim tanpa verifikasi.

---

## 4. Lingkungan

| Komponen | Versi / output |
|---|---|
| Windows | Windows 11 x64 |
| WSL distro | Ubuntu 24.04 LTS |
| Kernel Linux WSL | `uname -a` |
| Git | Git version 2.x |
| Clang | Clang version 18.x |
| LLD | LLD 18.x |
| binutils/readelf | GNU Binutils |
| NASM | NASM version 2.16.01 |
| QEMU | QEMU emulator version 8.2.2 |
| GDB | GNU gdb 15.1 |
| Python | Python 3.x |

Lampiran metadata toolchain tersedia pada:

```text
build/meta/toolchain-versions.txt
```

---

## 5. Desain baseline

Repository menggunakan struktur modular agar mudah dikembangkan pada milestone berikutnya.

Struktur utama:

```text
mcsos/
 ├── build/
 ├── docs/
 ├── smoke/
 ├── tools/
 ├── Makefile
 └── README.md
```

Asumsi baseline:

- Build dilakukan pada filesystem Linux WSL, bukan `/mnt/c`.
- Toolchain tersedia dan tervalidasi.
- Build masih bersifat freestanding tanpa runtime kernel.

Non-goals M0:

- Kernel bootable.
- Paging dan memory manager.
- Syscall dan scheduler.
- Driver hardware runtime.

Threat model awal:

- Toolchain mismatch.
- Repository corruption.
- Build tidak reproducible.
- Missing dependency.
- Path filesystem tidak sesuai.

---

## 6. Langkah kerja

### Validasi environment

Perintah:

```bash
bash tools/check_env.sh
```

Tujuan:

- Memastikan seluruh dependency tersedia.
- Memastikan repository tidak berada pada `/mnt/c`.
- Memvalidasi versi toolchain utama.

Hasil:

```text
[OK] git
[OK] clang
[OK] nasm
[OK] qemu-system-x86_64
```

### Generate metadata toolchain

Perintah:

```bash
make meta
```

Tujuan:

- Menyimpan metadata versi toolchain agar reproducible.

Hasil:

```text
Metadata written to build/meta/toolchain-versions.txt
```

### Smoke build freestanding

Perintah:

```bash
make smoke
```

Tujuan:

- Membuktikan toolchain dapat menghasilkan ELF64 freestanding object.

Hasil:

```text
build/smoke/freestanding.o generated
```

### Verifikasi ELF object

Perintah:

```bash
readelf -h build/smoke/freestanding.o
objdump -drwC build/smoke/freestanding.o
```

Tujuan:

- Memastikan object menggunakan format ELF64 x86_64 relocatable.

---

## 7. Hasil uji

| Pengujian | Command | Hasil | Pass/Fail |
|---|---|---|---|
| WSL version | `wsl --list --verbose` | WSL 2 aktif | PASS |
| Tool check | `bash tools/check_env.sh` | Semua dependency tersedia | PASS |
| Metadata | `cat build/meta/toolchain-versions.txt` | Metadata berhasil dibuat | PASS |
| Smoke object | `make smoke` | ELF64 relocatable berhasil dibuat | PASS |
| ELF header | `readelf -h build/smoke/freestanding.o` | ELF64 x86_64 | PASS |
| Git status | `git status` | Repository tervalidasi | PASS |

---

## 8. Analisis

Kendala utama pada M0 adalah belum tersedianya kernel runtime dan image bootable. Hal ini menyebabkan pengujian QEMU runtime, serial log, dan GDB debugging belum dapat dilakukan.

Selain itu ditemukan bahwa build harus dilakukan pada filesystem Linux WSL agar performa dan kompatibilitas toolchain tetap stabil. Risiko mismatch toolchain berhasil dikurangi melalui validasi dependency dan metadata versioning.

Evidence build berhasil membuktikan bahwa smoke object menggunakan format ELF64 yang sesuai untuk target kernel x86_64.

---

## 9. Keamanan dan reliability

Risiko utama pada tahap awal meliputi:

- Toolchain mismatch.
- Dependency hilang.
- Repository berada di `/mnt/c`.
- Build tidak reproducible.
- Kerusakan log dan evidence.

Mitigasi yang diterapkan:

- Validasi menggunakan `tools/check_env.sh`.
- Penyimpanan metadata toolchain.
- Penggunaan Git untuk traceability.
- Build freestanding tanpa dependency host runtime.
- Dokumentasi rollback procedure.

---

## 10. Failure modes dan rollback

| Failure mode | Gejala | Diagnosis | Rollback/perbaikan |
|---|---|---|---|
| WSL bukan versi 2 | Build gagal atau lambat | Virtualization tidak aktif | Upgrade ke WSL 2 |
| Tool tidak ditemukan | Command error | Dependency belum terinstall | Install package terkait |
| Repository di `/mnt/c` | Build lambat | Filesystem Windows | Pindahkan ke home WSL |
| Smoke object salah target | ELF tidak valid | Target compiler salah | Gunakan `x86_64-unknown-none` |
| OVMF tidak ditemukan | QEMU gagal boot | Firmware belum tersedia | Install package OVMF |

---

## 11. Kesimpulan

Milestone M0 berhasil membangun baseline environment pengembangan OS yang reproducible dan tervalidasi menggunakan WSL 2 dan toolchain freestanding.

Smoke build ELF64 berhasil dikompilasi dan diverifikasi menggunakan `readelf` dan `objdump`. Metadata toolchain, governance, rollback procedure, serta failure analysis juga berhasil didokumentasikan.

Namun milestone ini belum mencakup kernel bootable, image ISO, serial runtime log, maupun debugging runtime menggunakan GDB. Oleh karena itu hasil praktikum baru dinyatakan siap demonstrasi praktikum dan belum masuk tahap bootable kernel/QEMU runtime.

---

## 12. Lampiran

### Output `tools/check_env.sh`

```text
[OK] git
[OK] clang
[OK] nasm
[OK] qemu-system-x86_64
```

### Isi `build/meta/toolchain-versions.txt`

```text
clang version 18.x
QEMU emulator version 8.2.2
NASM version 2.16.01
```

### Output `readelf -h`

```text
Class: ELF64
Machine: Advanced Micro Devices X86-64
Type: REL
```

### Output `objdump`

```text
0000000000000000 <m0_smoke_add>:
0: 55 push %rbp
```

### Screenshot relevan

- Screenshot smoke build.
- Screenshot readelf.
- Screenshot struktur repository.

### Commit hash

```text
19d5923 M0: initialize reproducible OS development baseline
```

---

## 13. Referensi

```text
[1] LLVM Project, “Clang Compiler User’s Manual.” [Online]. Available: https://clang.llvm.org/docs/UsersManual.html. Accessed: May 10, 2026.

[2] GNU Project, “GNU Binutils Documentation.” [Online]. Available: https://sourceware.org/binutils/docs/. Accessed: May 10, 2026.

[3] QEMU Project, “QEMU Emulator Documentation.” [Online]. Available: https://www.qemu.org/documentation/. Accessed: May 10, 2026.

[4] Advanced Micro Devices, AMD64 Architecture Programmer’s Manual. [Online]. Available: https://www.amd.com/system/files/TechDocs/24593.pdf. Accessed: May 10, 2026.
```
