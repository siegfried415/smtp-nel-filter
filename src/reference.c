
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "mmapstring.h"
#include "smtp.h"
#include "mime.h"
#include "reference.h"
#include "msg-id.h"


struct smtp_references *
smtp_references_new (clist * mid_list)
{
	struct smtp_references *ref;

	ref = malloc (sizeof (*ref));
	if (ref == NULL)
		return NULL;
	DEBUG_SMTP (SMTP_MEM, "smtp_references_new: MALLOC pointer=%p\n",
		    ref);

	ref->mid_list = mid_list;

	return ref;
}


void
smtp_references_free (struct smtp_references *references)
{
	clist_foreach (references->mid_list,
		       (clist_func) smtp_msg_id_free, NULL);
	clist_free (references->mid_list);
	free (references);
	DEBUG_SMTP (SMTP_MEM, "smtp_references_free: FREE pointer=%p\n",
		    references);
}

/*
references      =       "References:" 1*msg-id CRLF
*/

int
smtp_references_parse (const char *message, size_t length,
		       size_t * index, struct smtp_references **result)
{
	struct smtp_references *references;
	size_t cur_token;
	clist *msg_id_list;
	int r;
	int res;

	cur_token = *index;
	r = smtp_msg_id_list_parse (message, length, &cur_token,
				    &msg_id_list);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	r = smtp_unstrict_crlf_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free_list;
	}

	references = smtp_references_new (msg_id_list);
	if (references == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free_list;
	}

	*result = references;
	*index = cur_token;

	return SMTP_NO_ERROR;

      free_list:
	clist_foreach (msg_id_list, (clist_func) smtp_msg_id_free, NULL);
	clist_free (msg_id_list);
      err:
	return res;
}
