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
#include "vm/swap.h"

//#include "kernel/hash.h"

//struct frame *frame_table[1024];	//change to max of pallocs
//int frame_index;

//struct list free_list;

struct hash frame_table;
struct hash_iterator i;

void 
insert_frame (struct frame *f)
{
	hash_insert (&frame_table, &f->hash_elem);


   //ASSERT(0);

	// eviction policy comes later implemented
}

void 
init_frame_table (void)  
{
	hash_init (&frame_table, frame_hash, hash_less, NULL);
    hash_first (&i, &frame_table);
}

void *
get_new_frame (struct page *p)  // pass in the page so that you can access it
{ 
	ASSERT(p!=NULL);
	/*if(p==NULL)
	{	
		// because of this kpage = get_new_frame (NULL); in setup stack

		// whatever we need to do!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		return NULL;
	}*/

	// make a new frame
	struct frame *f = malloc (sizeof (struct frame));

	// map that frame to the given page
	f->page_ptr = p;

	void * palloc_address = palloc_get_page (PAL_USER);

	// if memory is full
	if(palloc_address == NULL)
	{
		evict_page (); // make some memory available
		f->PA = palloc_get_page (PAL_USER); // 
	}

	// if memory is not full
	else
		f->PA = palloc_address;	

	insert_frame (f);

	return f->PA; 
}

void
evict_page ()
{
	if (hash_next (&i) != NULL)
	{
	  struct frame *f = hash_entry (hash_cur (&i), struct frame, hash_elem);
	  insert_swap (f->page_ptr);  // set bits 																// PUT INTO SWAP TABLE.		NEED TO FIND THE PAGE TO PUT INTO SWAP.
	  remove_frame (f);
	  palloc_free_page (f->PA);
	}

	// if it is at the end then go back to beginning
	else
		hash_first (&i, &frame_table);

}

void remove_frame (struct frame *f)
{
	hash_delete (&frame_table, &f->hash_elem);
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
