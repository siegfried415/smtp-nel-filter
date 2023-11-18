#ifndef TO_H
#define TO_H

/* To: =?gb2312?B?z8TT8A==?= <xia_yu@neusoft.com> */
struct smtp_to
{
#ifdef USE_NEL
	NEL_REF_DEF
#endif
	int len;
	struct smtp_address_list *to_addr_list;	/* != NULL */
	int err_cnt;
};

void smtp_msg_to_init (
#ifdef USE_NEL
			      struct nel_eng *eng
#endif
	);

void smtp_msg_to_free (struct smtp_to *to);

struct smtp_to *smtp_msg_to_new (struct smtp_address_list *to_addr_list);

int smtp_msg_to_parse (struct smtp_info *psmtp,
		       const char *message, size_t length,
		       size_t * index, struct smtp_to **result);

#endif
