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
   Block Device Registry
   ============================================================ */

static struct block_device *block_device_table[MCSOS_MAX_BLOCK_DEVICES];
static int num_registered_devices = 0;

int blk_register_device(struct block_device *dev)
{
    if (!dev || !dev->ops) {
        return BLK_ERR_INVALID_DEV;
    }
    
    if (num_registered_devices >= MCSOS_MAX_BLOCK_DEVICES) {
        return BLK_ERR_INVALID_DEV;
    }
    
    dev->dev_id = num_registered_devices;
    block_device_table[num_registered_devices] = dev;
    num_registered_devices++;
    
    return BLK_OK;
}

struct block_device *blk_get_device(uint32_t dev_id)
{
    if (dev_id >= MCSOS_MAX_BLOCK_DEVICES) {
        return NULL;
    }
    return block_device_table[dev_id];
}

/* ============================================================
   Buffer Cache - Simple 1 entry per LBA model
   ============================================================ */

static struct buffer_cache_entry buffer_cache[MCSOS_BUFFER_CACHE_ENTRIES];

/* Initialize buffer cache */
static void blk_cache_init(void)
{
    memset(buffer_cache, 0, sizeof(buffer_cache));
}

struct buffer_cache_entry *blk_cache_lookup(uint32_t dev_id, uint64_t lba)
{
    for (int i = 0; i < MCSOS_BUFFER_CACHE_ENTRIES; i++) {
        if (buffer_cache[i].valid && 
            buffer_cache[i].dev_id == dev_id && 
            buffer_cache[i].lba == lba) {
            return &buffer_cache[i];
        }
    }
    return NULL;
}

static struct buffer_cache_entry *blk_cache_find_empty(void)
{
    for (int i = 0; i < MCSOS_BUFFER_CACHE_ENTRIES; i++) {
        if (!buffer_cache[i].valid) {
            return &buffer_cache[i];
        }
    }
    return NULL;
}

int blk_cache_flush_all(void)
{
    int result = BLK_OK;
    
    for (int i = 0; i < MCSOS_BUFFER_CACHE_ENTRIES; i++) {
        if (buffer_cache[i].valid && buffer_cache[i].dirty) {
            struct block_device *dev = blk_get_device(buffer_cache[i].dev_id);
            if (!dev || !dev->ops || !dev->ops->write) {
                result = BLK_ERR_DRIVER_FAIL;
                continue;
            }
            
            int err = dev->ops->write(buffer_cache[i].lba, buffer_cache[i].data);
            if (err != BLK_OK) {
                result = BLK_ERR_DRIVER_FAIL;
                continue;
            }
            
            buffer_cache[i].dirty = 0;
        }
    }
    
    return result;
}

/* ============================================================
   Block I/O API with Buffer Cache
   ============================================================ */

int blk_read(uint32_t dev_id, uint64_t lba, uint8_t *buf)
{
    if (!buf) {
        return BLK_ERR_INVALID_LBA;
    }
    
    struct block_device *dev = blk_get_device(dev_id);
    if (!dev) {
        return BLK_ERR_INVALID_DEV;
    }
    
    if (lba >= dev->block_count) {
        return BLK_ERR_INVALID_LBA;
    }
    
    /* Check cache first */
    struct buffer_cache_entry *cached = blk_cache_lookup(dev_id, lba);
    if (cached && cached->valid) {
        memcpy(buf, cached->data, MCSOS_BLOCK_SIZE);
        return BLK_OK;
    }
    
    /* Cache miss - read from driver */
    if (!dev->ops || !dev->ops->read) {
        return BLK_ERR_DRIVER_FAIL;
    }
    
    int err = dev->ops->read(lba, buf);
    if (err != BLK_OK) {
        return err;
    }
    
    /* Fill cache entry */
    struct buffer_cache_entry *entry = blk_cache_find_empty();
    if (entry) {
        entry->dev_id = dev_id;
        entry->lba = lba;
        entry->valid = 1;
        entry->dirty = 0;
        memcpy(entry->data, buf, MCSOS_BLOCK_SIZE);
    }
    
    return BLK_OK;
}

int blk_write(uint32_t dev_id, uint64_t lba, const uint8_t *buf)
{
    if (!buf) {
        return BLK_ERR_INVALID_LBA;
    }
    
    struct block_device *dev = blk_get_device(dev_id);
    if (!dev) {
        return BLK_ERR_INVALID_DEV;
    }
    
    if (lba >= dev->block_count) {
        return BLK_ERR_INVALID_LBA;
    }
    
    /* Write-back: update cache, mark dirty */
    struct buffer_cache_entry *cached = blk_cache_lookup(dev_id, lba);
    if (!cached) {
        cached = blk_cache_find_empty();
    }
    
    if (cached) {
        cached->dev_id = dev_id;
        cached->lba = lba;
        cached->valid = 1;
        cached->dirty = 1;
        memcpy(cached->data, buf, MCSOS_BLOCK_SIZE);
    }
    
    return BLK_OK;
}

int blk_flush(uint32_t dev_id)
{
    struct block_device *dev = blk_get_device(dev_id);
    if (!dev) {
        return BLK_ERR_INVALID_DEV;
    }
    
    if (!dev->ops || !dev->ops->flush) {
        return BLK_ERR_DRIVER_FAIL;
    }
    
    /* Flush dirty entries for this device */
    for (int i = 0; i < MCSOS_BUFFER_CACHE_ENTRIES; i++) {
        if (buffer_cache[i].valid && 
            buffer_cache[i].dev_id == dev_id && 
            buffer_cache[i].dirty) {
            
            int err = dev->ops->write(buffer_cache[i].lba, buffer_cache[i].data);
            if (err != BLK_OK) {
                return BLK_ERR_DRIVER_FAIL;
            }
            buffer_cache[i].dirty = 0;
        }
    }
    
    return dev->ops->flush();
}

/* Initialize block layer */
void blk_init(void)
{
    blk_cache_init();
    num_registered_devices = 0;
}
