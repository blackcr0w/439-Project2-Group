/*#include <debug.h>
#include <list.h>
#include <stdint.h>*/

#include "vm/page.h"

struct frame // initialize?????
{
	//void *page_ptr;

	struct page *page;

	//struct hash_elem hash_elem;  //used to put in hash element
	int frame_in;

};
void insert_frame (struct frame *f);

void free_frame (void *adr);

void init_frame_table (void);

void * get_new_frame (void);

void evict_page (void);


