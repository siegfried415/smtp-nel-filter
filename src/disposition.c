

/*
 * $Id: disposition.c,v 1.16 2005/12/20 08:30:43 xiay Exp $
  RFC 2045, RFC 2046, RFC 2047, RFC 2048, RFC 2049, RFC 2231, RFC 2387
  RFC 2424, RFC 2557, RFC 2183 Content-Disposition, RFC 1766  Language
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "mmapstring.h"
#include "mem_pool.h"

#include "smtp.h"
#include "mime.h"
#include "disposition.h"
#include "fields.h"

#ifdef USE_NEL
#include "engine.h"
int dsp_id;
extern struct nel_eng *eng;
#endif

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif


ObjPool_t smtp_mime_disposition_pool;
ObjPool_t smtp_mime_dsp_parm_pool;
ObjPool_t smtp_mime_dsp_type_pool;


void
smtp_mime_disposition_init (
#ifdef USE_NEL
				   struct nel_eng *eng
#endif
	)
{
	/* Content-type */
	create_mem_pool (&smtp_mime_disposition_pool,
			 sizeof (struct smtp_mime_disposition),
			 SMTP_MEM_STACK_DEPTH);

	/* type */
	create_mem_pool (&smtp_mime_dsp_parm_pool,
			 sizeof (struct smtp_mime_disposition_parm),
			 SMTP_MEM_STACK_DEPTH);
	create_mem_pool (&smtp_mime_dsp_type_pool,
			 sizeof (struct smtp_mime_disposition_type),
			 SMTP_MEM_STACK_DEPTH);
}


char *
get_attachment_file (struct smtp_mime_disposition *dsp)
{
	clistiter *cur;

	for (cur = clist_begin (dsp->dsp_parms);
	     cur != NULL; cur = clist_next (cur)) {
		struct smtp_mime_disposition_parm *param;

		param = clist_content (cur);

		if (param->pa_type == SMTP_MIME_DISPOSITION_PARM_FILENAME) {
			return param->pa_data.pa_filename;
		}
	}

	return NULL;
}

void
smtp_mime_quoted_date_time_free (char *date)
{
	smtp_quoted_string_free (date);
}

void
smtp_mime_creation_date_parm_free (char *date)
{
	smtp_mime_quoted_date_time_free (date);
}

void
smtp_mime_modification_date_parm_free (char *date)
{
	smtp_mime_quoted_date_time_free (date);
}

void
smtp_mime_read_date_parm_free (char *date)
{
	smtp_mime_quoted_date_time_free (date);
}

void
smtp_mime_filename_parm_free (char *filename)
{
	smtp_mime_value_free (filename);
}


void
smtp_mime_disposition_parm_free (struct smtp_mime_disposition_parm *dsp_parm)
{
	switch (dsp_parm->pa_type) {
	case SMTP_MIME_DISPOSITION_PARM_FILENAME:
		smtp_mime_filename_parm_free (dsp_parm->pa_data.pa_filename);
		break;
	case SMTP_MIME_DISPOSITION_PARM_CREATION_DATE:
		smtp_mime_creation_date_parm_free (dsp_parm->pa_data.
						   pa_creation_date);
		break;
	case SMTP_MIME_DISPOSITION_PARM_MODIFICATION_DATE:
		smtp_mime_modification_date_parm_free (dsp_parm->pa_data.
						       pa_modification_date);
		break;
	case SMTP_MIME_DISPOSITION_PARM_READ_DATE:
		smtp_mime_read_date_parm_free (dsp_parm->pa_data.
					       pa_read_date);
		break;
	case SMTP_MIME_DISPOSITION_PARM_PARAMETER:
		smtp_mime_parameter_free (dsp_parm->pa_data.pa_parameter);
		break;
	}

	//free(dsp_parm);
	free_mem (&smtp_mime_dsp_parm_pool, dsp_parm);
	DEBUG_SMTP (SMTP_MEM,
		    "smtp_mime_disposition_parm_free: pointer=%p, elm=%p\n",
		    &smtp_mime_dsp_parm_pool, dsp_parm);
}


