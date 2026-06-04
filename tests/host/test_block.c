#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../../include/mcsos/block.h"

/* Forward declare implementation */
extern int blk_register_device(struct block_device *dev);
extern struct block_device *blk_get_device(uint32_t dev_id);
extern int blk_read(uint32_t dev_id, uint64_t lba, uint8_t *buf);
extern int blk_write(uint32_t dev_id, uint64_t lba, const uint8_t *buf);
extern int blk_flush(uint32_t dev_id);
extern struct buffer_cache_entry *blk_cache_lookup(uint32_t dev_id, uint64_t lba);
extern int blk_cache_flush_all(void);
extern int ramblk_init(void);
extern void ramblk_reset(void);

#define TEST_PASS(msg) printf("[PASS] %s\n", msg)
#define TEST_FAIL(msg) printf("[FAIL] %s\n", msg); exit(1)

void test_ramblk_init(void)
{
    ramblk_reset();
    int err = ramblk_init();
    assert(err == BLK_OK);
    
    struct block_device *dev = blk_get_device(0);
    assert(dev != NULL);
    assert(dev->block_count == 4096);
    assert(dev->block_size == MCSOS_BLOCK_SIZE);
    
    TEST_PASS("ramblk_init");
}

void test_blk_read_write(void)
{
    ramblk_reset();
    ramblk_init();
    
    uint8_t write_buf[512];
    uint8_t read_buf[512];
    
    /* Initialize write buffer with pattern */
    for (int i = 0; i < 512; i++) {
        write_buf[i] = (i % 256);
    }
    
    /* Write to LBA 0 */
    int err = blk_write(0, 0, write_buf);
    assert(err == BLK_OK);
    
    /* Read from LBA 0 */
    memset(read_buf, 0, 512);
    err = blk_read(0, 0, read_buf);
    assert(err == BLK_OK);
    
    /* Verify data matches */
    assert(memcmp(write_buf, read_buf, 512) == 0);
    
    TEST_PASS("blk_read_write");
}

void test_blk_cache_hit(void)
{
    ramblk_reset();
    ramblk_init();
    
    uint8_t write_buf[512];
    uint8_t read_buf1[512];
    uint8_t read_buf2[512];
    
    for (int i = 0; i < 512; i++) {
        write_buf[i] = 0xAA;
    }
    
    /* Write and read first time (cache miss) */
    blk_write(0, 5, write_buf);
    blk_read(0, 5, read_buf1);
    
    /* Modify write_buf in memory */
    write_buf[0] = 0xBB;
    
    /* Read again - should get from cache, not modified */
    blk_read(0, 5, read_buf2);
    assert(read_buf2[0] == 0xAA);
    
    TEST_PASS("blk_cache_hit");
}

void test_blk_invalid_lba(void)
{
    ramblk_reset();
    ramblk_init();
    
    uint8_t buf[512];
    
    /* Try to read beyond device size */
    int err = blk_read(0, 5000, buf);
    assert(err == BLK_ERR_INVALID_LBA);
    
    /* Try to write beyond device size */
    err = blk_write(0, 5000, buf);
    assert(err == BLK_ERR_INVALID_LBA);
    
    TEST_PASS("blk_invalid_lba");
}

void test_blk_invalid_device(void)
{
    ramblk_reset();
    ramblk_init();
    
    uint8_t buf[512];
    
    /* Try to access invalid device */
    int err = blk_read(99, 0, buf);
    assert(err == BLK_ERR_INVALID_DEV);
    
    TEST_PASS("blk_invalid_device");
}

void test_blk_dirty_and_flush(void)
{
    ramblk_reset();
    ramblk_init();
    
    uint8_t buf[512];
    memset(buf, 0x42, 512);
    
    /* Write to cache (marks dirty) */
    int err = blk_write(0, 10, buf);
    assert(err == BLK_OK);
    
    /* Look up cache entry and verify it's dirty */
    struct buffer_cache_entry *entry = blk_cache_lookup(0, 10);
    assert(entry != NULL);
    assert(entry->dirty == 1);
    
    /* Flush device */
    err = blk_flush(0);
    assert(err == BLK_OK);
    
    /* After flush, dirty bit should be cleared */
    entry = blk_cache_lookup(0, 10);
    assert(entry != NULL);
    assert(entry->dirty == 0);
    
    TEST_PASS("blk_dirty_and_flush");
}

int main(void)
{
    printf("=== Block Device Layer Unit Tests ===\n");
    
    test_ramblk_init();
    test_blk_read_write();
    test_blk_cache_hit();
    test_blk_invalid_lba();
    test_blk_invalid_device();
    test_blk_dirty_and_flush();
    
    printf("\n=== All tests PASSED ===\n");
    return 0;
}
