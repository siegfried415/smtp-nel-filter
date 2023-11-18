

/*
 * $Id: message-id.c,v 1.7 2005/11/29 06:29:26 xiay Exp $
  RFC 2045, RFC 2046, RFC 2047, RFC 2048, RFC 2049, RFC 2231, RFC 2387
  RFC 2424, RFC 2557, RFC 2183 Content-Disposition, RFC 1766  Language
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "mmapstring.h"

#include "smtp.h"
#include "msg-id.h"
#include "message-id.h"


#ifdef USE_NEL
#include "engine.h"
int message_id_id;
extern struct nel_eng *eng;
#endif


#define MAX_MESSAGE_ID 512

char *
smtp_get_message_id (void)
{
	char id[MAX_MESSAGE_ID];
	time_t now;
	char name[MAX_MESSAGE_ID];
	long value;

	now = time (NULL);
	value = random ();

	gethostname (name, MAX_MESSAGE_ID);
	snprintf (id, MAX_MESSAGE_ID, "etPan.%lx.%lx.%x@%s",
		  now, value, getpid (), name);

	return strdup (id);
}



struct smtp_message_id *
smtp_message_id_new (char *mid_value)
{
	struct smtp_message_id *message_id;

	message_id = malloc (sizeof (*message_id));
	if (message_id == NULL)
		return NULL;
	DEBUG_SMTP (SMTP_MEM_1, "smtp_message_id_new: MALLOC pointer=%p\n",
		    message_id);

	message_id->mid_value = mid_value;
	message_id->err_cnt = 0;

	return message_id;
}


void
smtp_message_id_free (struct smtp_message_id *message_id)
{
	if (message_id->mid_value != NULL)
		smtp_msg_id_free (message_id->mid_value);
	free (message_id);
	DEBUG_SMTP (SMTP_MEM_1, "smtp_message_id_free: FREE pointer=%p\n",
		    message_id);
}

/*
message-id      =       "Message-ID:" msg-id CRLF
*/


//todo, need add nel_env_analysis, wyong, 20231023 
int
smtp_message_id_parse (struct smtp_info *psmtp, const char *message,
		       size_t length, size_t * index,
		       struct smtp_message_id **result)
{
	char *value;
	size_t cur_token;
	struct smtp_message_id *message_id;
	int r;
	int res;

	cur_token = *index;

#if 0
	r = smtp_token_case_insensitive_parse (message, length,
					       &cur_token, "Message-ID");
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	r = smtp_colon_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}
#endif

	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	r = smtp_msg_id_parse (message, length, &cur_token, &value);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	r = smtp_unstrict_crlf_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free_value;
	}

	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	message_id = smtp_message_id_new (value);
	if (message_id == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free_value;
	}

	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	*result = message_id;
	*index = cur_token;

	return SMTP_NO_ERROR;

      free_value:
	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	smtp_msg_id_free (value);
      err:
	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	return res;
}
