
/*
 * $Id: fields.re,v 1.19 2005/12/02 08:29:59 xiay Exp $
  RFC 2045, RFC 2046, RFC 2047, RFC 2048, RFC 2049, RFC 2231, RFC 2387
  RFC 2424, RFC 2557, RFC 2183 Content-Disposition, RFC 1766  Language
 */

#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "mmapstring.h"
#include "mem_pool.h"

#include "smtp.h"
#include "fields.h"
#include "disposition.h"
#include "mime.h"
#include "language.h"
#include "version.h"
#include "encoding.h"
#include "content-type.h"
#include "reference.h"
#include "_time.h"
#include "sender.h"
#include "reply-to.h"
#include "cc.h"
#include "resent-cc.h"
#include "bcc.h"
#include "resent-bcc.h"
#include "message-id.h"
#include "resent-msg-id.h"
#include "in_reply_to.h"
#include "subject.h"
#include "comments.h"
#include "keywords.h"
#include "return.h"
//#include "path.h" 

#include "from.h"
#include "to.h"
#include "message.h"
#include "address.h"

#ifdef USE_NEL
#include "engine.h"

int eoh_id;
extern struct nel_eng *eng;
#endif

int
get_body_type_from_fields (struct smtp_mime_fields *fields)
{
	clistiter *cur;
	//xiayu 2005.12.9 intiate the body_type
	int body_type = SMTP_MIME_NONE;

	for (cur = clist_begin (fields->fld_list); cur != NULL;
	     cur = clist_next (cur)) {
		struct smtp_mime_field *field = clist_content (cur);
		DEBUG_SMTP (SMTP_DBG, "\n");
		if (field->fld_type == SMTP_MIME_FIELD_TYPE) {
			body_type =
				get_body_type_from_content_type (field->
								 fld_data.
								 fld_content_type);
			DEBUG_SMTP (SMTP_DBG, "body_type = %d\n", body_type);
			break;
		}
	}

	return body_type;

}

struct smtp_mime_content_type *
get_content_type_from_fields (struct smtp_mime_fields *fields)
{
	clistiter *cur;
	for (cur = clist_begin (fields->fld_list); cur != NULL;
	     cur = clist_next (cur)) {
		struct smtp_mime_field *field = clist_content (cur);
		if (field->fld_type == SMTP_MIME_FIELD_TYPE) {
			return (field->fld_data.fld_content_type);
		}
	}

	return NULL;

}

enum
{
	UNSTRUCTURED_START,
	UNSTRUCTURED_CR,
	UNSTRUCTURED_LF,
	UNSTRUCTURED_WSP,
	UNSTRUCTURED_OUT
};



int
smtp_ignore_field_parse (const char *message, size_t length, size_t * index)
{
	int has_field;
	size_t cur_token;
	int state;
	size_t terminal;

	has_field = FALSE;
	cur_token = *index;

	terminal = cur_token;
	state = UNSTRUCTURED_START;

	/* check if this is not a beginning CRLF */

	//xiayu 2005.11.16
	if (cur_token >= length)
		return SMTP_ERROR_CONTINUE;
	if (cur_token >= length)
		return SMTP_ERROR_PARSE;

	switch (message[cur_token]) {
	case '\r':
		return SMTP_ERROR_PARSE;
	case '\n':
		return SMTP_ERROR_PARSE;
	}

	while (state != UNSTRUCTURED_OUT) {

		switch (state) {
		case UNSTRUCTURED_START:
			//xiayu 2005.11.16
			if (cur_token >= length)
				return SMTP_ERROR_CONTINUE;
			if (cur_token >= length)
				return SMTP_ERROR_PARSE;

			switch (message[cur_token]) {
			case '\r':
				state = UNSTRUCTURED_CR;
				break;
			case '\n':
				state = UNSTRUCTURED_LF;
				break;
			case ':':
				has_field = TRUE;
				state = UNSTRUCTURED_START;
				break;
			default:
				state = UNSTRUCTURED_START;
				break;
			}
			break;
		case UNSTRUCTURED_CR:
			//xiayu 2005.11.16
			if (cur_token >= length)
				return SMTP_ERROR_CONTINUE;
			if (cur_token >= length)
				return SMTP_ERROR_PARSE;

			switch (message[cur_token]) {
			case '\n':
				state = UNSTRUCTURED_LF;
				break;
			case ':':
				has_field = TRUE;
				state = UNSTRUCTURED_START;
				break;
			default:
				state = UNSTRUCTURED_START;
				break;
			}
			break;
		case UNSTRUCTURED_LF:
			if (cur_token >= length) {
				terminal = cur_token;
				state = UNSTRUCTURED_OUT;
				break;
			}

			switch (message[cur_token]) {
			case '\t':
			case ' ':
				state = UNSTRUCTURED_WSP;
				break;
			default:
				terminal = cur_token;
				state = UNSTRUCTURED_OUT;
				break;
			}
			break;
		case UNSTRUCTURED_WSP:
			//xiayu 2005.11.16
			if (cur_token >= length)
				return SMTP_ERROR_CONTINUE;
			if (cur_token >= length)
				return SMTP_ERROR_PARSE;

			switch (message[cur_token]) {
			case '\r':
				state = UNSTRUCTURED_CR;
				break;
			case '\n':
				state = UNSTRUCTURED_LF;
				break;
			case ':':
				has_field = TRUE;
				state = UNSTRUCTURED_START;
				break;
			default:
				state = UNSTRUCTURED_START;
				break;
			}
			break;
		}

		cur_token++;
	}

	if (!has_field)
		return SMTP_ERROR_PARSE;

	*index = terminal;

	return SMTP_NO_ERROR;
}

/*
field-name      =       1*ftext
*/
int
smtp_field_name_parse (const char *message, size_t length,
		       size_t * index, char **result)
{
	char *field_name;
	size_t cur_token;
	size_t end;

	cur_token = *index;

	end = cur_token;
	//xiayu 2005.11.16
	if (end >= length)
		return SMTP_ERROR_CONTINUE;
	if (end >= length)
		return SMTP_ERROR_PARSE;

	while (is_ftext (message[end])) {
		end++;
		if (end >= length)
			break;
	}
	if (end == cur_token) {
		return SMTP_ERROR_PARSE;
	}

	/*  field_name = strndup(message + cur_token, end - cur_token); */
	field_name = malloc (end - cur_token + 1);
	if (field_name == NULL) {
		return SMTP_ERROR_MEMORY;
	}
	DEBUG_SMTP (SMTP_MEM_1, "smtp_field_name_parse: MALLOC pointer=%p\n",
		    field_name);

	strncpy (field_name, message + cur_token, end - cur_token);
	field_name[end - cur_token] = '\0';

	cur_token = end;

	*index = cur_token;
	*result = field_name;

	return SMTP_NO_ERROR;
}

void
smtp_mime_content_type_single_fields_init (struct smtp_mime_single_fields
					   *single_fields,
					   struct smtp_mime_content_type
					   *fld_content_type)
{
	clistiter *cur;

	single_fields->fld_content_type = fld_content_type;

	for (cur = clist_begin (fld_content_type->ct_parameters);
	     cur != NULL; cur = clist_next (cur)) {
		struct smtp_mime_parameter *param;

		param = clist_content (cur);

		if (strcasecmp (param->pa_name, "boundary") == 0)
			single_fields->fld_content_boundary = param->pa_value;

		if (strcasecmp (param->pa_name, "charset") == 0)
			single_fields->fld_content_charset = param->pa_value;

		if (strcasecmp (param->pa_name, "name") == 0)
			single_fields->fld_content_name = param->pa_value;

		//xiayu 2005.11.17 added
		if (strcasecmp (param->pa_name, "format") == 0)
			single_fields->fld_content_name = param->pa_value;
	}
}

void
smtp_mime_single_fields_init (struct smtp_mime_single_fields *single_fields,
			      struct smtp_mime_fields *fld_fields,
			      struct smtp_mime_content_type *fld_content_type)
{
	clistiter *cur;

	memset (single_fields, 0, sizeof (struct smtp_mime_single_fields));

	if (fld_content_type != NULL)
		smtp_mime_content_type_single_fields_init (single_fields,
							   fld_content_type);

	if (fld_fields == NULL)
		return;

	for (cur = clist_begin (fld_fields->fld_list); cur != NULL;
	     cur = clist_next (cur)) {
		struct smtp_mime_field *field;

		field = clist_content (cur);

		switch (field->fld_type) {
		case SMTP_MIME_FIELD_TYPE:
			smtp_mime_content_type_single_fields_init
				(single_fields,
				 field->fld_data.fld_content_type);
			break;

		case SMTP_MIME_FIELD_TRANSFER_ENCODING:
			single_fields->fld_encoding =
				field->fld_data.fld_encoding;
			break;

		case SMTP_MIME_FIELD_ID:
			single_fields->fld_id = field->fld_data.fld_id;
			break;

		case SMTP_MIME_FIELD_DESCRIPTION:
			single_fields->fld_description =
				field->fld_data.fld_description;
			break;

		case SMTP_MIME_FIELD_VERSION:
			single_fields->fld_version =
				field->fld_data.fld_version;
			break;

		case SMTP_MIME_FIELD_DISPOSITION:
			smtp_mime_disposition_single_fields_init
				(single_fields,
				 field->fld_data.fld_disposition);
			break;

		case SMTP_MIME_FIELD_LANGUAGE:
			single_fields->fld_language =
				field->fld_data.fld_language;
			break;
		}
	}
}

struct smtp_mime_single_fields *
smtp_mime_single_fields_new (struct smtp_mime_fields *fld_fields,
			     struct smtp_mime_content_type *fld_content_type)
{
	struct smtp_mime_single_fields *single_fields;

	single_fields = malloc (sizeof (struct smtp_mime_single_fields));
	if (single_fields == NULL)
		goto err;
	DEBUG_SMTP (SMTP_MEM_1,
		    "smtp_mime_single_fields_new: MALLOC pointer=%p\n",
		    single_fields);

	smtp_mime_single_fields_init (single_fields, fld_fields,
				      fld_content_type);

	return single_fields;

      err:
	return NULL;
}


void
smtp_mime_single_fields_free (struct smtp_mime_single_fields *single_fields)
{
	free (single_fields);
	DEBUG_SMTP (SMTP_MEM_1,
		    "smtp_mime_single_fields_free: FREE pointer=%p\n",
		    single_fields);
}

