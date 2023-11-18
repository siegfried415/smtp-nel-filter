#ifndef QUIT_H
#define QUIT_H

struct smtp_cmd_quit
{
#ifdef USE_NEL
	NEL_REF_DEF
#endif
	int len;
	/* no parameters */
};

void smtp_cmd_quit_init (
#ifdef USE_NEL
				struct nel_eng *eng
#endif
	);

int smtp_ack_quit_parse (struct smtp_info *psmtp);

void smtp_cmd_quit_free (struct smtp_cmd_quit *quit);

struct smtp_cmd_quit *smtp_cmd_quit_new (int len);

int smtp_cmd_quit_parse (struct smtp_info *psmtp,
			 char *message, size_t length, size_t * index);

#endif
