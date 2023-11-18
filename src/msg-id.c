
/*
 * $Id: msg-id.c,v 1.3 2005/11/29 06:29:26 xiay Exp $
  RFC 2045, RFC 2046, RFC 2047, RFC 2048, RFC 2049, RFC 2231, RFC 2387
  RFC 2424, RFC 2557, RFC 2183 Content-Disposition, RFC 1766  Language
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "mmapstring.h"

#include "smtp.h"
#include "mime.h"
#include "msg-id.h"

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
	DEBUG_SMTP (SMTP_MEM_1, "smtp_msg_id_free: FREE pointer=%p\n",
		    msg_id);
}

void
smtp_id_left_free (char *id_left)
{
	free (id_left);
	DEBUG_SMTP (SMTP_MEM_1, "smtp_id_left_free: FREE pointer=%p\n",
		    id_left);
}

void
smtp_id_right_free (char *id_right)
{
	free (id_right);
	DEBUG_SMTP (SMTP_MEM_1, "smtp_id_right_free: FREE pointer=%p\n",
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
#if 0
	char *id_left;
	char *id_right;
#endif
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
		DEBUG_SMTP (SMTP_MEM_1,
			    "smtp_msg_id_parse: FREE pointer=%p\n", msg_id);
		res = r;
		goto err;
	}

#if 0
	r = smtp_id_left_parse (message, length, &cur_token, &id_left);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	r = smtp_at_sign_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free_id_left;
	}

	r = smtp_id_right_parse (message, length, &cur_token, &id_right);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free_id_left;
	}

	r = smtp_greater_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free_id_right;
	}

	msg_id = malloc (strlen (id_left) + strlen (id_right) + 2);
	if (msg_id == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free_id_right;
	}
	DEBUG_SMTP (SMTP_MEM_1, "smtp_msg_id_parse: FREE pointer=%p\n",
		    msg_id);
	strcpy (msg_id, id_left);
	strcat (msg_id, "@");
	strcat (msg_id, id_right);

	smtp_id_left_free (id_left);
	smtp_id_right_free (id_right);
#endif

	*result = msg_id;
	*index = cur_token;

	return SMTP_NO_ERROR;

#if 0
      free_id_right:
	smtp_id_right_free (id_right);
      free_id_left:
	smtp_id_left_free (id_left);
#endif
	/*
	   free:
	   smtp_atom_free(msg_id);
	 */
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
