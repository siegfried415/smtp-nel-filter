#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "mmapstring.h"
#include "smtp.h"
#include "mime.h"
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
	DEBUG_SMTP (SMTP_MEM, "smtp_message_id_new: MALLOC pointer=%p\n",
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
	DEBUG_SMTP (SMTP_MEM, "smtp_message_id_free: FREE pointer=%p\n",
		    message_id);
}

/*
message-id      =       "Message-ID:" msg-id CRLF
*/


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
	r = smtp_msg_id_parse (message, length, &cur_token, &value);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	r = smtp_unstrict_crlf_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free_value;
	}

	message_id = smtp_message_id_new (value);
	if (message_id == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free_value;
	}

	*result = message_id;
	*index = cur_token;

	return SMTP_NO_ERROR;

      free_value:
	smtp_msg_id_free (value);
      err:
	return res;
}
