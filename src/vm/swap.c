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
//int BLOCKS_PER_PAGE;
int SECTOR_SIZE;

// assuming arbitrary 2GB swap disk
uint64_t HARD_DISK = 536870912; // in bytes

// size relationships
// page
  // block
      // sectors
          // bits/bytes

void 
init_swap_table (void)
{
  // block to insert to
  struct block *insertion_block = block_get_role (BLOCK_SWAP);

  // called before OS intitializes swap (just get out of here)
  if(insertion_block == NULL)
    return;

  // get the number of blocks needed for each page
 // BLOCKS_PER_PAGE = PGSIZE/BLOCK_SECTOR_SIZE;

  // get the size of each sector
  // SECTOR_SIZE = BLOCK_SECTOR_SIZE/

  // intialize bitmap where each bit represents a page
  swap_table_map = bitmap_create (HARD_DISK/PGSIZE);
}



// write to swap (return false if writes unsuccessfully)
bool
write_swap (struct page * p)
{
  // look into swap_table_map, starting at position 0, for enough space to fit the page
  int first_index = bitmap_scan_and_flip (swap_table_map, 0, 1, 0);
  
  // set the p's starting index
  p->start_swap_index = first_index;

  // current spot in the buffer to write to
  uint32_t buffer_offset = 0;

  // get all the blocks to write to
  /*int block_index;
  for(block_index=0; block_index<BLOCKS_PER_PAGE; block_index++)
  {*/
    // get a block
  struct block *insertion_block = block_get_role (BLOCK_SWAP);
  if(insertion_block == NULL)
    return false;

  p->block = insertion_block;
    



  printf("\n\ngot to write swap\n\n");
   printf("Block Size: %d", block_size (insertion_block));
   // printf("\n\ngot to write swap\n\n");
    // printf("\n\ngot to write swap\n\n");


    // get each sector in the block
    uint32_t sector_index;
    for(sector_index = 0; sector_index<block_size (insertion_block); sector_index++)
    {
      // get physical address of the page
      void * buffer = p->frame_ptr->PA;

      // write to the block
      block_write (insertion_block, sector_index, buffer+buffer_offset);

      // shift the buffer for the next sector
       buffer_offset += BLOCK_SECTOR_SIZE;
    }



 // }
}

void 
read_swap (struct page * p)
{
  uint32_t buffer_offset = 0;
  int bitmap_index = p-> start_swap_index;

   uint32_t sector_index;
  for(sector_index = 0; sector_index<block_size (p->block); sector_index++)
  {
    // get physical address of the page
    void * buffer = p->frame_ptr->PA;

    // write to the block
    block_read (p->block, sector_index, buffer+buffer_offset);

    // shift the buffer for the next sector
     buffer_offset += BLOCK_SECTOR_SIZE;
  }
  delete_swap(p);
}

void delete_swap (struct page * p)
{
  bitmap_set(swap_table_map, p->start_swap_index, 0); 
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