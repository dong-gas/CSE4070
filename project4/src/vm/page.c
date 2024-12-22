#include "page.h"

void vm_init(struct hash *vm) {
    hash_init(&thread_current()->vm, vm_hash_function, vm_cmp, NULL);
}

bool load_file(void *kernel_address, struct vm_entry *vme) {
    if ((int) vme->read_bytes != file_read_at(vme->file, kernel_address, vme->read_bytes, vme->offset)) return false;
    memset(vme->read_bytes + kernel_address, 0, vme->zero_bytes);
    return true;
}

static unsigned
vm_hash_function(const struct hash_elem *hash_e, void *aux) {
    return hash_int((int) hash_entry(hash_e, struct vm_entry, elem)->virtual_address);
}

static bool
vm_cmp(const struct hash_elem *p, const struct hash_elem *q) {
    return hash_entry(p, struct vm_entry, elem)->virtual_address < hash_entry(q, struct vm_entry, elem)->virtual_address;
}

bool insert_vme(struct hash *vm, struct vm_entry *vme) {
    return (hash_insert(vm, &vme->elem) == NULL);
}

struct vm_entry *
get_vme(void *virtual_address) {
    struct vm_entry vme;
    vme.virtual_address = pg_round_down(virtual_address);
    struct hash_elem *e = hash_find(&thread_current()->vm, &vme.elem);

    if (e) return hash_entry(e, struct vm_entry, elem);
    return e;
}