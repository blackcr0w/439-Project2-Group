#include "threads/thread.h"
#include "threads/flags.h"
#include "threads/interrupt.h"
#include "threads/intr-stubs.h"
#include "threads/palloc.h"
#include "threads/switch.h"
#include "threads/synch.h"
#include "threads/vaddr.h"


#include "vm/page.h"


//struct page *page_table[256];
//int page_index;
//struct list page_table;


void 
init_page_table (void)
{
	hash_init (thread_current()->page_table, page_hash, hash_page_less, NULL);
	//page_table[256] = {NULL};
	//page_index = 0;
	//list_init (&page_table);	

}

void 
insert_page (struct page *p)
{
	/*p->dirty = 0;
	p->access = 1;
	p->in_frame_table = 1;*/ //maybe do it for swapping

	//list_push_back (&page_table, &p -> page_elem);

	/* page_table[page_index] = p;
	 page_index++;*/

}

bool
hash_page_less (const struct hash_elem *h1, const struct hash_elem *h2,
           void *aux UNUSED)
{
  const struct page *p1 = hash_entry (h1, struct page, page_elem);
  const struct page *p2 = hash_entry (h2, struct page, page_elem);

  return p1->VA < p2->VA;
}

unsigned
page_hash (const struct hash_elem *p_elem, void *aux UNUSED)
{
  const struct page *p = hash_entry (p_elem, struct page, page_elem);
  return hash_bytes (&p->VA, sizeof p->VA);
}


