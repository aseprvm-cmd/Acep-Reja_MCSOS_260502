# Template Laporan Praktikum Sistem Operasi Lanjut — MCSOS

**Nama file laporan:** `laporan_praktikum_M13_Syududu.md`  
**Nama sistem operasi:** MCSOS versi 260502  
**Target default:** x86_64, QEMU, Windows 11 x64 + WSL 2, kernel monolitik pendidikan, C freestanding dengan assembly minimal, POSIX-like subset  
**Dosen:** Muhaemin Sidiq, S.Pd., M.Pd.  
**Program Studi:** Pendidikan Teknologi Informasi  
**Institusi:** Institut Pendidikan Indonesia

---

## 0. Metadata Laporan

| Atribut                       | Isi                                                                                 |
| ----------------------------- | ----------------------------------------------------------------------------------- |
| Kode praktikum                | `M13`                                                                               |
| Judul praktikum               | `Virtual Filesystem (VFS), File Descriptor Table, dan RAMFS Volatil pada MCSOS`    |
| Jenis pengerjaan              | `Kelompok`                                                                          |
| Nama mahasiswa                | `-`                                                                                 |
| NIM                           | `-`                                                                                 |
| Kelas                         | `PTI 1A`                                                                            |
| Nama kelompok                 | `Syududu`                                                                           |
| Anggota kelompok              | `Reja, 25832073004, Ketua / Implementasi / Pengujian` <br> `Asep Solihin, 25832071001, Anggota / Dokumentasi / Pengujian` |
| Tanggal praktikum             | `2026-06-02`                                                                        |
| Tanggal pengumpulan           | `-`                                                                                 |
| Repository                    | `~/src/mcsos`                                                                       |
| Branch                        | `praktikum-m13-vfs-ramfs`                                                           |
| Commit awal                   | `3a0ecc3`                                                                           |
| Commit akhir                  | `756532b`                                                                           |
| Status readiness yang diklaim | `siap uji QEMU untuk VFS/FD/RAMFS awal`                                             |

---

## 1. Sampul

# Laporan Praktikum M13

## Virtual Filesystem (VFS), File Descriptor Table, dan RAMFS Volatil pada MCSOS

Disusun oleh:

| Nama          | NIM           | Kelas   | Peran                                    |
| ------------- | ------------- | ------- | ---------------------------------------- |
| Reja          | 25832073004   | PTI 1A  | Ketua / Implementasi / Pengujian         |
| Asep Solihin  | 25832071001   | PTI 1A  | Anggota / Dokumentasi / Pengujian        |

Dosen Pengampu: **Muhaemin Sidiq, S.Pd., M.Pd.**  
Program Studi Pendidikan Teknologi Informasi  
Institut Pendidikan Indonesia  
2025/2026

---

## 2. Pernyataan Orisinalitas dan Integritas Akademik

Kami menyatakan bahwa laporan ini disusun berdasarkan pekerjaan praktikum kelompok sesuai pembagian peran yang tercatat. Bantuan eksternal, referensi, generator kode, AI assistant, dokumentasi resmi, diskusi, atau sumber lain dicatat pada bagian referensi dan lampiran. Kami tidak mengklaim hasil yang tidak dibuktikan oleh log, test, commit, atau artefak lain.

| Pernyataan                                      | Status |
| ----------------------------------------------- | ------ |
| Semua potongan kode eksternal diberi atribusi   | `Ya`   |
| Semua penggunaan AI assistant dicatat           | `Ya`   |
| Repository yang dikumpulkan sesuai commit akhir | `Ya`   |
| Tidak ada klaim readiness tanpa bukti           | `Ya`   |

Catatan penggunaan bantuan eksternal:

```text
Alat: Claude AI (Anthropic)
Bagian yang dibantu: Panduan langkah-langkah implementasi VFS/FD/RAMFS M13,
debugging integrasi Makefile (OBJS list backslash), penjelasan konsep VFS
layer dan file descriptor table, panduan verifikasi ELF artifact (nm, readelf,
objdump), dan penyusunan laporan M13.
Verifikasi mandiri: Seluruh perintah build, host unit test (PASS), freestanding
compile, audit (nm/readelf/objdump/sha256sum), integrasi kernel, dan QEMU smoke
test dijalankan dan diverifikasi sendiri di lingkungan WSL 2. Output terminal
yang dicantumkan adalah hasil nyata dari eksekusi di mesin kelompok.
```

---

## 3. Tujuan Praktikum

1. Merancang header VFS (`mcs_vfs.h`) yang mendefinisikan struktur RAMFS, inode, file descriptor table, dan process dengan `fd_table` embedded.
2. Mengimplementasikan RAMFS volatil (`ramfs.c`) dengan operasi `init`, `lookup`, `create_file`, `seed_file`, `read`, dan `write`.
3. Mengimplementasikan FD table (`fd.c`) dengan operasi `open`, `read`, `write`, `lseek`, `close`, dan `dup` yang memetakan file descriptor integer ke inode RAMFS.
4. Mengimplementasikan syscall wrappers (`sys_vfs.c`) sebagai lapisan tipis antara dispatcher syscall kernel dan VFS/FD internal.
5. Mengintegrasikan VFS syscall (open/read/write/close/lseek) ke syscall dispatcher kernel via function pointer table `g_table`.
6. Membuktikan tidak ada dependency libc tersembunyi pada object freestanding VFS dengan `nm -u`.
7. Membuktikan object VFS adalah ELF64 relocatable x86_64 dengan `readelf`.
8. Menjalankan host unit test tanpa QEMU dan memastikan hasilnya PASS.
9. Membuktikan kernel MCSOS tidak crash setelah VFS objects di-link melalui QEMU smoke test.
10. Mendokumentasikan seluruh evidence sebagai dasar readiness review M13.

---

## 4. Capaian Pembelajaran Praktikum

Setelah praktikum ini, mahasiswa mampu:

| CPL/CPMK praktikum | Bukti yang harus ditunjukkan |
| ------------------- | ---------------------------- |
| Menjelaskan arsitektur VFS layer dan perbedaannya dengan filesystem konkret (RAMFS) | Dasar teori Bagian 6.1, desain Bagian 9.1 |
| Mengimplementasikan file descriptor table yang memetakan FD integer ke inode | `kernel/vfs/fd.c`, host test PASS |
| Mengintegrasikan syscall VFS ke dispatcher kernel dengan function pointer table | `kernel/syscall/syscall.c` — `g_table[5..9]` terisi |
| Membuktikan object freestanding tidak memiliki dependency libc | `build/m13/nm-undefined.txt` kosong |
| Membuktikan object VFS adalah ELF64 relocatable x86_64 | `build/m13/readelf-vfs.txt` — Type: REL, Machine: X86-64 |
| Menjalankan kernel dengan VFS terintegrasi tanpa regresi | QEMU smoke test — boot sequence M5–M11 semua lulus, tidak ada panic |
| Menjelaskan keterbatasan RAMFS volatil dan rencana persistensi | Bagian 15 dan 22.2 laporan ini |

---

## 5. Peta Milestone MCSOS

Centang milestone yang menjadi fokus laporan ini. Jika praktikum mencakup lebih dari satu milestone, jelaskan batas cakupan.

