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
	DEBUG_SMTP (SMTP_MEM, "smtp_sender_new: FREE pointer=%p\n", sender);

	sender->snd_mb = snd_mb;

	return sender;
}


void
smtp_sender_free (struct smtp_sender *sender)
{
	if (sender->snd_mb != NULL)
		smtp_mailbox_free (sender->snd_mb);
	free (sender);
	DEBUG_SMTP (SMTP_MEM, "smtp_sender_free: FREE pointer=%p\n",
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
