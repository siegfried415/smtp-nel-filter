
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "mmapstring.h"
#include "smtp.h"
#include "mime.h"
#include "address.h"
#include "reply-to.h"



struct smtp_reply_to *
smtp_reply_to_new (struct smtp_address_list *rt_addr_list)
{
	struct smtp_reply_to *reply_to;

	reply_to = malloc (sizeof (*reply_to));
	if (reply_to == NULL)
		return NULL;
	DEBUG_SMTP (SMTP_MEM, "smtp_reply_to_new: MALLOC pointer=%p\n",
		    reply_to);

	reply_to->rt_addr_list = rt_addr_list;

	return reply_to;
}


void
smtp_reply_to_free (struct smtp_reply_to *reply_to)
{
	if (reply_to->rt_addr_list != NULL)
		smtp_address_list_free (reply_to->rt_addr_list);
	free (reply_to);
	DEBUG_SMTP (SMTP_MEM, "smtp_reply_to_free: FREE pointer=%p\n",
		    reply_to);
}

/*
reply-to        =       "Reply-To:" address-list CRLF
*/


int
smtp_reply_to_parse (const char *message, size_t length,
		     size_t * index, struct smtp_reply_to **result)
{
	struct smtp_address_list *addr_list;
	struct smtp_reply_to *reply_to;
	size_t cur_token;
	int r;
	int res;

	cur_token = *index;

	r = smtp_address_list_parse (message, length, &cur_token, &addr_list);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	r = smtp_unstrict_crlf_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free_addr_list;
	}

	reply_to = smtp_reply_to_new (addr_list);
	if (reply_to == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free_addr_list;
	}

	*result = reply_to;
	*index = cur_token;

	return SMTP_NO_ERROR;

      free_addr_list:
	smtp_address_list_free (addr_list);
      err:
	return res;
}