| Milestone | Fokus                                                           | Status dalam laporan                                      |
| --------- | --------------------------------------------------------------- | --------------------------------------------------------- |
| M0        | Requirements, governance, baseline arsitektur                   | `[ ] tidak dibahas / [ ] dibahas / [v] selesai praktikum` |
| M1        | Toolchain reproducible, Git, QEMU, GDB, metadata build          | `[ ] tidak dibahas / [ ] dibahas / [v] selesai praktikum` |
| M2        | Boot image, kernel ELF64, early console                         | `[ ] tidak dibahas / [ ] dibahas / [v] selesai praktikum` |
| M3        | Panic path, linker map, GDB, observability awal                 | `[ ] tidak dibahas / [ ] dibahas / [v] selesai praktikum` |
| M4        | Trap, exception, interrupt, timer                               | `[ ] tidak dibahas / [ ] dibahas / [v] selesai praktikum` |
| M5        | PMM, VMM, page table, kernel heap                               | `[ ] tidak dibahas / [ ] dibahas / [v] selesai praktikum` |
| M6        | Thread, scheduler, synchronization                              | `[ ] tidak dibahas / [ ] dibahas / [v] selesai praktikum` |
| M7        | Syscall ABI dan user program loader                             | `[ ] tidak dibahas / [ ] dibahas / [v] selesai praktikum` |
| M8        | VFS, file descriptor, ramfs                                     | `[ ] tidak dibahas / [ ] dibahas / [v] selesai praktikum` |
| M9        | Block layer dan device model                                    | `[ ] tidak dibahas / [ ] dibahas / [v] selesai praktikum` |
| M10       | Persistent filesystem, mcsfs/ext2-like, recovery                | `[ ] tidak dibahas / [ ] dibahas / [v] selesai praktikum` |
| M11       | Networking stack, packet parsing, UDP/TCP subset                | `[ ] tidak dibahas / [ ] dibahas / [v] selesai praktikum` |
| M12       | Security model, capability/ACL, syscall fuzzing, hardening      | `[ ] tidak dibahas / [ ] dibahas / [v] selesai praktikum` |
| M13       | SMP, scalability, lock stress, NUMA-aware preparation           | `[ ] tidak dibahas / [v] dibahas / [ ] selesai praktikum` |
| M14       | Framebuffer, graphics console, visual regression                | `[ ] tidak dibahas / [ ] dibahas / [ ] selesai praktikum` |
| M15       | Virtualization/container subset                                 | `[ ] tidak dibahas / [ ] dibahas / [ ] selesai praktikum` |
| M16       | Observability, update/rollback, release image, readiness review | `[ ] tidak dibahas / [ ] dibahas / [ ] selesai praktikum` |

Batas cakupan praktikum:

```text
M13 mencakup: desain header VFS (mcs_vfs.h), implementasi RAMFS volatil
(init/lookup/create_file/seed_file/read/write), implementasi FD table
(open/read/write/lseek/close/dup), syscall wrappers (sys_open/sys_read/
sys_write/sys_close/sys_lseek), integrasi ke syscall dispatcher kernel
(g_table[5..9]), host unit test, audit nm/readelf/objdump/sha256, integrasi
ke main Makefile, dan QEMU smoke test tanpa panic.

Non-goals M13: permission model (uid/gid/mode), copyin/copyout penuh dari/ke
userspace, persistent storage (fsync, journal, recovery, fsck), directory
traversal penuh, symlink, hard link, inotify, mmap berbasis file, block device
backing, multi-process FD sharing, crash consistency, dan kompatibilitas POSIX
penuh.
```

---

## 6. Dasar Teori Ringkas

### 6.1 Konsep Sistem Operasi yang Diuji

```text
Virtual Filesystem (VFS) adalah lapisan abstraksi di atas filesystem konkret
yang memungkinkan kernel menyediakan antarmuka file seragam (open/read/write/
close/lseek) terlepas dari implementasi storage di bawahnya. VFS mendefinisikan
operasi generik; filesystem konkret (seperti RAMFS) mengimplementasikannya.

File Descriptor (FD) adalah integer yang mewakili file terbuka dari sudut
pandang proses. Kernel memetakan FD ke inode atau struktur file internal melalui
file descriptor table. Tabel ini bersifat per-proses: dua proses berbeda dapat
memiliki FD nomor 3 yang menunjuk ke file berbeda.

RAMFS (RAM-based Filesystem) adalah filesystem volatil yang menyimpan seluruh
data di memori. Tidak ada persistensi — seluruh isi hilang saat sistem restart
atau power off. RAMFS cocok untuk temporary files, initramfs, dan sebagai
proof-of-concept awal VFS sebelum backend storage nyata diintegrasikan.

Inode adalah struktur metadata file yang memisahkan identitas file dari
direktori entry. Dalam RAMFS M13, setiap inode menyimpan nama file, ukuran,
dan blok data (flat buffer). Operasi lookup mencari inode berdasarkan nama
path.

Syscall wrapper adalah fungsi tipis di sisi kernel yang menerima argumen dari
dispatcher syscall (dalam format integer 64-bit), memvalidasi pointer userspace,
kemudian mendelegasikan ke fungsi VFS internal. Pola ini memisahkan validasi
keamanan dari logika filesystem.
```

### 6.2 Konsep Arsitektur x86_64 yang Relevan

| Konsep | Relevansi pada praktikum | Bukti/verifikasi |
| ------ | ------------------------ | ---------------- |
| Syscall ABI x86_64 (INT 0x80 / SYSCALL) | Jalur masuk dari userspace ke kernel untuk operasi VFS | `g_table[5..9]` di `kernel/syscall/syscall.c` |
| User pointer validation | `mcsos_user_check_range()` memastikan pointer dari userspace tidak menunjuk ke kernel space | Implementasi `sys_open`, `sys_read`, `sys_write` |
| Higher-half kernel address | VFS symbols berada di `0xffffffff8xxxxxxx` — membuktikan link ke kernel image berhasil | `nm -n build/mcsos-m5.elf` — semua simbol VFS di range kernel |
| Freestanding compile | Object VFS tidak boleh bergantung pada libc — wajib compile dengan `-ffreestanding -fno-builtin` | `nm -u build/m13/vfs.o` kosong |
| ELF64 relocatable object | Object VFS di-link ke kernel menggunakan `ld -r` untuk menghasilkan satu relocatable object gabungan | `readelf -h build/m13/vfs.o` — Type: REL |

### 6.3 Konsep Implementasi Freestanding

| Aspek | Keputusan praktikum |
| ----- | ------------------- |
| Bahasa | C17 freestanding untuk semua VFS source; C17 hosted untuk host unit test |
| Runtime | Tanpa hosted libc untuk semua object VFS; `nm -u vfs.o` harus kosong |
| ABI | x86_64 System V untuk boundary C internal kernel |
| Compiler flags kritis | `--target=x86_64-elf`, `-ffreestanding`, `-fno-builtin`, `-fno-stack-protector`, `-fno-pic`, `-mno-red-zone` |
| Risiko undefined behavior | Pointer NULL dicek sebelum akses; panjang path dibatasi `MCS_MAX_PATH`; FD dibatasi `MCS_MAX_FD` |

### 6.4 Referensi Teori yang Digunakan

