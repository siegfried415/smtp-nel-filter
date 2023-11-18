#ifndef BCC_H
#define BCC_H

/*
  smtp_bcc is the parsed Bcc field
  - bcc_addr_list is the parsed addres list
*/
struct smtp_bcc
{
	struct smtp_address_list *bcc_addr_list;	/* can be NULL */
};

struct smtp_bcc *smtp_bcc_new (struct smtp_address_list *bcc_addr_list);
void smtp_bcc_free (struct smtp_bcc *bcc);


/*
bcc             =       "Bcc:" (address-list / [CFWS]) CRLF
*/

int
smtp_bcc_parse (const char *message, size_t length,
		size_t * index, struct smtp_bcc **result);



#endif
