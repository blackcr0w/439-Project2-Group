#include "kernel/hash.h"
#include "vm/page.h"

struct hash swap_table;

unsigned swap_hash (const struct hash_elem *s_elem, void *aux);

bool hash_swap_less (const struct hash_elem *h1, const struct hash_elem *h2, void *aux);

void init_swap_table (void);

void remove_swap (struct page *p);

void insert_swap (struct page *p);

bool write_swap (struct page * p);

void read_swap (struct page * p);

void delete_swap (struct page * p);