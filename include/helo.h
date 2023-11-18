#ifndef HELO_H
#define HELO_H

struct smtp_cmd_helo
{
#ifdef USE_NEL
	NEL_REF_DEF
#endif
	int len;
	short host_name_len;
	char *host_name;
};

void smtp_cmd_helo_init (
#ifdef USE_NEL
				struct nel_eng *eng
#endif
	);

void smtp_helo_reset (struct smtp_info *psmtp);

int smtp_ack_helo_parse (struct smtp_info *psmtp);

void smtp_cmd_helo_free (struct smtp_cmd_helo *helo);

struct smtp_cmd_helo *smtp_cmd_helo_new (int len, char *host_name);

int smtp_cmd_helo_parse (struct smtp_info *psmtp, char *message,
			 size_t length, size_t * index);

#endif
