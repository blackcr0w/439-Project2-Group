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
#include <stdio.h>

// struct hash frame_table;
// struct hash_iterator i;
struct list frame_table_list; // the list for the frame table
void *curr_ptr; // where we are currently looking at (frame table)

void 
insert_frame (struct frame *f)
{	
	// hash_insert (&frame_table, &f->hash_elem); // hash
	list_push_back (&frame_table_list, &f->frame_elem);
}

void 
init_frame_table (void)  
{
	// hash_init (&frame_table, frame_hash, hash_less, NULL); // hash
    // hash_first (&i, &frame_table); // hash
    list_init (&frame_table_list);
    curr_ptr = NULL;

}

void *
get_new_frame (struct page *p)  // pass in the page so that you can access it
{ 
	ASSERT(p!=NULL);	

	// make a new frame
	struct frame *f = malloc (sizeof (struct frame));
	sema_init (&f->sema_evict, 1);
  
	// map that frame to the given page

	p -> frame_ptr = f;

	void * palloc_address = palloc_get_page (PAL_USER |  PAL_ZERO);

	// if memory is full
	if (palloc_address == NULL)
	{
		evict_page (); // make some memory available
		f->PA = palloc_get_page (PAL_USER | PAL_ZERO);
	}

	// if memory is not full
	else
		f->PA = palloc_address;	

	f->page_ptr = p;
  	// printf("\nMAPPING FROM GET_NEW_FRAME Pointer to Frame: %p\n", f);
  	 // printf("MAPPING FROM GET_NEW_FRAME Pointer to Page: %p\n", f->page_ptr->VA);

	insert_frame (f);
		// printf("\nhiiiiiiiiiiiiiii\n");

	// struct hash_elem * result = hash_find (&frame_table, &f->hash_elem); 
	// if(result!=NULL)
		// printf("I FOUND %p\n\n\n", f);

	return f->PA; 
}

/*void clear_thread_frames()
{
	struct thread *t = thread_current();
	struct hash_iterator i;

	if(hash_next (&i) != NULL)
	{
		struct page *p = hash_entry (hash_cur (&i), struct page, page_elem);
		palloc_free_page (p->frame_ptr->PA);

		hash_delete (&frame_table, &p->frame_ptr->hash_elem);
		hash_delete (&swap_table, &p->swap_elem);
	}
	// destroy page table
}*/

void
evict_page ()
{ 
	// Clock
	// while we haven't found a page to evict
/*	while (true)
	{
		// list
		// if at the end of the frame table, go to the beginning
		bool b = curr == NULL;
		if(!b)
			curr = list_begin (frame_table_list);

		// get the frame
		// struct frame *f = hash_entry (hash_cur (&i), struct frame, hash_elem); // hash
		struct frame *f = list_entry (&curr, struct frame, frame_elem);

		// check if the page has been accessed
		bool accessed = pagedir_is_accessed 
			(thread_current ()->pagedir, f->page_ptr);

		// if it has been accessed, move on
		if(accessed)
		{
			// set accessed to false
			pagedir_set_accessed 
				(thread_current ()->pagedir, f->page_ptr, false);
		} 

		// if it has not been accessed, evict
		else
		{
	 		// printf("\nEVICT PAGE Pointer to Frame: %p\n", f);
  			// printf("EVICT PAGE Pointer to Page: %p\n", f->page_ptr);


			// prevent multiple evictions
			sema_down (&f->sema_evict);

			// evict curr
			curr = list_next (&curr);

			// check if it's the end of the list
			if(curr != NULL)
				list_remove (list_prev (&curr));
			else
				list_remove (list_back (&frame_table_list));
			
			// if it's dirty, swap
			if(pagedir_is_dirty (thread_current ()->pagedir, f->page_ptr))
				insert_swap (f->page_ptr);

			// printf("I kill myself right here\n\n\n");
			pagedir_clear_page (thread_current () ->pagedir, f->page_ptr->VA);
			// remove frame stuff
			remove_frame (f);
			palloc_free_page (f->PA);
			//	printf("got here\n");
			//	printf("%p\n", f->page_ptr);
			
			// allow other evicitons
			sema_up (&f->sema_evict);

			// get out of the loop
			return;
		}
		curr = list_next (&curr);
	}
 
*/ 
	struct list_elem *front = list_begin (&frame_table_list);

	struct frame *f = list_entry(front, struct frame, frame_elem);

//	sema_down (&f->sema_evict);
	// printf("\nEVICTION Pointer to Frame VA %p\n", f);
  	// printf("EVICTION Pointer to Page VA %p\n", f->page_ptr->VA);

	list_pop_front (&frame_table_list); 

	if(pagedir_is_dirty (thread_current ()->pagedir, f->page_ptr))
	{
		// printf ("\nIS DIRTY!\n");
	  	insert_swap (f->page_ptr);
	  	f->page_ptr->present = 1;
	}
	else
		f->page_ptr->present = 0;

 	pagedir_clear_page (thread_current () ->pagedir, f->page_ptr->VA);
	//remove_frame (f);
	palloc_free_page (f->PA);
	free (f);

	//printf("\nhiiiiiiiiiiiiiii\n");
//	sema_up (&f->sema_evict);
}

/*void remove_frame (struct frame *f)
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
}*/