struct smtp_mime_disposition_parm *
smtp_mime_disposition_parm_new (int pa_type,
				char *pa_filename,
				char *pa_creation_date,
				char *pa_modification_date,
				char *pa_read_date,
				size_t pa_size,
				struct smtp_mime_parameter *pa_parameter)
{
	struct smtp_mime_disposition_parm *dsp_parm;

	//dsp_parm = malloc(sizeof(* dsp_parm));
	dsp_parm =
		(struct smtp_mime_disposition_parm *)
		alloc_mem (&smtp_mime_dsp_parm_pool);
	if (dsp_parm == NULL)
		return NULL;
	DEBUG_SMTP (SMTP_MEM,
		    "smtp_mime_disposition_parm_new: pointer=%p, elm=%p\n",
		    &smtp_mime_dsp_parm_pool, dsp_parm);

	dsp_parm->pa_type = pa_type;
	switch (pa_type) {
	case SMTP_MIME_DISPOSITION_PARM_FILENAME:
		dsp_parm->pa_data.pa_filename = pa_filename;
		break;
	case SMTP_MIME_DISPOSITION_PARM_CREATION_DATE:
		dsp_parm->pa_data.pa_creation_date = pa_creation_date;
		break;
	case SMTP_MIME_DISPOSITION_PARM_MODIFICATION_DATE:
		dsp_parm->pa_data.pa_modification_date = pa_modification_date;
		break;
	case SMTP_MIME_DISPOSITION_PARM_READ_DATE:
		dsp_parm->pa_data.pa_read_date = pa_read_date;
		break;
	case SMTP_MIME_DISPOSITION_PARM_SIZE:
		dsp_parm->pa_data.pa_size = pa_size;
		break;
	case SMTP_MIME_DISPOSITION_PARM_PARAMETER:
		dsp_parm->pa_data.pa_parameter = pa_parameter;
		break;
	}

	return dsp_parm;
}


void
smtp_mime_disposition_type_free (struct smtp_mime_disposition_type *dsp_type)
{
	if (dsp_type->dsp_extension != NULL) {
		free (dsp_type->dsp_extension);
		DEBUG_SMTP (SMTP_MEM_1,
			    "smtp_mime_disposition_type_free: FREE pointer=%p\n",
			    dsp_type->dsp_extension);
	}
	//free(dsp_type);
	free_mem (&smtp_mime_dsp_type_pool, dsp_type);
	DEBUG_SMTP (SMTP_MEM,
		    "smtp_mime_disposition_type_free: pointer=%p, elm=%p\n",
		    &smtp_mime_dsp_type_pool, dsp_type);
}


struct smtp_mime_disposition_type *
smtp_mime_disposition_type_new (int dsp_type, char *dsp_extension)
{
	struct smtp_mime_disposition_type *m_dsp_type;

	//m_dsp_type = malloc(sizeof(* m_dsp_type));
	m_dsp_type =
		(struct smtp_mime_disposition_type *)
		alloc_mem (&smtp_mime_dsp_type_pool);
	if (m_dsp_type == NULL)
		return NULL;
	DEBUG_SMTP (SMTP_MEM,
		    "smtp_mime_disposition_type_new: pointer=%p, elm=%p\n",
		    &smtp_mime_dsp_type_pool, m_dsp_type);

	m_dsp_type->dsp_type = dsp_type;
	m_dsp_type->dsp_extension = dsp_extension;

	return m_dsp_type;
}



void
smtp_mime_disposition_free (struct smtp_mime_disposition *dsp)
{
	smtp_mime_disposition_type_free (dsp->dsp_type);
	clist_foreach (dsp->dsp_parms,
		       (clist_func) smtp_mime_disposition_parm_free, NULL);
	clist_free (dsp->dsp_parms);
	//free(dsp);
	free_mem (&smtp_mime_disposition_pool, dsp);
	DEBUG_SMTP (SMTP_MEM,
		    "smtp_mime_disposition_free: pointer=%p, elm=%p\n",
		    &smtp_mime_disposition_pool, dsp);
}


