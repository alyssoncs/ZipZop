#include "sllist.h"

struct sllist {
	void *key;
	struct sllist *next;
};

struct sllist *sll_init(void)
{
	return NULL;
}

void sll_insert_first(struct sllist **l, void *a)
{
	struct sllist *node = malloc(sizeof(struct sllist));

	if (node) {
		node->key = a;
		node->next = *l;
		*l = node;
	}
}


void sll_insert_last(struct sllist **l, void *a)
{
	struct sllist *node = malloc(sizeof(struct sllist));

	if (node) {
		while (*l)
			l = &(*l)->next;
		node->key = a;
		node->next = *l;
		*l = node;
	}
}

struct sllist *sll_remove_first(struct sllist **l)
{
	struct sllist *node = *l;

	if (*l)
		*l = (*l)->next;

	return node;
}


struct sllist *sll_remove_last(struct sllist **l)
{
	struct sllist *node = *l;

	while (*l) {
		if (!(*l)->next) {
			node = *l;
			*l = (*l)->next;
			break;
		}
		l = &(*l)->next;
	}

	return node;
}

void *sll_get_key(struct sllist *l)
{
	if (l)
		return l->key;
	return NULL;
}

