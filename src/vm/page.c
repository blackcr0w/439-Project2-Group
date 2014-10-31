#include "threads/thread.h"
#include "threads/flags.h"
#include "threads/interrupt.h"
#include "threads/intr-stubs.h"
#include "threads/palloc.h"
#include "threads/switch.h"
#include "threads/synch.h"
#include "threads/vaddr.h"

#include "vm/page.h"


//struct page *page_table[256];
//int page_index;
struct list page_table;


void 
init_page_table (void)
{
	//page_table[256] = {NULL};
	//page_index = 0;
	list_init (&page_table);	

}

void 
insert_page (struct page *p)
{
	/*p->dirty = 0;
	p->access = 1;
	p->in_frame_table = 1;*/ //maybe do it for swapping

	list_push_back (&page_table, &p -> page_elem);

	/* page_table[page_index] = p;
	 page_index++;*/

}