struct smtp_mime_disposition *
smtp_mime_disposition_new (struct smtp_mime_disposition_type *dsp_type,
			   clist * dsp_parms)
{
	struct smtp_mime_disposition *dsp;

	//dsp = malloc(sizeof(* dsp));
	dsp = (struct smtp_mime_disposition *)
		alloc_mem (&smtp_mime_disposition_pool);
	if (dsp == NULL)
		return NULL;
	DEBUG_SMTP (SMTP_MEM,
		    "smtp_mime_disposition_new: pointer=%p, elm=%p\n",
		    &smtp_mime_disposition_pool, dsp);


#ifdef USE_NEL
	//xiayu 2005.11.28
	//dsp->count = 0;
	NEL_REF_INIT (dsp);
#endif

	dsp->dsp_type = dsp_type;
	dsp->dsp_parms = dsp_parms;

	DEBUG_SMTP (SMTP_DBG, "NEW DISPOSITION get_disposition_file: %s\n",
		    get_attachment_file (dsp));

	dsp->err_cnt = 0;
	return dsp;
}


struct smtp_mime_disposition *
smtp_mime_disposition_new_with_data (int type,
				     char *filename, char *creation_date,
				     char *modification_date, char *read_date,
				     size_t size)
{
	struct smtp_mime_disposition_type *dsp_type;
	clist *list;
	int r;
	struct smtp_mime_disposition_parm *parm;
	struct smtp_mime_disposition *dsp;

	dsp_type = smtp_mime_disposition_type_new (type, NULL);
	if (dsp_type == NULL)
		goto err;

	list = clist_new ();
	if (list == NULL)
		goto free_dsp_type;

	if (filename != NULL) {
		parm = smtp_mime_disposition_parm_new
			(SMTP_MIME_DISPOSITION_PARM_FILENAME, filename, NULL,
			 NULL, NULL, 0, NULL);
		if (parm == NULL)
			goto free_list;

		r = clist_append (list, parm);
		if (r < 0) {
			smtp_mime_disposition_parm_free (parm);
			goto free_list;
		}
	}

	if (creation_date != NULL) {
		parm = smtp_mime_disposition_parm_new
			(SMTP_MIME_DISPOSITION_PARM_CREATION_DATE, NULL,
			 creation_date, NULL, NULL, 0, NULL);
		if (parm == NULL)
			goto free_list;

		r = clist_append (list, parm);
		if (r < 0) {
			smtp_mime_disposition_parm_free (parm);
			goto free_list;
		}
	}

	if (modification_date != NULL) {
		parm = smtp_mime_disposition_parm_new
			(SMTP_MIME_DISPOSITION_PARM_MODIFICATION_DATE, NULL,
			 NULL, modification_date, NULL, 0, NULL);
		if (parm == NULL)
			goto free_list;

		r = clist_append (list, parm);
		if (r < 0) {
			smtp_mime_disposition_parm_free (parm);
			goto free_list;
		}
	}

	if (read_date != NULL) {
		parm = smtp_mime_disposition_parm_new
			(SMTP_MIME_DISPOSITION_PARM_READ_DATE, NULL, NULL,
			 NULL, read_date, 0, NULL);
		if (parm == NULL)
			goto free_list;

		r = clist_append (list, parm);
		if (r < 0) {
			smtp_mime_disposition_parm_free (parm);
			goto free_list;
		}
	}

	if (size != (size_t) - 1) {
		parm = smtp_mime_disposition_parm_new
			(SMTP_MIME_DISPOSITION_PARM_SIZE, NULL, NULL, NULL,
			 NULL, size, NULL);
		if (parm == NULL)
			goto free_list;

		r = clist_append (list, parm);
		if (r < 0) {
			smtp_mime_disposition_parm_free (parm);
			goto free_list;
		}
	}

	dsp = smtp_mime_disposition_new (dsp_type, list);

	return dsp;

      free_list:
	clist_foreach (list, (clist_func) smtp_mime_disposition_parm_free,
		       NULL);
	clist_free (list);
      free_dsp_type:
	smtp_mime_disposition_type_free (dsp_type);
      err:
	return NULL;
}



struct smtp_mime_disposition *
smtp_mime_disposition_new_filename (int type, char *filename)
{
	return smtp_mime_disposition_new_with_data (type, filename,
						    NULL, NULL, NULL,
						    (size_t) - 1);
}



