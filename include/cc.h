#ifndef CC_H
#define CC_H

/*
  smtp_cc is the parsed Cc field
  - cc_addr_list is the parsed addres list
*/

struct smtp_cc
{
	struct smtp_address_list *cc_addr_list;	/* != NULL */
};

struct smtp_cc *smtp_cc_new (struct smtp_address_list *cc_addr_list);
void smtp_cc_free (struct smtp_cc *cc);

/*
cc              =       "Cc:" address-list CRLF
*/

int
smtp_cc_parse (const char *message, size_t length,
	       size_t * index, struct smtp_cc **result);


#endif
