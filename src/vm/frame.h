/*#include <debug.h>
#include <list.h>
#include <stdint.h>*/

#include "vm/page.h"
#include "kernel/hash.h"
#include "threads/synch.h"

struct frame
{
	void * PA;	// physcial address

	struct page *page_ptr;

	struct hash_elem hash_elem;  // used to put in hash element

	struct semaphore sema_evict; // manages synchronization in evicting frame

};

void insert_frame (struct frame *f);

void free_frame (void *adr);

void init_frame_table (void);

void * get_new_frame (struct page *p);

void evict_page (void);

bool hash_less (const struct hash_elem *a, const struct hash_elem *b,
           void *aux);

unsigned
frame_hash (const struct hash_elem *h_elem, void *aux);

void remove_frame (struct frame *f);
