#include "userprog/exception.h"
#include <inttypes.h>
#include <stdio.h>
#include <bitmap.h>
#include "userprog/gdt.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/syscall.h"
#include "kernel/hash.h"
#include "vm/swap.h"
#include "devices/block.h"
#include "vm/frame.h"


struct bitmap *swap_table_map;
int sectors_per_page;
int sectors_in_disk;
struct block *swap; // the swap disk

void 
init_swap_table (void)
{
  // block to insert to
  swap = block_get_role (BLOCK_SWAP);

  // called before OS intitializes swap (just get out of here)
  if(swap == NULL)
    return;

  sectors_in_disk = block_size (swap);
  sectors_per_page = PGSIZE / BLOCK_SECTOR_SIZE;
  int bitmap_entries = sectors_in_disk / sectors_per_page;

  swap_table_map = bitmap_create (bitmap_entries);
}

// write to swap (return false if writes unsuccessfully)
bool
write_swap (struct page * p)
{
  printf("\n\n\nbegin write");
  // look into swap_table_map, starting at position 0, for enough space to fit the page
  int first_index = bitmap_scan_and_flip (swap_table_map, 0, 1, 0);
  // printf("first index: %d\n\n", first_index);
  
  // set the p's starting index
  p->swap_index = first_index;

  // current spot in the buffer to write to
  uint32_t buffer_offset = 0;

  if(swap == NULL)
    return false;

  p->block = swap;
  p->in_frame_table = 0;

  // get physical address of the page
  void * buffer = p->frame_ptr->PA;
  // printf("buffer address: %p\n\n\n", buffer);

  // get each sector in the block
  uint32_t sector_index = first_index * sectors_per_page; // get the start index
  uint32_t limit = sector_index + sectors_per_page;
  for(; sector_index < limit; sector_index++)
  {
    // write to the block
    block_write (swap, sector_index, buffer+buffer_offset);

    // shift the buffer for the next sector
    buffer_offset += BLOCK_SECTOR_SIZE; 
    // printf("buffer offset+buffer address: %p\n\n\n", buffer_offset + buffer);
  }
  // printf("\n\ngot to write swap\n\n");
  return true;
}

// read from the swap into the page
void 
read_swap (struct page * p)
{ 
  // printf("started to read\n\n\n");
  uint32_t buffer_offset = 0;
  uint32_t bitmap_index = p-> swap_index;
    
  // get physical address of the page
  void * buffer = p->frame_ptr->PA;

 // get each sector in the block
  uint32_t sector_index = bitmap_index * sectors_per_page; // get the start index
  uint32_t limit = sector_index + sectors_per_page;
  for(; sector_index < limit; sector_index++)
  {
    // printf("started to loop read %d: \n\n", sector_index);
    // write to the block
    block_read (p->block, sector_index, buffer+buffer_offset);

    // shift the buffer for the next sector
    buffer_offset += BLOCK_SECTOR_SIZE;
  }
  // printf("buffer offset: %d\n\n\n\n", buffer_offset); // always 4K
  delete_swap (p);
}

// delete from swap
void delete_swap (struct page * p)
{
  // printf("bitmap size: %d\n", bitmap_size(swap_table_map));
  // printf("p->swap_index: %d\n\n", p->swap_index);
  bitmap_set (swap_table_map, p->swap_index, 0); 
}









/*


// delete
void remove_swap (struct page *p)
{
  hash_delete (&swap_table, &p->swap_elem);
}

// insert
void insert_swap (struct page *p)
{


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

*/