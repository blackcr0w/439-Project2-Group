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
#include "devices/block.h"


struct bitmap *swap_table_map;

// size relationships
// page
  // block
      // sectors
          // bits/bytes

void 
init_swap_table (void)
{
//  hash_init (&swap_table, swap_hash, hash_swap_less, NULL);
  struct block *insertion_block = block_get_role(BLOCK_SWAP);

  // called before OS intitializes swap (just get out of here)
  if(insertion_block == NULL)
    return;

  // initialize the bitmap to number of needed bits
  swap_block_sectors = insertion_block->size;


  // get the number of sectors that make up a block
  int sectors_per_block = insertion_block->size;

  // intialize bitmap where each bit represents a sector
  bitmap_create (sectors_per_block * BLOCK_SECTOR_SIZE / PGSIZE);

  // each bit in the bitmap should represent one page



  // 8 blocks per page *
  // sectors that make up a block
  // = number of necessary sectors per page
}

// delete
void remove_swap (struct page *p)
{
	hash_delete (&swap_table, &p->swap_elem);
}

// insert
void insert_swap (struct page *p)
{

  // look into swap_table_map, starting at position 0, for enough space to fit the page
  bitmap_scan_and_flip (swap_table_map, 0, 1, 0)

  // printf ("GOT TO INSERT SWAP");
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

