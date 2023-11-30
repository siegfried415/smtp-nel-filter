#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "mmapstring.h"

#include "smtp.h"
#include "mime.h"
#include "msg-id.h"
#include "message-id.h"
#include "resent-msg-id.h"

/*
resent-msg-id   =       "Resent-Message-ID:" msg-id CRLF
*/

int
smtp_resent_msg_id_parse (const char *message, size_t length,
			  size_t * index, struct smtp_message_id **result)
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
