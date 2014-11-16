#ifndef PAGE
#define PAGE

#include <debug.h>
#include <list.h>
#include <stdint.h>
#include "kernel/hash.h"

struct page
{
	void *VA;  // where address is in virtual memory

	int dirty;  
	int present;  // page has been previously loaded in memory
	int in_frame_table;  // if in
	struct frame *frame_ptr;
	struct hash_elem page_elem;
	struct hash_elem swap_elem;

	void *block;

	struct file *file;
	int ofs;

	int page_read_bytes;
	int page_zero_bytes;
	bool writable;

	int swap_index; // index in swap it is stored
};
#endif

void init_page_table (void);
void insert_page (struct page *p);

bool
hash_page_less (const struct hash_elem *h1, const struct hash_elem *h2,
           void *aux);

unsigned page_hash (const struct hash_elem *p_elem, void *aux);

void remove_page (struct page *p);
