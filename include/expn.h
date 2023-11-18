#ifndef EXPN_H
#define EXPN_H

struct smtp_cmd_expn
{
#ifdef USE_NEL
	NEL_REF_DEF
#endif
	int len;
	short user_name_len;
	char *user_name;
	short mail_box_len;
	char *mail_box;
};

struct smtp_ack_expn
{
#ifdef USE_NEL
	NEL_REF_DEF
#endif
	int len;
	short code;
	clist *mb_list;
};

void smtp_cmd_expn_init (
#ifdef USE_NEL
				struct nel_eng *eng
#endif
	);

void smtp_ack_expn_free (struct smtp_ack_expn *ack);

struct smtp_ack_expn *smtp_ack_expn_new (int len, int code, clist * mb_list);

int smtp_ack_expn_parse (struct smtp_info *psmtp);

void smtp_cmd_expn_free (struct smtp_cmd_expn *expn);

struct smtp_cmd_expn *smtp_cmd_expn_new (int len,
					 char *user_name, char *mail_box);

int smtp_cmd_expn_parse (struct smtp_info *psmtp,
			 char *message, size_t length, size_t * index);

#endif
