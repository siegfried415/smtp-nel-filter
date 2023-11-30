#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "mmapstring.h"
#include "smtp.h"
#include "mime.h"
#include "address.h"
#include "bcc.h"
#include "resent-bcc.h"

/*
resent-bcc      =       "Resent-Bcc:" (address-list / [CFWS]) CRLF
*/

int
smtp_resent_bcc_parse (const char *message, size_t length,
		       size_t * index, struct smtp_bcc **result)
{
	struct smtp_address_list *addr_list;
	struct smtp_bcc *bcc;
	size_t cur_token;
	int r;
	int res;

	cur_token = *index;
	bcc = NULL;

	r = smtp_address_list_parse (message, length, &cur_token, &addr_list);
	if ((r != SMTP_NO_ERROR) && (r != SMTP_ERROR_PARSE)) {
		res = r;
		goto err;
	}

	r = smtp_unstrict_crlf_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free_addr_list;
	}

	bcc = smtp_bcc_new (addr_list);
	if (bcc == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free_addr_list;
	}

	*result = bcc;
	*index = cur_token;

	return TRUE;

      free_addr_list:
	smtp_address_list_free (addr_list);
      err:
	return res;
}
