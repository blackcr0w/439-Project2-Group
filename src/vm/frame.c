#include "threads/thread.h"
#include "threads/flags.h"
#include "threads/interrupt.h"
#include "threads/intr-stubs.h"
#include "threads/palloc.h"
#include "threads/switch.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include "userprog/process.h"
#include "vm/frame.h"
#include "threads/malloc.h"


struct frame *frame_table[256];	//change to max of pallocs
int frame_index;

//void insert_frame (struct frame *f)


void 
insert_frame (struct frame *f)
{
   if (frame_index != 256)
   {
   	 f -> frame_in = frame_index;
	 frame_table[frame_index] = f;
	 frame_index++;
   }
   else
   	ASSERT(0);

	// eviction policy comes later implemented
}

void
free_frame (void *adr)
{
	int i;
	for (i = 0; i <= 256; i++)
	{
		if (frame_table[i] != NULL)
			if (frame_table[i] -> page -> VA == adr)  //can you compare void pointers
			{
				palloc_free_page(adr);
				frame_table[i] = NULL;
				break;
			}
	}

	//go through swap table and check if it is in there
}

void 
init_frame_table (void)  
{
  int i;
  for(i = 0; i < 256; i++)	//change 256 later
  	frame_table[i] = NULL;
  ///frame_table[256] = {NULL};
  frame_index = 0;
}

void *
get_new_frame (void)
{ 
	struct frame *f = malloc (sizeof(struct frame));
	//struct page *p = f->page;

	f->page = malloc (sizeof (struct page));
	f->page->VA = palloc_get_page (PAL_USER);

	f->page->dirty = 0;
	f->page->access = 1;
	f->page->in_frame_table = 1;

	// add to page table
	insert_page (f->page);
	insert_frame (f);

	return f->page->VA;

	//return palloc_get_page (PAL_USER);
	 
}

void
evict_page (void)
{

}