struct smtp_mime_fields *
smtp_mime_fields_new_filename (int dsp_type,
			       char *filename, int encoding_type)
{
	struct smtp_mime_disposition *dsp;
	struct smtp_mime_mechanism *encoding;
	struct smtp_mime_fields *mime_fields;

	dsp = smtp_mime_disposition_new_with_data (dsp_type,
						   filename, NULL, NULL, NULL,
						   (size_t) - 1);
	if (dsp == NULL)
		goto err;

	encoding = smtp_mime_mechanism_new (encoding_type, NULL);
	if (encoding == NULL)
		goto free_dsp;

	mime_fields = smtp_mime_fields_new_with_data (encoding,
						      NULL, NULL, dsp, NULL);
	if (mime_fields == NULL)
		goto free_encoding;

	return mime_fields;

      free_encoding:
	smtp_mime_encoding_free (encoding);
      free_dsp:
	smtp_mime_disposition_free (dsp);
      err:
	return NULL;
}

/* create MIME fields with only the field Content-Transfer-Encoding */

struct smtp_mime_fields *
smtp_mime_fields_new_encoding (int type)
{
	struct smtp_mime_mechanism *encoding;
	struct smtp_mime_fields *mime_fields;

	encoding = smtp_mime_mechanism_new (type, NULL);
	if (encoding == NULL)
		goto err;

	mime_fields = smtp_mime_fields_new_with_data (encoding,
						      NULL, NULL, NULL, NULL);
	if (mime_fields == NULL)
		goto free;

	return mime_fields;

      free:
	smtp_mime_mechanism_free (encoding);
      err:
	return NULL;
}

struct smtp_mime_fields *
smtp_mime_fields_new_empty (void)
{
	clist *list;
	struct smtp_mime_fields *fields;

	list = clist_new ();
	if (list == NULL)
		goto err;

	fields = smtp_mime_fields_new (list);
	if (fields == NULL)
		goto free;

	return fields;

      free:
	clist_free (list);
      err:
	return NULL;
}

int
smtp_mime_fields_add (struct smtp_mime_fields *fields,
		      struct smtp_mime_field *field)
{
	int r;

	r = clist_append (fields->fld_list, field);
	if (r < 0)
		return SMTP_ERROR_MEMORY;

	return SMTP_NO_ERROR;
}

void
smtp_mime_field_detach (struct smtp_mime_field *field)
{
	switch (field->fld_type) {
	case SMTP_MIME_FIELD_TYPE:
		field->fld_data.fld_content_type = NULL;
		break;
	case SMTP_MIME_FIELD_TRANSFER_ENCODING:
		field->fld_data.fld_encoding = NULL;
		break;
	case SMTP_MIME_FIELD_ID:
		field->fld_data.fld_id = NULL;
		break;
	case SMTP_MIME_FIELD_DESCRIPTION:
		field->fld_data.fld_description = NULL;
		break;
	case SMTP_MIME_FIELD_DISPOSITION:
		field->fld_data.fld_disposition = NULL;
		break;
	case SMTP_MIME_FIELD_LANGUAGE:
		field->fld_data.fld_language = NULL;
		break;
	}
}

struct smtp_mime_fields *
smtp_mime_fields_new_with_data (struct smtp_mime_mechanism *encoding,
				char *id,
				char *description,
				struct smtp_mime_disposition *disposition,
				struct smtp_mime_language *language)
{
	struct smtp_mime_field *field;
	struct smtp_mime_fields *fields;
	int r;

	fields = smtp_mime_fields_new_empty ();
	if (fields == NULL)
		goto err;

#if 0
	if (content != NULL) {
		field = smtp_mime_field_new (SMTP_MIME_FIELD_TYPE,
					     content, NULL, NULL, NULL, 0,
					     NULL, NULL);
		if (field == NULL)
			goto free;

		r = smtp_mime_fields_add (fields, field);
		if (r != SMTP_NO_ERROR) {
			smtp_mime_field_detach (field);
			smtp_mime_field_free (field);
			goto free;
		}
	}
#endif

	if (encoding != NULL) {
		field = smtp_mime_field_new
			(SMTP_MIME_FIELD_TRANSFER_ENCODING, encoding);
		if (field == NULL)
			goto free;

		r = smtp_mime_fields_add (fields, field);
		if (r != SMTP_NO_ERROR) {
			smtp_mime_field_detach (field);
			smtp_mime_field_free (field);
			goto free;
		}
	}

	if (id != NULL) {
		field = smtp_mime_field_new (SMTP_MIME_FIELD_ID, id);
		if (field == NULL)
			goto free;

		r = smtp_mime_fields_add (fields, field);
		if (r != SMTP_NO_ERROR) {
			smtp_mime_field_detach (field);
			smtp_mime_field_free (field);
			goto free;
		}
	}

	if (description != NULL) {
		field = smtp_mime_field_new (SMTP_MIME_FIELD_DESCRIPTION,
					     description);
		if (field == NULL)
			goto free;

		r = smtp_mime_fields_add (fields, field);
		if (r != SMTP_NO_ERROR) {
			smtp_mime_field_detach (field);
			smtp_mime_field_free (field);
			goto free;
		}
	}

	if (disposition != NULL) {
		field = smtp_mime_field_new (SMTP_MIME_FIELD_DISPOSITION,
					     disposition);
		if (field == NULL)
			goto free;

		r = smtp_mime_fields_add (fields, field);
		if (r != SMTP_NO_ERROR) {
			smtp_mime_field_detach (field);
			smtp_mime_field_free (field);
			goto free;
		}
	}

	if (language != NULL) {
		field = smtp_mime_field_new (SMTP_MIME_FIELD_LANGUAGE,
					     language);
		if (field == NULL)
			goto free;

		r = smtp_mime_fields_add (fields, field);
		if (r != SMTP_NO_ERROR) {
			smtp_mime_field_detach (field);
			smtp_mime_field_free (field);
			goto free;
		}
	}

	return fields;

      free:
	clist_foreach (fields->fld_list, (clist_func) smtp_mime_field_detach,
		       NULL);
	smtp_mime_fields_free (fields);
      err:
	return NULL;
}

struct smtp_mime_fields *
smtp_mime_fields_new_with_version (struct smtp_mime_mechanism *encoding,
				   char *id,
				   char *description,
				   struct smtp_mime_disposition *disposition,
				   struct smtp_mime_language *language)
{
	struct smtp_mime_field *field;
	struct smtp_mime_fields *fields;
	int r;

	fields = smtp_mime_fields_new_with_data (encoding, id, description,
						 disposition, language);
	if (fields == NULL)
		goto err;

	field = smtp_mime_field_new (SMTP_MIME_FIELD_VERSION, MIME_VERSION);
	if (field == NULL)
		goto free;

	r = smtp_mime_fields_add (fields, field);
	if (r != SMTP_NO_ERROR) {
		smtp_mime_field_detach (field);
		smtp_mime_field_free (field);
		goto free;
	}

	return fields;

      free:
	clist_foreach (fields->fld_list, (clist_func) smtp_mime_field_detach,
		       NULL);
	smtp_mime_fields_free (fields);
      err:
	return NULL;
}

struct smtp_optional_field *
smtp_optional_field_new (char *fld_name, char *fld_value)
{
	struct smtp_optional_field *opt_field;

	opt_field = malloc (sizeof (*opt_field));
	if (opt_field == NULL)
		return NULL;
	DEBUG_SMTP (SMTP_MEM_1,
		    "smtp_optional_field_new: MALLOC pointer=%p\n",
		    opt_field);

	opt_field->fld_name = fld_name;
	opt_field->fld_value = fld_value;

	return opt_field;
}


void
smtp_optional_field_free (struct smtp_optional_field *opt_field)
{
	smtp_field_name_free (opt_field->fld_name);
	smtp_unstructured_free (opt_field->fld_value);
	free (opt_field);
	DEBUG_SMTP (SMTP_MEM_1, "smtp_optional_field_free: FREE pointer=%p\n",
		    opt_field);
}

void
smtp_field_name_free (char *field_name)
{
	free (field_name);
	DEBUG_SMTP (SMTP_MEM_1, "smtp_field_name_free: FREE pointer=%p\n",
		    field_name);
}

time_t mkgmtime (struct tm *tmp);


struct smtp_date_time *
smtp_get_current_date (void)
{
	struct tm gmt;
	struct tm lt;
	int off;
	time_t now;
	struct smtp_date_time *date_time;

	now = time (NULL);

	if (gmtime_r (&now, &gmt) == NULL)
		return NULL;

	if (localtime_r (&now, &lt) == NULL)
		return NULL;

	off = (mkgmtime (&lt) - mkgmtime (&gmt)) / (60 * 60) * 100;

	date_time =
		smtp_date_time_new (lt.tm_mday, lt.tm_mon + 1,
				    lt.tm_year + 1900, lt.tm_hour, lt.tm_min,
				    lt.tm_sec, off);

	return date_time;
}

#ifndef WRONG
#define WRONG	(-1)
#endif /* !defined WRONG */

int
tmcomp (struct tm *atmp, struct tm *btmp)
{
	register int result;

	if ((result = (atmp->tm_year - btmp->tm_year)) == 0 &&
	    (result = (atmp->tm_mon - btmp->tm_mon)) == 0 &&
	    (result = (atmp->tm_mday - btmp->tm_mday)) == 0 &&
	    (result = (atmp->tm_hour - btmp->tm_hour)) == 0 &&
	    (result = (atmp->tm_min - btmp->tm_min)) == 0)
		result = atmp->tm_sec - btmp->tm_sec;
	return result;
}

time_t
mkgmtime (struct tm * tmp)
{
	register int dir;
	register int bits;
	register int saved_seconds;
	time_t t;
	struct tm yourtm, *mytm;

	yourtm = *tmp;
	saved_seconds = yourtm.tm_sec;
	yourtm.tm_sec = 0;
	/*
	 ** Calculate the number of magnitude bits in a time_t
	 ** (this works regardless of whether time_t is
	 ** signed or unsigned, though lint complains if unsigned).
	 */
	for (bits = 0, t = 1; t > 0; ++bits, t <<= 1);
	/*
	 ** If time_t is signed, then 0 is the median value,
	 ** if time_t is unsigned, then 1 << bits is median.
	 */
	t = (t < 0) ? 0 : ((time_t) 1 << bits);
	for (;;) {
		mytm = gmtime (&t);
		dir = tmcomp (mytm, &yourtm);
		if (dir != 0) {
			if (bits-- < 0)
				return WRONG;
			if (bits < 0)
				--t;
			else if (dir > 0)
				t -= (time_t) 1 << bits;
			else
				t += (time_t) 1 << bits;
			continue;
		}
		break;
	}
	t += saved_seconds;
	return t;
}







void
smtp_single_fields_init (struct smtp_single_fields *single_fields,
			 struct smtp_fields *fields)
{
	clistiter *cur;

	memset (single_fields, 0, sizeof (struct smtp_single_fields));

