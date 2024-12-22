#ifndef SWAP_H
#define SWAP_H
#include "lib/kernel/bitmap.h"
#include "devices/block.h"

#define SECTOR_CNT 8
#define EIGHT_MB (1 << 13)

struct bitmap *swap_bitmap;
struct block *swap_block;

void swap_initialize(void);
void swap_in(size_t idx, void *kernel_address);
size_t swap_out(void *kernel_address);

#endif