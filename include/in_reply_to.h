#ifndef IN_REPLY_TO_H
#define IN_REPLY_TO_H


/*
  smtp_in_reply_to is the parsed In-Reply-To field
  - mid_list is the list of message identifers
*/
struct smtp_in_reply_to
{
	clist *mid_list;	/* list of (char *), != NULL */
};

struct smtp_in_reply_to *smtp_in_reply_to_new (clist * mid_list);
void smtp_in_reply_to_free (struct smtp_in_reply_to *in_reply_to);

#endif