	cur = clist_begin (fields->fld_list);
	while (cur != NULL) {
		struct smtp_field *field;

		field = clist_content (cur);

		switch (field->fld_type) {
		case SMTP_FIELD_ORIG_DATE:
			if (single_fields->fld_orig_date == NULL)
				single_fields->fld_orig_date =
					field->fld_data.fld_orig_date;
			cur = clist_next (cur);
			break;
		case SMTP_FIELD_FROM:
			if (single_fields->fld_from == NULL) {
				single_fields->fld_from =
					field->fld_data.fld_from;
				cur = clist_next (cur);
			}
			else {
				clist_concat (single_fields->fld_from->
					      frm_mb_list->mb_list,
					      field->fld_data.fld_from->
					      frm_mb_list->mb_list);
				smtp_field_free (field);
				cur = clist_delete (fields->fld_list, cur);
			}
			break;
		case SMTP_FIELD_SENDER:
			if (single_fields->fld_sender == NULL)
				single_fields->fld_sender =
					field->fld_data.fld_sender;
			cur = clist_next (cur);
			break;
		case SMTP_FIELD_REPLY_TO:
			if (single_fields->fld_reply_to == NULL) {
				single_fields->fld_reply_to =
					field->fld_data.fld_reply_to;
				cur = clist_next (cur);
			}
			else {
				clist_concat (single_fields->fld_reply_to->
					      rt_addr_list->ad_list,
					      field->fld_data.fld_reply_to->
					      rt_addr_list->ad_list);
				smtp_field_free (field);
				cur = clist_delete (fields->fld_list, cur);
			}
			break;
		case SMTP_FIELD_TO:
			if (single_fields->fld_to == NULL) {
				single_fields->fld_to =
					field->fld_data.fld_to;
				cur = clist_next (cur);
			}
			else {
				clist_concat (single_fields->fld_to->
					      to_addr_list->ad_list,
					      field->fld_data.fld_to->
					      to_addr_list->ad_list);
				smtp_field_free (field);
				cur = clist_delete (fields->fld_list, cur);
			}
			break;
		case SMTP_FIELD_CC:
			if (single_fields->fld_cc == NULL) {
				single_fields->fld_cc =
					field->fld_data.fld_cc;
				cur = clist_next (cur);
			}
			else {
				clist_concat (single_fields->fld_cc->
					      cc_addr_list->ad_list,
					      field->fld_data.fld_cc->
					      cc_addr_list->ad_list);
				smtp_field_free (field);
				cur = clist_delete (fields->fld_list, cur);
			}
			break;
		case SMTP_FIELD_BCC:
			if (single_fields->fld_bcc == NULL) {
				single_fields->fld_bcc =
					field->fld_data.fld_bcc;
				cur = clist_next (cur);
			}
			else {
				clist_concat (single_fields->fld_bcc->
					      bcc_addr_list->ad_list,
					      field->fld_data.fld_bcc->
					      bcc_addr_list->ad_list);
				smtp_field_free (field);
				cur = clist_delete (fields->fld_list, cur);
			}
			break;
		case SMTP_FIELD_MESSAGE_ID:
			if (single_fields->fld_message_id == NULL)
				single_fields->fld_message_id =
					field->fld_data.fld_message_id;
			cur = clist_next (cur);
			break;
		case SMTP_FIELD_IN_REPLY_TO:
			if (single_fields->fld_in_reply_to == NULL)
				single_fields->fld_in_reply_to =
					field->fld_data.fld_in_reply_to;
			cur = clist_next (cur);
			break;
		case SMTP_FIELD_REFERENCES:
			if (single_fields->fld_references == NULL)
				single_fields->fld_references =
					field->fld_data.fld_references;
			cur = clist_next (cur);
			break;
		case SMTP_FIELD_SUBJECT:
			if (single_fields->fld_subject == NULL)
				single_fields->fld_subject =
					field->fld_data.fld_subject;
			cur = clist_next (cur);
			break;
		case SMTP_FIELD_COMMENTS:
			if (single_fields->fld_comments == NULL)
				single_fields->fld_comments =
					field->fld_data.fld_comments;
			cur = clist_next (cur);
			break;
		case SMTP_FIELD_KEYWORDS:
			if (single_fields->fld_keywords == NULL)
				single_fields->fld_keywords =
					field->fld_data.fld_keywords;
			cur = clist_next (cur);
			break;
		default:
			cur = clist_next (cur);
			break;
		}
	}
}


struct smtp_single_fields *
smtp_single_fields_new (struct smtp_fields *fields)
{
	struct smtp_single_fields *single_fields;

	single_fields = malloc (sizeof (struct smtp_single_fields));
	if (single_fields == NULL)
		goto err;
	DEBUG_SMTP (SMTP_MEM_1, "smtp_single_fields_new: MALLOC pointer=%p\n",
		    single_fields);

	smtp_single_fields_init (single_fields, fields);

	return single_fields;

      err:
	return NULL;
}

void
smtp_single_fields_free (struct smtp_single_fields *single_fields)
{
	free (single_fields);
	DEBUG_SMTP (SMTP_MEM_1, "smtp_single_fields_free: FREE pointer=%p\n",
		    single_fields);
}

struct smtp_field *
smtp_field_new_custom (char *name, char *value)
{
	struct smtp_optional_field *opt_field;
	struct smtp_field *field;

	opt_field = smtp_optional_field_new (name, value);
	if (opt_field == NULL)
		goto err;

	field = smtp_field_new (SMTP_FIELD_OPTIONAL_FIELD, opt_field);
	if (field == NULL)
		goto free_opt_field;

	return field;

      free_opt_field:
	smtp_optional_field_free (opt_field);
      err:
	return NULL;
}

void
detach_free_common_fields (struct smtp_orig_date *imf_date,
			   struct smtp_from *imf_from,
			   struct smtp_sender *imf_sender,
			   struct smtp_to *imf_to,
			   struct smtp_cc *imf_cc,
			   struct smtp_bcc *imf_bcc,
			   struct smtp_message_id *imf_msg_id)
{
	if (imf_date != NULL) {
		imf_date->dt_date_time = NULL;
		smtp_orig_date_free (imf_date);
	}
	if (imf_from != NULL) {
		imf_from->frm_mb_list = NULL;
		smtp_msg_from_free (imf_from);
	}
	if (imf_sender != NULL) {
		imf_sender->snd_mb = NULL;
		smtp_sender_free (imf_sender);
	}
	if (imf_to != NULL) {
		imf_to->to_addr_list = NULL;
		smtp_msg_to_free (imf_to);
	}
	if (imf_cc != NULL) {
		imf_cc->cc_addr_list = NULL;
		smtp_msg_to_free (imf_to);
	}
	if (imf_bcc != NULL) {
		imf_bcc->bcc_addr_list = NULL;
		smtp_bcc_free (imf_bcc);
	}
	if (imf_msg_id != NULL) {
		imf_msg_id->mid_value = NULL;
		smtp_message_id_free (imf_msg_id);
	}
}

void
detach_resent_field (struct smtp_field *field)
{
	field->fld_type = SMTP_FIELD_NONE;
	smtp_field_free (field);
}

int
smtp_resent_fields_add_data (struct smtp_fields *fields,
			     struct smtp_date_time *resent_date,
			     struct smtp_mailbox_list *resent_from,
			     struct smtp_mailbox *resent_sender,
			     struct smtp_address_list *resent_to,
			     struct smtp_address_list *resent_cc,
			     struct smtp_address_list *resent_bcc,
			     char *resent_msg_id)
{
	struct smtp_orig_date *imf_resent_date;
	struct smtp_from *imf_resent_from;
	struct smtp_sender *imf_resent_sender;
	struct smtp_to *imf_resent_to;
	struct smtp_cc *imf_resent_cc;
	struct smtp_bcc *imf_resent_bcc;
	struct smtp_message_id *imf_resent_msg_id;
	struct smtp_field *field;
	int r;

	imf_resent_date = NULL;
	imf_resent_from = NULL;
	imf_resent_sender = NULL;
	imf_resent_to = NULL;
	imf_resent_cc = NULL;
	imf_resent_bcc = NULL;
	imf_resent_msg_id = NULL;
	field = NULL;

	if (resent_date != NULL) {
		imf_resent_date = smtp_orig_date_new (resent_date);
		if (imf_resent_date == NULL)
			goto free;
		field = smtp_field_new (SMTP_FIELD_RESENT_DATE,
					imf_resent_date);
		if (field == NULL)
			goto free;
		r = smtp_fields_add (fields, field);
		if (r != SMTP_NO_ERROR)
			goto free_field;
	}

	if (resent_from != NULL) {
		imf_resent_from = smtp_msg_from_new (resent_from);
		if (imf_resent_from == NULL)
			goto free_field;
		field = smtp_field_new (SMTP_FIELD_RESENT_FROM,
					imf_resent_from);
		if (field == NULL)
			goto free;
		r = smtp_fields_add (fields, field);
		if (r != SMTP_NO_ERROR)
			goto free_field;
	}

	if (resent_sender != NULL) {
		imf_resent_sender = smtp_sender_new (resent_sender);
		if (imf_resent_sender == NULL)
			goto free;
		field = smtp_field_new (SMTP_FIELD_RESENT_SENDER,
					imf_resent_sender);
		if (field == NULL)
			goto free;
		r = smtp_fields_add (fields, field);
		if (r != SMTP_NO_ERROR)
			goto free_field;
	}

	if (resent_to != NULL) {
		imf_resent_to = smtp_msg_to_new (resent_to);
		if (imf_resent_to == NULL)
			goto free;
		field = smtp_field_new (SMTP_FIELD_RESENT_TO, imf_resent_to);
		if (field == NULL)
			goto free;
		r = smtp_fields_add (fields, field);
		if (r != SMTP_NO_ERROR)
			goto free_field;
	}

	if (resent_cc != NULL) {
		imf_resent_cc = smtp_cc_new (resent_cc);
		if (imf_resent_cc == NULL)
			goto free;
		field = smtp_field_new (SMTP_FIELD_RESENT_CC, imf_resent_cc);
		if (field == NULL)
			goto free;
		r = smtp_fields_add (fields, field);
		if (r != SMTP_NO_ERROR)
			goto free_field;
	}

	if (resent_bcc != NULL) {
		imf_resent_bcc = smtp_bcc_new (resent_bcc);
		if (imf_resent_bcc == NULL)
			goto free;
		field = smtp_field_new (SMTP_FIELD_RESENT_BCC,
					imf_resent_bcc);
		if (field == NULL)
			goto free;
		r = smtp_fields_add (fields, field);
		if (r != SMTP_NO_ERROR)
			goto free_field;
	}

	if (resent_msg_id != NULL) {
		imf_resent_msg_id = smtp_message_id_new (resent_msg_id);
		if (imf_resent_msg_id == NULL)
			goto free;
		field = smtp_field_new (SMTP_FIELD_RESENT_MSG_ID,
					imf_resent_msg_id);
		if (field == NULL)
			goto free;
		r = smtp_fields_add (fields, field);
		if (r != SMTP_NO_ERROR)
			goto free_field;
	}

	return SMTP_NO_ERROR;

      free_field:
	if (field != NULL) {
		detach_resent_field (field);
		smtp_field_free (field);
	}
      free:
	detach_free_common_fields (imf_resent_date,
				   imf_resent_from,
				   imf_resent_sender,
				   imf_resent_to,
				   imf_resent_cc,
				   imf_resent_bcc, imf_resent_msg_id);
	return SMTP_ERROR_MEMORY;
}

