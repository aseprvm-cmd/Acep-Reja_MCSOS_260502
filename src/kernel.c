#include "idt.h"
#include "io.h"
#include "panic.h"
#include "pic.h"
#include "pit.h"
#include "serial.h"
#include "pmm.h"
#include "vmm.h"
#include "mcsos/kmem.h"
#include "mcsos_thread.h"
#include "mcsos/syscall.h"
/* forward declaration untuk callback M10 */

/* forward declaration untuk callback M10 */
static mcsos_scheduler_t g_sched;

/* ── M10 syscall callbacks ──────────────────────────────── */
static uint64_t k_get_ticks(void) {
    return timer_ticks();
}
static void k_yield_current(void) {
    mcsos_sched_yield(&g_sched);
}
static void k_exit_current(int code) {
    serial_write_string("[M10] exit_thread code=");
    serial_write_hex64((uint64_t)(unsigned int)code);
    serial_write_string("\n");
    for (;;) { cpu_hlt(); }
}
static int64_t k_write_serial(const char *buf, size_t len) {
    for (size_t i = 0; i < len; i++) {
        char tmp[2] = { buf[i], '\0' };
        serial_write_string(tmp);
    }
    return (int64_t)len;
}
static void m10_syscall_smoke(void) {
    int64_t r = mcsos_syscall_dispatch(MCSOS_SYS_PING, 0,0,0,0,0,0);
    if (r != 0x2605020AL) { kernel_panic("M10 ping failed", (uint64_t)r); }
    serial_write_string("[M10] syscall ping ok\n");
    r = mcsos_syscall_dispatch(MCSOS_SYS_GET_TICKS, 0,0,0,0,0,0);
    serial_write_string("[M10] syscall get_ticks=");
    serial_write_hex64((uint64_t)r);
    serial_write_string("\n");
    serial_write_string("[M10] syscall smoke done\n");
}
/* ── end M10 ─────────────────────────────────────────────── */

static mcsos_scheduler_t g_sched;
static mcsos_thread_t    g_boot_thread;
static mcsos_thread_t    g_thread_a;
static mcsos_thread_t    g_thread_b;
static unsigned char g_stack_a[8192] __attribute__((aligned(16)));
static unsigned char g_stack_b[8192] __attribute__((aligned(16)));

static void demo_thread_a(void *arg) {
    (void)arg;
    for (;;) {
        serial_write_string("[M9] thread A tick\n");
        mcsos_sched_yield(&g_sched);
    }
}

static void demo_thread_b(void *arg) {
    (void)arg;
    for (;;) {
        serial_write_string("[M9] thread B tick\n");
        mcsos_sched_yield(&g_sched);
    }
}

/* --------------------------------------------------------->
 * PMM M6
 * --------------------------------------------------------->
 */
static struct pmm_state kernel_pmm;
static uint8_t kernel_pmm_bitmap[PMM_BITMAP_BYTES] __attribute__((aligned(4096)));

/* --------------------------------------------------------->
 * VMM M7 — adapter dan state
 * --------------------------------------------------------->
 */
static struct vmm_space kernel_space;

/* HHDM offset dummy — ganti dengan nilai dari Limine nanti */
static uint64_t hhdm_offset = 0x0ULL;

static uint64_t kernel_vmm_alloc(void *ctx) {
    (void)ctx;
    return pmm_alloc_frame(&kernel_pmm);
}

static void kernel_vmm_free(void *ctx, uint64_t frame_paddr) {
    (void)ctx;
    pmm_free_frame(&kernel_pmm, frame_paddr);
}

static void *kernel_phys_to_virt(void *ctx, uint64_t paddr) {
    const uint64_t off = *(const uint64_t *)ctx;
    return (void *)(off + paddr);
}

/* --------------------------------------------------------->
 * PMM init M6
 * --------------------------------------------------------->
 */
static void kernel_memory_init(void) {
    static struct boot_mem_region early_map[] = {
        { 0x00000000ULL, 0x0009f000ULL, BOOT_MEM_USABLE },
        { 0x0009f000ULL, 0x00001000ULL, BOOT_MEM_RESERVED },
        { 0x00100000ULL, 0x00300000ULL, BOOT_MEM_USABLE },
        { 0x00400000ULL, 0x00100000ULL, BOOT_MEM_KERNEL_AND_MODULES },
        { 0x00500000ULL, 0x07b00000ULL, BOOT_MEM_USABLE },
    };
    size_t count = sizeof(early_map) / sizeof(early_map[0]);

    bool ok = pmm_init_from_map(&kernel_pmm,
                                early_map, count,
                                kernel_pmm_bitmap,
                                sizeof(kernel_pmm_bitmap),
                                PMM_MAX_PHYS_BYTES);
    if (!ok) {
        serial_write_string("[m6] pmm_init_from_map failed\n");
        return;
    }

    serial_write_string("M6 PMM initialized\n");
    serial_write_hex64(pmm_frame_count(&kernel_pmm));
    serial_write_string(" frames managed\n");
    serial_write_hex64(pmm_free_count(&kernel_pmm));
    serial_write_string(" frames free\n");

    uint64_t f = pmm_alloc_frame(&kernel_pmm);
    if (f == PMM_INVALID_FRAME) {
        serial_write_string("[m6] alloc failed\n");
        return;
    }
    serial_write_string("[m6] sample frame = ");
    serial_write_hex64(f);
    serial_write_string("\n");

    if (!pmm_free_frame(&kernel_pmm, f)) {
        serial_write_string("[m6] free failed\n");
        return;
    }
    serial_write_string("[m6] frame freed ok\n");
}

