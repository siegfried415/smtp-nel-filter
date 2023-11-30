
#ifndef CLIST_H
#define CLIST_H

#ifdef __cplusplus
extern "C"
{
#endif

	typedef struct clistcell_s
	{
		void *data;
		struct clistcell_s *previous;
		struct clistcell_s *next;
	} clistcell;

	struct clist_s
	{
		int count;
		clistcell *first;
		clistcell *last;
		int number;
	};

	typedef struct clist_s clist;
	typedef clistcell clistiter;

/* Allocate a new pointer list */
	clist *clist_new ();

/* Destroys a list. Data pointed by data pointers is NOT freed. */
	void clist_free (clist *);


/* Some of the following routines can be implemented as macros to
   be faster. If you don't want it, define NO_MACROS */
#ifdef NO_MACROS

/* Returns TRUE if list is empty */
	int clist_isempty (clist *);

/* Returns the number of elements in the list */
	int clist_count (clist *);

/* Returns an iterator to the first element of the list */
	clistiter *clist_begin (clist *);

/* Returns an iterator to the last element of the list */
	clistiter *clist_end (clist *);

/* Returns an iterator to the next element of the list */
	clistiter *clist_next (clistiter *);

/* Returns an iterator to the previous element of the list */
	clistiter *clist_previous (clistiter *);

/* Returns the data pointer of this element of the list */
	void *clist_content (clistiter *);

/* Inserts this data pointer at the beginning of the list */
	int clist_prepend (clist *, void *);

/* Inserts this data pointer at the end of the list */
	int clist_append (clist *, void *);
#else
#define     clist_isempty(lst)             ((lst->first==lst->last) && (lst->last==NULL))
#define     clist_count(lst)               (lst->size)
#define     clist_begin(lst)               (lst->first)
#define     clist_end(lst)                 (lst->last)
#define     clist_next(iter)               (iter ? iter->next : NULL)
#define     clist_previous(iter)           (iter ? iter->previous : NULL)
#define     clist_content(iter)            (iter ? iter->data : NULL)
#define     clist_prepend(lst, data)  (clist_insert_before(lst, lst->first, data))
#define     clist_append(lst, data)   (clist_insert_after(lst, lst->last, data))
#endif

/* Inserts this data pointer before the element pointed by the iterator */
	int clist_insert_before (clist *, clistiter *, void *);

/* Inserts this data pointer after the element pointed by the iterator */
	int clist_insert_after (clist *, clistiter *, void *);

/* Deletes the element pointed by the iterator.
   Returns an iterator to the next element. */
	clistiter *clist_delete (clist *, clistiter *);

	typedef void (*clist_func) (void *, void *);

	void clist_foreach (clist * lst, clist_func func, void *data);

	void clist_concat (clist * dest, clist * src);

	void *clist_nth_data (clist * lst, int index);

	clistiter *clist_nth (clist * lst, int index);

#ifdef __cplusplus
}
#endif

#endif