struct smtp_fields *
smtp_resent_fields_new_with_data_all (struct smtp_date_time *resent_date,
				      struct smtp_mailbox_list *resent_from,
				      struct smtp_mailbox *resent_sender,
				      struct smtp_address_list *resent_to,
				      struct smtp_address_list *resent_cc,
				      struct smtp_address_list *resent_bcc,
				      char *resent_msg_id)
{
	struct smtp_fields *resent_fields;
	int r;

	resent_fields = smtp_fields_new_empty ();
	if (resent_fields == NULL)
		goto err;

	r = smtp_resent_fields_add_data (resent_fields,
					 resent_date, resent_from,
					 resent_sender, resent_to,
					 resent_cc, resent_bcc,
					 resent_msg_id);
	if (r != SMTP_NO_ERROR)
		goto free;

	return resent_fields;

      free:
	smtp_fields_free (resent_fields);
      err:
	return NULL;
}


struct smtp_fields *
smtp_resent_fields_new_with_data (struct smtp_mailbox_list *from,
				  struct smtp_mailbox *sender,
				  struct smtp_address_list *to,
				  struct smtp_address_list *cc,
				  struct smtp_address_list *bcc)
{
	struct smtp_date_time *date;
	char *msg_id;
	struct smtp_fields *fields;

	date = smtp_get_current_date ();
	if (date == NULL)
		goto err;

	msg_id = smtp_get_message_id ();
	if (msg_id == NULL)
		goto free_date;

	fields = smtp_resent_fields_new_with_data_all (date,
						       from, sender, to, cc,
						       bcc, msg_id);
	if (fields == NULL)
		goto free_msg_id;

	return fields;

      free_msg_id:
	free (msg_id);
	DEBUG_SMTP (SMTP_MEM_1,
		    "smtp_resent_fields_new_with_data: FREE pointer=%p\n",
		    msg_id);
      free_date:
	smtp_date_time_free (date);
      err:
	return NULL;
}


struct smtp_fields *
smtp_fields_new_empty (void)
{
	clist *list;
	struct smtp_fields *fields_list;

	list = clist_new ();
	if (list == NULL)
		return NULL;

	fields_list = smtp_fields_new (list);
	if (fields_list == NULL)
		return NULL;

	return fields_list;
}

int
smtp_fields_add (struct smtp_fields *fields, struct smtp_field *field)
{
	int r;

	r = clist_append (fields->fld_list, field);
	if (r < 0)
		return SMTP_ERROR_MEMORY;

	return SMTP_NO_ERROR;
}

void
detach_free_fields (struct smtp_orig_date *date,
		    struct smtp_from *from,
		    struct smtp_sender *sender,
		    struct smtp_reply_to *reply_to,
		    struct smtp_to *to,
		    struct smtp_cc *cc,
		    struct smtp_bcc *bcc,
		    struct smtp_message_id *msg_id,
		    struct smtp_in_reply_to *in_reply_to,
		    struct smtp_references *references,
		    struct smtp_subject *subject)
{
	detach_free_common_fields (date, from, sender, to, cc, bcc, msg_id);

	if (reply_to != NULL) {
		reply_to->rt_addr_list = NULL;
		smtp_reply_to_free (reply_to);
	}

	if (in_reply_to != NULL) {
		in_reply_to->mid_list = NULL;
		smtp_in_reply_to_free (in_reply_to);
	}

	if (references != NULL) {
		references->mid_list = NULL;
		smtp_references_free (references);
	}

	if (subject != NULL) {
		subject->sbj_value = NULL;
		smtp_subject_free (subject);
	}
}


void
detach_field (struct smtp_field *field)
{
	field->fld_type = SMTP_FIELD_NONE;
	smtp_field_free (field);
}

int
smtp_fields_add_data (struct smtp_fields *fields,
		      struct smtp_date_time *date,
		      struct smtp_mailbox_list *from,
		      struct smtp_mailbox *sender,
		      struct smtp_address_list *reply_to,
		      struct smtp_address_list *to,
		      struct smtp_address_list *cc,
		      struct smtp_address_list *bcc,
		      char *msg_id,
		      clist * in_reply_to, clist * references, char *subject)
{
	struct smtp_orig_date *imf_date;
	struct smtp_from *imf_from;
	struct smtp_sender *imf_sender;
	struct smtp_reply_to *imf_reply_to;
	struct smtp_to *imf_to;
	struct smtp_cc *imf_cc;
	struct smtp_bcc *imf_bcc;
	struct smtp_message_id *imf_msg_id;
	struct smtp_references *imf_references;
	struct smtp_in_reply_to *imf_in_reply_to;
	struct smtp_subject *imf_subject;
	struct smtp_field *field;
	int r;

	imf_date = NULL;
	imf_from = NULL;
	imf_sender = NULL;
	imf_reply_to = NULL;
	imf_to = NULL;
	imf_cc = NULL;
	imf_bcc = NULL;
	imf_msg_id = NULL;
	imf_references = NULL;
	imf_in_reply_to = NULL;
	imf_subject = NULL;
	field = NULL;

	if (date != NULL) {
		imf_date = smtp_orig_date_new (date);
		if (imf_date == NULL)
			goto free;
		field = smtp_field_new (SMTP_FIELD_ORIG_DATE, imf_date);
		if (field == NULL)
			goto free;
		r = smtp_fields_add (fields, field);
		if (r != SMTP_NO_ERROR)
			goto free_field;
	}

	if (from != NULL) {
		imf_from = smtp_msg_from_new (from);
		if (imf_from == NULL)
			goto free_field;
		field = smtp_field_new (SMTP_FIELD_FROM, imf_from);
		if (field == NULL)
			goto free;
		r = smtp_fields_add (fields, field);
		if (r != SMTP_NO_ERROR)
			goto free_field;
	}

	if (sender != NULL) {
		imf_sender = smtp_sender_new (sender);
		if (imf_sender == NULL)
			goto free;
		field = smtp_field_new (SMTP_FIELD_SENDER, imf_sender);
		if (field == NULL)
			goto free;
		r = smtp_fields_add (fields, field);
		if (r != SMTP_NO_ERROR)
			goto free_field;
	}

	if (reply_to != NULL) {
		imf_reply_to = smtp_reply_to_new (reply_to);
		if (imf_reply_to == NULL)
			goto free;
		field = smtp_field_new (SMTP_FIELD_REPLY_TO, imf_reply_to);
		if (field == NULL)
			goto free;
		r = smtp_fields_add (fields, field);
		if (r != SMTP_NO_ERROR)
			goto free_field;
	}

	if (to != NULL) {
		imf_to = smtp_msg_to_new (to);
		if (imf_to == NULL)
			goto free;
		field = smtp_field_new (SMTP_FIELD_TO, imf_to);
		if (field == NULL)
			goto free;
		r = smtp_fields_add (fields, field);
		if (r != SMTP_NO_ERROR)
			goto free_field;
	}

	if (cc != NULL) {
		imf_cc = smtp_cc_new (cc);
		if (imf_cc == NULL)
			goto free;
		field = smtp_field_new (SMTP_FIELD_CC, imf_cc);
		if (field == NULL)
			goto free;
		r = smtp_fields_add (fields, field);
		if (r != SMTP_NO_ERROR)
			goto free_field;
	}

	if (bcc != NULL) {
		imf_bcc = smtp_bcc_new (bcc);
		if (imf_bcc == NULL)
			goto free;
		field = smtp_field_new (SMTP_FIELD_BCC, imf_bcc);
		if (field == NULL)
			goto free;
		r = smtp_fields_add (fields, field);
		if (r != SMTP_NO_ERROR)
			goto free_field;
	}

	if (msg_id != NULL) {
		imf_msg_id = smtp_message_id_new (msg_id);
		if (imf_msg_id == NULL)
			goto free;
		field = smtp_field_new (SMTP_FIELD_MESSAGE_ID, imf_msg_id);
		if (field == NULL)
			goto free;
		r = smtp_fields_add (fields, field);
		if (r != SMTP_NO_ERROR)
			goto free_field;
	}

	if (in_reply_to != NULL) {
		imf_in_reply_to = smtp_in_reply_to_new (in_reply_to);
		if (imf_in_reply_to == NULL)
			goto free;
		field = smtp_field_new (SMTP_FIELD_IN_REPLY_TO,
					imf_in_reply_to);
		if (field == NULL)
			goto free;
		r = smtp_fields_add (fields, field);
		if (r != SMTP_NO_ERROR)
			goto free_field;
	}

	if (references != NULL) {
		imf_references = smtp_references_new (references);
		if (imf_references == NULL)
			goto free;
		field = smtp_field_new (SMTP_FIELD_REFERENCES,
					imf_references);
		if (field == NULL)
			goto free;
		r = smtp_fields_add (fields, field);
		if (r != SMTP_NO_ERROR)
			goto free_field;
	}

	if (subject != NULL) {
		imf_subject = smtp_subject_new (subject);
		if (imf_subject == NULL)
			goto free;
		field = smtp_field_new (SMTP_FIELD_SUBJECT, imf_subject);
		if (field == NULL)
			goto free;
		r = smtp_fields_add (fields, field);
		if (r != SMTP_NO_ERROR)
			goto free_field;
	}

	return SMTP_NO_ERROR;

      free_field:
	if (field != NULL) {
		detach_field (field);
		smtp_field_free (field);
	}
      free:
	detach_free_fields (imf_date,
			    imf_from,
			    imf_sender,
			    imf_reply_to,
			    imf_to,
			    imf_cc,
			    imf_bcc,
			    imf_msg_id,
			    imf_in_reply_to, imf_references, imf_subject);

	return SMTP_ERROR_MEMORY;
}

struct smtp_fields *
smtp_fields_new_with_data_all (struct smtp_date_time *date,
			       struct smtp_mailbox_list *from,
			       struct smtp_mailbox *sender,
			       struct smtp_address_list *reply_to,
			       struct smtp_address_list *to,
			       struct smtp_address_list *cc,
			       struct smtp_address_list *bcc,
			       char *message_id,
			       clist * in_reply_to,
			       clist * references, char *subject)
{
	struct smtp_fields *fields;
	int r;

	fields = smtp_fields_new_empty ();
	if (fields == NULL)
		goto err;

	r = smtp_fields_add_data (fields,
				  date,
				  from,
				  sender,
				  reply_to,
				  to,
				  cc,
				  bcc,
				  message_id,
				  in_reply_to, references, subject);
	if (r != SMTP_NO_ERROR)
		goto free;

	return fields;

      free:
	smtp_fields_free (fields);
      err:
	return NULL;
}

