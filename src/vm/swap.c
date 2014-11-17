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


struct bitmap *swap_table_map;  // represents the free spots in swap
int sectors_per_page;           // number of sectors needed for each page
int sectors_in_disk;            // number of sectors in disk
struct block *swap;             // the swap

// Spencer and Jeff driving
// intialize the swap table and global variables
void 
init_swap_table (void)
{
  // block to insert to
  swap = block_get_role (BLOCK_SWAP);

  // called before OS intitializes swap (just get out of here)
  if(swap == NULL)
    return;

  // set the global variables
  sectors_in_disk = block_size (swap);
  sectors_per_page = PGSIZE / BLOCK_SECTOR_SIZE;
  int bitmap_entries = sectors_in_disk / sectors_per_page;

  // intialize the bitmap (to false)
  swap_table_map = bitmap_create (bitmap_entries);
}

// Spencer and Dakota driving
// write to swap (return false on successful write)
bool
insert_swap (struct page * p)
{
  // look into swap_table_map, starting at position 0, for enough space to fit the page
  int first_index = bitmap_scan_and_flip (swap_table_map, 0, 1, 0);
  
  // make sure disk has been initialized
  if(swap == NULL)
    return false;

  
  p->swap_index = first_index;  // set the p's starting index
  uint32_t buffer_offset = 0;   // current spot in the buffer to write to
  p->in_frame_table = 0;        // page is no longer in frame table

  // get each sector needed to write to
  void * buffer = p->frame_ptr->PA; // get physical address of given page
  uint32_t sector_index = first_index * sectors_per_page; // get start index
  uint32_t limit = sector_index + sectors_per_page;
  for(; sector_index < limit; sector_index++)
  {
    block_write (swap, sector_index, buffer+buffer_offset); // write to the block
    buffer_offset += BLOCK_SECTOR_SIZE; // shift the buffer for the next sector
  }
  return true; // successfully wrote
}

// Cohen driving
// read from the swap into the page
void 
remove_swap (struct page * p)
{
  // get index to swap into
  uint32_t bitmap_index = p-> swap_index;
  ASSERT (bitmap_index!=-2)  // should be reset, never -2
  
  uint32_t buffer_offset = 0;

  // get physical address of the page
  void * buffer = p->frame_ptr->PA;

  // get each sector in the block
  uint32_t sector_index = bitmap_index * sectors_per_page; // get the start index
  uint32_t limit = sector_index + sectors_per_page;
  for(; sector_index < limit; sector_index++)
  {
    // write to the block
    block_read (swap, sector_index, buffer+buffer_offset);

    // shift the buffer for the next sector
    buffer_offset += BLOCK_SECTOR_SIZE;
  }
  delete_swap (p);
}

// Dakota driving
// delete from swap
void delete_swap (struct page * p)
{
  bitmap_set (swap_table_map, p->swap_index, 0);
  p->swap_index = -2; // should always be overwritten (error checking)
}