| No. | Sumber | Bagian yang digunakan | Alasan relevansi |
| --- | ------ | --------------------- | ---------------- |
| [1] | Panduan Praktikum M13 (OS_panduan_M13.md) | Section 3–14, source code baseline | Desain VFS, invariants, host test, failure modes |
| [2] | Linux Kernel Documentation — VFS Overview | Konsep inode, dentry, file operations | Perbandingan arsitektur VFS Linux vs M13 |
| [3] | POSIX.1-2018 (The Open Group) | Semantik open/read/write/close/lseek | Referensi kontrak syscall VFS |
| [4] | Intel SDM Vol. 3A | Syscall mechanism, user/supervisor bit | Dasar isolasi userspace-kernel untuk pointer validation |
| [5] | QEMU Documentation | GDB stub, serial log | Smoke test dan debug image kernel |
| [6] | Clang Command Line Reference | Target triple freestanding | Flags compile untuk `x86_64-unknown-none` |
| [7] | GNU Binutils Documentation | nm, readelf, objdump | Audit artifact ELF |

---

## 7. Lingkungan Praktikum

### 7.1 Host dan Target

| Komponen | Nilai |
| --------- | ----- |
| Host OS | Windows 11 x64 |
| Lingkungan build | WSL 2 Ubuntu 24 |
| Target ISA | `x86_64` |
| Target ABI | `x86_64-unknown-none-elf` |
| Emulator | `qemu-system-x86_64` |
| Firmware emulator | Limine (boot path dari M2/M3/M4/M5) |
| Build system | `make` dengan `.RECIPEPREFIX := >` |
| Bahasa utama | C17 freestanding (VFS) + C17 hosted (host test) |

### 7.2 Versi Toolchain

```text
Ubuntu clang version 18.1.3 (1ubuntu1)
GNU ld (GNU Binutils for Ubuntu) 2.42
GNU Make 4.3
QEMU emulator version 8.2.2 (Debian 1:8.2.2+ds-0ubuntu1.16)
Linux LAPTOP-CHG1JJE6 6.6.87.2-microsoft-standard-WSL2 x86_64 GNU/Linux
```

### 7.3 Lokasi Repository

| Item | Nilai |
| ---- | ----- |
| Path repository di WSL | `~/src/mcsos` |
| Apakah berada di filesystem Linux WSL, bukan `/mnt/c` | `Ya` |
| Branch | `praktikum-m13-vfs-ramfs` |
| Commit hash awal | `3a0ecc3` |
| Commit hash akhir | `756532b` |

---

## 8. Repository dan Struktur File

### 8.1 Struktur Direktori yang Relevan

```text
mcsos/
├── Makefile                          ← diperbarui (tambah VFS ke OBJS + compile rules)
├── Makefile.m13                      ← baru (M13) — build standalone + audit VFS
├── include/
│   ├── mcs_vfs.h                     ← baru (M13) — header VFS/FD/RAMFS
│   └── mcsos/
│       └── syscall.h                 ← diperbarui (M13) — tambah SYS_OPEN..SYS_LSEEK
├── kernel/
│   ├── vfs/
│   │   ├── ramfs.c                   ← baru (M13) — implementasi RAMFS volatil
│   │   ├── fd.c                      ← baru (M13) — FD table operations
│   │   └── sys_vfs.c                 ← baru (M13) — syscall wrappers VFS
│   └── syscall/
│       └── syscall.c                 ← diperbarui (M13) — hook g_table[5..9]
├── tests/
│   └── m13_vfs_host_test.c           ← baru (M13) — host unit test
└── docs/
    └── reports/
        └── m13-evidence/
            ├── host-test.log
            ├── nm-undefined.txt
            ├── nm-vfs-symbols.txt
            ├── readelf-vfs.txt
            ├── sha256sums.txt
            ├── sha256-kernel-m13.txt
            ├── smoke-result.txt
            └── qemu-smoke-head.log
```

### 8.2 File yang Dibuat atau Diubah

| File | Jenis perubahan | Alasan perubahan | Risiko |
| ---- | --------------- | ---------------- | ------ |
| `include/mcs_vfs.h` | Baru | Header publik VFS: struct inode, RAMFS, FD table, process, konstanta, error codes, deklarasi fungsi | Rendah — hanya definisi tipe |
| `kernel/vfs/ramfs.c` | Baru | Implementasi RAMFS volatil: init, lookup, create_file, seed_file, read, write | Sedang — filesystem baru; diuji host test |
| `kernel/vfs/fd.c` | Baru | Implementasi FD table: open, read, write, lseek, close, dup | Sedang — menyentuh area FD baru; diuji host test |
| `kernel/vfs/sys_vfs.c` | Baru | Syscall wrappers: sys_open, sys_read, sys_write, sys_close, sys_lseek | Sedang — antarmuka kernel-userspace; pointer validation |
| `include/mcsos/syscall.h` | Diubah | Tambah enum SYS_OPEN(5)..SYS_LSEEK(9) dan error codes ENOENT, EBADF, EACCES, dll | Rendah — backward compatible, SYS_MAX naik dari 5 ke 10 |
| `kernel/syscall/syscall.c` | Diubah | Tambah mcs_vfs_init_once(), global g_kernel_ramfs/g_kernel_main_process, dan g_table[5..9] | Sedang — mengubah dispatcher kernel; diverifikasi smoke test |
| `Makefile.m13` | Baru | Build target standalone untuk host test, freestanding compile, dan audit artifact M13 | Rendah — tidak mempengaruhi build kernel utama |
| `Makefile` | Diubah | Tambah ramfs.o, fd.o, sys_vfs.o ke OBJS + compile rules | Sedang — perubahan build system; diverifikasi `make clean && make` |

### 8.3 Ringkasan Diff

```text
756532b M13: VFS/RAMFS/FD implementation with kernel integration
 9 files changed, 873 insertions(+), 10 deletions(-)
 create mode 100644 Makefile.m13
 create mode 100644 include/mcs_vfs.h
 create mode 100644 kernel/vfs/fd.c
 create mode 100644 kernel/vfs/ramfs.c
 create mode 100644 kernel/vfs/sys_vfs.c
 create mode 100644 tests/m13_vfs_host_test.c
```

---

## 9. Desain Teknis

### 9.1 Masalah yang Diselesaikan

```text
Setelah M12 menyediakan primitif sinkronisasi (spinlock, mutex, lockdep),
kernel MCSOS belum memiliki abstraksi penyimpanan file. Tanpa VFS, kernel
tidak dapat: menyimpan data ke nama path, membaca kembali data yang disimpan,
atau menyediakan antarmuka file descriptor yang familiar ke userspace.

M13 menyelesaikan masalah ini dengan tiga lapisan:

1. RAMFS — filesystem konkret yang menyimpan data di array inode statik di
   memori kernel. Setiap file memiliki nama (maksimal MCS_MAX_PATH), ukuran,
   dan buffer data (maksimal MCS_FILE_MAX_SIZE). RAMFS sepenuhnya volatil.

2. FD Table — struktur per-proses yang memetakan file descriptor integer
   (0..MCS_MAX_FD-1) ke inode RAMFS. Operasi open mengalokasikan slot FD
   kosong dan mengisi pointer ke inode. Operasi close membebaskan slot.

3. Syscall Wrappers — lapisan tipis yang menerima argumen raw 64-bit dari
   dispatcher syscall, memvalidasi pointer userspace dengan
   mcsos_user_check_range(), lalu mendelegasikan ke VFS/FD internal.
   Inisialisasi RAMFS dan FD table dilakukan sekali (lazy init via
   mcs_vfs_init_once()) saat syscall VFS pertama dipanggil.
```

