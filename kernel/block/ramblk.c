#include "../../include/mcsos/block.h"

/* Freestanding memset implementation */
static void *memset(void *s, int c, unsigned long n)
{
    unsigned char *p = (unsigned char *)s;
    for (unsigned long i = 0; i < n; i++) {
        p[i] = (unsigned char)c;
    }
    return s;
}

/* Freestanding memcpy implementation */
static void *memcpy(void *dest, const void *src, unsigned long n)
{
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    for (unsigned long i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dest;
}

/* ============================================================
   RAM Block Driver - Synthetic block device backed by RAM
   ============================================================ */

#define RAMBLK_DEVICE_SIZE_BLOCKS 4096  /* 4096 * 512 = 2 MB */
#define RAMBLK_MAGIC 0xDEADBEEF

struct ramblk_device {
    uint32_t magic;
    uint8_t storage[RAMBLK_DEVICE_SIZE_BLOCKS][MCSOS_BLOCK_SIZE];
};

static struct ramblk_device ramblk_dev = {
    .magic = RAMBLK_MAGIC,
};

/* Forward declaration */
void blk_init(void);

/* Driver operation: read from RAM */
static int ramblk_read(uint64_t lba, uint8_t *buf)
{
    if (!buf) {
        return BLK_ERR_INVALID_LBA;
    }

    if (lba >= RAMBLK_DEVICE_SIZE_BLOCKS) {
        return BLK_ERR_INVALID_LBA;
    }

    if (ramblk_dev.magic != RAMBLK_MAGIC) {
        return BLK_ERR_DRIVER_FAIL;
    }

    memcpy(buf, ramblk_dev.storage[lba], MCSOS_BLOCK_SIZE);
    return BLK_OK;
}

/* Driver operation: write to RAM */
static int ramblk_write(uint64_t lba, const uint8_t *buf)
{
    if (!buf) {
        return BLK_ERR_INVALID_LBA;
    }

    if (lba >= RAMBLK_DEVICE_SIZE_BLOCKS) {
        return BLK_ERR_INVALID_LBA;
    }

    if (ramblk_dev.magic != RAMBLK_MAGIC) {
        return BLK_ERR_DRIVER_FAIL;
    }

    memcpy(ramblk_dev.storage[lba], buf, MCSOS_BLOCK_SIZE);
    return BLK_OK;
}

/* Driver operation: flush (no-op for RAM) */
static int ramblk_flush(void)
{
    /* RAM device has no persistent media, flush is no-op */
    return BLK_OK;
}

/* Driver operation table */
static struct block_driver_ops ramblk_ops = {
    .read = ramblk_read,
    .write = ramblk_write,
    .flush = ramblk_flush,
};

/* ← TAMBAH: Static block device structure */
static struct block_device ramblk_block_device = {
    .dev_id = 0,
    .block_size = MCSOS_BLOCK_SIZE,
    .block_count = RAMBLK_DEVICE_SIZE_BLOCKS,
    .ops = &ramblk_ops,
    .driver_private = (void *)&ramblk_dev,
};

/* Register RAM block device */
int ramblk_init(void)
{
    blk_init();

    /* ← UBAH: Register pointer ke static variable, bukan local variable */
    return blk_register_device(&ramblk_block_device);
}

/* Utility: get RAM device pointer (for testing) */
struct ramblk_device *ramblk_get_device(void)
{
    return &ramblk_dev;
}

/* Utility: reset RAM device (for testing) */
void ramblk_reset(void)
{
    memset(&ramblk_dev, 0, sizeof(ramblk_dev));
    ramblk_dev.magic = RAMBLK_MAGIC;
}
