#ifndef RCPT_TO_H
#define RCPT_TO_H

struct smtp_cmd_rcpt
{
#ifdef USE_NEL
	NEL_REF_DEF
#endif
	int len;
	short addr_len;
	char *addr;
};

void smtp_cmd_rcpt_init (
#ifdef USE_NEL
				struct nel_eng *eng
#endif
	);

void smtp_rcpt_reset (struct smtp_info *psmtp);

int smtp_ack_rcpt_parse (struct smtp_info *psmtp);

struct smtp_cmd_rcpt *smtp_cmd_rcpt_new (int len, char *addr);

void smtp_cmd_rcpt_free (struct smtp_cmd_rcpt *rcpt);

int smtp_cmd_rcpt_parse (struct smtp_info *psmtp,
			 char *message, size_t length, size_t * index);

#endif
