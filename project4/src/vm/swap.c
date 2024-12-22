#include "swap.h"

//block: 4096
//page: 512
//sector_num = 8
//block_sector_size = 512 // block.h
//8MB = 2^13 = 1<<13

void
swap_initialize(void) {
	swap_block = block_get_role(BLOCK_SWAP);
	if (!swap_block) return;
	swap_bitmap = bitmap_create(EIGHT_MB);
	if(swap_bitmap) bitmap_set_all(swap_bitmap, 0);
}

void
swap_in(size_t idx, void *kernel_address)
{
	if(bitmap_test(swap_bitmap, idx)) {
		for (int i = 0; i < SECTOR_CNT; i++) block_read(swap_block, SECTOR_CNT * idx + i, (uint8_t *)kernel_address + i * BLOCK_SECTOR_SIZE);
		bitmap_flip(swap_bitmap, idx);
	}
}

size_t
swap_out(void *kernel_address)
{
	size_t empty_idx = bitmap_scan_and_flip(swap_bitmap, 0, 1, 0);
	if (empty_idx != BITMAP_ERROR) {
		for (int i = 0; i < SECTOR_CNT; i++) block_write(swap_block, SECTOR_CNT * empty_idx + i, (uint8_t *)kernel_address + i * BLOCK_SECTOR_SIZE);
	}
	return empty_idx;
}