void
smtp_mime_disposition_single_fields_init (struct
					  smtp_mime_single_fields
					  *single_fields,
					  struct smtp_mime_disposition
					  *fld_disposition)
{
	clistiter *cur;

	single_fields->fld_disposition = fld_disposition;

	for (cur = clist_begin (fld_disposition->dsp_parms); cur != NULL;
	     cur = clist_next (cur)) {
		struct smtp_mime_disposition_parm *param;

		param = clist_content (cur);

		switch (param->pa_type) {
		case SMTP_MIME_DISPOSITION_PARM_FILENAME:
			single_fields->fld_disposition_filename =
				param->pa_data.pa_filename;
			break;

		case SMTP_MIME_DISPOSITION_PARM_CREATION_DATE:
			single_fields->fld_disposition_creation_date =
				param->pa_data.pa_creation_date;
			break;

		case SMTP_MIME_DISPOSITION_PARM_MODIFICATION_DATE:
			single_fields->fld_disposition_modification_date =
				param->pa_data.pa_modification_date;
			break;

		case SMTP_MIME_DISPOSITION_PARM_READ_DATE:
			single_fields->fld_disposition_read_date =
				param->pa_data.pa_read_date;
			break;

		case SMTP_MIME_DISPOSITION_PARM_SIZE:
			single_fields->fld_disposition_size =
				param->pa_data.pa_size;
			break;
		}
	}
}




/*
     disposition-parm := filename-parm
                       / creation-date-parm
                       / modification-date-parm
                       / read-date-parm
                       / size-parm
                       / parameter
*/
int
smtp_mime_disposition_guess_type (const char *message, size_t length,
				  size_t index)
{
	if (index >= length)
		return SMTP_MIME_DISPOSITION_PARM_PARAMETER;

	switch ((char) toupper ((unsigned char) message[index])) {
	case 'F':
		return SMTP_MIME_DISPOSITION_PARM_FILENAME;
	case 'C':
		return SMTP_MIME_DISPOSITION_PARM_CREATION_DATE;
	case 'M':
		return SMTP_MIME_DISPOSITION_PARM_MODIFICATION_DATE;
	case 'R':
		return SMTP_MIME_DISPOSITION_PARM_READ_DATE;
	case 'S':
		return SMTP_MIME_DISPOSITION_PARM_SIZE;
	default:
		return SMTP_MIME_DISPOSITION_PARM_PARAMETER;
	}
}



/*
 * filename-parm := "filename" "=" value
 */
static int
smtp_mime_filename_parm_parse (const char *message, size_t length,
			       size_t * index, char **result)
{
	char *value;
	int r;
	size_t cur_token;

	cur_token = *index;

	r = smtp_token_case_insensitive_parse (message, length,
					       &cur_token, "filename");
	if (r != SMTP_NO_ERROR)
		return r;

	r = smtp_unstrict_char_parse (message, length, &cur_token, '=');
	if (r != SMTP_NO_ERROR)
		return r;

	r = smtp_mime_value_parse (message, length, &cur_token, &value);
	if (r != SMTP_NO_ERROR)
		return r;

	*index = cur_token;
	*result = value;

	return SMTP_NO_ERROR;
}



/*
 * quoted-date-time := quoted-string
 *                     ; contents MUST be an RFC 822 `date-time'
 *                     ; numeric timezones (+HHMM or -HHMM) MUST be used
 */
static int
smtp_mime_quoted_date_time_parse (const char *message, size_t length,
				  size_t * index, char **result)
{
	return smtp_quoted_string_parse (message, length, index, result);
}



/*
 * creation-date-parm := "creation-date" "=" quoted-date-time
 */
static int
smtp_mime_creation_date_parm_parse (const char *message, size_t length,
				    size_t * index, char **result)
{
	char *value;
	int r;
	size_t cur_token;

	cur_token = *index;

	r = smtp_token_case_insensitive_parse (message, length,
					       &cur_token, "creation-date");
	if (r != SMTP_NO_ERROR)
		return r;

	r = smtp_unstrict_char_parse (message, length, &cur_token, '=');
	if (r != SMTP_NO_ERROR)
		return r;

	r = smtp_mime_quoted_date_time_parse (message, length, &cur_token,
					      &value);
	if (r != SMTP_NO_ERROR)
		return r;

	*index = cur_token;
	*result = value;

	return SMTP_NO_ERROR;
}

