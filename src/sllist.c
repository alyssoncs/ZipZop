#include "sllist.h"

struct sllist {
	void *key;
	struct sllist *next;
};

struct sllist *sll_init(void)
{
	return NULL;
}

struct sllist *sll_get_next(struct sllist **l)
{
	return (*l)->next;
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

void *sll_remove_first(struct sllist **l)
{
	struct sllist *node = *l;

	if (*l)
		*l = (*l)->next;

	void *key = sll_get_key(node);
	free(node);
	return key;
}


void *sll_remove_last(struct sllist **l)
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

	void *key = sll_get_key(node);
	free(node);
	return key;
}

void *sll_get_key(struct sllist *l)
{
	if (l)
		return l->key;
	return NULL;
}

