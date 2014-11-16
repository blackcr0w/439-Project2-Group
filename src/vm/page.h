#ifndef PAGE
#define PAGE

#include <debug.h>
#include <list.h>
#include <stdint.h>
#include "kernel/hash.h"

struct page
{
	void *VA;  // where address is in virtual memory

	int dirty;  // if it has been changed
	int present;  // page has been previously loaded in memory
	int in_frame_table;  // if in the frame table
	struct frame *frame_ptr; // pointer to related from (for mapping)
	struct hash_elem page_elem; // used for the supplemntal page table

	struct file *file; // info to read into memory (for loading)
	int ofs; // offset

	int page_read_bytes; // used for reading
	int page_zero_bytes; // used for reading
	bool writable; // dictates if it is read only or can be written to

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