### 9.2 Keputusan Desain

| Keputusan | Alternatif yang dipertimbangkan | Alasan memilih | Konsekuensi |
| --------- | ------------------------------- | -------------- | ----------- |
| Satu global RAMFS dan satu global process di kernel | Per-proses filesystem atau mount table | Menyederhanakan M13 agar fokus pada antarmuka VFS; mount table dapat ditambah di modul berikutnya | Hanya ada satu namespace file global; tidak ada isolasi antar proses |
| Lazy init via `mcs_vfs_init_once()` | Init di kernel_main | Menghindari dependency order di kernel init; RAMFS hanya aktif jika ada syscall VFS | RAMFS tidak tersedia sebelum syscall VFS pertama |
| FD table embedded di `mcs_process_t` | FD table sebagai pointer heap | Menghindari alokasi heap untuk struktur kernel awal; ownership jelas | Ukuran proses statik; tidak dapat expand FD table saat runtime |
| Fail-closed: semua operasi cek pointer NULL dan batas FD | Lewati validasi untuk performa | Mencegah kernel crash akibat FD atau pointer tidak valid dari userspace | Sedikit overhead validasi di setiap syscall VFS |
| Object audit dengan `ld -r` menjadi satu `vfs.o` | Audit per-object | Membuktikan semua object dapat di-link bersama tanpa konflik simbol | Audit mencakup interaksi antar tiga file sekaligus |

### 9.3 Struktur Data Utama

```c
/* Inode RAMFS — satu per file */
typedef struct mcs_inode {
    char     name[MCS_MAX_PATH];          /* nama file (path flat) */
    uint8_t  data[MCS_FILE_MAX_SIZE];     /* isi file */
    uint32_t size;                        /* byte yang tertulis */
    int      used;                        /* slot aktif? */
} mcs_inode_t;

/* Filesystem RAMFS — array inode statik */
typedef struct mcs_ramfs {
    mcs_inode_t inodes[MCS_MAX_FILES];
    int         count;
} mcs_ramfs_t;

/* File Descriptor Table — per proses */
typedef struct mcs_fd_table {
    mcs_file_t files[MCS_MAX_FD];         /* slot FD */
} mcs_fd_table_t;

/* Process — memiliki FD table embedded */
typedef struct mcs_process {
    int            pid;
    mcs_fd_table_t fd_table;
} mcs_process_t;
```

### 9.4 Alur Syscall VFS

```text
Userspace: INT 0x80 / SYSCALL (nr=5, path_ptr, flags)
    ↓
mcsos_syscall_dispatch() → g_table[5] = sys_open()
    ↓
sys_open(): validasi path_ptr dengan mcsos_user_check_range()
    ↓
mcs_vfs_init_once(): inisialisasi g_kernel_ramfs + g_kernel_main_process
    ↓
mcs_sys_open(): delegasi ke mcs_vfs_open()
    ↓
mcs_vfs_open(): lookup inode di RAMFS, alokasikan FD slot, return fd integer
    ↓
Return ke userspace: nilai fd atau error code negatif
```

### 9.5 Invariants

```text
1. nm -u build/m13/vfs.o harus selalu kosong — tidak ada dependency libc
   tersembunyi pada object VFS freestanding.

2. FD valid selalu dalam rentang [0, MCS_MAX_FD). Operasi apapun dengan fd
   di luar rentang ini mengembalikan MCSOS_EBADF tanpa akses memori.

3. mcs_vfs_init_once() bersifat idempoten — pemanggilan kedua dan seterusnya
   tidak menginisialisasi ulang RAMFS atau FD table.

4. Pointer userspace yang bernilai 0 (NULL) selalu ditolak dengan MCSOS_EINVAL
   sebelum dideref di sisi kernel.

5. RAMFS bersifat volatil — tidak ada persistensi ke disk. Data hilang saat
   kernel restart.
```

---

## 10. Langkah Kerja

### 10.1 Preflight — Verifikasi Readiness M0–M12

```bash
ls -la include/mcs_vfs.h \
        kernel/vfs/ramfs.c kernel/vfs/fd.c kernel/vfs/sys_vfs.c \
        tests/m13_vfs_host_test.c \
        Makefile.m13
```

Output:
```text
-rw-r--r-- 1 acep acep 2774 Jun  1 18:37 include/mcs_vfs.h
-rw-r--r-- 1 acep acep 7214 Jun  1 18:38 kernel/vfs/fd.c
-rw-r--r-- 1 acep acep 6611 Jun  1 18:37 kernel/vfs/ramfs.c
-rw-r--r-- 1 acep acep  178 Jun  1 18:38 kernel/vfs/sys_vfs.c
-rw-r--r-- 1 acep acep 2710 Jun  1 18:39 tests/m13_vfs_host_test.c
-rw-r--r-- 1 acep acep 1713 Jun  1 18:40 Makefile.m13
```

### 10.2 Build dan Test Standalone M13

```bash
make -f Makefile.m13 clean && \
make -f Makefile.m13 m13-all 2>&1 | tee /tmp/m13-build.log
```

Output ringkas:
```text
rm -rf build/m13
mkdir -p build/m13
cc -std=c17 -Wall -Wextra -Werror -O2 -Iinclude tests/m13_vfs_host_test.c \
   kernel/vfs/ramfs.c kernel/vfs/fd.c kernel/vfs/sys_vfs.c \
   -o build/m13/m13_vfs_host_test
./build/m13/m13_vfs_host_test | tee build/m13/host-test.log
M13 VFS/FD/RAMFS host tests: PASS
clang -target x86_64-elf ... -c kernel/vfs/ramfs.c -o build/m13/ramfs.o
clang -target x86_64-elf ... -c kernel/vfs/fd.c    -o build/m13/fd.o
clang -target x86_64-elf ... -c kernel/vfs/sys_vfs.c -o build/m13/sys_vfs.o
ld -r -m elf_x86_64 build/m13/ramfs.o build/m13/fd.o build/m13/sys_vfs.o \
   -o build/m13/vfs.o
nm -u build/m13/vfs.o > build/m13/nm-undefined.txt
readelf -h build/m13/vfs.o > build/m13/readelf-vfs.txt
sha256sum ... > build/m13/sha256sums.txt
test ! -s build/m13/nm-undefined.txt   ← PASS (kosong)
```

### 10.3 Integrasi ke Main Makefile

Tambah VFS objects ke OBJS list dan compile rules ke `Makefile`:

```makefile
# Bagian OBJS — tambah tiga baris setelah m11_kernel_integration.o
$(BUILD)/ramfs.o \
$(BUILD)/fd.o \
$(BUILD)/sys_vfs.o

# Compile rules M13
$(BUILD)/ramfs.o: kernel/vfs/ramfs.c include/mcs_vfs.h
> $(CC) $(CFLAGS) -c kernel/vfs/ramfs.c -o $(BUILD)/ramfs.o

$(BUILD)/fd.o: kernel/vfs/fd.c include/mcs_vfs.h
> $(CC) $(CFLAGS) -c kernel/vfs/fd.c -o $(BUILD)/fd.o

$(BUILD)/sys_vfs.o: kernel/vfs/sys_vfs.c include/mcs_vfs.h
> $(CC) $(CFLAGS) -c kernel/vfs/sys_vfs.c -o $(BUILD)/sys_vfs.o
```

