

/*
 * $Id: received.c,v 1.6 2005/11/29 06:29:26 xiay Exp $
  RFC 2045, RFC 2046, RFC 2047, RFC 2048, RFC 2049, RFC 2231, RFC 2387
  RFC 2424, RFC 2557, RFC 2183 Content-Disposition, RFC 1766  Language
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "mmapstring.h"

#include "smtp.h"
#include "address.h"
#include "_time.h"
#include "received.h"
#include "mime.h"


struct smtp_received *
smtp_received_new (char *server)
{
	struct smtp_received *received;

	received = malloc (sizeof (*received));
	if (received == NULL)
		return NULL;
	DEBUG_SMTP (SMTP_MEM_1, "smtp_received_new: MALLOC pointer=%p\n",
		    received);

	received->pt_server_spec = server;

	return received;
}


void
smtp_received_free (struct smtp_received *received)
{
	free (received->pt_server_spec);
	DEBUG_SMTP (SMTP_MEM_1, "smtp_received_free: FREE pointer=%p\n",
		    received->pt_server_spec);
	free (received);
	DEBUG_SMTP (SMTP_MEM_1, "smtp_received_free: FREE pointer=%p\n",
		    received);
}


#if 1
/*
received        =       "Received:" unstructured CRLF
*/

int
smtp_received_parse (const char *message, size_t length,
		     size_t * index, struct smtp_received **result)
{
	struct smtp_received *received;
	char *value;
	size_t cur_token;
	int r;
	int res;

	cur_token = *index;

	//r = smtp_token_case_insensitive_parse(message, length,
	//                                       &cur_token, "Comments");
	//if (r != SMTP_NO_ERROR) {
	//  res = r;
	//  goto err;
	//}
	//
	//r = smtp_colon_parse(message, length, &cur_token);
	//if (r != SMTP_NO_ERROR) {
	//  res = r;
	//  goto err;
	//}

	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	r = smtp_unstructured_parse (message, length, &cur_token, &value);
	DEBUG_SMTP (SMTP_DBG, "RETURN VAL = %d\n", r);
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
	received = smtp_received_new (value);
	if (received == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free_value;
	}

	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	DEBUG_SMTP (SMTP_DBG, "RAIN message+cur_token = %s\n",
		    message + cur_token);
	*result = received;
	*index = cur_token;

	return SMTP_NO_ERROR;

      free_value:
	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	smtp_unstructured_free (value);
      err:
	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	return res;
}


#else
int
smtp_received_parse (const char *message, size_t length,
		     size_t * index, struct smtp_received **result)
{
	struct smtp_mailbox_list *mb_list;
	struct smtp_from *from;
	size_t cur_token;
	int r;
	int res;

	cur_token = *index;

	//r = smtp_token_case_insensitive_parse(message, length,
	//                                       &cur_token, "From");
	//if (r != SMTP_NO_ERROR) {
	//  res = r;
	//  goto err;
	//}

	//r = smtp_colon_parse(message, length, &cur_token);
	//if (r != SMTP_NO_ERROR) {
	//  res = r;
	//  goto err;
	//}

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

	from = smtp_from_new (mb_list);
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

#endif

/*
received        =       "Received:" name-val-list ";" date-time CRLF
*/

#if 0
int
smtp_received_parse (const char *message, size_t length,
		     size_t * index, struct smtp_received **result)
{
	size_t cur_token;
	struct smtp_received *received;
	struct smtp_name_val_list *name_val_list;
	struct smtp_date_time *date_time;
	int r;
	int res;

	cur_token = *index;

	r = smtp_token_case_insensitive_parse (message, length,
					       &cur_token, "Received");
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	r = smtp_colon_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	r = smtp_name_val_list_parse (message, length,
				      &cur_token, &name_val_list);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	r = smtp_semi_colon_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free_name_val_list;
	}

	r = smtp_date_time_parse (message, length, &cur_token, &date_time);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free_name_val_list;
	}

	r = smtp_unstrict_crlf_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free_date_time;
	}

	received = smtp_received_new (name_val_list, date_time);
	if (received == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free_date_time;
	}

	*index = cur_token;
	*result = received;

	return SMTP_NO_ERROR;

      free_date_time:
	smtp_date_time_free (date_time);
      free_name_val_list:
	smtp_name_val_list_free (name_val_list);
      err:
	return res;
}
#endif
