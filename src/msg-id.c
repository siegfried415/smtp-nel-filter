
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "mmapstring.h"
#include "smtp.h"
#include "mime.h"
#include "msg-id.h"
#include "address.h" 

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif


void
smtp_mime_id_free (char *id)
{
	smtp_msg_id_free (id);
}

void
smtp_msg_id_free (char *msg_id)
{
	free (msg_id);
	DEBUG_SMTP (SMTP_MEM, "smtp_msg_id_free: FREE pointer=%p\n",
		    msg_id);
}

void
smtp_id_left_free (char *id_left)
{
	free (id_left);
	DEBUG_SMTP (SMTP_MEM, "smtp_id_left_free: FREE pointer=%p\n",
		    id_left);
}

void
smtp_id_right_free (char *id_right)
{
	free (id_right);
	DEBUG_SMTP (SMTP_MEM, "smtp_id_right_free: FREE pointer=%p\n",
		    id_right);
}


/*
msg-id          =       [CFWS] "<" id-left "@" id-right ">" [CFWS]
*/

int
smtp_msg_id_parse (const char *message, size_t length,
		   size_t * index, char **result)
{
	size_t cur_token;
	char *msg_id;
	int r;
	int res;

	cur_token = *index;

	r = smtp_cfws_parse (message, length, &cur_token);
	if ((r != SMTP_NO_ERROR) && (r != SMTP_ERROR_PARSE))
		return r;

	r = smtp_lower_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	r = smtp_addr_spec_parse (message, length, &cur_token, &msg_id);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	r = smtp_greater_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		free (msg_id);
		DEBUG_SMTP (SMTP_MEM,
			    "smtp_msg_id_parse: FREE pointer=%p\n", msg_id);
		res = r;
		goto err;
	}


	*result = msg_id;
	*index = cur_token;

	return SMTP_NO_ERROR;

      err:
	return res;
}

int
smtp_parse_unwanted_msg_id (const char *message, size_t length,
			    size_t * index)
{
	size_t cur_token;
	int r;
	char *word;
	int token_parsed;

	cur_token = *index;

	token_parsed = TRUE;
	while (token_parsed) {
		token_parsed = FALSE;
		r = smtp_word_parse (message, length, &cur_token, &word);
		if (r == SMTP_NO_ERROR) {
			smtp_word_free (word);
			token_parsed = TRUE;
		}
		else if (r == SMTP_ERROR_PARSE) {
			/* do nothing */
		}
		else
			return r;
		r = smtp_semi_colon_parse (message, length, &cur_token);
		if (r == SMTP_NO_ERROR)
			token_parsed = TRUE;
		else if (r == SMTP_ERROR_PARSE) {
			/* do nothing */
		}
		else
			return r;
		r = smtp_comma_parse (message, length, &cur_token);
		if (r == SMTP_NO_ERROR)
			token_parsed = TRUE;
		else if (r == SMTP_ERROR_PARSE) {
			/* do nothing */
		}
		else
			return r;
		r = smtp_plus_parse (message, length, &cur_token);
		if (r == SMTP_NO_ERROR)
			token_parsed = TRUE;
		else if (r == SMTP_ERROR_PARSE) {
			/* do nothing */
		}
		else
			return r;
		r = smtp_colon_parse (message, length, &cur_token);
		if (r == SMTP_NO_ERROR)
			token_parsed = TRUE;
		else if (r == SMTP_ERROR_PARSE) {
			/* do nothing */
		}
		else
			return r;
		r = smtp_point_parse (message, length, &cur_token);
		if (r == SMTP_NO_ERROR)
			token_parsed = TRUE;
		else if (r == SMTP_ERROR_PARSE) {
			/* do nothing */
		}
		else
			return r;
		r = smtp_at_sign_parse (message, length, &cur_token);
		if (r == SMTP_NO_ERROR)
			token_parsed = TRUE;
		else if (r == SMTP_ERROR_PARSE) {
			/* do nothing */
		}
		else
			return r;
	}

	return SMTP_NO_ERROR;
}

int
smtp_unstrict_msg_id_parse (const char *message, size_t length,
			    size_t * index, char **result)
{
	char *msgid;
	size_t cur_token;
	int r;

	cur_token = *index;

	r = smtp_cfws_parse (message, length, &cur_token);
	if ((r != SMTP_NO_ERROR) && (r != SMTP_ERROR_PARSE))
		return r;

	r = smtp_parse_unwanted_msg_id (message, length, &cur_token);
	if (r != SMTP_NO_ERROR)
		return r;

	r = smtp_msg_id_parse (message, length, &cur_token, &msgid);
	if (r != SMTP_NO_ERROR)
		return r;

	r = smtp_parse_unwanted_msg_id (message, length, &cur_token);
	if (r != SMTP_NO_ERROR)
		return r;

	*result = msgid;
	*index = cur_token;

	return SMTP_NO_ERROR;
}