struct smtp_fields *
smtp_fields_new_with_data (struct smtp_mailbox_list *from,
			   struct smtp_mailbox *sender,
			   struct smtp_address_list *reply_to,
			   struct smtp_address_list *to,
			   struct smtp_address_list *cc,
			   struct smtp_address_list *bcc,
			   clist * in_reply_to,
			   clist * references, char *subject)
{
	struct smtp_date_time *date;
	char *msg_id;
	struct smtp_fields *fields;

	date = smtp_get_current_date ();
	if (date == NULL)
		goto err;

	msg_id = smtp_get_message_id ();
	if (msg_id == NULL)
		goto free_date;

	fields = smtp_fields_new_with_data_all (date,
						from, sender, reply_to,
						to, cc, bcc,
						msg_id,
						in_reply_to, references,
						subject);
	if (fields == NULL)
		goto free_msg_id;

	return fields;

      free_msg_id:
	free (msg_id);
	DEBUG_SMTP (SMTP_MEM_1,
		    "smtp_fields_new_with_data: FREE pointer=%p\n", msg_id);
      free_date:
	smtp_date_time_free (date);
      err:
	return NULL;
}

struct smtp_field *
smtp_field_new (int fld_type, void *value)
{

	struct smtp_field *field;

	field = malloc (sizeof (*field));
	if (field == NULL)
		return NULL;
	DEBUG_SMTP (SMTP_MEM_1, "smtp_field_new: MALLOC pointer=%p\n", field);

	field->fld_type = fld_type;
	switch (fld_type) {
	case SMTP_FIELD_RETURN_PATH:
		field->fld_data.fld_return_path =
			(struct smtp_return *) value;
		break;
	case SMTP_FIELD_RESENT_DATE:
		field->fld_data.fld_resent_date =
			(struct smtp_orig_date *) value;
		break;
	case SMTP_FIELD_RESENT_FROM:
		field->fld_data.fld_resent_from = (struct smtp_from *) value;
		break;
	case SMTP_FIELD_RESENT_SENDER:
		field->fld_data.fld_resent_sender =
			(struct smtp_sender *) value;
		break;
	case SMTP_FIELD_RESENT_TO:
		field->fld_data.fld_resent_to = (struct smtp_to *) value;
		break;
	case SMTP_FIELD_RESENT_CC:
		field->fld_data.fld_resent_cc = (struct smtp_cc *) value;
		break;
	case SMTP_FIELD_RESENT_BCC:
		field->fld_data.fld_resent_bcc = (struct smtp_bcc *) value;
		break;
	case SMTP_FIELD_RESENT_MSG_ID:
		field->fld_data.fld_resent_msg_id =
			(struct smtp_message_id *) value;
		break;
	case SMTP_FIELD_ORIG_DATE:
		field->fld_data.fld_orig_date =
			(struct smtp_orig_date *) value;
		break;
	case SMTP_FIELD_FROM:
		field->fld_data.fld_from = (struct smtp_from *) value;
		break;
	case SMTP_FIELD_SENDER:
		field->fld_data.fld_sender = (struct smtp_sender *) value;
		break;
	case SMTP_FIELD_REPLY_TO:
		field->fld_data.fld_reply_to = (struct smtp_reply_to *) value;
		break;
	case SMTP_FIELD_TO:
		field->fld_data.fld_to = (struct smtp_to *) value;
		break;
	case SMTP_FIELD_CC:
		field->fld_data.fld_cc = (struct smtp_cc *) value;
		break;
	case SMTP_FIELD_BCC:
		field->fld_data.fld_bcc = (struct smtp_bcc *) value;
		break;
	case SMTP_FIELD_MESSAGE_ID:
		field->fld_data.fld_message_id =
			(struct smtp_message_id *) value;
		break;
	case SMTP_FIELD_IN_REPLY_TO:
		field->fld_data.fld_in_reply_to =
			(struct smtp_in_reply_to *) value;
		break;
	case SMTP_FIELD_REFERENCES:
		field->fld_data.fld_references =
			(struct smtp_references *) value;
		break;
	case SMTP_FIELD_SUBJECT:
		field->fld_data.fld_subject = (struct smtp_subject *) value;
		break;
	case SMTP_FIELD_COMMENTS:
		field->fld_data.fld_comments = (struct smtp_comments *) value;
		break;
	case SMTP_FIELD_KEYWORDS:
		field->fld_data.fld_keywords = (struct smtp_keywords *) value;
		break;
	case SMTP_FIELD_RECEIVED:
		field->fld_data.fld_received = (struct smtp_received *) value;
		break;
	case SMTP_FIELD_OPTIONAL_FIELD:
		field->fld_data.fld_optional_field =
			(struct smtp_optional_field *) value;
		break;
	}

	return field;
}



void
smtp_field_free (struct smtp_field *field)
{
	switch (field->fld_type) {
	case SMTP_FIELD_RETURN_PATH:
		smtp_return_free (field->fld_data.fld_return_path);
		break;
	case SMTP_FIELD_RESENT_DATE:
		smtp_orig_date_free (field->fld_data.fld_resent_date);
		break;
	case SMTP_FIELD_RESENT_FROM:
		smtp_msg_from_free (field->fld_data.fld_resent_from);
		break;
	case SMTP_FIELD_RESENT_SENDER:
		smtp_sender_free (field->fld_data.fld_resent_sender);
		break;
	case SMTP_FIELD_RESENT_TO:
		smtp_msg_to_free (field->fld_data.fld_resent_to);
		break;
	case SMTP_FIELD_RESENT_CC:
		smtp_cc_free (field->fld_data.fld_resent_cc);
		break;
	case SMTP_FIELD_RESENT_BCC:
		smtp_bcc_free (field->fld_data.fld_resent_bcc);
		break;
	case SMTP_FIELD_RESENT_MSG_ID:
		smtp_message_id_free (field->fld_data.fld_resent_msg_id);
		break;
	case SMTP_FIELD_ORIG_DATE:
		smtp_orig_date_free (field->fld_data.fld_orig_date);
		break;
	case SMTP_FIELD_FROM:
		smtp_msg_from_free (field->fld_data.fld_from);
		break;
	case SMTP_FIELD_SENDER:
		smtp_sender_free (field->fld_data.fld_sender);
		break;
	case SMTP_FIELD_REPLY_TO:
		smtp_reply_to_free (field->fld_data.fld_reply_to);
		break;
	case SMTP_FIELD_TO:
		smtp_msg_to_free (field->fld_data.fld_to);
		break;
	case SMTP_FIELD_CC:
		smtp_cc_free (field->fld_data.fld_cc);
		break;
	case SMTP_FIELD_BCC:
		smtp_bcc_free (field->fld_data.fld_bcc);
		break;
	case SMTP_FIELD_MESSAGE_ID:
		smtp_message_id_free (field->fld_data.fld_message_id);
		break;
	case SMTP_FIELD_IN_REPLY_TO:
		smtp_in_reply_to_free (field->fld_data.fld_in_reply_to);
		break;
	case SMTP_FIELD_REFERENCES:
		smtp_references_free (field->fld_data.fld_references);
		break;
	case SMTP_FIELD_SUBJECT:
		smtp_subject_free (field->fld_data.fld_subject);
		break;
	case SMTP_FIELD_COMMENTS:
		smtp_comments_free (field->fld_data.fld_comments);
		break;
	case SMTP_FIELD_KEYWORDS:
		smtp_keywords_free (field->fld_data.fld_keywords);
		break;
	case SMTP_FIELD_OPTIONAL_FIELD:
		smtp_optional_field_free (field->fld_data.fld_optional_field);
		break;
	}

	free (field);
	DEBUG_SMTP (SMTP_MEM_1, "smtp_field_free: FREE pointer=%p\n", field);
}


struct smtp_fields *
smtp_fields_new (clist * fld_list)
{
	struct smtp_fields *fields;

	fields = malloc (sizeof (*fields));
	if (fields == NULL)
		return NULL;
	DEBUG_SMTP (SMTP_MEM_1, "smtp_fields_new: MALLOC pointer=%p\n",
		    fields);

	fields->fld_list = fld_list;

	return fields;
}


void
smtp_fields_free (struct smtp_fields *fields)
{
	if (fields->fld_list != NULL) {
		clist_foreach (fields->fld_list, (clist_func) smtp_field_free,
			       NULL);
		clist_free (fields->fld_list);
	}
	free (fields);
	DEBUG_SMTP (SMTP_MEM_1, "smtp_fields_free: FREE pointer=%p\n",
		    fields);
}

#if 1
struct smtp_mime_field *
smtp_mime_field_new (int fld_type, void *value)
{
	struct smtp_mime_field *field;

	field = malloc (sizeof (*field));
	if (field == NULL)
		return NULL;
	DEBUG_SMTP (SMTP_MEM_1, "smtp_mime_field_new: MALLOC pointer=%p\n",
		    field);

#ifdef USE_NEL
	//xiayu 2005.11.30
	//field->count = 0;
	NEL_REF_INIT (field);
#endif

	field->fld_type = fld_type;

	switch (fld_type) {

		//xiayu 2005.11.28 added
	case SMTP_FIELD_FROM:
		field->fld_data.fld_from = (struct smtp_msg_from *) value;
		break;
	case SMTP_FIELD_TO:
		field->fld_data.fld_to = (struct smtp_msg_to *) value;
		break;
		//add end

	case SMTP_MIME_FIELD_TYPE:
		field->fld_data.fld_content_type =
			(struct smtp_mime_content_type *) value;
		break;
	case SMTP_MIME_FIELD_TRANSFER_ENCODING:
		field->fld_data.fld_encoding =
			(struct smtp_mime_mechanism *) value;
		break;
	case SMTP_MIME_FIELD_ID:
		field->fld_data.fld_id = (char *) value;
		break;
	case SMTP_MIME_FIELD_DESCRIPTION:
		field->fld_data.fld_description = (char *) value;
		break;
	case SMTP_MIME_FIELD_VERSION:
		field->fld_data.fld_version = (uint32_t) value;
		break;
	case SMTP_MIME_FIELD_DISPOSITION:
		field->fld_data.fld_disposition =
			(struct smtp_mime_disposition *) value;
		break;
	case SMTP_MIME_FIELD_LANGUAGE:
		field->fld_data.fld_language =
			(struct smtp_mime_language *) value;
		break;
	}
	return field;
}