### 10.4 Rebuild Kernel dengan VFS Terintegrasi

```bash
make clean && make 2>&1 | tee /tmp/m13-kernel-build.log
```

Output (bagian VFS):
```text
clang ... -c kernel/vfs/ramfs.c -o build/ramfs.o
clang ... -c kernel/vfs/fd.c    -o build/fd.o
clang ... -c kernel/vfs/sys_vfs.c -o build/sys_vfs.o
ld.lld -nostdlib ... build/ramfs.o build/fd.o build/sys_vfs.o \
       ... -o build/mcsos-m5.elf
```

### 10.5 QEMU Smoke Test

```bash
mkdir -p build/m13
cp build/mcsos-m5.elf build/kernel.elf
bash tools/scripts/make_iso.sh
timeout 10s qemu-system-x86_64 -M q35 -m 512M \
  -cdrom build/mcsos.iso \
  -serial stdio \
  -no-reboot -no-shutdown 2>&1 | tee build/m13/qemu-smoke.log || true
```

---

## 11. Hasil Pengujian

### 11.1 Host Unit Test

```bash
cat build/m13/host-test.log
```

Output:
```text
M13 VFS/FD/RAMFS host tests: PASS
```

**Hasil: PASS** — semua kasus uji dalam `m13_vfs_host_test.c` lulus.

### 11.2 Undefined Symbol Audit

```bash
cat build/m13/nm-undefined.txt
```

Output:
```text
[kosong — tidak ada output]
```

**Hasil: PASS** — tidak ada dependency libc tersembunyi pada object VFS freestanding.

### 11.3 ELF Header Verification

```bash
cat build/m13/readelf-vfs.txt
```

Output:
```text
ELF Header:
  Magic:   7f 45 4c 46 02 01 01 00 00 00 00 00 00 00 00 00
  Class:                             ELF64
  Data:                              2's complement, little endian
  Version:                           1 (current)
  OS/ABI:                            UNIX - System V
  Type:                              REL (Relocatable file)
  Machine:                           Advanced Micro Devices X86-64
  Version:                           0x1
  Entry point address:               0x0
  Number of section headers:         11
```

**Hasil: PASS** — ELF64 relocatable x86_64 sesuai target kernel.

### 11.4 VFS Symbols di Kernel

```bash
nm -n build/mcsos-m5.elf | grep -E "mcs_vfs|mcs_ramfs|mcs_fd|mcs_sys|sys_open|sys_read|sys_write|sys_close|sys_lseek|g_table|g_kernel"
```

Output:
```text
ffffffff80002fa0 t sys_write_serial
ffffffff80003050 t sys_open
ffffffff80003110 t sys_read
ffffffff800031d0 t sys_write
ffffffff80003290 t sys_close
ffffffff800032f0 t sys_lseek
ffffffff80004670 T mcs_ramfs_init
ffffffff80004720 T mcs_ramfs_lookup
ffffffff800048d0 T mcs_ramfs_create_file
ffffffff80004bb0 T mcs_ramfs_seed_file
ffffffff80004cc0 T mcs_fd_table_init
ffffffff80004d10 T mcs_vfs_open
ffffffff80004ff0 T mcs_vfs_read
ffffffff80005140 T mcs_vfs_write
ffffffff800052c0 T mcs_vfs_lseek
ffffffff80005330 T mcs_vfs_close
ffffffff80005380 T mcs_vfs_dup
ffffffff80005590 T mcs_sys_open
ffffffff800055c0 T mcs_sys_read
ffffffff80005710 T mcs_sys_write
ffffffff80005740 T mcs_sys_close
ffffffff800057a0 T mcs_sys_lseek
ffffffff80005820 T mcs_vfs_set_active_ramfs_for_test
ffffffff800067c0 r g_table
ffffffff80009060 b g_kernel_main_process
ffffffff80009268 b g_kernel_ramfs
```

**Hasil: PASS** — 17 simbol VFS ter-link di kernel address space `0xffffffff8xxxxxxx`.

### 11.5 QEMU Smoke Test

```bash
head -30 build/m13/qemu-smoke.log
```

Output:
```text
limine: Loading executable `boot():/boot/kernel.elf`...
MCSOS M8 boot
[MCSOS:M5] boot: external interrupt bring-up start
[MCSOS:M5] idt: loaded
[MCSOS:M5] pic: remapped; mask master=0x00000000000000fe slave=0x00000000000000ff
[MCSOS:M5] pit: configured 100Hz
[MCSOS:M5] sti: enabling interrupts
M6 PMM initialized
0x0000000001000000 frames managed
M7 VMM core initialized
M8 kmem initialized: total=0x0000000000010000 free=0x000000000000ffd0
M8 ready
[M9] scheduler initialized
[M11] elf: starting integration smoke test
[M11] elf: plan ok entry=0x0000000000401000
[M10] IDT vector 0x80 installed
[M10] syscall init
[M10] syscall ping ok
[M10] syscall get_ticks=0x0000000000000005
[M10] syscall smoke done
[M9] thread A tick
[M9] thread B tick
...
```

**Hasil: PASS** — Kernel boot lengkap M5→M6→M7→M8→M9→M10→M11 tanpa panic. VFS objects ter-link dan kernel tetap stabil.

### 11.6 Ringkasan Hasil Uji

| Pengujian | Perintah | Expected | Hasil |
| --------- | -------- | -------- | ----- |
| Host unit test | `make -f Makefile.m13 m13-all` | `PASS` | ✅ PASS |
| nm-undefined kosong | `cat build/m13/nm-undefined.txt` | Kosong | ✅ Kosong |
| ELF64 relocatable | `readelf -h build/m13/vfs.o` | Type: REL, Machine: X86-64 | ✅ Sesuai |
| VFS symbols di kernel | `nm -n build/mcsos-m5.elf \| grep mcs_vfs` | 17+ simbol | ✅ 17 simbol |
| Kernel build tanpa error | `make clean && make` | Exit 0 | ✅ Sukses |
| QEMU smoke — no panic | `timeout 10s qemu-system-x86_64 ...` | Boot normal | ✅ Boot normal |

---

## 12. Checksum Artifact

```text
--- build/m13/sha256sums.txt ---
0ab9bf2e3dffe92fd9cc5fedfa5e9309f657a67ab878699e95cb61255a30ef34  build/m13/ramfs.o
dd820072ee8aa0b2ffe46a8f4c6943c6444fbf427d911fb1a2e7b215a5ecb2be  build/m13/fd.o
ccd6b4637ba8223802385e8cc6f55b42711c341d7230a2499aededc7383169ac  build/m13/sys_vfs.o
5adf9ba241daf4af332695a0f9094d81cf766a350ad261792dfc3f276675d03d  build/m13/vfs.o
7a967e87f2d17c0bdb14237ab41e9205882f87a7cc1a9cd0dcfb50111dd136b6  build/m13/m13_vfs_host_test

--- build/m13/sha256-kernel-m13.txt ---
ef14baad334bfb26d8ab2ce949c93af4b3df3a21d7ab56976c723423fc78d397  build/mcsos-m5.elf
```

