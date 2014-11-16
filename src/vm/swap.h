#include "kernel/hash.h"
#include "vm/page.h"

void init_swap_table (void);
void remove_swap (struct page *p);
bool insert_swap (struct page *p);
void delete_swap (struct page *p);