#else
struct smtp_mime_field *
smtp_mime_field_new (int fld_type,
		     struct smtp_mime_content_type *fld_content_type,
		     struct smtp_mime_mechanism *fld_encoding,
		     char *fld_id,
		     char *fld_description,
		     uint32_t fld_version,
		     struct smtp_mime_disposition *fld_disposition,
		     struct smtp_mime_language *fld_language)
{
	struct smtp_mime_field *field;

	field = malloc (sizeof (*field));
	if (field == NULL)
		return NULL;
	field->fld_type = fld_type;

	switch (fld_type) {
	case SMTP_MIME_FIELD_TYPE:
		field->fld_data.fld_content_type = fld_content_type;
		break;
	case SMTP_MIME_FIELD_TRANSFER_ENCODING:
		field->fld_data.fld_encoding = fld_encoding;
		break;
	case SMTP_MIME_FIELD_ID:
		field->fld_data.fld_id = fld_id;
		break;
	case SMTP_MIME_FIELD_DESCRIPTION:
		field->fld_data.fld_description = fld_description;
		break;
	case SMTP_MIME_FIELD_VERSION:
		field->fld_data.fld_version = fld_version;
		break;
	case SMTP_MIME_FIELD_DISPOSITION:
		field->fld_data.fld_disposition = fld_disposition;
		break;
	case SMTP_MIME_FIELD_LANGUAGE:
		field->fld_data.fld_language = fld_language;
		break;
	}
	return field;
}
#endif


void
smtp_mime_field_free (struct smtp_mime_field *field)
{
	DEBUG_SMTP (SMTP_DBG, "smtp_mime_field_free\n");
	switch (field->fld_type) {
		//xiayu 2005.11.28
	case SMTP_FIELD_FROM:
		if (field->fld_data.fld_from != NULL)
			smtp_msg_from_free (field->fld_data.fld_from);
		break;
	case SMTP_FIELD_TO:
		if (field->fld_data.fld_to != NULL)
			smtp_msg_to_free (field->fld_data.fld_to);
		break;
		//add end

	case SMTP_MIME_FIELD_TYPE:
		if (field->fld_data.fld_content_type != NULL)
			smtp_mime_content_type_free (field->fld_data.
						     fld_content_type);
		break;
	case SMTP_MIME_FIELD_TRANSFER_ENCODING:
		if (field->fld_data.fld_encoding != NULL)
			smtp_mime_encoding_free (field->fld_data.
						 fld_encoding);
		break;
	case SMTP_MIME_FIELD_ID:
		if (field->fld_data.fld_id != NULL)
			smtp_mime_id_free (field->fld_data.fld_id);
		break;
	case SMTP_MIME_FIELD_DESCRIPTION:
		if (field->fld_data.fld_description != NULL)
			smtp_mime_description_free (field->fld_data.
						    fld_description);
		break;
	case SMTP_MIME_FIELD_DISPOSITION:
		if (field->fld_data.fld_disposition != NULL)
			smtp_mime_disposition_free (field->fld_data.
						    fld_disposition);
		break;
	case SMTP_MIME_FIELD_LANGUAGE:
		if (field->fld_data.fld_language != NULL)
			smtp_mime_language_free (field->fld_data.
						 fld_language);
		break;
	}

	free (field);
	DEBUG_SMTP (SMTP_MEM_1, "smtp_mime_field_free: FREE pointer=%p\n",
		    field);
}

struct smtp_mime_fields *
smtp_mime_fields_new (clist * fld_list)
{
	struct smtp_mime_fields *fields;

	fields = malloc (sizeof (*fields));
	if (fields == NULL)
		return NULL;
	DEBUG_SMTP (SMTP_MEM_1, "smtp_mime_fields_new: MALLOC pointer=%p\n",
		    fields);

#ifdef USE_NEL
	//xiayu 2005.11.30
	//fields->count = 0;
	NEL_REF_INIT (fields);
#endif

	fields->fld_list = fld_list;

	return fields;
}

void
smtp_mime_fields_free (struct smtp_mime_fields *fields)
{
	clist_foreach (fields->fld_list, (clist_func) smtp_mime_field_free,
		       NULL);
	clist_free (fields->fld_list);
	free (fields);
	DEBUG_SMTP (SMTP_MEM_1, "smtp_mime_fields_free: FREE pointer=%p\n",
		    fields);
}

enum
{
	HEADER_START,
	HEADER_C,
	HEADER_R,
	HEADER_RE,
	HEADER_S,
	HEADER_RES,
};

int
guess_header_type (const char *message, size_t length, size_t index)
{
	int state;
	int r;

	state = HEADER_START;

	//DEBUG_SMTP(SMTP_DBG,"\n");
	//printf("%s[%d,%d]\n", message, length, index );
	while (1) {

		if (index >= length)
			return SMTP_FIELD_NONE;

		switch (state) {
		case HEADER_START:
			switch ((char)
				toupper ((unsigned char) message[index])) {
			case 'B':
				return SMTP_FIELD_BCC;
			case 'C':
				state = HEADER_C;
				break;
			case 'D':
				return SMTP_FIELD_ORIG_DATE;
			case 'F':
				return SMTP_FIELD_FROM;
			case 'I':
				return SMTP_FIELD_IN_REPLY_TO;
			case 'K':
				return SMTP_FIELD_KEYWORDS;
			case 'M':
				return SMTP_FIELD_MESSAGE_ID;
			case 'R':
				state = HEADER_R;
				break;
			case 'T':
				return SMTP_FIELD_TO;
				break;
			case 'S':
				state = HEADER_S;
				break;
			default:
				return SMTP_FIELD_NONE;
			}
			break;
		case HEADER_C:
			switch ((char)
				toupper ((unsigned char) message[index])) {
			case 'O':
				return SMTP_FIELD_COMMENTS;
			case 'C':
				return SMTP_FIELD_CC;
			default:
				return SMTP_FIELD_NONE;
			}
			break;
		case HEADER_R:
			switch ((char)
				toupper ((unsigned char) message[index])) {
			case 'E':
				state = HEADER_RE;
				break;
			default:
				return SMTP_FIELD_NONE;
			}
			break;
		case HEADER_RE:
			switch ((char)
				toupper ((unsigned char) message[index])) {
			case 'F':
				return SMTP_FIELD_REFERENCES;
			case 'P':
				return SMTP_FIELD_REPLY_TO;
			case 'S':
				state = HEADER_RES;
				break;
			case 'T':
				return SMTP_FIELD_RETURN_PATH;
			default:
				return SMTP_FIELD_NONE;
			}
			break;
		case HEADER_S:
			switch ((char)
				toupper ((unsigned char) message[index])) {
			case 'E':
				return SMTP_FIELD_SENDER;
			case 'U':
				return SMTP_FIELD_SUBJECT;
			default:
				return SMTP_FIELD_NONE;
			}
			break;

		case HEADER_RES:
			r = smtp_token_case_insensitive_parse (message,
							       length, &index,
							       "ent-");
			if (r != SMTP_NO_ERROR)
				return SMTP_FIELD_NONE;

			if (index >= length)
				return SMTP_FIELD_NONE;

			switch ((char)
				toupper ((unsigned char) message[index])) {
			case 'D':
				return SMTP_FIELD_RESENT_DATE;
			case 'F':
				return SMTP_FIELD_RESENT_FROM;
			case 'S':
				return SMTP_FIELD_RESENT_SENDER;
			case 'T':
				return SMTP_FIELD_RESENT_TO;
			case 'C':
				return SMTP_FIELD_RESENT_CC;
			case 'B':
				return SMTP_FIELD_RESENT_BCC;
			case 'M':
				return SMTP_FIELD_RESENT_MSG_ID;
			default:
				return SMTP_FIELD_NONE;
			}
			break;
		}
		index++;
	}
}