---

## 13. Analisis

### 13.1 Analisis Keberhasilan

```text
VFS layer M13 berhasil diimplementasikan dengan tiga komponen yang berfungsi
independen namun terintegrasi: RAMFS sebagai backend storage, FD table sebagai
abstraksi per-proses, dan syscall wrappers sebagai antarmuka kernel-userspace.

Host unit test membuktikan logika VFS bekerja benar di lingkungan hosted tanpa
dependensi kernel. nm -u kosong membuktikan implementasi sepenuhnya freestanding.
readelf membuktikan object sesuai format target kernel. QEMU smoke test
membuktikan integrasi tidak merusak boot sequence M5–M11 yang sudah ada.

Syscall dispatcher berhasil di-extend dari 5 entry (SYS_MAX=5) ke 10 entry
(SYS_MAX=10) dengan backward compatibility penuh — syscall lama (0..4) tidak
terpengaruh.
```

### 13.2 Analisis Keterbatasan

```text
1. Satu namespace global: seluruh proses berbagi satu RAMFS dan satu FD table
   kernel. Isolasi antar proses belum ada — proses A dapat membuka file yang
   dibuat proses B.

2. RAMFS volatil: tidak ada persistensi. Data hilang saat kernel restart.
   Tidak ada fsync, journal, recovery, atau fsck.

3. Pointer validation terbatas: mcsos_user_check_range() hanya memeriksa
   rentang alamat, bukan apakah halaman benar-benar mapped. Akses ke alamat
   yang valid secara rentang tetapi tidak mapped dapat menyebabkan page fault.

4. Tidak ada permission model: setiap proses dapat membuka, membaca, dan
   menulis file apapun tanpa pemeriksaan mode/uid/gid.

5. Kapasitas statik: jumlah file dibatasi MCS_MAX_FILES, ukuran file dibatasi
   MCS_FILE_MAX_SIZE, jumlah FD terbuka dibatasi MCS_MAX_FD. Tidak ada
   mekanisme ekspansi dinamis.
```

---

## 14. Keamanan dan Reliability

### 14.1 Boundary dan Input Validation

| Titik validasi | Implementasi | Kasus yang ditangani |
| -------------- | ------------ | -------------------- |
| FD range check | `fd < 0 \|\| fd >= MCS_MAX_FD` → return `MCSOS_EBADF` | FD negatif atau terlalu besar |
| Pointer NULL check | `path_ptr == 0` → return `MCSOS_EINVAL` | Pointer NULL dari userspace |
| User range check | `mcsos_user_check_range(ptr, len)` | Pointer ke kernel space |
| Path length | Dibatasi `MCS_MAX_PATH` | Path terlalu panjang |
| File size | Dibatasi `MCS_FILE_MAX_SIZE` | Write melebihi kapasitas inode |
| FD slot exhaustion | Tidak ada slot kosong → return `MCSOS_ENFILE` | Terlalu banyak file terbuka |

### 14.2 Failure Mode yang Diketahui

| Failure mode | Trigger | Behavior saat ini | Perbaikan yang diperlukan |
| ------------ | ------- | ----------------- | ------------------------- |
| Page fault dari copyin | Alamat valid secara rentang tapi tidak mapped | Kernel fault | Implementasi copyin/copyout yang proper dengan recovery |
| Race condition pada FD table | Dua thread membuka file secara bersamaan | Data race (undefined behavior) | Gunakan spinlock M12 untuk proteksi FD table |
| RAMFS penuh | Lebih dari MCS_MAX_FILES file dibuat | Return MCSOS_ENOSPC | Ekspansi dinamis atau cleanup policy |
| Name collision | Dua file dengan nama sama di-create | Behavior tergantung implementasi lookup | Cek eksplisit di create_file |

---

## 15. Readiness Review

Berdasarkan panduan M13 section 23:

| Gate | Status M13 | Bukti |
| ---- | ---------- | ----- |
| Toolchain/devenv | Siap uji | Clang 18.1.3, ld 2.42, QEMU 8.2.2, Make 4.3 tersedia |
| Kernel integration | Kandidat integrasi terbatas | VFS objects ter-link di kernel; 17 simbol di `0xffffffff8xxxxxxx` |
| Filesystem | Siap uji RAMFS volatil | Host test PASS; read/write/create/lseek/close terbukti berfungsi |
| Security | Belum siap | Permission dan copyin/copyout penuh belum ada |
| Crash consistency | Tidak siap | Tidak ada fsync, journal, recovery, fsck |
| QEMU | Diuji — boot normal | Kernel boot tanpa panic; serial log tersimpan |

**Kesimpulan readiness:** M13 **siap uji QEMU untuk VFS/FD/RAMFS awal**. Hasil ini belum siap demonstrasi produksi karena tidak ada permission model, crash consistency, dan persistent storage.

---

## 21. Rubrik Penilaian

| Komponen | Bobot | Indikator nilai penuh | Nilai |
| -------- | ----: | --------------------- | ----: |
| Kebenaran fungsional | 30 | Host test PASS, nm kosong, kernel build sukses, QEMU boot normal | `[0-30]` |
| Kualitas desain dan invariants | 20 | VFS layering jelas, invariants terdokumentasi, FD ownership explicit | `[0-20]` |
| Pengujian dan bukti | 20 | Host test, nm audit, readelf, sha256, QEMU smoke, nm-vfs-symbols | `[0-20]` |
| Debugging dan failure analysis | 10 | Failure modes dianalisis, pointer validation didokumentasikan | `[0-10]` |
| Keamanan dan robustness | 10 | Boundary check, input validation, failure mode dibahas | `[0-10]` |
| Dokumentasi dan laporan | 10 | Laporan lengkap, evidence terlampir, referensi IEEE | `[0-10]` |
| **Total** | **100** | | `[0-100]` |

Catatan penilai:
```text
[Diisi dosen/asisten.]
```

---

## 22. Kesimpulan

### 22.1 Yang Berhasil

```text
1. Implementasi VFS tiga lapis (RAMFS, FD table, syscall wrappers) berhasil
   dikompilasi sebagai object freestanding x86_64 tanpa dependency libc.

2. Host unit test lulus (PASS) membuktikan logika VFS benar di lingkungan
   hosted tanpa QEMU.

3. Object audit: nm-undefined kosong, readelf menunjukkan ELF64 REL x86_64,
   sha256 tersimpan untuk reproducibility.

4. Integrasi ke main kernel berhasil: 17 simbol VFS ter-link di higher-half
   kernel address space.

5. Syscall dispatcher berhasil di-extend dengan 5 syscall baru (open/read/
   write/close/lseek) tanpa regresi pada syscall lama.

6. QEMU smoke test: kernel boot dari M5 hingga M11 tanpa panic setelah VFS
   di-link.
```

### 22.2 Yang Belum Berhasil

