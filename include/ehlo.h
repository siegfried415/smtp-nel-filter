#ifndef EHLO_H
#define EHLO_H

struct smtp_cmd_ehlo
{
#ifdef USE_NEL
	NEL_REF_DEF
#endif
	int len;
	short host_name_len;
	char *host_name;
};

void smtp_cmd_ehlo_init (
#ifdef USE_NEL
				struct nel_eng *eng
#endif
	);

int smtp_ack_ehlo_param_auth_parse (struct smtp_info *psmtp,
				    char *message, int length, size_t *index);

int smtp_ack_ehlo_param_parse (struct smtp_info *psmtp,
			       char *message, int length, size_t *index);

int smtp_ack_ehlo_parse (struct smtp_info *psmtp);


void smtp_cmd_ehlo_free (struct smtp_cmd_ehlo *ehlo);

struct smtp_cmd_ehlo *smtp_cmd_ehlo_new (int len, char *host_name);

int smtp_cmd_ehlo_parse (struct smtp_info *psmtp,
			 char *message, size_t length, size_t * index);

#endif
