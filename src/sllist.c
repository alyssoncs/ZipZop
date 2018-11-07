#include "sllist.h"

/**
 * @brief A struct representing node in a singly linked list.
 */
struct sllist {
	void *key; 				/**< The element that will be stored in the node */
	struct sllist *next; 	/**< A pointer to the next node */
};

/**
 * @brief Initilize a sllist to be a valid empty list.
 *
 * @return An empty list.
 *
 * @warning One should not test the return against NULL. NULL is the default value.
 * @see SLL_INIT
 */
struct sllist *sll_init(void)
{
	return NULL;
}

/**
 * @brief Get the next node in the list.
 *
 * @param[in,out] l An address to a pointer to the list.
 *
 * @return A pointer to the next node in the list; NULL if there is no next element.
 *
 * Example to interate over a list:
 * @code
 * struct sllist *l = sll_init();
 * // fill the list
 * for (struct sllist *p = l; p; p = sll_get_next(&p)) {
 *     void *key = sll_get_key(p);
 *     // do stuff with p
 * }
 * @endcode
 */
struct sllist *sll_get_next(struct sllist **l)
{
	if (*l) {
		return (*l)->next;
	}
	return NULL;
}

/**
 * @brief Insert an element on the head of the list.
 *
 * @param[in,out] l An address to a pointer to the list.
 * @param[in] a The element.
 */
void sll_insert_first(struct sllist **l, void *a)
{
	struct sllist *node = malloc(sizeof(struct sllist));

	if (node) {
		node->key = a;
		node->next = *l;
		*l = node;
	}
}

/**
 * @brief Insert an element on the tail of the list.
 *
 * @param[in,out] l An address to a pointer to the list.
 * @param[in] a The element.
 */
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

/**
 * @brief Remove the first element of the list.
 *
 * The list node will be freed.
 *
 * @param[in,out] l An address to a pointer to the list.
 *
 * @return The element in case of success. NULL if the list is empty.
 */
void *sll_remove_first(struct sllist **l)
{
	struct sllist *node = *l;

	if (*l)
		*l = (*l)->next;

	void *key = sll_get_key(node);
	free(node);
	return key;
}

/**
 * @brief Remove the last element of the list.
 *
 * The list node will be freed.
 *
 * @param[in,out] l An address to a pointer to the list.
 *
 * @return The element in case of success. NULL if the list is empty.
 */
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

/**
 * @brief Remove the specified element of the list.
 *
 * @param[in,out] l An address to a pointer to the list.
 * @param[in] elm The element.
 *
 * @return The element in case of success. NULL if the list is empty or the element doesn't exit.
 */
void *sll_remove_elm(struct sllist **l, void *elm)
{
	if (*l) {
		for ( ; *l; l = &(*l)->next) {
			void *key = sll_get_key(*l);
			if (key == elm) {
				sll_remove_first(l);
				break;
			}
		}
	}

	if (*l) 
		return elm;
	return NULL;
}

/** 
 * @brief Get the element stored in the especified list node.
 *
 * @param[in] l A pointer to the list node.
 *
 * @return The element.
 */
void *sll_get_key(struct sllist *l)
{
	if (l)
		return l->key;
	return NULL;
}

