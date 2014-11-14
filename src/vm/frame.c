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



struct hash frame_table;
struct hash_iterator i;

void 
insert_frame (struct frame *f)
{	
	hash_insert (&frame_table, &f->hash_elem);
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

	// make a new frame
	struct frame *f = malloc (sizeof (struct frame));
	sema_init (&f->sema_evict, 1);

	// map that frame to the given page

	p -> frame_ptr = f;

	void * palloc_address = palloc_get_page (PAL_USER);

	// if memory is full
	if (palloc_address == NULL)
	{
		
		evict_page (); // make some memory available

		f->PA = palloc_get_page (PAL_USER); // 
	}

	// if memory is not full
	else
		f->PA = palloc_address;	

	f->page_ptr = p;

	insert_frame (f);



  //printf("\nPointer to Frame %p\n", f);
  //printf("Pointer to Page %p\n", p);
	return f->PA; 
}

void clear_thread_frames()
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
}

void
evict_page ()
{
	// Clock


	// while we haven't found a page to evict
	while (true)
	{

		// if at the end, go to the beginning
		if(hash_next (&i) == NULL)
		{
			hash_first (&i, &frame_table);
			hash_next (&i);
		}

		// get the frame
		struct frame *f = hash_entry (hash_cur (&i), struct frame, hash_elem);
		

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
	 // printf("\nPointer to Frame %p\n", f);
  		//printf("Pointer to Page %p\n", f->page_ptr);
			// prevent multiple evictions
			sema_down (&f->sema_evict);

			// if it's dirty, swap
			if(pagedir_is_dirty (thread_current ()->pagedir, f->page_ptr))
				insert_swap (f->page_ptr);

			pagedir_clear_page (thread_current () ->pagedir, f->page_ptr->VA);
			// remove frame stuff
			remove_frame (f);
			palloc_free_page (f->PA);
		//	printf("got here\n");
		//	printf("%p\n", f->page_ptr);
			
			// allow other evicitons
			sema_up (&f->sema_evict);

			// get out of the loop
			break;
		}

	}












/*
	// FIFO
	if (hash_next (&i) != NULL)
	{
	  struct frame *f = hash_entry (hash_cur (&i), struct frame, hash_elem);

	  sema_down (&f->sema_evict);

	  if(pagedir_is_dirty (thread_current ()->pagedir, f->page_ptr))
	  	insert_swap (f->page_ptr);

	  remove_frame (f);
	  palloc_free_page (f->PA);
	  pagedir_clear_page(thread_current () ->pagedir, f->page_ptr->VA);
	  sema_up (&f->sema_evict);
	}

	// if it is at the end then go back to beginning
	else
	{
		hash_first (&i, &frame_table);
		evict_page ();
	}*/
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