/*
 * modification-date-parm := "modification-date" "=" quoted-date-time
 */

static int
smtp_mime_modification_date_parm_parse (const char *message, size_t length,
					size_t * index, char **result)
{
	char *value;
	size_t cur_token;
	int r;

	cur_token = *index;

	r = smtp_token_case_insensitive_parse (message, length,
					       &cur_token,
					       "modification-date");
	if (r != SMTP_NO_ERROR)
		return r;

	r = smtp_unstrict_char_parse (message, length, &cur_token, '=');
	if (r != SMTP_NO_ERROR)
		return r;

	r = smtp_mime_quoted_date_time_parse (message, length, &cur_token,
					      &value);
	if (r != SMTP_NO_ERROR)
		return r;

	*index = cur_token;
	*result = value;

	return SMTP_NO_ERROR;
}


/*
 * read-date-parm := "read-date" "=" quoted-date-time
 */
static int
smtp_mime_read_date_parm_parse (const char *message, size_t length,
				size_t * index, char **result)
{
	char *value;
	size_t cur_token;
	int r;

	cur_token = *index;

	r = smtp_token_case_insensitive_parse (message, length,
					       &cur_token, "read-date");
	if (r != SMTP_NO_ERROR)
		return r;

	r = smtp_unstrict_char_parse (message, length, &cur_token, '=');
	if (r != SMTP_NO_ERROR)
		return r;

	r = smtp_mime_quoted_date_time_parse (message, length, &cur_token,
					      &value);
	if (r != SMTP_NO_ERROR)
		return r;

	*index = cur_token;
	*result = value;

	return SMTP_NO_ERROR;
}


/*
 * size-parm := "size" "=" 1*DIGIT
 */
static int
smtp_mime_size_parm_parse (const char *message, size_t length,
			   size_t * index, size_t * result)
{
	uint32_t value;
	size_t cur_token;
	int r;

	cur_token = *index;

	r = smtp_token_case_insensitive_parse (message, length,
					       &cur_token, "size");
	if (r != SMTP_NO_ERROR)
		return r;

	r = smtp_unstrict_char_parse (message, length, &cur_token, '=');
	if (r != SMTP_NO_ERROR)
		return r;

	r = smtp_number_parse (message, length, &cur_token, &value);
	if (r != SMTP_NO_ERROR)
		return r;

	*index = cur_token;
	*result = value;

	return SMTP_NO_ERROR;
}



static int
smtp_mime_disposition_parm_parse (const char *message, size_t length,
				  size_t * index,
				  struct smtp_mime_disposition_parm **result)
{
	char *filename;
	char *creation_date;
	char *modification_date;
	char *read_date;
	size_t size;
	struct smtp_mime_parameter *parameter;
	size_t cur_token;
	struct smtp_mime_disposition_parm *dsp_parm;
	int type;
	int guessed_type;
	int r;
	int res;

	cur_token = *index;

	filename = NULL;
	creation_date = NULL;
	modification_date = NULL;
	read_date = NULL;
	size = 0;
	parameter = NULL;

	r = smtp_cfws_parse (message, length, &cur_token);
	if ((r != SMTP_NO_ERROR) && (r != SMTP_ERROR_PARSE)) {
		res = r;
		goto err;
	}

	guessed_type =
		smtp_mime_disposition_guess_type (message, length, cur_token);

	type = SMTP_MIME_DISPOSITION_PARM_PARAMETER;

