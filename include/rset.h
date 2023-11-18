#ifndef RSET_H
#define RSET_H

struct smtp_cmd_rset
{
#ifdef USE_NEL
	NEL_REF_DEF
#endif
	int len;
	/* no parameters */
};

void smtp_cmd_rset_init (
#ifdef USE_NEL
				struct nel_eng *eng
#endif
	);

int smtp_ack_rset_parse (struct smtp_info *psmtp);

void smtp_cmd_rset_free (struct smtp_cmd_rset *rset);

struct smtp_cmd_rset *smtp_cmd_rset_new (int len);

int smtp_cmd_rset_parse (struct smtp_info *psmtp,
			 char *message, size_t length, size_t * index);

#endif
