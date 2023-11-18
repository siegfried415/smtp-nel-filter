#ifndef MAIL_FROM_H
#define MAIL_FROM_H

enum smtp_mail_param_type
{
	SMTP_MAIL_NO_PARAM,
	SMTP_MAIL_SIZE,
	SMTP_MAIL_BODY,
	SMTP_MAIL_TRANSID,
	SMTP_MAIL_RET,
	SMTP_MAIL_BY,
	SMTP_MAIL_ENVID,
	SMTP_MAIL_SOLICIT,
	SMTP_MAIL_MTRK,
	SMTP_MAIL_AUTH,
};

struct smtp_cmd_mail
{
#ifdef USE_NEL
	NEL_REF_DEF
#endif
	int len;
	short name_len;
	char *name;
	short addr_len;
	char *addr;
	int key;		//SIZE, TRANSID, RET, ENVID,
	// SOLICIT, MTRK, AUTH
};

void smtp_cmd_mail_init (
#ifdef USE_NEL
				struct nel_eng *eng
#endif
	);

int smtp_mail_size_parse (char *message,
			  size_t length, int *digit_len, int *digit_val);

void smtp_mail_reset (struct smtp_info *psmtp);

int smtp_ack_mail_parse (struct smtp_info *psmtp);

void smtp_cmd_mail_free (struct smtp_cmd_mail *mail);

struct smtp_cmd_mail *smtp_cmd_mail_new (int len, char *addr, int key);

int smtp_cmd_mail_parse (struct smtp_info *psmtp,
			 char *message, size_t length, size_t * index);

#endif
