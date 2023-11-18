#ifndef SAML_FROM_H
#define SAML_FROM_H

struct smtp_cmd_saml
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

void smtp_cmd_saml_init (
#ifdef USE_NEL
				struct nel_eng *eng
#endif
	);

int smtp_ack_saml_parse (struct smtp_info *psmtp);

void smtp_cmd_saml_free (struct smtp_cmd_saml *saml);

struct smtp_cmd_saml *smtp_cmd_saml_new (int len, char *addr);

int smtp_cmd_saml_parse (struct smtp_info *psmtp,
			 char *message, size_t length, size_t * index);

#endif
