#ifndef USER_H
#define USER_H

struct smtp_cmd_user
{
#ifdef USE_NEL
	NEL_REF_DEF
#endif
	int len;
	short user_len;
	char *user;
};

void smtp_cmd_user_init (
#ifdef USE_NEL
				struct nel_eng *eng
#endif
	);

int smtp_ack_user_parse (struct smtp_info *psmtp);

void smtp_cmd_user_free (struct smtp_cmd_user *user);

struct smtp_cmd_user *smtp_cmd_user_new (int len, char *user_name);

int smtp_cmd_user_parse (struct smtp_info *psmtp,
			 char *message, size_t length, size_t * index);

#endif