```text
1. Tidak ada pengujian eksplisit syscall VFS dari userspace di QEMU —
   mcs_vfs_init_once() belum terpanggil dalam skenario smoke test ini
   karena belum ada user program yang memanggil syscall open/read/write.

2. Permission model (mode bits, uid/gid) belum ada.

3. Crash consistency (fsync, journal, recovery) tidak diimplementasikan
   sesuai non-goal M13.

4. Race condition pada FD table belum diproteksi spinlock.
```

### 22.3 Rencana Perbaikan

```text
1. Modul berikutnya: integrasikan spinlock M12 ke FD table agar operasi
   open/close thread-safe.

2. Implementasikan copyin/copyout yang proper dengan page fault recovery
   agar pointer validation lebih kuat.

3. Tambahkan user program sederhana yang memanggil syscall open/write/read
   dari ring 3 untuk membuktikan VFS end-to-end via QEMU.

4. Pertimbangkan mount table untuk mendukung multiple filesystem instance.
```

---

## 23. Lampiran

### Lampiran A — Commit Log

```text
756532b (HEAD -> praktikum-m13-vfs-ramfs) M13: VFS/RAMFS/FD implementation with kernel integration
3a0ecc3 (praktikum/m12-sync) M12: tambah evidence preflight, build log, dan qemu smoke test log
9d71a1b M12: add synchronization primitives (spinlock, mutex, lockdep)
```

### Lampiran B — Diff Ringkas

```diff
--- /dev/null
+++ include/mcs_vfs.h
@@ -0,0 +1,~97 @@
+#pragma once
+#include <stddef.h>
+#include <stdint.h>
+// [MCS_MAX_FILES, MCS_MAX_FD, MCS_MAX_PATH, MCS_FILE_MAX_SIZE, MCS_O_*]
+// [mcs_inode_t, mcs_ramfs_t, mcs_file_t, mcs_fd_table_t, mcs_process_t]
+// [deklarasi mcs_ramfs_init, mcs_vfs_open, mcs_vfs_read, mcs_vfs_write,
+//  mcs_vfs_lseek, mcs_vfs_close, mcs_vfs_dup, mcs_sys_open, mcs_sys_read,
+//  mcs_sys_write, mcs_sys_close, mcs_sys_lseek]

--- /dev/null
+++ kernel/vfs/ramfs.c
@@ -0,0 +1,~220 @@
+// mcs_ramfs_init, mcs_ramfs_lookup, mcs_ramfs_create_file,
+// mcs_ramfs_seed_file, mcs_ramfs_read, mcs_ramfs_write

--- /dev/null
+++ kernel/vfs/fd.c
@@ -0,0 +1,~240 @@
+// mcs_fd_table_init, mcs_vfs_open, mcs_vfs_read, mcs_vfs_write,
+// mcs_vfs_lseek, mcs_vfs_close, mcs_vfs_dup

--- /dev/null
+++ kernel/vfs/sys_vfs.c
@@ -0,0 +1,~6 @@
+// mcs_sys_open, mcs_sys_read, mcs_sys_write, mcs_sys_close, mcs_sys_lseek
+// (thin wrappers delegating to mcs_vfs_*)

--- a/include/mcsos/syscall.h
+++ b/include/mcsos/syscall.h
@@ -13,7 +13,21 @@
+    MCSOS_SYS_OPEN = 5,
+    MCSOS_SYS_READ = 6,
+    MCSOS_SYS_WRITE = 7,
+    MCSOS_SYS_CLOSE = 8,
+    MCSOS_SYS_LSEEK = 9,
+    MCSOS_SYS_MAX = 10

--- a/kernel/syscall/syscall.c
+++ b/kernel/syscall/syscall.c
+#include "mcs_vfs.h"
+static mcs_ramfs_t g_kernel_ramfs;
+static mcs_process_t g_kernel_main_process;
+static void mcs_vfs_init_once(void) { ... }
+static int64_t sys_open(...) { ... }
+static int64_t sys_read(...) { ... }
+static int64_t sys_write(...) { ... }
+static int64_t sys_close(...) { ... }
+static int64_t sys_lseek(...) { ... }
+// g_table[5..9] = sys_open..sys_lseek
```

### Lampiran C — Log Build Lengkap

```text
--- make -f Makefile.m13 m13-all ---
rm -rf build/m13
mkdir -p build/m13
cc -std=c17 -Wall -Wextra -Werror -O2 -Iinclude \
   tests/m13_vfs_host_test.c kernel/vfs/ramfs.c kernel/vfs/fd.c \
   kernel/vfs/sys_vfs.c -o build/m13/m13_vfs_host_test
./build/m13/m13_vfs_host_test | tee build/m13/host-test.log
M13 VFS/FD/RAMFS host tests: PASS
clang -target x86_64-elf -std=c17 -ffreestanding -fno-builtin \
   -fno-stack-protector -fno-pic -mno-red-zone -Wall -Wextra -Werror \
   -O2 -Iinclude -c kernel/vfs/ramfs.c -o build/m13/ramfs.o
clang -target x86_64-elf ... -c kernel/vfs/fd.c -o build/m13/fd.o
clang -target x86_64-elf ... -c kernel/vfs/sys_vfs.c -o build/m13/sys_vfs.o
ld -r -m elf_x86_64 build/m13/ramfs.o build/m13/fd.o build/m13/sys_vfs.o \
   -o build/m13/vfs.o
nm -u build/m13/vfs.o > build/m13/nm-undefined.txt
readelf -h build/m13/vfs.o > build/m13/readelf-vfs.txt
objdump -dr build/m13/vfs.o > build/m13/objdump-vfs.txt
sha256sum ... > build/m13/sha256sums.txt
test ! -s build/m13/nm-undefined.txt   [OK]

--- make clean && make (kernel build dengan VFS) ---
[22 object files compiled, termasuk ramfs.o, fd.o, sys_vfs.o]
ld.lld ... build/ramfs.o build/fd.o build/sys_vfs.o ... -o build/mcsos-m5.elf
[audit: readelf, nm, objdump, undefined.txt check — semua PASS]
```

### Lampiran D — Log QEMU

```text
Log QEMU tersimpan di: docs/reports/m13-evidence/qemu-smoke-head.log (50 baris)
Log lengkap: build/m13/qemu-smoke.log (161.571 baris — tidak di-commit karena
di-ignore .gitignore; head 50 baris disimpan sebagai evidence)

Isi head 50 baris:
limine: Loading executable `boot():/boot/kernel.elf`...
MCSOS M8 boot
[MCSOS:M5] boot: external interrupt bring-up start
[MCSOS:M5] idt: loaded
[MCSOS:M5] pic: remapped; mask master=0x00000000000000fe slave=...
[MCSOS:M5] pit: configured 100Hz
[MCSOS:M5] sti: enabling interrupts
M6 PMM initialized
...
[M10] syscall smoke done
[M9] thread A tick
[M9] thread B tick
...
```

### Lampiran E — Output Readelf

```text
ELF Header (build/m13/vfs.o):
  Magic:   7f 45 4c 46 02 01 01 00 00 00 00 00 00 00 00 00
  Class:                             ELF64
  Data:                              2's complement, little endian
  Version:                           1 (current)
  OS/ABI:                            UNIX - System V
  ABI Version:                       0
  Type:                              REL (Relocatable file)
  Machine:                           Advanced Micro Devices X86-64
  Version:                           0x1
  Entry point address:               0x0
  Start of program headers:          0 (bytes into file)
  Start of section headers:          6824 (bytes into file)
  Flags:                             0x0
  Size of this header:               64 (bytes)
  Number of program headers:         0
  Size of section headers:           64 (bytes)
  Number of section headers:         11
  Section header string table index: 10
```

