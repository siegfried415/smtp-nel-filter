#ifndef PASSWORD_H
#define PASSWORD_H

struct smtp_cmd_passwd
{
#ifdef USE_NEL
	NEL_REF_DEF
#endif
	int len;
	short pass_len;
	char *pass;
};

void smtp_cmd_passwd_init (
#ifdef USE_NEL
				  struct nel_eng *eng
#endif
	);

int smtp_ack_passwd_parse (struct smtp_info *psmtp);

void smtp_cmd_passwd_free (struct smtp_cmd_passwd *passwd);

struct smtp_cmd_passwd *smtp_cmd_passwd_new (int len, char *pass);

int smtp_cmd_passwd_parse (struct smtp_info *psmtp,
			   char *message, size_t length, size_t * index);

#endif
