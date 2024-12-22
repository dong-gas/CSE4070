#include "frame.h"

void
lru_initialize(void)
{
	list_init(&lru_list), victim_pointer = NULL;
}

void
remove_page(struct page *pg)
{
	if (!pg || victim_pointer != pg) return;

	//victim pointer 다음 거로 변경
	victim_pointer = list_entry(list_remove(&pg->lru), struct page, lru);
}

static struct list_elem *
get_next_victim_pointer_elem()
{
	if(!list_size(&lru_list)) return NULL; // 비어있는 경우 예외처리
	

	if (!victim_pointer) { //끝인 경우 begin으로 예외처리
		victim_pointer = list_entry(list_begin(&lru_list), struct page, lru);
		return list_begin(&lru_list);
	} 

	// 바로 다음 거로 넘기기
	struct list_elem *el = list_next(&victim_pointer->lru);
	if (el == list_end(&lru_list)) { 
		if (&victim_pointer->lru == list_begin(&victim_pointer)) return NULL; // 1개 뿐
		el = list_begin(&lru_list);
	}
	victim_pointer = list_entry(el, struct page, lru);
	return el;
}

static void
try_to_free_pages(enum palloc_flags flags)
{
	if (list_empty(&lru_list)) return;

	struct list_elem *e = get_next_victim_pointer_elem();
	if(!e) return;
	do { // second chance algorithm
		struct page* now = list_entry(e, struct page, lru);
		if (!pagedir_is_accessed(now->t->pagedir, now->vme->virtual_address)) { //accessed가 0이면 evict
			if (pagedir_is_dirty(now->t->pagedir, now->vme->virtual_address) || now->vme->type == VM_ANONYMOUS) {
				if (now->vme->type == VM_FILE) {
					lock_acquire(&filesys_lock);
					file_write_at(now->vme->file, now->kernel_address, now->vme->read_bytes, now->vme->offset);
					lock_release(&filesys_lock);
				} 
				else {
					now->vme->type = VM_ANONYMOUS;
					now->vme->swap_slot = swap_out(now->kernel_address);
				}
			}
			now->vme->is_loaded = false;
			pagedir_clear_page(now->t->pagedir, now->vme->virtual_address);

			remove_page(now);
			palloc_free_page(now->kernel_address);
			free(now);
			break;
		} 
		// 0으로 바꾸고 한 번 더 기회
		pagedir_set_accessed(now->t->pagedir, now->vme->virtual_address, false);
		e = get_next_victim_pointer_elem();
	} while(e);
}

struct page *
allocate_page(enum palloc_flags flag)
{
	void *kernel_address = NULL;

	//PAL_USER = 4 (= 100 (2))
	if ((flag & PAL_USER) == 0) return NULL;

	do {
		kernel_address = palloc_get_page(flag);
		if (!kernel_address) try_to_free_pages(PAL_USER);
	} while(!kernel_address);

	struct page *pg = malloc(sizeof(struct page));
	if (!pg) {
		palloc_free_page(kernel_address);
		return NULL;
	}

	pg->kernel_address = kernel_address;
	pg->vme = NULL;
	pg->t = thread_current();

	if (pg) list_push_back(&lru_list, &pg->lru);

	return pg;
}

void
free_page(void *kernel_address)
{
	for (struct list_elem* elem = list_begin(&lru_list); elem != list_end(&lru_list); elem = list_next(elem)) {
		struct page *pg = list_entry(elem, struct page, lru);
		if(pg->kernel_address != kernel_address) continue;
		remove_page(pg);
		palloc_free_page(pg->kernel_address);
		free(pg);
		return;
	}
}