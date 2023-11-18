#ifndef FROM_H
#define FROM_H

/* From: =?gb2312?B?z8TT8A==?= <xia_yu@neusoft.com> */
struct smtp_from
{
#ifdef USE_NEL
	NEL_REF_DEF
#endif
	int len;
	struct smtp_mailbox_list *frm_mb_list;	/* != NULL */
	int err_cnt;
};

void smtp_msg_from_init (
#ifdef USE_NEL
				struct nel_eng *eng
#endif
	);

void smtp_msg_from_free (struct smtp_from *from);

struct smtp_from *smtp_msg_from_new (struct smtp_mailbox_list *frm_mb_list);

int smtp_msg_from_parse (struct smtp_info *psmtp,
			 const char *message, size_t length,
			 size_t * index, struct smtp_from **result);

#endif
