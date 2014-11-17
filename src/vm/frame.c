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
#include "userprog/pagedir.h"

// prevent multiple pages from getting the same frame
static struct semaphore sema_get_frame;

struct list frame_table_list; // the list for the frame table

void 
insert_frame (struct frame *f)
{	
	list_push_back (&frame_table_list, &f->frame_elem);
}
   
void 
init_frame_table (void)  
{
    list_init (&frame_table_list);
    sema_init (&sema_get_frame, 1);
}

void *
get_new_frame (struct page *p)  // pass in the page so that you can access it
{
	ASSERT(p!=NULL);	

	sema_down (&sema_get_frame);

	// make a new frame
	struct frame *f = calloc (1 , sizeof (struct frame));
	sema_init (&f->sema_evict, 1);

	printf("Got Here: %p\n\n", p->VA);
  
	// map that frame to the given page

	f -> cur_tid = thread_current() -> tid;
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

	p -> frame_ptr = f;
	f->page_ptr = p; 

	insert_frame (f);

	sema_up (&sema_get_frame);

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
	struct list_elem *front = list_begin (&frame_table_list);

	struct frame *f = calloc (1, sizeof (struct frame));
	f = list_entry(front, struct frame, frame_elem);

	struct thread *occupied = get_thread_tid (f -> cur_tid);

	sema_down (&f->sema_evict);
	list_pop_front (&frame_table_list); 

	/*if(pagedir_is_dirty (thread_current ()->pagedir, f->page_ptr))
	{
	  	if(!insert_swap (f->page_ptr))
	  	{
	  		printf ("unsuccessful write");
	  		return;
	  	}
	  	f->page_ptr->present = 1;
	}
	else*/
 	pagedir_clear_page (occupied ->pagedir, f->page_ptr->VA);
	if(!insert_swap (f->page_ptr))
  	{
  		printf ("unsuccessful write");
  		return;
  	}
	f->page_ptr->present = 1;


	sema_up (&f->sema_evict);
	palloc_free_page (pg_round_down(f->PA));
	free (f);	
}




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
