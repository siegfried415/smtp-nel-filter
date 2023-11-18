
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "smtp.h"
#include "mime.h"
#include "turn.h"

extern int var_disable_turn_cmd;
struct smtp_cmd_turn *
smtp_cmd_turn_new (void)
{
	struct smtp_cmd_turn *turn;
	turn = malloc (sizeof (struct smtp_cmd_turn));
	if (turn == NULL)
		return NULL;

	return turn;
}


int
smtp_turn_parse (char *message, size_t length, size_t * index,
		 struct smtp_cmd_turn **result)
{
	struct smtp_cmd_turn *turn = NULL;
	char *domain;
	size_t cur_token = *index;
	int r;
	int res;

	/* TURN has no paramters accroding to rfc2822 */
	if (length != 0) {
		r = smtp_cfws_parse (message, length, &cur_token);
		res = SMTP_ERROR_PARSE;
		if ((r != SMTP_NO_ERROR) && (r != SMTP_ERROR_PARSE)) {
			res = r;
		}
		goto err;
	}


	/* if nel configureable variable var_disable_turn_cmd 
	   is set to 0, don't allow the command pass through. wyong */
	if (var_disable_turn_cmd == 1) {
		res = SMTP_ERROR_POLICY;
		goto err;
	}

	turn = smtp_cmd_turn_new ();
	if (turn == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto err;
	}

	*result = turn;
	*index = cur_token;
	return SMTP_NO_ERROR;

      err:
	return res;
}
