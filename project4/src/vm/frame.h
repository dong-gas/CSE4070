#ifndef FRAME_H
#define FRAME_H

#include "lib/kernel/list.h"
#include "page.h"
#include "threads/palloc.h"

struct list lru_list;
struct page *victim_pointer;

void lru_initialize(void);
void remove_page(struct page *page);
struct page *allocate_page(enum palloc_flags flags);
void free_page(void *kernel_address);

#endif