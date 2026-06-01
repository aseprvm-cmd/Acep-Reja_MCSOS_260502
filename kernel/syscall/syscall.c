#include "mcsos/syscall.h"
#include "mcs_vfs.h"

// Global kernel RAMFS and process for file operations
static mcs_ramfs_t g_kernel_ramfs;
static mcs_process_t g_kernel_main_process;
static int g_vfs_initialized = 0;

static void mcs_vfs_init_once(void) {
    if (!g_vfs_initialized) {
        mcs_ramfs_init(&g_kernel_ramfs);
        g_kernel_main_process.pid = 0;
        mcs_fd_table_init(&g_kernel_main_process.fd_table);
        g_vfs_initialized = 1;
    }
}

static mcsos_syscall_ops_t g_ops;
static mcsos_user_region_t g_user_region;

static int64_t default_write_serial(const char *buf, size_t len) {
    (void)buf;
    return (int64_t)len;
}

void mcsos_syscall_init(const mcsos_syscall_ops_t *ops) {
    g_ops.get_ticks = 0;
    g_ops.yield_current = 0;
    g_ops.exit_current = 0;
    g_ops.write_serial = default_write_serial;
    if (ops != 0) {
        if (ops->get_ticks != 0) g_ops.get_ticks = ops->get_ticks;
        if (ops->yield_current != 0) g_ops.yield_current = ops->yield_current;
        if (ops->exit_current != 0) g_ops.exit_current = ops->exit_current;
        if (ops->write_serial != 0) g_ops.write_serial = ops->write_serial;
    }
}

void mcsos_syscall_set_user_region(mcsos_user_region_t region) {
    g_user_region = region;
}

int mcsos_user_check_range(uintptr_t addr, size_t len) {
    if (len == 0u) return 1;
    if (g_user_region.base == 0u || g_user_region.limit <= g_user_region.base) return 0;
    if (addr < g_user_region.base) return 0;
    if (addr > g_user_region.limit) return 0;
    uintptr_t last = addr + (uintptr_t)len - 1u;
    if (last < addr) return 0;
    if (last >= g_user_region.limit) return 0;
    return 1;
}

int mcsos_copy_from_user(void *dst, const void *src, size_t len) {
    if (len == 0u) return MCSOS_OK;
    if (dst == 0 || src == 0) return MCSOS_EINVAL;
    if (!mcsos_user_check_range((uintptr_t)src, len)) return MCSOS_EFAULT;
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    for (size_t i = 0; i < len; ++i) d[i] = s[i];
    return MCSOS_OK;
}

