/*#include <debug.h>
#include <list.h>
#include <stdint.h>*/

//#include "vm/page.h"
#include "kernel/hash.h"

struct frame // initialize?????
{
	//void *page_ptr;
	void * PA;

	//struct page page;

	struct hash_elem hash_elem;  //used to put in hash element

};

void insert_frame (struct frame *f);

void free_frame (void *adr);

void init_frame_table (void);

void * get_new_frame (struct page *p);

void evict_page (struct page *p);

bool hash_less (const struct hash_elem *a, const struct hash_elem *b,
           void *aux);

unsigned
frame_hash (const struct hash_elem *h_elem, void *aux);

void remove_frame (struct frame *f);
 