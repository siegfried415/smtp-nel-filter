#ifndef TURN_H
#define TURN_H

struct smtp_cmd_turn
{
#ifdef USE_NEL
	NEL_REF_DEF
#endif
	int len;
	/* no parameters */
};

struct smtp_cmd_turn *smtp_cmd_turn_new (void);

int smtp_turn_parse (char *message, size_t length,
		     size_t * index, struct smtp_cmd_turn **result);

#endif