### Lampiran F — Screenshot

| No. | File | Keterangan |
| --- | ---- | ---------- |
| 1 | `docs/reports/m13-evidence/host-test.log` | Host unit test PASS |
| 2 | `docs/reports/m13-evidence/nm-undefined.txt` | Kosong — tidak ada dependency libc |
| 3 | `docs/reports/m13-evidence/readelf-vfs.txt` | ELF64 REL x86_64 |
| 4 | `docs/reports/m13-evidence/nm-vfs-symbols.txt` | 17 simbol VFS di kernel |
| 5 | `docs/reports/m13-evidence/sha256sums.txt` | Checksum 5 artifact M13 |
| 6 | `docs/reports/m13-evidence/qemu-smoke-head.log` | 50 baris awal QEMU serial log |
| 7 | `docs/reports/m13-evidence/smoke-result.txt` | Ringkasan smoke test PASS |

### Lampiran G — Bukti Tambahan

```text
--- docs/reports/m13-evidence/sha256sums.txt ---
0ab9bf2e3dffe92fd9cc5fedfa5e9309f657a67ab878699e95cb61255a30ef34  build/m13/ramfs.o
dd820072ee8aa0b2ffe46a8f4c6943c6444fbf427d911fb1a2e7b215a5ecb2be  build/m13/fd.o
ccd6b4637ba8223802385e8cc6f55b42711c341d7230a2499aededc7383169ac  build/m13/sys_vfs.o
5adf9ba241daf4af332695a0f9094d81cf766a350ad261792dfc3f276675d03d  build/m13/vfs.o
7a967e87f2d17c0bdb14237ab41e9205882f87a7cc1a9cd0dcfb50111dd136b6  build/m13/m13_vfs_host_test

--- docs/reports/m13-evidence/sha256-kernel-m13.txt ---
ef14baad334bfb26d8ab2ce949c93af4b3df3a21d7ab56976c723423fc78d397  build/mcsos-m5.elf

--- docs/reports/m13-evidence/smoke-result.txt ---
=== M13 QEMU SMOKE: PASS ===
Kernel booted without panic after VFS integration
Syscall dispatcher active (M10 smoke done)
VFS objects linked: ramfs.o fd.o sys_vfs.o

--- docs/reports/m13-evidence/nm-vfs-symbols.txt (ringkas) ---
ffffffff80004670 T mcs_ramfs_init
ffffffff80004d10 T mcs_vfs_open
ffffffff80005590 T mcs_sys_open
ffffffff800067c0 r g_table
ffffffff80009060 b g_kernel_main_process
ffffffff80009268 b g_kernel_ramfs
[17 simbol total]
```

---

## 24. Daftar Referensi

```text
[1] M. Sidiq, "Panduan Praktikum M13 — Virtual Filesystem (VFS), File
    Descriptor Table, dan RAMFS Volatil pada MCSOS," Institut Pendidikan
    Indonesia, 2026.

[2] Linux Kernel Documentation, "Overview of the Linux Virtual File System,"
    kernel.org. [Online]. Available: https://docs.kernel.org/filesystems/vfs.html.
    Accessed: Jun. 2026.

[3] The Open Group, "open - open a file," The Open Group Base Specifications
    Issue 7/IEEE Std 1003.1, 2018 edition. [Online]. Available:
    https://pubs.opengroup.org/onlinepubs/9699919799/functions/open.html.
    Accessed: Jun. 2026.

[4] Intel Corporation, "Intel® 64 and IA-32 Architectures Software Developer
    Manuals," Intel Developer Zone, updated Apr. 2026. [Online]. Available:
    https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html.
    Accessed: Jun. 2026.

[5] QEMU Project, "GDB usage / gdbstub documentation," QEMU Documentation.
    [Online]. Available: https://www.qemu.org/docs/master/system/gdb.html.
    Accessed: Jun. 2026.

[6] LLVM Project, "Clang command line argument reference," Clang Documentation,
    2026. [Online]. Available:
    https://clang.llvm.org/docs/ClangCommandLineReference.html.
    Accessed: Jun. 2026.

[7] GNU Binutils, "readelf, objdump, nm," GNU documentation. [Online].
    Available: https://sourceware.org/binutils/docs/.
    Accessed: Jun. 2026.
```

---

## 25. Checklist Final Sebelum Pengumpulan

| Checklist | Status |
| --------- | ------ |
| Semua placeholder `[isi ...]` sudah diganti | `Ya` — semua diisi dengan output terminal nyata |
| Metadata laporan lengkap | `Ya` |
| Commit awal dan akhir dicatat | `Ya` — `3a0ecc3` dan `756532b` |
| Perintah build dan test dapat dijalankan ulang | `Ya` — `make -f Makefile.m13 m13-all` dan `make clean && make` |
| Log build dilampirkan | `Ya` — Lampiran C |
| Log QEMU/test dilampirkan | `Ya` — Lampiran D dan G |
| Artefak penting diberi hash | `Ya` — SHA256 di Lampiran G |
| Desain, invariants, ownership, dan failure modes dijelaskan | `Ya` |
| Security/reliability dibahas | `Ya` — Bagian 14 |
| Readiness review tidak berlebihan | `Ya` — status "siap uji QEMU terbatas" |
| Rubrik penilaian diisi atau disiapkan | `Ya` (kolom nilai menunggu penilaian dosen) |
| Referensi memakai format IEEE | `Ya` |
| Laporan disimpan sebagai Markdown | `Ya` |

---

## 26. Pernyataan Pengumpulan

Kami mengumpulkan laporan ini bersama artefak pendukung pada commit:

```text
756532b — M13: VFS/RAMFS/FD implementation with kernel integration
```

Status akhir yang diklaim:

```text
siap uji QEMU untuk VFS/FD/RAMFS awal
```

Ringkasan satu paragraf:

```text
Praktikum M13 berhasil mengimplementasikan Virtual Filesystem (VFS) tiga lapis
pada kernel MCSOS 260502 untuk target x86_64: RAMFS volatil sebagai backend
storage, FD table per-proses sebagai abstraksi file descriptor, dan syscall
wrappers sebagai antarmuka kernel-userspace. Syscall dispatcher di-extend dari
5 ke 10 entry (open/read/write/close/lseek) dengan backward compatibility penuh.
Host unit test lulus (PASS). nm -u vfs.o kosong membuktikan tidak ada dependency
libc. readelf menunjukkan ELF64 REL x86_64. 17 simbol VFS ter-link di kernel
higher-half address space (0xffffffff8xxxxxxx). QEMU smoke test membuktikan
kernel boot M5–M11 tetap stabil setelah VFS di-link tanpa panic. Keterbatasan
M13: tidak ada permission model, tidak ada copyin/copyout penuh, tidak ada
crash consistency, dan FD table belum diproteksi spinlock. Langkah berikutnya
adalah integrasi spinlock M12 ke FD table, implementasi copyin/copyout dengan
page fault recovery, dan user program ring 3 yang memanggil syscall VFS secara
end-to-end.
```
