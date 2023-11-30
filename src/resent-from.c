#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "smtp.h"
#include "mime.h"
#include "from.h"
#include "address.h"

/*
resent-from     =       "Resent-From:" mailbox-list CRLF
*/

int
smtp_resent_from_parse (const char *message, size_t length,
			size_t * index, struct smtp_msg_from **result)
{
	struct smtp_mailbox_list *mb_list;
	struct smtp_msg_from *from;
	size_t cur_token;
	int r;
	int res;

	cur_token = *index;

	r = smtp_mailbox_list_parse (message, length, &cur_token, &mb_list);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	r = smtp_unstrict_crlf_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free_mb_list;
	}

	from = smtp_msg_from_new (mb_list);
	if (from == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free_mb_list;
	}

	*result = from;
	*index = cur_token;

	return SMTP_NO_ERROR;

      free_mb_list:
	smtp_mailbox_list_free (mb_list);
      err:
	return res;
}