	switch (guessed_type) {
	case SMTP_MIME_DISPOSITION_PARM_FILENAME:
		r = smtp_mime_filename_parm_parse (message, length,
						   &cur_token, &filename);
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

	case SMTP_MIME_DISPOSITION_PARM_CREATION_DATE:
		r = smtp_mime_creation_date_parm_parse (message, length,
							&cur_token,
							&creation_date);
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

	case SMTP_MIME_DISPOSITION_PARM_MODIFICATION_DATE:
		r = smtp_mime_modification_date_parm_parse (message, length,
							    &cur_token,
							    &modification_date);
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

	case SMTP_MIME_DISPOSITION_PARM_READ_DATE:
		r = smtp_mime_read_date_parm_parse (message, length,
						    &cur_token, &read_date);
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

	case SMTP_MIME_DISPOSITION_PARM_SIZE:
		r = smtp_mime_size_parm_parse (message, length, &cur_token,
					       &size);
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

	if (type == SMTP_MIME_DISPOSITION_PARM_PARAMETER) {
		r = smtp_mime_parameter_parse (message, length, &cur_token,
					       &parameter);
		if (r != SMTP_NO_ERROR) {
			type = guessed_type;
			res = r;
			goto err;
		}
	}

	dsp_parm =
		smtp_mime_disposition_parm_new (type, filename, creation_date,
						modification_date, read_date,
						size, parameter);

	if (dsp_parm == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free;
	}

	*result = dsp_parm;
	*index = cur_token;

	return SMTP_NO_ERROR;

      free:
	if (filename != NULL)
		smtp_mime_filename_parm_free (dsp_parm->pa_data.pa_filename);
	if (creation_date != NULL)
		smtp_mime_creation_date_parm_free (dsp_parm->pa_data.
						   pa_creation_date);
	if (modification_date != NULL)
		smtp_mime_modification_date_parm_free (dsp_parm->pa_data.
						       pa_modification_date);
	if (read_date != NULL)
		smtp_mime_read_date_parm_free (dsp_parm->pa_data.
					       pa_read_date);
	if (parameter != NULL)
		smtp_mime_parameter_free (dsp_parm->pa_data.pa_parameter);
      err:
	return res;
}


/*		    
 * disposition-type := "inline"
 *                      / "attachment"
 *                      / extension-token
 *                      ; values are not case-sensitive
 *
 */
int
smtp_mime_disposition_type_parse (const char *message, size_t length,
				  size_t * index,
				  struct smtp_mime_disposition_type **result)
{
	size_t cur_token;
	int type;
	char *extension;
	struct smtp_mime_disposition_type *dsp_type;
	int r;
	int res;

	cur_token = *index;

	DEBUG_SMTP (SMTP_DBG, "message+cur_token : %s\n",
		    message + cur_token);
	r = smtp_cfws_parse (message, length, &cur_token);
	if ((r != SMTP_NO_ERROR) && (r != SMTP_ERROR_PARSE)) {
		res = r;
		goto err;
	}

	DEBUG_SMTP (SMTP_DBG, "message+cur_token : %s\n",
		    message + cur_token);
	type = SMTP_MIME_DISPOSITION_TYPE_ERROR;	/* XXX - removes a gcc warning */

	extension = NULL;

	r = smtp_token_case_insensitive_parse (message, length,
					       &cur_token, "inline");
	if (r == SMTP_NO_ERROR) {
		DEBUG_SMTP (SMTP_DBG, "\n");
		type = SMTP_MIME_DISPOSITION_TYPE_INLINE;
	}

	DEBUG_SMTP (SMTP_DBG, "message+cur_token : %s\n",
		    message + cur_token);
	if (r == SMTP_ERROR_PARSE) {
		DEBUG_SMTP (SMTP_DBG, "\n");
		r = smtp_token_case_insensitive_parse (message, length,
						       &cur_token,
						       "attachment");
		if (r == SMTP_NO_ERROR) {
			DEBUG_SMTP (SMTP_DBG, "\n");
			type = SMTP_MIME_DISPOSITION_TYPE_ATTACHMENT;
		}
	}

	if (r == SMTP_ERROR_PARSE) {
		DEBUG_SMTP (SMTP_DBG, "\n");
		r = smtp_mime_extension_token_parse (message, length,
						     &cur_token, &extension);
		if (r == SMTP_NO_ERROR) {
			DEBUG_SMTP (SMTP_DBG, "\n");
			type = SMTP_MIME_DISPOSITION_TYPE_EXTENSION;
		}
	}

	if (r != SMTP_NO_ERROR) {
		DEBUG_SMTP (SMTP_DBG, "\n");
		res = r;
		goto err;
	}

