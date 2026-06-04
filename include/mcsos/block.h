#ifndef MCSOS_BLOCK_H
#define MCSOS_BLOCK_H

#include <stdint.h>
#include <stddef.h>

/* ============================================================
   Block Device Layer - Minimal Interface
   ============================================================ */

#define MCSOS_BLOCK_SIZE 512
#define MCSOS_MAX_BLOCK_DEVICES 16
#define MCSOS_BUFFER_CACHE_ENTRIES 32

/* Error codes */
typedef enum {
    BLK_OK = 0,
    BLK_ERR_INVALID_LBA = -1,
    BLK_ERR_INVALID_DEV = -2,
    BLK_ERR_NOT_VALID = -3,
    BLK_ERR_DRIVER_FAIL = -4,
} blk_error_t;

/* Block device driver operation table */
struct block_driver_ops {
    int (*read)(uint64_t lba, uint8_t *buf);
    int (*write)(uint64_t lba, const uint8_t *buf);
    int (*flush)(void);
};

/* Block device entry */
struct block_device {
    uint32_t dev_id;
    uint32_t block_size;
    uint64_t block_count;
    struct block_driver_ops *ops;
    void *driver_private;
};

/* Buffer cache entry */
struct buffer_cache_entry {
    uint32_t dev_id;
    uint64_t lba;
    uint8_t valid;
    uint8_t dirty;
    uint8_t data[MCSOS_BLOCK_SIZE];
};

/* Public API - Block Device Registry */
int blk_register_device(struct block_device *dev);
struct block_device *blk_get_device(uint32_t dev_id);

/* Public API - Block I/O */
int blk_read(uint32_t dev_id, uint64_t lba, uint8_t *buf);
int blk_write(uint32_t dev_id, uint64_t lba, const uint8_t *buf);
int blk_flush(uint32_t dev_id);

/* Public API - Buffer Cache */
struct buffer_cache_entry *blk_cache_lookup(uint32_t dev_id, uint64_t lba);
int blk_cache_flush_all(void);

/* ============================================================
   RAM Block Driver Interface
   ============================================================ */

struct ramblk_device;

int ramblk_init(void);
struct ramblk_device *ramblk_get_device(void);
void ramblk_reset(void);

#endif /* MCSOS_BLOCK_H */

/* Block layer initialization */
void blk_init(void);

