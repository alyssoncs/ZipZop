#ifndef SLLIST_H
#define SLLIST_H

#include <stdlib.h>

struct sllist;

 
/**
 * @brief Macro that initialize a sllist to be a valid empty list.
 *
 * @return An empty list.
 *
 * @warning One should not test the return against NULL. NULL is the default value.
 * @see sll_init
 */
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
