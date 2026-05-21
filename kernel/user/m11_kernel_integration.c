#include <mcsos/user/m11_elf_loader.h>
#include <mcsos/kernel/log.h>

static void m11_log_plan(const struct m11_process_image_plan *plan) {
    log_write("[M11] elf: plan ok entry=");
    log_hex64(plan->entry);
    log_writeln("");
    log_write("[M11] elf: segment_count=");
    log_hex64((uint64_t)plan->segment_count);
    log_writeln("");
    for (uint32_t i = 0u; i < plan->segment_count; i++) {
        const struct m11_segment_plan *s = &plan->segments[i];
        log_write("[M11] elf: seg vaddr=");
        log_hex64(s->vaddr);
        log_write(" filesz=");
        log_hex64(s->filesz);
        log_write(" memsz=");
        log_hex64(s->memsz);
        log_write(" flags=");
        log_hex64((uint64_t)s->flags);
        log_writeln("");
    }
}

void m11_integration_smoke_test(void) {
    /* ELF sintetis minimal — sama persis dengan make_valid_image di host test */
    static unsigned char image[12288u];
    for (uint32_t i = 0u; i < 12288u; i++) { image[i] = 0u; }

    struct m11_elf64_ehdr *eh = (struct m11_elf64_ehdr *)(void *)image;
    eh->e_ident[0] = M11_ELFMAG0; eh->e_ident[1] = M11_ELFMAG1;
    eh->e_ident[2] = M11_ELFMAG2; eh->e_ident[3] = M11_ELFMAG3;
    eh->e_ident[4] = M11_ELFCLASS64;
    eh->e_ident[5] = M11_ELFDATA2LSB;
    eh->e_ident[6] = M11_EV_CURRENT;
    eh->e_type      = M11_ET_EXEC;
    eh->e_machine   = M11_EM_X86_64;
    eh->e_version   = M11_EV_CURRENT;
    eh->e_entry     = 0x0000000000401000ull;
    eh->e_phoff     = sizeof(struct m11_elf64_ehdr);
    eh->e_ehsize    = (uint16_t)sizeof(struct m11_elf64_ehdr);
    eh->e_phentsize = (uint16_t)sizeof(struct m11_elf64_phdr);
    eh->e_phnum     = 2u;

    struct m11_elf64_phdr *ph =
        (struct m11_elf64_phdr *)(void *)(image + eh->e_phoff);
    ph[0].p_type   = M11_PT_LOAD;
    ph[0].p_flags  = M11_PF_R | M11_PF_X;
    ph[0].p_offset = 0x1000u;
    ph[0].p_vaddr  = 0x0000000000400000ull;
    ph[0].p_filesz = 16u;
    ph[0].p_memsz  = 4096u;
    ph[0].p_align  = M11_PAGE_SIZE;
    ph[1].p_type   = M11_PT_LOAD;
    ph[1].p_flags  = M11_PF_R | M11_PF_W;
    ph[1].p_offset = 0x2000u;
    ph[1].p_vaddr  = 0x0000000000401000ull;
    ph[1].p_filesz = 8u;
    ph[1].p_memsz  = 4096u;
    ph[1].p_align  = M11_PAGE_SIZE;

    struct m11_user_region region;
    region.base  = 0x0000000000400000ull;
    region.limit = 0x0000008000000000ull;

    struct m11_process_image_plan plan;
    log_writeln("[M11] elf: starting integration smoke test");
    int rc = m11_elf64_plan_load(image, 12288u, region, &plan);
    if (rc != M11_OK) {
        log_write("[M11] elf: plan FAILED rc=");
        log_hex64((uint64_t)(unsigned int)(rc < 0 ? (unsigned int)-rc : (unsigned int)rc));
        log_writeln("");
        return;
    }
    m11_log_plan(&plan);
    log_writeln("[M11] user image plan ready");
}
