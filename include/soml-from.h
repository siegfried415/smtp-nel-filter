#ifndef SOML_FROM_H
#define SOML_FROM_H

struct smtp_cmd_soml
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

void smtp_cmd_soml_init (
#ifdef USE_NEL
				struct nel_eng *eng
#endif
	);

int smtp_ack_soml_parse (struct smtp_info *psmtp);

void smtp_cmd_soml_free (struct smtp_cmd_soml *soml);

struct smtp_cmd_soml *smtp_cmd_soml_new (int len, char *addr);

int smtp_cmd_soml_parse (struct smtp_info *psmtp,
			 char *message, size_t length, size_t * index);

#endif
