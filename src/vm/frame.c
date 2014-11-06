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
#include "kernel/hash.h"

//#include "kernel/hash.h"

//struct frame *frame_table[1024];	//change to max of pallocs
//int frame_index;

//struct list free_list;

struct hash frame_table;


void 
insert_frame (struct frame *f)
{
	hash_insert (&frame_table, &f->hash_elem);

  /* if (frame_index != 1024)
   {
   	 f -> frame_in = frame_index;
	 frame_table[frame_index] = f;
	 frame_index++;
   }
   else
   	ASSERT(0);*/

	// eviction policy comes later implemented
}

void
free_frame (void *addr)
{
	/*int i;
	for (i = 0; i <= 256; i++)
	{
		if (frame_table[i] != NULL)
			if (frame_table[i] -> PA == adr)  //can you compare void pointers
			{
				palloc_free_page(adr);
				frame_table[i] = NULL;
				break;
			}
	}*/

	//go through swap table and check if it is in there
}

void 
init_frame_table (void)  
{
	//list_init(&free_list);
	hash_init (&frame_table, frame_hash, hash_less, NULL);

 // int i;

  //for(i = 0; i < 1024; i++)	//change 256 later
 // {
  //	frame_table[i] = NULL;
  	//struct free_list_loc *loc;
  //	loc->index = i;
  //	list_push_front(&free_list, &loc->free_elem);
 // }
  	
 
 // frame_index = 0;
}

void *
get_new_frame (void * upage)
{ 
	struct frame *f = malloc (sizeof(struct frame));
	//struct page *p = f->page;

	f->page = malloc (sizeof (struct page));
	void * palloc_address = palloc_get_page (PAL_USER);

	if(palloc_address == NULL)
	{
		ASSERT(0);//out of memory
	//	evict_page();
	}

	bool success = pagedir_set_page (thread_current()->pagedir, upage, palloc_address, 1); // should we make writable???   //maps the upage to kpage
	//problem here hanging???

	//ASSERT(success); // assuming that the mapping is successful

	f->PA = palloc_address;

	f->page->dirty = 0;
	f->page->access = 1;
	f->page->in_frame_table = 1;
	f->page->VA = upage;

	// add to page table
	//insert_page (f->page);
	insert_frame (f);

	return f->PA; 

	//return palloc_get_page (PAL_USER);
	 
}

void
evict_page (void)
{

}

bool
hash_less (const struct hash_elem *h1, const struct hash_elem *h2,
           void *aux UNUSED)
{
  const struct frame *f1 = hash_entry (h1, struct frame, hash_elem);
  const struct frame *f2 = hash_entry (h2, struct frame, hash_elem);

  return f1->PA < f2->PA;
}

unsigned
frame_hash (const struct hash_elem *h_elem, void *aux UNUSED)
{
  const struct frame *f = hash_entry (h_elem, struct frame, hash_elem);
  return hash_bytes (&f->PA, sizeof f->PA);
}

