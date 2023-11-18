#ifndef COMMENTS_H
#define COMMENTS_H

/*
  smtp_comments is the parsed Comments field
  - cm_value is the value of the field
*/
struct smtp_comments
{
	char *cm_value;		/* != NULL */
};

struct smtp_comments *smtp_comments_new (char *cm_value);
void smtp_comments_free (struct smtp_comments *comments);

/*
comments        =       "Comments:" unstructured CRLF
*/

int smtp_comments_parse (const char *message, size_t length,
			 size_t * index, struct smtp_comments **result);

#endif
