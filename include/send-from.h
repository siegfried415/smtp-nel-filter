#ifndef SEND_FROM_H
#define SEND_FROM_H

struct smtp_cmd_send
{
#ifdef USE_NEL
	NEL_REF_DEF
#endif
	int len;
	short name_len;
	char *name;
	short addr_len;
	char *addr;
};

void smtp_cmd_send_init (
#ifdef USE_NEL
				struct nel_eng *eng
#endif
	);

int smtp_ack_send_parse (struct smtp_info *psmtp);

void smtp_cmd_send_free (struct smtp_cmd_send *send);

struct smtp_cmd_send *smtp_cmd_send_new (int len, char *addr);

int smtp_cmd_send_parse (struct smtp_info *psmtp,
			 char *message, size_t length, size_t * index);

#endif
