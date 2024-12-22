#ifndef PAGE_H
#define PAGE_H

#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "lib/kernel/list.h"
#include "lib/kernel/hash.h"
#include "filesys/file.h"
#include "userprog/syscall.h"

#define VM_BINARY 0  //바이너리 파일
#define VM_FILE 1 //매핑된 파일
#define VM_ANONYMOUS 2 //스왑영역

struct vm_entry {
	uint8_t type;
	void *virtual_address;
	bool can_write, is_loaded;
	struct file* file;

	size_t offset, read_bytes, zero_bytes, swap_slot;

	struct hash_elem elem;
};

struct page {
	void *kernel_address;
	struct vm_entry *vme;
	struct thread *t;
	struct list_elem lru;
};

void vm_init(struct hash *vm);
bool load_file(void* kernel_address, struct vm_entry *vme);
static unsigned vm_hash_function(const struct hash_elem *hash_e, void *aux);
static bool vm_cmp(const struct hash_elem *p, const struct hash_elem *q);
bool insert_vme(struct hash *vm, struct vm_entry *vme);
bool delete_vme(struct hash *vm, struct vm_entry *vme);
struct vm_entry *get_vme(void *virtual_address);

#endif