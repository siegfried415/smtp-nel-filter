#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "mmapstring.h"
#include "smtp.h"
#include "mime.h"
#include "address.h"
#include "cc.h"
#include "resent-cc.h"

/*
resent-cc       =       "Resent-Cc:" address-list CRLF
*/

int
smtp_resent_cc_parse (const char *message, size_t length,
		      size_t * index, struct smtp_cc **result)
{
	struct smtp_address_list *addr_list;
	struct smtp_cc *cc;
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

	cc = smtp_cc_new (addr_list);
	if (cc == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free_addr_list;
	}

	*result = cc;
	*index = cur_token;

	return SMTP_NO_ERROR;

      free_addr_list:
	smtp_address_list_free (addr_list);
      err:
	return res;
}
