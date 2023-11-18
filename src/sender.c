


/*
 * $Id: sender.c,v 1.4 2005/11/29 06:29:26 xiay Exp $
  RFC 2045, RFC 2046, RFC 2047, RFC 2048, RFC 2049, RFC 2231, RFC 2387
  RFC 2424, RFC 2557, RFC 2183 Content-Disposition, RFC 1766  Language
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "mmapstring.h"

#include "smtp.h"
#include "address.h"
#include "sender.h"
#include "mime.h"


struct smtp_sender *
smtp_sender_new (struct smtp_mailbox *snd_mb)
{
	struct smtp_sender *sender;

	sender = malloc (sizeof (*sender));
	if (sender == NULL)
		return NULL;
	DEBUG_SMTP (SMTP_MEM_1, "smtp_sender_new: FREE pointer=%p\n", sender);

	sender->snd_mb = snd_mb;

	return sender;
}


void
smtp_sender_free (struct smtp_sender *sender)
{
	if (sender->snd_mb != NULL)
		smtp_mailbox_free (sender->snd_mb);
	free (sender);
	DEBUG_SMTP (SMTP_MEM_1, "smtp_sender_free: FREE pointer=%p\n",
		    sender);
}

/*
sender          =       "Sender:" mailbox CRLF
*/

int
smtp_sender_parse (const char *message, size_t length,
		   size_t * index, struct smtp_sender **result)
{
	struct smtp_mailbox *mb;
	struct smtp_sender *sender;
	size_t cur_token;
	int r;
	int res;

	cur_token = *index;

#if 0				//xiayu 2005.11.18 commented
	r = smtp_token_case_insensitive_parse (message, length,
					       &cur_token, "Sender");
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
	r = smtp_mailbox_parse (message, length, &cur_token, &mb);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	r = smtp_unstrict_crlf_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free_mb;
	}

	sender = smtp_sender_new (mb);
	if (sender == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free_mb;
	}

	*result = sender;
	*index = cur_token;

	return SMTP_NO_ERROR;

      free_mb:
	smtp_mailbox_free (mb);
      err:
	return res;
}
