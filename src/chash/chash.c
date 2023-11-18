/*
 * $Id: chash.c,v 1.4 2005/12/07 09:44:31 wyong Exp $
 */

#include <stdlib.h>
#include <string.h>

#include "chash.h"

/* This defines the maximum (average) number of entries per bucket.
   The hash is resized everytime inserting an entry makes the
   average go over that value. */
#define CHASH_MAXDEPTH    3

static inline unsigned int
chash_func (const char *key, unsigned int len)
{
#if 0
	register unsigned int c = 0, t;
	register const char *k = key;

	while (len--) {
		c += (c << 4) + *k++;
		if ((t = c & 0xF0000000)) {
			c ^= t >> 24;
			c ^= t;
		}
	}
	return c;
#endif
	register unsigned int c = 5381;
	register const char *k = key;

	while (len--) {
		c = ((c << 5) + c) + *k++;
	}

	return c;
}

static inline char *
chash_dup (const void *data, unsigned int len)
{
	void *r;

	r = (char *) malloc (len);
	if (!r)
		return NULL;
	//DEBUG_SMTP(SMTP_MEM_1, "chash_dup: MALLOC pointer=%p\n", r);

	memcpy (r, data, len);
	return r;
}


chash *
chash_new (unsigned int size, int flags)
{
	chash *h;

	h = (chash *) malloc (sizeof (chash));
	if (h == NULL)
		return NULL;
	//DEBUG_SMTP(SMTP_MEM_1, "chash_new: MALLOC pointer=%p\n", h);

	if (size < CHASH_DEFAULTSIZE)
		size = CHASH_DEFAULTSIZE;

	h->count = 0;
	h->cells =
		(struct chashcell **) calloc (size,
					      sizeof (struct chashcell *));
	if (h->cells == NULL) {
		free (h);
		//DEBUG_SMTP(SMTP_MEM_1, "chash_new: FREE pointer=%p\n", h);
		return NULL;
	}
	//DEBUG_SMTP(SMTP_MEM_1, "chash_new: CALLOC pointer=%p\n", h->cells);

	h->size = size;
	h->copykey = flags & CHASH_COPYKEY;
	h->copyvalue = flags & CHASH_COPYVALUE;

	return h;
}


int
chash_get (chash * hash, chashdatum * key, chashdatum * result)
{
	unsigned int func;
	chashiter *iter;

	func = chash_func (key->data, key->len);

	/* look for the key in existing cells */
	iter = hash->cells[func % hash->size];
	while (iter) {
		if (iter->key.len == key->len && iter->func == func
		    && !memcmp (iter->key.data, key->data, key->len)) {
			*result = iter->value;	/* found */

			return 0;
		}
		iter = iter->next;
	}

	return -1;
}


int
chash_set (chash * hash,
	   chashdatum * key, chashdatum * value, chashdatum * oldvalue)
{
	unsigned int func, indx;
	chashiter *iter, *cell;
	int r;

	if (hash->count > hash->size * CHASH_MAXDEPTH) {
		r = chash_resize (hash,
				  (hash->count / CHASH_MAXDEPTH) * 2 + 1);
		if (r < 0)
			goto err;
	}

	func = chash_func (key->data, key->len);
	indx = func % hash->size;

	/* look for the key in existing cells */
	iter = hash->cells[indx];
	while (iter) {
		if (iter->key.len == key->len && iter->func == func
		    && !memcmp (iter->key.data, key->data, key->len)) {
			/* found, replacing entry */
			if (hash->copyvalue) {
				char *data;

				data = chash_dup (value->data, value->len);
				if (data == NULL)
					goto err;

				free (iter->value.data);
				//DEBUG_SMTP(SMTP_MEM_1, "chash_set: FREE pointer=%p\n", iter->value.data);

				iter->value.data = data;
				iter->value.len = value->len;
			}
			else {
				if (oldvalue != NULL) {
					oldvalue->data = iter->value.data;
					oldvalue->len = iter->value.len;
				}
				iter->value.data = value->data;
				iter->value.len = value->len;
			}
			if (!hash->copykey)
				iter->key.data = key->data;

			if (oldvalue != NULL) {
				oldvalue->data = value->data;
				oldvalue->len = value->len;
			}

			return 0;
		}
		iter = iter->next;
	}

	if (oldvalue != NULL) {
		oldvalue->data = NULL;
		oldvalue->len = 0;
	}

	/* not found, adding entry */
	cell = (struct chashcell *) malloc (sizeof (struct chashcell));
	if (cell == NULL)
		goto err;
	//DEBUG_SMTP(SMTP_MEM_1, "chash_set: MALLOC pointer=%p\n", cell);

	if (hash->copykey) {
		cell->key.data = chash_dup (key->data, key->len);
		if (cell->key.data == NULL)
			goto free;
	}
	else
		cell->key.data = key->data;

	cell->key.len = key->len;
	if (hash->copyvalue) {
		cell->value.data = chash_dup (value->data, value->len);
		if (cell->value.data == NULL)
			goto free_key_data;
	}
	else
		cell->value.data = value->data;

	cell->value.len = value->len;
	cell->func = func;
	cell->next = hash->cells[indx];
	hash->cells[indx] = cell;
	hash->count++;

	return 0;

      free_key_data:
	if (hash->copykey) {
		free (cell->key.data);
		//DEBUG_SMTP(SMTP_MEM_1, "chash_set: FREE pointer=%p\n", cell->key.data);
	}

      free:
	free (cell);
	//DEBUG_SMTP(SMTP_MEM_1, "chash_set: FREE pointer=%p\n", cell);

      err:
	return -1;
}


