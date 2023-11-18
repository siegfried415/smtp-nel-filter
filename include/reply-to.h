#ifndef REPLY_TO_H
#define REPLY_TO_H

/*
  smtp_reply_to is the parsed Reply-To field
  - rt_addr_list is the parsed address list
 */
struct smtp_reply_to
{
	struct smtp_address_list *rt_addr_list;	/* != NULL */
};

struct smtp_reply_to *smtp_reply_to_new (struct smtp_address_list
					 *rt_addr_list);

void smtp_reply_to_free (struct smtp_reply_to *reply_to);


/*
reply-to        =       "Reply-To:" address-list CRLF
*/

int
smtp_reply_to_parse (const char *message, size_t length,
		     size_t * index, struct smtp_reply_to **result);

#endif
