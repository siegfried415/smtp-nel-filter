#ifndef VRFY_H
#define VRFY_H

struct smtp_cmd_vrfy
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

struct smtp_ack_vrfy
{
#ifdef USE_NEL
	NEL_REF_DEF
#endif
	int len;
	short code;
	struct smtp_mailbox *mailbox;
};

void smtp_cmd_vrfy_init (
#ifdef USE_NEL
				struct nel_eng *eng
#endif
	);

void smtp_ack_vrfy_free (struct smtp_ack_vrfy *ack);

struct smtp_ack_vrfy *smtp_ack_vrfy_new (int len,
					 int code,
					 struct smtp_mailbox *mailbox);

int smtp_ack_vrfy_parse (struct smtp_info *psmtp);

void smtp_cmd_vrfy_free (struct smtp_cmd_vrfy *vrfy);

struct smtp_cmd_vrfy *smtp_cmd_vrfy_new (int len,
					 char *user_name, char *mail_box);

int smtp_cmd_vrfy_parse (struct smtp_info *psmtp,
			 char *message, size_t length, size_t * index);

#endif
