#ifndef REFERENCES_H
#define REFERENCES_H

/*
  smtp_references is the parsed References field
  - msg_id_list is the list of message identifiers
 */
struct smtp_references
{
	clist *mid_list;	/* list of (char *) */
	/* != NULL */
};

struct smtp_references *smtp_references_new (clist * mid_list);
void smtp_references_free (struct smtp_references *references);

int smtp_references_parse (const char *message, size_t length,
			   size_t * index, struct smtp_references **result);

#endif
