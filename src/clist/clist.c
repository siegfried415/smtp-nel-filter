/*
 * $Id: clist.c,v 1.6 2005/12/07 09:44:31 wyong Exp $
 */

#include <stdlib.h>
#include "clist.h"


clist *
clist_new ()
{
	clist *lst;

	lst = (clist *) malloc (sizeof (clist));
	if (!lst)
		return NULL;
	//DEBUG_SMTP(SMTP_MEM_1, "clist_new: MALLOC pointer=%p\n", lst);

	//xiayu 2005.11.30
	//lst->count = 0;  
	//NEL_REF_INIT(lst);

	lst->first = lst->last = NULL;
	lst->number = 0;

	return lst;
}

void
clist_free (clist * lst)
{
	clistcell *l1, *l2;

	l1 = lst->first;
	while (l1) {
		l2 = l1->next;
		free (l1);
		//DEBUG_SMTP(SMTP_MEM_1, "clist_free: FREE pointer=%p\n", l1);
		l1 = l2;
	}

	free (lst);
	//DEBUG_SMTP(SMTP_MEM_1, "clist_free: FREE pointer=%p\n", lst);
}

#ifdef NO_MACROS
int
clist_isempty (clist * lst)
{
	return ((lst->first == lst->last) && (lst->last == NULL));
}

clistiter *
clist_begin (clist * lst)
{
	return lst->first;
}

clistiter *
clist_end (clist * lst)
{
	return lst->last;
}

clistiter *
clist_next (clistiter * iter)
{
	if (iter)
		return iter->next;
	else
		return NULL;
}

clistiter *
clist_previous (clistiter * iter)
{
	if (iter)
		return iter->previous;
	else
		return NULL;
}

void *
clist_content (clistiter * iter)
{
	if (iter)
		return iter->data;
	else
		return NULL;
}

int
clist_count (clist * lst)
{
	return lst->number;
}

int
clist_prepend (clist * lst, void *data)
{
	return clist_insert_before (lst, lst->first, data);
}

int
clist_append (clist * lst, void *data)
{
	return clist_insert_after (lst, lst->last, data);
}
#endif

int
clist_insert_before (clist * lst, clistiter * iter, void *data)
{
	clistcell *c;

	c = (clistcell *) malloc (sizeof (clistcell));
	if (!c)
		return -1;
	//DEBUG_SMTP(SMTP_MEM_1, "clist_insert_before: MALLOC pointer=%p\n", c);

	c->data = data;
	lst->number++;

	if (clist_isempty (lst)) {
		c->previous = c->next = NULL;
		lst->first = lst->last = c;
		return 0;
	}

	if (!iter) {
		c->previous = lst->last;
		c->previous->next = c;
		c->next = NULL;
		lst->last = c;
		return 0;
	}

	c->previous = iter->previous;
	c->next = iter;
	c->next->previous = c;
	if (c->previous)
		c->previous->next = c;
	else
		lst->first = c;

	return 0;
}

int
clist_insert_after (clist * lst, clistiter * iter, void *data)
{
	clistcell *c;

	c = (clistcell *) malloc (sizeof (clistcell));
	if (!c)
		return -1;
	//DEBUG_SMTP(SMTP_MEM_1, "clist_insert_after: MALLOC pointer=%p\n", c);

	c->data = data;
	lst->number++;

	if (clist_isempty (lst)) {
		c->previous = c->next = NULL;
		lst->first = lst->last = c;
		return 0;
	}

	if (!iter) {
		c->previous = lst->last;
		c->previous->next = c;
		c->next = NULL;
		lst->last = c;
		return 0;
	}

	c->previous = iter;
	c->next = iter->next;
	if (c->next)
		c->next->previous = c;
	else
		lst->last = c;
	c->previous->next = c;

	return 0;
}

clistiter *
clist_delete (clist * lst, clistiter * iter)
{
	clistiter *ret;

	if (!iter)
		return NULL;

	if (iter->previous)
		iter->previous->next = iter->next;
	else
		lst->first = iter->next;

	if (iter->next) {
		iter->next->previous = iter->previous;
		ret = iter->next;
	}
	else {
		lst->last = iter->previous;
		ret = NULL;
	}

	free (iter);
	//DEBUG_SMTP(SMTP_MEM_1, "clist_delete: FREE pointer=%p\n", iter);
	lst->number--;

	return ret;
}



void
clist_foreach (clist * lst, clist_func func, void *data)
{
	clistiter *cur;

	for (cur = clist_begin (lst); cur != NULL; cur = cur->next)
		func (cur->data, data);
}

void
clist_concat (clist * dest, clist * src)
{
	if (src->first == NULL) {
		/* do nothing */
	}
	else if (dest->last == NULL) {
		dest->first = src->first;
		dest->last = src->last;
	}
	else {
		dest->last->next = src->first;
		src->first->previous = dest->last;
		dest->last = src->last;
	}

	dest->number += src->number;
	src->last = src->first = NULL;
}

static clistiter *
internal_clist_nth (clist * lst, int index)
{
	clistiter *cur;

	cur = clist_begin (lst);
	while ((index > 0) && (cur != NULL)) {
		cur = cur->next;
		index--;
	}

	if (cur == NULL)
		return NULL;

	return cur;
}

void *
clist_nth_data (clist * lst, int index)
{
	clistiter *cur;

	cur = internal_clist_nth (lst, index);
	if (cur == NULL)
		return NULL;

	return cur->data;
}

clistiter *
clist_nth (clist * lst, int index)
{
	return internal_clist_nth (lst, index);
}