int
chash_delete (chash * hash, chashdatum * key, chashdatum * oldvalue)
{
	/*  chashdatum result = { NULL, TRUE }; */
	unsigned int func, indx;
	chashiter *iter, *old;

	/*  
	   if (!keylen)
	   keylen = strlen(key) + 1;  
	 */

	func = chash_func (key->data, key->len);
	indx = func % hash->size;

	/* look for the key in existing cells */
	old = NULL;
	iter = hash->cells[indx];
	while (iter) {
		if (iter->key.len == key->len && iter->func == func
		    && !memcmp (iter->key.data, key->data, key->len)) {
			/* found, deleting */
			if (old)
				old->next = iter->next;
			else
				hash->cells[indx] = iter->next;
			if (hash->copykey) {
				free (iter->key.data);
				//DEBUG_SMTP(SMTP_MEM_1, "chash_delete: FREE pointer=%p\n", iter->key.data);
			}
			if (hash->copyvalue) {
				free (iter->value.data);
				//DEBUG_SMTP(SMTP_MEM_1, "chash_delete: FREE pointer=%p\n", iter->value.data);
			}
			else {
				if (oldvalue != NULL) {
					oldvalue->data = iter->value.data;
					oldvalue->len = iter->value.len;
				}
			}
			free (iter);
			//DEBUG_SMTP(SMTP_MEM_1, "chash_delete: FREE pointer=%p\n", iter);

			hash->count--;
			return 0;
		}
		old = iter;
		iter = iter->next;
	}

	return -1;		/* not found */
}


void
chash_free (chash * hash)
{
	unsigned int indx;
	chashiter *iter, *next;

	/* browse the hash table */
	for (indx = 0; indx < hash->size; indx++) {
		iter = hash->cells[indx];
		while (iter) {
			next = iter->next;
			if (hash->copykey) {
				free (iter->key.data);
				//DEBUG_SMTP(SMTP_MEM_1, "chash_free: FREE pointer=%p\n", iter->key.data);
			}
			if (hash->copyvalue) {
				free (iter->value.data);
				//DEBUG_SMTP(SMTP_MEM_1, "chash_free: FREE pointer=%p\n", iter->value.data);
			}
			free (iter);
			//DEBUG_SMTP(SMTP_MEM_1, "chash_free: FREE pointer=%p\n", iter);
			iter = next;
		}
	}
	free (hash->cells);
	//DEBUG_SMTP(SMTP_MEM_1, "chash_free: FREE pointer=%p\n", hash->cells);
	free (hash);
	//DEBUG_SMTP(SMTP_MEM_1, "chash_free: FREE pointer=%p\n", hash);
}


void
chash_clear (chash * hash)
{
	unsigned int indx;
	chashiter *iter, *next;

	/* browse the hash table */
	for (indx = 0; indx < hash->size; indx++) {
		iter = hash->cells[indx];
		while (iter) {
			next = iter->next;
			if (hash->copykey) {
				free (iter->key.data);
				//DEBUG_SMTP(SMTP_MEM_1, "chash_clear: FREE pointer=%p\n", iter->key.data);
			}
			if (hash->copyvalue) {
				free (iter->value.data);
				//DEBUG_SMTP(SMTP_MEM_1, "chash_clear: FREE pointer=%p\n", iter->value.data);
			}
			free (iter);
			//DEBUG_SMTP(SMTP_MEM_1, "chash_clear: FREE pointer=%p\n", iter);
			iter = next;
		}
	}
	memset (hash->cells, 0, hash->size * sizeof (*hash->cells));
	hash->count = 0;
}


chashiter *
chash_begin (chash * hash)
{
	chashiter *iter;
	unsigned int indx = 0;

	iter = hash->cells[0];
	while (!iter) {
		indx++;
		if (indx >= hash->size)
			return NULL;
		iter = hash->cells[indx];
	}
	return iter;
}


chashiter *
chash_next (chash * hash, chashiter * iter)
{
	unsigned int indx;

	if (!iter)
		return NULL;

	indx = iter->func % hash->size;
	iter = iter->next;

	while (!iter) {
		indx++;
		if (indx >= hash->size)
			return NULL;
		iter = hash->cells[indx];
	}
	return iter;
}


int
chash_resize (chash * hash, unsigned int size)
{
	struct chashcell **cells;
	unsigned int indx, nindx;
	chashiter *iter, *next;

	if (hash->size == size)
		return 0;

	cells = (struct chashcell **) calloc (size,
					      sizeof (struct chashcell *));
	if (!cells)
		return -1;
	//DEBUG_SMTP(SMTP_MEM_1, "chash_resize: CALLOC pointer=%p\n", cells);

	/* browse initial hash and copy items in second hash */
	for (indx = 0; indx < hash->size; indx++) {
		iter = hash->cells[indx];
		while (iter) {
			next = iter->next;
			nindx = iter->func % size;
			iter->next = cells[nindx];
			cells[nindx] = iter;
			iter = next;
		}
	}
	free (hash->cells);
	//DEBUG_SMTP(SMTP_MEM_1, "chash_resize: FREE pointer=%p\n", hash->cells);
	hash->size = size;
	hash->cells = cells;

	return 0;
}

#ifdef NO_MACROS

int
chash_count (chash * hash)
{
	return hash->count;
}


int
chash_size (chash * hash)
{
	return hash->size;
}


void
chash_value (chashiter * iter, chashdatum * result)
{
	*result = iter->value;
}


void
chash_key (chashiter * iter, chashdatum * result)
{
	*result = iter->key;
}
#endif
