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
	DEBUG_SMTP (SMTP_MEM, "smtp_received_new: MALLOC pointer=%p\n",
		    received);

	received->pt_server_spec = server;

	return received;
}


void
smtp_received_free (struct smtp_received *received)
{
	free (received->pt_server_spec);
	DEBUG_SMTP (SMTP_MEM, "smtp_received_free: FREE pointer=%p\n",
		    received->pt_server_spec);
	free (received);
	DEBUG_SMTP (SMTP_MEM, "smtp_received_free: FREE pointer=%p\n",
		    received);
}


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
	r = smtp_unstructured_parse (message, length, &cur_token, &value);
	DEBUG_SMTP (SMTP_DBG, "RETURN VAL = %d\n", r);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	r = smtp_unstrict_crlf_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free_value;
	}

	received = smtp_received_new (value);
	if (received == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free_value;
	}

	DEBUG_SMTP (SMTP_DBG, "RAIN message+cur_token = %s\n",
		    message + cur_token);
	*result = received;
	*index = cur_token;

	return SMTP_NO_ERROR;

      free_value:
	smtp_unstructured_free (value);
      err:
	return res;
}

