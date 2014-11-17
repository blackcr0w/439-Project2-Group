/*#include <debug.h>
#include <list.h>
#include <stdint.h>*/
 
#include "vm/page.h"
#include "kernel/hash.h"
#include "threads/synch.h"

// Spencer driving
struct frame 
{
	void * PA;					 // physcial address
	struct page *page_ptr; 		 // pointer to related page
	struct semaphore sema_evict; // manages synchronization in evicting frame
	struct list_elem frame_elem; // for the frame table
};

void insert_frame (struct frame *f); 	// inserts frame f into the frame table
void init_frame_table (void); 			// intializes the frame table
void * get_new_frame (struct page *p);	// returns a pointer to a new frame mapped to the given page
void evict_page (void);					// evicts a page
void remove_frame (struct frame *f);	// removes frame f from the table
void clear_thread_frames(void);			// when a thread dies, remove all its frames
