#ifndef SLLIST_H
#define SLLIST_H

#include <stdlib.h>

struct sllist;

#define SLL_INIT() NULL;

struct sllist *sll_init(void);
struct sllist *sll_get_next(struct sllist **l);
void sll_insert_first(struct sllist **l, void *a);
void sll_insert_last(struct sllist **l, void *a);
void *sll_remove_first(struct sllist **l);
void *sll_remove_last(struct sllist **l);
void *sll_remove_elm(struct sllist **l, void *elm);
void *sll_get_key(struct sllist *l);
#endif
