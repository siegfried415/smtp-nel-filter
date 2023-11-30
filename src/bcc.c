#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "mmapstring.h"
#include "smtp.h"
#include "address.h"
#include "bcc.h"
#include "mime.h"


struct smtp_bcc *
smtp_bcc_new (struct smtp_address_list *bcc_addr_list)
{
	struct smtp_bcc *bcc;

	bcc = malloc (sizeof (*bcc));
	if (bcc == NULL)
		return NULL;
	DEBUG_SMTP (SMTP_MEM, "smtp_bcc_new: MALLOC pointer=%p\n", bcc);

	bcc->bcc_addr_list = bcc_addr_list;

	return bcc;
}


void
smtp_bcc_free (struct smtp_bcc *bcc)
{
	if (bcc->bcc_addr_list != NULL)
		smtp_address_list_free (bcc->bcc_addr_list);
	free (bcc);
	DEBUG_SMTP (SMTP_MEM, "smtp_bcc_free: FREE pointer=%p\n", bcc);
}

/*
bcc             =       "Bcc:" (address-list / [CFWS]) CRLF
*/


int
smtp_bcc_parse (const char *message, size_t length,
		size_t * index, struct smtp_bcc **result)
{
	struct smtp_address_list *addr_list;
	struct smtp_bcc *bcc;
	size_t cur_token;
	int r;
	int res;

	cur_token = *index;
	addr_list = NULL;

	r = smtp_address_list_parse (message, length, &cur_token, &addr_list);
	switch (r) {
	case SMTP_NO_ERROR:
		/* do nothing */
		break;
	case SMTP_ERROR_PARSE:
		r = smtp_cfws_parse (message, length, &cur_token);
		if ((r != SMTP_NO_ERROR) && (r != SMTP_ERROR_PARSE)) {
			res = r;
			goto err;
		}
		break;
	default:
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

	return SMTP_NO_ERROR;

free_addr_list:
	smtp_address_list_free (addr_list);
err:
	return res;
}
