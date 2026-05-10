# Invariants MCSOS 260502 — Baseline M0

## Repository invariants

1. Repository utama berada di filesystem Linux WSL.
2. Semua generated artifact berada di `build/` atau lokasi generated yang terdokumentasi.
3. Source, dokumen, dan script validasi dikomit ke Git.
4. File generated besar seperti image, object, ISO, dan log penuh tidak dikomit kecuali diminta sebagai fixture.

## Toolchain invariants

1. Setiap praktikum mencatat versi tool pada `build/meta/toolchain-versions.txt` atau file metadata setara.
2. Compiler target harus dinyatakan eksplisit; kernel tidak boleh diam-diam memakai ABI host.
3. Object smoke test harus diperiksa dengan `readelf`, `objdump`, atau tool setara.
4. Flag freestanding dan red-zone policy harus terdokumentasi sebelum kode kernel nyata dibuat.

## Documentation invariants

1. Requirement harus memiliki metode verifikasi.
2. Risiko harus memiliki mitigasi atau trigger review.
3. Threat model harus ada sejak M0 dan diperbarui ketika subsistem baru ditambahkan.
4. Readiness label harus berbasis bukti.

## Evidence invariants

1. Klaim “berhasil” harus memiliki command output, log, checksum, screenshot, commit, atau artefak yang dapat diperiksa.
2. Error tidak boleh dihapus dari laporan; error harus diklasifikasi dan dianalisis.
3. Setiap rollback harus didokumentasikan.


# MCSOS Toolchain and Environment Invariants

## M1 invariants

1. Repository MCSOS berada di filesystem Linux WSL, bukan di `/mnt/c` atau mount Windows lain.
2. Semua generated artifact berada di `build/` dan tidak dikomit ke Git.
3. Semua build tool wajib tersedia melalui PATH WSL dan tercatat di `build/meta/toolchain-versions.txt`.
4. Proof object harus bertipe ELF64 x86_64 dan dihasilkan dengan mode freestanding.
5. Proof ELF tidak boleh memiliki undefined symbol.
6. Kompilasi kernel/proof tidak boleh bergantung pada hosted libc, startup object, dynamic linker, exception runtime, atau stack protector runtime host.
7. QEMU x86_64, machine q35, dan OVMF harus terdeteksi sebelum M2 dimulai.
8. Setiap perubahan toolchain atau versi distro harus dicatat dalam readiness review.