/* --------------------------------------------------------->
 * VMM init M7
 * --------------------------------------------------------->
 */
static void kernel_vmm_init(void) {
    uint64_t root = pmm_alloc_frame(&kernel_pmm);
    if (root == PMM_INVALID_FRAME) {
        serial_write_string("[m7] ERROR: cannot allocate root page table\n");
        return;
    }

    int rc = vmm_space_init(&kernel_space, root, &hhdm_offset,
                            kernel_vmm_alloc, kernel_vmm_free,
                            kernel_phys_to_virt);
    if (rc != VMM_MAP_OK) {
        serial_write_string("[m7] ERROR: vmm_space_init failed\n");
        pmm_free_frame(&kernel_pmm, root);
        return;
    }

    serial_write_string("M7 VMM core initialized\n");
    /* JANGAN panggil vmm_write_cr3() di sini —
     * mapping kernel/stack/IDT belum lengkap */
}

/* --------------------------------------------------------->
 * M8 — Kernel Heap Bootstrap
 * --------------------------------------------------------->
 */
#define M8_BOOT_HEAP_SIZE (64u * 1024u)
static unsigned char m8_boot_heap[M8_BOOT_HEAP_SIZE] __attribute__((aligned(4096)));

static void kernel_heap_init(void) {
    int rc = kmem_init(m8_boot_heap, sizeof(m8_boot_heap));
    if (rc != 0) {
        serial_write_string("[m8] ERROR: kmem_init failed rc=");
        serial_write_hex64((uint64_t)(unsigned int)rc);
        serial_write_string("\n");
        return;
    }

    void *probe = kmem_alloc(128);
    if (probe == (void *)0) {
        serial_write_string("[m8] ERROR: kmem_alloc probe failed\n");
        return;
    }
    if (kmem_free_checked(probe) != 0) {
        serial_write_string("[m8] ERROR: kmem_free_checked probe failed\n");
        return;
    }

    kmem_stats_t st;
    kmem_get_stats(&st);
    serial_write_string("M8 kmem initialized: total=");
    serial_write_hex64((uint64_t)st.total_bytes);
    serial_write_string(" free=");
    serial_write_hex64((uint64_t)st.free_bytes);
    serial_write_string(" largest=");
    serial_write_hex64((uint64_t)st.largest_free);
    serial_write_string("\n");
}

/* --------------------------------------------------------->
 * kmain
 * --------------------------------------------------------->
 */
void kmain(void) {
    cpu_cli();
    serial_init();
    serial_write_string("MCSOS M8 boot\n");
    serial_write_string("[MCSOS:M5] boot: external interrupt bring-up start\n");
    idt_init();
    serial_write_string("[MCSOS:M5] idt: loaded\n");
    pic_remap(PIC_MASTER_OFFSET, PIC_SLAVE_OFFSET);
    pic_mask_all();
    pic_unmask_irq(0);
    serial_write_string("[MCSOS:M5] pic: remapped; mask master=");
    serial_write_hex64(pic_read_master_mask());
    serial_write_string(" slave=");
    serial_write_hex64(pic_read_slave_mask());
    serial_write_string("\n");
    pit_configure_hz(100u);
    serial_write_string("[MCSOS:M5] pit: configured 100Hz\n");
    serial_write_string("[MCSOS:M5] sti: enabling interrupts\n");
    cpu_sti();
#if defined(MCSOS_TEST_BREAKPOINT)
    __asm__ volatile ("int3");
#endif
    kernel_memory_init();
    kernel_vmm_init();
    kernel_heap_init();
    serial_write_string("M8 ready\n");

/* M9 — scheduler init */
mcsos_scheduler_init(&g_sched, &g_boot_thread);
mcsos_thread_prepare(&g_thread_a, "demo-a", demo_thread_a, 0,
                     g_stack_a, sizeof(g_stack_a), g_sched.next_id++);
mcsos_thread_prepare(&g_thread_b, "demo-b", demo_thread_b, 0,
                     g_stack_b, sizeof(g_stack_b), g_sched.next_id++);
mcsos_sched_enqueue(&g_sched, &g_thread_a);
mcsos_sched_enqueue(&g_sched, &g_thread_b);
serial_write_string("[M9] scheduler initialized\n");
    /* M10 — syscall init */
    mcsos_syscall_ops_t ops = {
        .get_ticks     = k_get_ticks,
        .yield_current = k_yield_current,
        .exit_current  = k_exit_current,
        .write_serial  = k_write_serial,
    };
    mcsos_syscall_init(&ops);
    extern void x86_64_syscall_int80_stub(void);
    idt_install_gate(0x80u, x86_64_syscall_int80_stub, 0x8Eu);
    serial_write_string("[M10] IDT vector 0x80 installed\n");
    mcsos_syscall_set_user_region((mcsos_user_region_t){
        .base  = 0x0000000000400000ULL,
        .limit = 0x0000800000000000ULL,
    });
    serial_write_string("[M10] syscall init\n");
    m10_syscall_smoke();
mcsos_sched_yield(&g_sched);
    for (;;) {
        cpu_hlt();
    }
}