static int64_t sys_ping(uint64_t a0, uint64_t a1, uint64_t a2,
                        uint64_t a3, uint64_t a4, uint64_t a5) {
    (void)a0; (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
    return 0x2605020AL;
}

static int64_t sys_get_ticks(uint64_t a0, uint64_t a1, uint64_t a2,
                             uint64_t a3, uint64_t a4, uint64_t a5) {
    (void)a0; (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
    if (g_ops.get_ticks == 0) return MCSOS_EBUSY;
    return (int64_t)g_ops.get_ticks();
}

static int64_t sys_write_serial(uint64_t ptr, uint64_t len, uint64_t a2,
                                uint64_t a3, uint64_t a4, uint64_t a5) {
    (void)a2; (void)a3; (void)a4; (void)a5;
    if (ptr == 0u) return MCSOS_EINVAL;
    if (len > 4096u) return MCSOS_EINVAL;
    if (!mcsos_user_check_range((uintptr_t)ptr, (size_t)len)) return MCSOS_EFAULT;
    return g_ops.write_serial((const char *)(uintptr_t)ptr, (size_t)len);
}

static int64_t sys_yield(uint64_t a0, uint64_t a1, uint64_t a2,
                         uint64_t a3, uint64_t a4, uint64_t a5) {
    (void)a0; (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
    if (g_ops.yield_current == 0) return MCSOS_EBUSY;
    g_ops.yield_current();
    return MCSOS_OK;
}

static int64_t sys_exit_thread(uint64_t code, uint64_t a1, uint64_t a2,
                               uint64_t a3, uint64_t a4, uint64_t a5) {
    (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
    if (g_ops.exit_current == 0) return MCSOS_EBUSY;
    g_ops.exit_current((int)code);
    return MCSOS_OK;
}

// ========== VFS SYSCALL WRAPPERS ==========

static int64_t sys_open(uint64_t path_ptr, uint64_t flags, uint64_t a2,
                        uint64_t a3, uint64_t a4, uint64_t a5) {
    (void)a2; (void)a3; (void)a4; (void)a5;
    
    if (path_ptr == 0u) return MCSOS_EINVAL;
    if (flags == 0u) flags = MCS_O_RDONLY;
    if (!mcsos_user_check_range(path_ptr, MCS_MAX_PATH)) return MCSOS_EFAULT;
    
    mcs_vfs_init_once();
    return mcs_sys_open(&g_kernel_main_process, &g_kernel_ramfs,
                       (const char *)(uintptr_t)path_ptr, (uint32_t)flags);
}

static int64_t sys_read(uint64_t fd, uint64_t buf_ptr, uint64_t len,
                        uint64_t a3, uint64_t a4, uint64_t a5) {
    (void)a3; (void)a4; (void)a5;
    
    if (buf_ptr == 0u && len != 0u) return MCSOS_EINVAL;
    if (len > 0u && !mcsos_user_check_range(buf_ptr, len)) return MCSOS_EFAULT;
    
    mcs_vfs_init_once();
    return mcs_sys_read(&g_kernel_main_process, (int)fd,
                       (void *)(uintptr_t)buf_ptr, (size_t)len);
}

static int64_t sys_write(uint64_t fd, uint64_t buf_ptr, uint64_t len,
                         uint64_t a3, uint64_t a4, uint64_t a5) {
    (void)a3; (void)a4; (void)a5;
    
    if (buf_ptr == 0u && len != 0u) return MCSOS_EINVAL;
    if (len > 0u && !mcsos_user_check_range(buf_ptr, len)) return MCSOS_EFAULT;
    
    mcs_vfs_init_once();
    return mcs_sys_write(&g_kernel_main_process, (int)fd,
                        (const void *)(uintptr_t)buf_ptr, (size_t)len);
}

static int64_t sys_close(uint64_t fd, uint64_t a1, uint64_t a2,
                         uint64_t a3, uint64_t a4, uint64_t a5) {
    (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
    
    mcs_vfs_init_once();
    return mcs_sys_close(&g_kernel_main_process, (int)fd);
}

static int64_t sys_lseek(uint64_t fd, uint64_t offset, uint64_t whence,
                         uint64_t a3, uint64_t a4, uint64_t a5) {
    (void)a3; (void)a4; (void)a5;
    
    mcs_vfs_init_once();
    return mcs_sys_lseek(&g_kernel_main_process, (int)fd, (long)offset, (int)whence);
}

typedef int64_t (*syscall_fn_t)(uint64_t, uint64_t, uint64_t,
                                uint64_t, uint64_t, uint64_t);

static syscall_fn_t g_table[MCSOS_SYS_MAX] = {
    sys_ping,           // 0: MCSOS_SYS_PING
    sys_get_ticks,      // 1: MCSOS_SYS_GET_TICKS
    sys_write_serial,   // 2: MCSOS_SYS_WRITE_SERIAL
    sys_yield,          // 3: MCSOS_SYS_YIELD
    sys_exit_thread,    // 4: MCSOS_SYS_EXIT_THREAD
    sys_open,           // 5: MCSOS_SYS_OPEN
    sys_read,           // 6: MCSOS_SYS_READ
    sys_write,          // 7: MCSOS_SYS_WRITE
    sys_close,          // 8: MCSOS_SYS_CLOSE
    sys_lseek           // 9: MCSOS_SYS_LSEEK
};

int64_t mcsos_syscall_dispatch(uint64_t nr, uint64_t arg0, uint64_t arg1,
                               uint64_t arg2, uint64_t arg3, uint64_t arg4,
                               uint64_t arg5) {
    if (nr >= (uint64_t)MCSOS_SYS_MAX) return MCSOS_ENOSYS;
    syscall_fn_t fn = g_table[nr];
    if (fn == 0) return MCSOS_ENOSYS;
    return fn(arg0, arg1, arg2, arg3, arg4, arg5);
}

void mcsos_syscall_dispatch_frame(mcsos_syscall_frame_t *frame) {
    if (frame == 0) return;
    frame->ret = mcsos_syscall_dispatch(frame->nr, frame->arg0, frame->arg1,
                                        frame->arg2, frame->arg3, frame->arg4,
                                        frame->arg5);
}
