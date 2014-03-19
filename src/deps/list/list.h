#ifndef _h_LIST
#define _h_LIST

#include <unistd.h>
#include <stddef.h>

typedef struct list_node_t
{
	struct list_node_t *next, *prev;
} list_node;
static inline void __list_add(list_node *prev, list_node *next, list_node *node)
{
	prev->next = node; node->prev = prev; next->prev = node; node->next = next;
}
static inline void list_add(list_node *head, list_node *node)
{
	__list_add(head->prev, head, node);
}
static inline void __list_rmv(list_node *prev, list_node *next)
{
	prev->next = next; next->prev = prev;
}
static inline void list_rmv(list_node *node)
{
	__list_rmv(node->prev, node->next);
}

static inline void init_list(list_node *head)
{
	head->prev = head->next = head;
}

static inline int list_empty(list_node *head)
{
	return head->next == head;
}

static inline int list_alone(list_node *head)
{
	return (head->next == head->prev) && !list_empty(head);
}

static inline list_node *list_get(list_node *head)
{
	list_node *node = head->next; return (node == head) ? NULL : (list_rmv(node), node);
}

static inline list_node *list_foreach(list_node *head, list_node *node)
{
	if (node == NULL) node = head; return node->next == head ? NULL : node->next;
}

#define node2enty(node, type, member1) \
	((void *)node - offsetof(type, member1))

#define node2item(node, type, member1, member2) \
	((void *)node - offsetof(type, member1) + offsetof(type, member2))

#endif /* _h_LIST */