#if 0
int
smtp_field_parse (struct smtp_info *psmtp, const char *message, size_t length,
		  size_t * index, struct smtp_field **result)
{
	size_t cur_token;
	int type;
	struct smtp_return *return_path;
	struct smtp_orig_date *resent_date;
	struct smtp_from *resent_from;
	struct smtp_sender *resent_sender;
	struct smtp_to *resent_to;
	struct smtp_cc *resent_cc;
	struct smtp_bcc *resent_bcc;
	struct smtp_message_id *resent_msg_id;
	struct smtp_orig_date *orig_date;
	struct smtp_from *from;
	struct smtp_sender *sender;
	struct smtp_reply_to *reply_to;
	struct smtp_to *to;
	struct smtp_cc *cc;
	struct smtp_bcc *bcc;
	struct smtp_message_id *message_id;
	struct smtp_in_reply_to *in_reply_to;
	struct smtp_references *references;
	struct smtp_subject *subject;
	struct smtp_comments *comments;
	struct smtp_keywords *keywords;
	struct smtp_optional_field *optional_field;
	struct smtp_field *field;
	int guessed_type;
	int r;
	int res;

	cur_token = *index;

	return_path = NULL;
	resent_date = NULL;
	resent_from = NULL;
	resent_sender = NULL;
	resent_to = NULL;
	resent_cc = NULL;
	resent_bcc = NULL;
	resent_msg_id = NULL;
	orig_date = NULL;
	from = NULL;
	sender = NULL;
	reply_to = NULL;
	to = NULL;
	cc = NULL;
	bcc = NULL;
	message_id = NULL;
	in_reply_to = NULL;
	references = NULL;
	subject = NULL;
	comments = NULL;
	keywords = NULL;
	optional_field = NULL;

	guessed_type = guess_header_type (message, length, cur_token);
	type = SMTP_FIELD_NONE;

	switch (guessed_type) {
	case SMTP_FIELD_ORIG_DATE:
		r = smtp_orig_date_parse (message, length, &cur_token,
					  &orig_date);
		if (r == SMTP_NO_ERROR)
			type = SMTP_FIELD_ORIG_DATE;
		else if (r == SMTP_ERROR_PARSE) {
			/* do nothing */
		}
		else {
			res = r;
			goto err;
		}
		break;
	case SMTP_FIELD_FROM:
		r = smtp_msg_from_parse (message, length, &cur_token, &from);
		if (r == SMTP_NO_ERROR)
			type = guessed_type;
		else if (r == SMTP_ERROR_PARSE) {
			/* do nothing */
		}
		else {
			res = r;
			goto err;
		}
		break;
	case SMTP_FIELD_SENDER:
		r = smtp_sender_parse (message, length, &cur_token, &sender);
		if (r == SMTP_NO_ERROR)
			type = guessed_type;
		else if (r == SMTP_ERROR_PARSE) {
			/* do nothing */
		}
		else {
			res = r;
			goto err;
		}
		break;
	case SMTP_FIELD_REPLY_TO:
		r = smtp_reply_to_parse (message, length, &cur_token,
					 &reply_to);
		if (r == SMTP_NO_ERROR)
			type = guessed_type;
		else if (r == SMTP_ERROR_PARSE) {
			/* do nothing */
		}
		else {
			res = r;
			goto err;
		}
		break;
	case SMTP_FIELD_TO:
		r = smtp_msg_to_parse (message, length, &cur_token, &to);
		if (r == SMTP_NO_ERROR)
			type = guessed_type;
		else if (r == SMTP_ERROR_PARSE) {
			/* do nothing */
		}
		else {
			res = r;
			goto err;
		}
		break;
	case SMTP_FIELD_CC:
		r = smtp_cc_parse (message, length, &cur_token, &cc);
		if (r == SMTP_NO_ERROR)
			type = guessed_type;
		else if (r == SMTP_ERROR_PARSE) {
			/* do nothing */
		}
		else {
			res = r;
			goto err;
		}
		break;
	case SMTP_FIELD_BCC:
		r = smtp_bcc_parse (message, length, &cur_token, &bcc);
		if (r == SMTP_NO_ERROR)
			type = guessed_type;
		else if (r == SMTP_ERROR_PARSE) {
			/* do nothing */
		}
		else {
			res = r;
			goto err;
		}
		break;
	case SMTP_FIELD_MESSAGE_ID:
		r = smtp_message_id_parse (psmtp, message, length, &cur_token,
					   &message_id);
		if (r == SMTP_NO_ERROR)
			type = guessed_type;
		else if (r == SMTP_ERROR_PARSE) {
			/* do nothing */
		}
		else {
			res = r;
			goto err;
		}
		break;
	case SMTP_FIELD_IN_REPLY_TO:
		r = smtp_in_reply_to_parse (message, length, &cur_token,
					    &in_reply_to);
		if (r == SMTP_NO_ERROR)
			type = guessed_type;
		else if (r == SMTP_ERROR_PARSE) {
			/* do nothing */
		}
		else {
			res = r;
			goto err;
		}
		break;
	case SMTP_FIELD_REFERENCES:
		r = smtp_references_parse (message, length, &cur_token,
					   &references);
		if (r == SMTP_NO_ERROR)
			type = guessed_type;
		else if (r == SMTP_ERROR_PARSE) {
			/* do nothing */
		}
		else {
			res = r;
			goto err;
		}
		break;
	case SMTP_FIELD_SUBJECT:
		r = smtp_subject_parse (message, length, &cur_token,
					&subject);
		if (r == SMTP_NO_ERROR)
			type = guessed_type;
		else if (r == SMTP_ERROR_PARSE) {
			/* do nothing */
		}
		else {
			res = r;
			goto err;
		}
		break;
	case SMTP_FIELD_COMMENTS:
		r = smtp_comments_parse (message, length, &cur_token,
					 &comments);
		if (r == SMTP_NO_ERROR)
			type = guessed_type;
		else if (r == SMTP_ERROR_PARSE) {
			/* do nothing */
		}
		else {
			res = r;
			goto err;
		}
		break;
	case SMTP_FIELD_KEYWORDS:
		r = smtp_keywords_parse (message, length, &cur_token,
					 &keywords);
		if (r == SMTP_NO_ERROR)
			type = guessed_type;
		else if (r == SMTP_ERROR_PARSE) {
			/* do nothing */
		}
		else {
			res = r;
			goto err;
		}
		break;
	case SMTP_FIELD_RETURN_PATH:
		r = smtp_return_parse (message, length, &cur_token,
				       &return_path);
		if (r == SMTP_NO_ERROR)
			type = guessed_type;
		else if (r == SMTP_ERROR_PARSE) {
			/* do nothing */
		}
		else {
			res = r;
			goto err;
		}
		break;
	case SMTP_FIELD_RESENT_DATE:
		r = smtp_resent_date_parse (message, length, &cur_token,
					    &resent_date);
		if (r == SMTP_NO_ERROR)
			type = guessed_type;
		else if (r == SMTP_ERROR_PARSE) {
			/* do nothing */
		}
		else {
			res = r;
			goto err;
		}
		break;
	case SMTP_FIELD_RESENT_FROM:
		r = smtp_resent_from_parse (message, length, &cur_token,
					    &resent_from);
		if (r == SMTP_NO_ERROR)
			type = guessed_type;
		else if (r == SMTP_ERROR_PARSE) {
			/* do nothing */
		}
		else {
			res = r;
			goto err;
		}
		break;
	case SMTP_FIELD_RESENT_SENDER:
		r = smtp_resent_sender_parse (message, length, &cur_token,
					      &resent_sender);
		if (r == SMTP_NO_ERROR)
			type = guessed_type;
		else if (r == SMTP_ERROR_PARSE) {
			/* do nothing */
		}
		else {
			res = r;
			goto err;
		}
		break;
	case SMTP_FIELD_RESENT_TO:
		r = smtp_resent_to_parse (message, length, &cur_token,
					  &resent_to);
		if (r == SMTP_NO_ERROR)
			type = guessed_type;
		else if (r == SMTP_ERROR_PARSE) {
			/* do nothing */
		}
		else {
			res = r;
			goto err;
		}
		break;
	case SMTP_FIELD_RESENT_CC:
		r = smtp_resent_cc_parse (message, length, &cur_token,
					  &resent_cc);
		if (r == SMTP_NO_ERROR)
			type = guessed_type;
		else if (r == SMTP_ERROR_PARSE) {
			/* do nothing */
		}
		else {
			res = r;
			goto err;
		}
		break;
	case SMTP_FIELD_RESENT_BCC:
		r = smtp_resent_bcc_parse (message, length, &cur_token,
					   &resent_bcc);
		if (r == SMTP_NO_ERROR)
			type = guessed_type;
		else if (r == SMTP_ERROR_PARSE) {
			/* do nothing */
		}
		else {
			res = r;
			goto err;
		}
		break;
	case SMTP_FIELD_RESENT_MSG_ID:
		r = smtp_resent_msg_id_parse (message, length, &cur_token,
					      &resent_msg_id);
		if (r == SMTP_NO_ERROR)
			type = guessed_type;
		else if (r == SMTP_ERROR_PARSE) {
			/* do nothing */
		}
		else {
			res = r;
			goto err;
		}
		break;
	}

	if (type == SMTP_FIELD_NONE) {
		r = smtp_optional_field_parse (message, length, &cur_token,
					       &optional_field);
		if (r != SMTP_NO_ERROR) {
			res = r;
			goto err;
		}

		type = SMTP_FIELD_OPTIONAL_FIELD;
	}

#if 0
	/* NOTE,NOTE,NOTE, must change the following to individuals */
	field = smtp_field_new (type, return_path, resent_date,
				resent_from, resent_sender, resent_to,
				resent_cc, resent_bcc, resent_msg_id,
				orig_date, from, sender, reply_to, to, cc,
				bcc, message_id, in_reply_to, references,
				subject, comments, keywords, optional_field);
#endif


	if (field == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free_field;
	}

	*result = field;
	*index = cur_token;

	return SMTP_NO_ERROR;

      free_field:
	if (return_path != NULL)
		smtp_return_free (return_path);
	if (resent_date != NULL)
		smtp_orig_date_free (resent_date);
	if (resent_from != NULL)
		smtp_msg_from_free (resent_from);
	if (resent_sender != NULL)
		smtp_sender_free (resent_sender);
	if (resent_to != NULL)
		smtp_msg_to_free (resent_to);
	if (resent_cc != NULL)
		smtp_cc_free (resent_cc);
	if (resent_bcc != NULL)
		smtp_bcc_free (resent_bcc);
	if (resent_msg_id != NULL)
		smtp_message_id_free (resent_msg_id);
	if (orig_date != NULL)
		smtp_orig_date_free (orig_date);
	if (from != NULL)
		smtp_msg_from_free (from);
	if (sender != NULL)
		smtp_sender_free (sender);
	if (reply_to != NULL)
		smtp_reply_to_free (reply_to);
	if (to != NULL)
		smtp_msg_to_free (to);
	if (cc != NULL)
		smtp_cc_free (cc);
	if (bcc != NULL)
		smtp_bcc_free (bcc);
	if (message_id != NULL)
		smtp_message_id_free (message_id);
	if (in_reply_to != NULL)
		smtp_in_reply_to_free (in_reply_to);
	if (references != NULL)
		smtp_references_free (references);
	if (subject != NULL)
		smtp_subject_free (subject);
	if (comments != NULL)
		smtp_comments_free (comments);
	if (keywords != NULL)
		smtp_keywords_free (keywords);
	if (optional_field != NULL)
		smtp_optional_field_free (optional_field);
      err:
	return res;
}
#endif


#if 0				//xiayu we didn't use the following stuff
int
smtp_fields_parse (const char *message, size_t length,
		   size_t * index, struct smtp_fields **result)
{
	size_t cur_token;
	clist *list;
	struct smtp_fields *fields;
	int r;
	int res;

	cur_token = *index;

	list = NULL;

	r = smtp_struct_multiple_parse (message, length, &cur_token,
					&list, (smtp_struct_parser *)
					smtp_field_parse,
					(smtp_struct_destructor *)
					smtp_field_free);
	/*
	   if ((r != SMTP_NO_ERROR) && (r != SMTP_ERROR_PARSE)) {
	   res = r;
	   goto err;
	   }
	 */

	switch (r) {
	case SMTP_NO_ERROR:
		/* do nothing */
		break;

	case SMTP_ERROR_PARSE:
		list = clist_new ();
		if (list == NULL) {
			res = SMTP_ERROR_MEMORY;
			goto err;
		}
		break;

	default:
		res = r;
		goto err;
	}

	fields = smtp_fields_new (list);
	if (fields == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free;
	}

	*result = fields;
	*index = cur_token;

	return SMTP_NO_ERROR;

      free:
	if (list != NULL) {
		clist_foreach (list, (clist_func) smtp_field_free, NULL);
		clist_free (list);
	}
      err:
	return res;
}
#endif

/*
optional-field  =       field-name ":" unstructured CRLF
*/

int
smtp_optional_field_parse (struct smtp_info *psmtp, const char *message,
			   size_t length, size_t * index,
			   struct smtp_optional_field **result)
{
	char *name;
	char *value;
	struct smtp_optional_field *optional_field;
	size_t cur_token;
	int r;
	int res;

	cur_token = *index;

	r = smtp_field_name_parse (message, length, &cur_token, &name);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	r = smtp_colon_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free_name;
	}

	r = smtp_unstructured_parse (message, length, &cur_token, &value);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free_name;
	}

	r = smtp_unstrict_crlf_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free_value;
	}

	optional_field = smtp_optional_field_new (name, value);
	if (optional_field == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free_value;
	}

	*result = optional_field;
	*index = cur_token;

	return SMTP_NO_ERROR;

      free_value:
	smtp_unstructured_free (value);
      free_name:
	smtp_field_name_free (name);
      err:
	return res;
}




