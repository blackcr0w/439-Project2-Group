#ifndef PAGE
#define PAGE

#include <debug.h>
#include <list.h>
#include <stdint.h>
#include "kernel/hash.h"
// #include "vm/frame.h"




struct page
{
	void *VA; //where address is in virtual memory
	//struct hash_elem hash_elem;  //used to put in hash element
	int dirty;
	int present;
	int in_frame_table;
	struct frame *frame_ptr;
	struct hash_elem page_elem;
	struct hash_elem swap_elem;

	struct file *file;
	int ofs;
	// int *upage;
	int page_read_bytes;
	int page_zero_bytes;
	bool writable;
};
#endif

void init_page_table (void);
void insert_page (struct page *p);

bool
hash_page_less (const struct hash_elem *h1, const struct hash_elem *h2,
           void *aux);

unsigned page_hash (const struct hash_elem *p_elem, void *aux);

void remove_page (struct page *p);
