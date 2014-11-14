#include "userprog/exception.h"
#include <inttypes.h>
#include <stdio.h>
#include "userprog/gdt.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/syscall.h"
#include "kernel/hash.h"
#include "vm/swap.h"

void 
init_swap_table (void)
{
	hash_init (&swap_table, swap_hash, hash_swap_less, NULL);
}

// delete
void remove_swap (struct page *p)
{
	hash_delete (&swap_table, &p->swap_elem);
}

// insert
void insert_swap (struct page *p)
{
	p -> in_frame_table = 0; 
	hash_insert (&swap_table, &p->swap_elem);
}

bool
hash_swap_less (const struct hash_elem *h1, const struct hash_elem *h2,
           void *aux UNUSED)
{
  const struct page *p1 = hash_entry (h1, struct page, swap_elem);
  const struct page *p2 = hash_entry (h2, struct page, swap_elem);

  return p1->VA < p2->VA;
}

unsigned
swap_hash (const struct hash_elem *s_elem, void *aux UNUSED)
{
  const struct page *p = hash_entry (s_elem, struct page, swap_elem);
  return hash_bytes (&p->VA, sizeof p->VA);
}

