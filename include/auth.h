#ifndef AUTH_H
#define AUTH_H

enum smtp_auth_type
{
	SMTP_AUTH_UNCERTAIN,
	SMTP_AUTH_CRAM_MD5,
	SMTP_AUTH_DIGEST_MD5,
	SMTP_AUTH_FOOBAR,
	SMTP_AUTH_GSSAPI,	//from Exchange
	SMTP_AUTH_NTLM,		//from Exchange
	SMTP_AUTH_LOGIN,
	SMTP_AUTH_PLAIN,
	SMTP_AUTH_TYPE_NUM
};

struct smtp_cmd_auth
{
#ifdef USE_NEL
	NEL_REF_DEF
#endif
	int len;
	int mech;
};


void smtp_cmd_auth_init (
#ifdef USE_NEL
				struct nel_eng *eng
#endif
	);

struct smtp_cmd_auth *smtp_cmd_auth_new (int len, int mech);

void smtp_cmd_auth_free (struct smtp_cmd_auth *auth);

int smtp_cmd_auth_parse (struct smtp_info *psmtp,
			 char *message, size_t length, size_t * index);

int smtp_ack_auth_parse (struct smtp_info *psmtp);


#endif