#define YYCURSOR  p1
#define YYCTYPE   char
#define YYLIMIT   p2
#define YYMARKER  p3
#define YYRESCAN  yy2
#define YYFILL(n)

/*!re2c
any     = [\000-\377];
print	= [\040-\176];
digit   = [0-9];
letter  = [a-zA-Z];
space	= [\040];
*/

int
smtp_envelope_field_parse (struct smtp_info *psmtp, const char *message,
			   size_t length, size_t * index,
			   struct smtp_mime_field **result, int *fld_type)
{
	size_t cur_token;
	int type;
	struct smtp_from *from;
	struct smtp_to *to;

	/* MIME stuff */
	struct smtp_mime_content_type *content_type;
	struct smtp_mime_disposition *disposition;

	struct smtp_mime_field *field;
	int r, res;
	char *p, *p1, *p2, *p3;

	cur_token = *index;
	p1 = (char *) (message + cur_token);
	p2 = p1 + length;

	from = NULL;
	to = NULL;

	/* MIME stuff */
	content_type = NULL;
	disposition = NULL;

	/* *INDENT-OFF* */
	/*!re2c
	"\r\n"  
	{ 
		DEBUG_SMTP(SMTP_DBG, "smtp_envelope_field_parse: CRLF\n");
		cur_token = ( p1 - message); 

#ifdef USE_NEL
		if (( r = nel_env_analysis(eng, &(psmtp->env), eoh_id,  
				(struct smtp_simple_event *)0  )) < 0 ) {
			DEBUG_SMTP(SMTP_DBG, "smtp_from_parse: nel_env_analysis error\n");
			res = SMTP_ERROR_ENGINE;
			goto err;
		}
#endif
		* fld_type = SMTP_FIELD_EOH;
		goto succ;
	}

	[Ff][Rr][Oo][Mm][:]
	{ 
		DEBUG_SMTP(SMTP_DBG, "smtp_envelope_field_parse: From:\n");
		cur_token = ( p1 - message); 
		r = smtp_msg_from_parse(/*ptcp,*/ psmtp, message, length, &cur_token,&from);
		if (r == SMTP_NO_ERROR) {
			* fld_type = SMTP_FIELD_FROM;
			goto succ;
		}
		else {
			res = r;
			goto err;
		}
	}

	[Tt][Oo][:]
	{ 
		DEBUG_SMTP(SMTP_DBG, "smtp_envelope_field_parse: To:\n");
		cur_token = ( p1 - message); 
		r = smtp_msg_to_parse(/*ptcp,*/ psmtp, message, length, &cur_token, &to);
		if (r == SMTP_NO_ERROR) {
			* fld_type = SMTP_FIELD_TO;
			goto succ;
		}
		else {
			res = r;
			goto err;
		}

	}

	[Cc][Oo][Nn][Tt][Ee][Nn][Tt][-][Tt][Yy][Pp][Ee][:]
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_envelope_field_parse: Content-Type:\n");
		cur_token = ( p1 - message); 

		r = smtp_mime_content_type_parse(psmtp, message, length, &cur_token, &content_type);
		if (r == SMTP_NO_ERROR){
			DEBUG_SMTP(SMTP_DBG, "content=%p\n", content_type);
			* fld_type = SMTP_MIME_FIELD_TYPE;
			goto succ;
		}
		else {
			res = r;
			goto err;
		}
	} 

	[Cc][Oo][Nn][Tt][Ee][Nn][Tt][-][Tt][Rr][Aa][Nn][Ss][Ff][Ee][Rr][-][Ee][Nn][Cc][Oo][Dd][Ii][Nn][Gg][:]
	{ 
		int encode_type;
		DEBUG_SMTP(SMTP_DBG, "smtp_envelope_field_parse: Content-Transfer-Encoding:\n");
		cur_token = ( p1 - message); 

		r = smtp_mime_encoding_parse(&psmtp->part_stack[psmtp->part_stack_top], message, length, &cur_token, &encode_type);
		if (r == SMTP_NO_ERROR){
			* fld_type = SMTP_MIME_FIELD_TRANSFER_ENCODING;
			goto succ;
		}
		else {
			res = r;
			goto err;
		}
	}

	[Cc][Oo][Nn][Tt][Ee][Nn][Tt][-][Dd][Ii][Ss][Pp][Oo][Ss][Ii][Tt][Ii][Oo][Nn][:]
	{ 
		struct smtp_mime_disposition *disposition;
		DEBUG_SMTP(SMTP_DBG, "smtp_envelope_field_parse: Content-Disposition:\n");
		cur_token = ( p1 - message); 

		r = smtp_mime_disposition_parse(psmtp, message, length, &cur_token, &disposition);

		if (r == SMTP_NO_ERROR){
			* fld_type = SMTP_MIME_FIELD_DISPOSITION;
			goto succ;
		}
		else {
			DEBUG_SMTP(SMTP_DBG, "\n");
			res = r;
			goto err;
		}
	}

	".\r\n"
	{
		//xiayu 2005.12.2 dealing with telnet end with CRLF.CRLF
		//NOTE: this is a temperory branch, 
		//would be delete in the future
		DEBUG_SMTP(SMTP_DBG, "\n");
		res = SMTP_ERROR_PARSE;
		goto err;
	}

	any
	{
		DEBUG_SMTP(SMTP_DBG, "ANY cur_token = %d, length = %d\n", cur_token, length);
		DEBUG_SMTP(SMTP_DBG, "message+cur_token: %s\n", message+cur_token);

		r = smtp_str_crlf_parse (message, length, &cur_token);
		DEBUG_SMTP(SMTP_DBG, "message+cur_token: %s\n", message+cur_token);
		if (r != SMTP_NO_ERROR) {
			/* xiay 2005.11.8 Fixme: 
			   when meet unknown keywords and didn't find it's CRLF,
			   we temperaly skip those data to avoid system crashing down */
			//xiayu 2005.11.23
			// *index = length;
			DEBUG_SMTP(SMTP_DBG, "\n");
			res = r;
			goto err;

		} else {
			DEBUG_SMTP(SMTP_DBG, "\n");
			*fld_type = SMTP_FIELD_NONE;
			*result = NULL;
			*index = cur_token;
			return SMTP_NO_ERROR;
		}

	}
	*/
	/* *INDENT-ON* */

#if 0
	if (type == SMTP_FIELD_NONE) {
		r = smtp_optional_field_parse (message, length, &cur_token,
					       &optional_field);
		if (r != SMTP_NO_ERROR) {
			res = r;
			goto err;
		}
		type = SMTP_FIELD_OPTIONAL_FIELD;
	}
#endif

      succ:

	DEBUG_SMTP (SMTP_DBG, "\n");
	DEBUG_SMTP (SMTP_DBG, "cur_token = %d\n", cur_token);
	*result = field;
	*index = cur_token;
	return SMTP_NO_ERROR;

      err:
	DEBUG_SMTP (SMTP_DBG, "\n");
	return res;

}



int
smtp_envelope_or_optional_field_parse (struct smtp_info *psmtp,
				       const char *message, size_t length,
				       size_t * index,
				       struct smtp_mime_field **result,
				       int *fld_type)
{
	int r;
	size_t cur_token;
	struct smtp_optional_field *optional_field;
	struct smtp_mime_field *field;

	DEBUG_SMTP (SMTP_DBG, "index=%p, *index=%d\n", index, *index);

	cur_token = *index;
	r = smtp_envelope_field_parse ( /* ptcp, */ psmtp, message, length,
				       &cur_token, result, fld_type);
	if (r == SMTP_NO_ERROR) {
		DEBUG_SMTP (SMTP_DBG, "\n");
		*index = cur_token;
		return r;
	}

#if 1				//xiayu 2005.11.16 SMTP_ERROR_CONTINUE
	else {
		DEBUG_SMTP (SMTP_DBG, "\n");
		return r;
	}

#else
	else if (r == SMTP_ERROR_POLICY) {
		DEBUG_SMTP (SMTP_DBG, "\n");
		return r;

	}
	else {
		DEBUG_SMTP (SMTP_DBG, "\n");
		return SMTP_ERROR_CONTINUE;
	}
#endif


	r = smtp_optional_field_parse (psmtp, message, length, &cur_token,
				       &optional_field);
	DEBUG_SMTP (SMTP_DBG, "\n");
	if (r != SMTP_NO_ERROR)
		return r;

	DEBUG_SMTP (SMTP_DBG, "\n");
	field = smtp_mime_field_new (SMTP_FIELD_OPTIONAL_FIELD,
				     optional_field);
	DEBUG_SMTP (SMTP_DBG, "\n");
	if (field == NULL) {
		DEBUG_SMTP (SMTP_DBG, "\n");
		smtp_optional_field_free (optional_field);
		return SMTP_ERROR_MEMORY;
	}

	DEBUG_SMTP (SMTP_DBG, "\n");
//xiayu 2005.10.28 fixme smtp_field or smtp_mime_field
	*result = field;
	*index = cur_token;
	*fld_type = SMTP_FIELD_OPTIONAL_FIELD;

	return SMTP_NO_ERROR;
}

/*
x  entity-headers := [ content CRLF ]
                    [ encoding CRLF ]
                    [ id CRLF ]
                    [ description CRLF ]
                    *( MIME-extension-field CRLF )
		    */

enum
{
	FIELD_STATE_START,
	FIELD_STATE_T,
	FIELD_STATE_D
};

int
guess_field_type (char *name)
{
	int state;

	if (*name == 'M')
		return SMTP_MIME_FIELD_VERSION;

	if (strncasecmp (name, "Content-", 8) != 0)
		return SMTP_MIME_FIELD_NONE;

	name += 8;

	state = FIELD_STATE_START;

	while (1) {

		switch (state) {

		case FIELD_STATE_START:
			switch ((char) toupper ((unsigned char) *name)) {
			case 'T':
				state = FIELD_STATE_T;
				break;
			case 'I':
				return SMTP_MIME_FIELD_ID;
			case 'D':
				state = FIELD_STATE_D;
				break;
			case 'L':
				return SMTP_MIME_FIELD_LANGUAGE;
			default:
				return SMTP_MIME_FIELD_NONE;
			}
			break;

		case FIELD_STATE_T:
			switch ((char) toupper ((unsigned char) *name)) {
			case 'Y':
				return SMTP_MIME_FIELD_TYPE;
			case 'R':
				return SMTP_MIME_FIELD_TRANSFER_ENCODING;
			default:
				return SMTP_MIME_FIELD_NONE;
			}
			break;

		case FIELD_STATE_D:
			switch ((char) toupper ((unsigned char) *name)) {
			case 'E':
				return SMTP_MIME_FIELD_DESCRIPTION;
			case 'I':
				return SMTP_MIME_FIELD_DISPOSITION;
			default:
				return SMTP_MIME_FIELD_NONE;
			}
			break;
		}
		name++;
	}
}