	DEBUG_SMTP (SMTP_DBG, "\n");
	dsp_type = smtp_mime_disposition_type_new (type, extension);
	if (dsp_type == NULL) {
		DEBUG_SMTP (SMTP_DBG, "\n");
		res = SMTP_ERROR_MEMORY;
		goto free;
	}

	*result = dsp_type;
	*index = cur_token;

	return SMTP_NO_ERROR;

      free:
	DEBUG_SMTP (SMTP_DBG, "\n");
	if (extension != NULL) {
		free (extension);
		DEBUG_SMTP (SMTP_MEM_1,
			    "smtp_mime_disposition_type_parse: FREE pointer=%p\n",
			    extension);
	}
      err:
	DEBUG_SMTP (SMTP_DBG, "\n");
	return res;
}

/*
 * disposition := "Content-Disposition" ":"
 *                 disposition-type
 *                 *(";" disposition-parm)
 *
 */
int
smtp_mime_disposition_parse ( /* struct neti_tcp_stream *ptcp, */ struct
			     smtp_info *psmtp,
			     const char *message, size_t length,
			     size_t * index,
			     struct smtp_mime_disposition **result)
{
	size_t final_token;
	size_t cur_token;
	struct smtp_mime_disposition_type *dsp_type;
	clist *list;
	struct smtp_mime_disposition *dsp;
	int r;
	int res;

	cur_token = *index;

	r = smtp_mime_disposition_type_parse (message, length, &cur_token,
					      &dsp_type);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	list = clist_new ();
	if (list == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free_type;
	}

	while (1) {
		struct smtp_mime_disposition_parm *param;

		final_token = cur_token;
		r = smtp_unstrict_char_parse (message, length, &cur_token,
					      ';');
		if (r == SMTP_NO_ERROR) {
			/* do nothing */
		}
		else if (r == SMTP_ERROR_PARSE) {
			break;
		}
		else {
			res = r;
			goto free_list;
		}

		param = NULL;
		r = smtp_mime_disposition_parm_parse (message, length,
						      &cur_token, &param);
		if (r == SMTP_NO_ERROR) {
			/* do nothing */
		}
		else if (r == SMTP_ERROR_PARSE) {
			cur_token = final_token;
			break;
		}
		else {
			res = r;
			goto free_list;
		}

		r = clist_append (list, param);
		if (r < 0) {
			res = SMTP_ERROR_MEMORY;
			goto free_list;
		}
	}

	r = smtp_unstrict_crlf_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = SMTP_ERROR_PARSE;
		return r;
	}

	dsp = smtp_mime_disposition_new (dsp_type, list);
	if (dsp == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free_list;
	}

	DEBUG_SMTP (SMTP_DBG, "Disposition Name: %s\n",
		    get_attachment_file (dsp));

#ifdef USE_NEL
	if ((r = nel_env_analysis (eng, &(psmtp->env), dsp_id,
				   (struct smtp_simple_event *) dsp)) < 0) {
		DEBUG_SMTP (SMTP_DBG,
			    "smtp_from_parse: nel_env_analysis error\n");
		res = SMTP_ERROR_ENGINE;
		smtp_mime_disposition_free (dsp);
		goto err;
	}
#endif

	if (psmtp->permit & SMTP_PERMIT_DENY) {
		//fprintf(stderr, "found a deny event\n");

		/* uncommented by wyong, 2005.11.8 */
		//smtp_close_connection(psmtp); 

		res = SMTP_ERROR_POLICY;
		goto err;

	}
	else if (psmtp->permit & SMTP_PERMIT_DROP) {
		fprintf (stderr,
			 "found a drop event, no drop for message, denied.\n");
		/* uncommented by wyong, 2005.11.8 */
		//smtp_close_connection(psmtp);
		res = SMTP_ERROR_POLICY;
		goto err;
	}

	DEBUG_SMTP (SMTP_DBG, "\n");
	*result = dsp;
	*index = cur_token;

	return SMTP_NO_ERROR;

      free_list:
	clist_foreach (list, (clist_func) smtp_mime_disposition_parm_free,
		       NULL);
	clist_free (list);

      free_type:
	smtp_mime_disposition_type_free (dsp_type);

      err:
	// * result = dsp;
	// *index = cur_token;
	return res;

}
