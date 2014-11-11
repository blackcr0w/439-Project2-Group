#include "threads/thread.h"
#include "threads/flags.h"
#include "threads/interrupt.h"
#include "threads/intr-stubs.h"
#include "threads/palloc.h"
#include "threads/switch.h"
#include "threads/synch.h"
#include "threads/vaddr.h"


#include "vm/page.h"

void 
init_page_table (void)
{

	hash_init (&thread_current()->page_table, page_hash, hash_page_less, NULL);
}

void 
insert_page (struct page *p)
{
	hash_insert (&thread_current()->page_table, &p->page_elem);
}

void remove_page (struct page *p)
{
	hash_delete (&thread_current()->page_table, &p->page_elem);
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
