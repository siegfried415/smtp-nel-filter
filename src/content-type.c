

/*
 * $Id: content-type.c,v 1.23 2005/12/20 08:30:43 xiay Exp $
  RFC 2045, RFC 2046, RFC 2047, RFC 2048, RFC 2049, RFC 2231, RFC 2387
  RFC 2424, RFC 2557, RFC 2183 Content-Disposition, RFC 1766  Language
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "mmapstring.h"
#include "mem_pool.h"

#include "smtp.h"
#include "content-type.h"
#include "mime.h"
#include "fields.h"
#include "message.h"

//#include "match.h"

#ifdef USE_NEL
#include "engine.h"
extern struct nel_eng *eng;
#endif

ObjPool_t smtp_mime_content_type_pool;
ObjPool_t smtp_mime_type_pool;
ObjPool_t smtp_mime_composite_type_pool;
ObjPool_t smtp_mime_discrete_type_pool;

int content_type_id;


#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

char *
smtp_mime_content_type_param_get (struct smtp_mime_content_type *content_type,
				  char *name)
{
	clistiter *cur;

	for (cur = clist_begin (content_type->ct_parameters);
	     cur != NULL; cur = clist_next (cur)) {
		struct smtp_mime_parameter *param;

		param = clist_content (cur);

		if (strcasecmp (param->pa_name, name) == 0)
			return param->pa_value;
	}

	return NULL;
}

char *
smtp_mime_extract_boundary (struct smtp_mime_content_type *content_type)
{
	char *boundary;

	boundary =
		smtp_mime_content_type_param_get (content_type, "boundary");

	if (boundary != NULL) {
		int len;
		char *new_boundary;

		len = strlen (boundary);
		new_boundary = malloc (len + 1);
		if (new_boundary == NULL)
			return NULL;
		DEBUG_SMTP (SMTP_MEM_1,
			    "smtp_mime_extract_boundary: MALLOC pointer=%p\n",
			    new_boundary);

		if (boundary[0] == '"') {
			strncpy (new_boundary, boundary + 1, len - 2);
			new_boundary[len - 2] = 0;
		}
		else
			strcpy (new_boundary, boundary);

		boundary = new_boundary;
	}

	return boundary;
}

char *
get_charset_of_content_type (struct smtp_mime_content_type *content_type)
{
	clistiter *cur;

	for (cur = clist_begin (content_type->ct_parameters);
	     cur != NULL; cur = clist_next (cur)) {
		struct smtp_mime_parameter *param;

		param = clist_content (cur);

		if (strcasecmp (param->pa_name, "charset") == 0)
			return param->pa_value;
	}

	return NULL;
}

void
smtp_mime_content_type_init (
#ifdef USE_NEL
				    struct nel_eng *eng
#endif
	)
{
	/* Content-type */
	create_mem_pool (&smtp_mime_content_type_pool,
			 sizeof (struct smtp_mime_content_type),
			 SMTP_MEM_STACK_DEPTH);

#ifdef USE_NEL
	//todo, move this to smtp.nel, wyong, 20231022 
	//nel_func_name_call (eng, (char *)&content_type_id, "nel_id_of", "content_type");
#endif


	/* type */
	create_mem_pool (&smtp_mime_type_pool,
			 sizeof (struct smtp_mime_type),
			 SMTP_MEM_STACK_DEPTH);
	create_mem_pool (&smtp_mime_composite_type_pool,
			 sizeof (struct smtp_mime_composite_type),
			 SMTP_MEM_STACK_DEPTH);
	create_mem_pool (&smtp_mime_discrete_type_pool,
			 sizeof (struct smtp_mime_discrete_type),
			 SMTP_MEM_STACK_DEPTH);
}


void
smtp_mime_discrete_type_free (struct smtp_mime_discrete_type *discrete_type)
{
	if (discrete_type->dt_extension != NULL)
		smtp_mime_extension_token_free (discrete_type->dt_extension);
	//free(discrete_type);
	free_mem (&smtp_mime_discrete_type_pool, discrete_type);
	DEBUG_SMTP (SMTP_MEM,
		    "smtp_mime_discrete_type_free: pointer=%p, elm=%p\n",
		    &smtp_mime_discrete_type_pool, discrete_type);
}

struct smtp_mime_discrete_type *
smtp_mime_discrete_type_new (int dt_type, char *dt_extension)
{
	struct smtp_mime_discrete_type *discrete_type;
	//discrete_type = malloc(sizeof(* discrete_type));
	discrete_type =
		(struct smtp_mime_discrete_type *)
		alloc_mem (&smtp_mime_discrete_type_pool);
	if (discrete_type == NULL)
		return NULL;
	DEBUG_SMTP (SMTP_MEM,
		    "smtp_mime_discrete_type_new: pointer=%p, elm=%p\n",
		    &smtp_mime_discrete_type_pool, discrete_type);
	discrete_type->dt_type = dt_type;
	discrete_type->dt_extension = dt_extension;
	//discrete_type->err_cnt = 0;
	return discrete_type;
}



void
smtp_mime_composite_type_free (struct smtp_mime_composite_type *ct)
{
	if (ct->ct_token != NULL)
		smtp_mime_extension_token_free (ct->ct_token);
	//free(ct);
	free_mem (&smtp_mime_composite_type_pool, ct);
	DEBUG_SMTP (SMTP_MEM,
		    "smtp_mime_composite_type_free: pointer=%p, elm=%p\n",
		    &smtp_mime_composite_type_pool, ct);
}


struct smtp_mime_composite_type *
smtp_mime_composite_type_new (int ct_type, char *ct_token)
{
	struct smtp_mime_composite_type *ct;

	//ct = malloc(sizeof(* ct));
	ct = (struct smtp_mime_composite_type *)
		alloc_mem (&smtp_mime_composite_type_pool);
	if (ct == NULL)
		return NULL;
	DEBUG_SMTP (SMTP_MEM,
		    "smtp_mime_composite_type_new: pointer=%p, elm=%p\n",
		    &smtp_mime_composite_type_pool, ct);

	ct->ct_type = ct_type;
	ct->ct_token = ct_token;
	//ct->err_cnt  = 0;

	return ct;
}


void
smtp_mime_type_free (struct smtp_mime_type *type)
{
	switch (type->tp_type) {
	case SMTP_MIME_TYPE_DISCRETE_TYPE:
		smtp_mime_discrete_type_free (type->tp_data.tp_discrete_type);
		break;
	case SMTP_MIME_TYPE_COMPOSITE_TYPE:
		smtp_mime_composite_type_free (type->tp_data.
					       tp_composite_type);
		break;
	}
	//free(type);
	free_mem (&smtp_mime_type_pool, type);
	DEBUG_SMTP (SMTP_MEM, "smtp_mime_type_free: pointer=%p, elm=%p\n",
		    &smtp_mime_type_pool, type);
}

struct smtp_mime_type *
smtp_mime_type_new (int tp_type,
		    struct smtp_mime_discrete_type *tp_discrete_type,
		    struct smtp_mime_composite_type *tp_composite_type)
{
	struct smtp_mime_type *mime_type;

	//mime_type = malloc(sizeof(* mime_type));
	mime_type =
		(struct smtp_mime_type *) alloc_mem (&smtp_mime_type_pool);
	if (mime_type == NULL)
		return NULL;
	DEBUG_SMTP (SMTP_MEM, "smtp_mime_type_new: pointer=%p, elm=%p\n",
		    &smtp_mime_type_pool, mime_type);

	mime_type->tp_type = tp_type;
	switch (tp_type) {
	case SMTP_MIME_TYPE_DISCRETE_TYPE:
		mime_type->tp_data.tp_discrete_type = tp_discrete_type;
		break;
	case SMTP_MIME_TYPE_COMPOSITE_TYPE:
		mime_type->tp_data.tp_composite_type = tp_composite_type;
		break;
	}

	return mime_type;
}


void
smtp_mime_content_type_free (struct smtp_mime_content_type *content_type)
{
	DEBUG_SMTP (SMTP_DBG, "smtp_mime_content_type_free\n");

	smtp_mime_type_free (content_type->ct_type);
	DEBUG_SMTP (SMTP_DBG, "content_type->ct_subtype=%s\n",
		    content_type->ct_subtype);
	smtp_mime_subtype_free (content_type->ct_subtype);
	DEBUG_SMTP (SMTP_DBG, "content_type=%p\n", content_type);
	if (content_type->ct_parameters != NULL) {
		clist_foreach (content_type->ct_parameters,
			       (clist_func) smtp_mime_parameter_free, NULL);
		clist_free (content_type->ct_parameters);
	}
	//free(content);
	free_mem (&smtp_mime_content_type_pool, (void *) content_type);
	DEBUG_SMTP (SMTP_MEM,
		    "smtp_mime_content_type_free: pointer=%p, elm=%p\n",
		    &smtp_mime_content_type_pool, (void *) content_type);
}

struct smtp_mime_content_type *
smtp_mime_content_type_new (struct smtp_mime_type *ct_type,
			    char *ct_subtype, clist * ct_parameters)
{
	struct smtp_mime_content_type *content_type;

	content_type =
		(struct smtp_mime_content_type *)
		alloc_mem (&smtp_mime_content_type_pool);
	if (content_type == NULL)
		return NULL;
	DEBUG_SMTP (SMTP_MEM,
		    "smtp_mime_content_type_new: pointer=%p, elm=%p\n",
		    &smtp_mime_content_type_pool, (void *) content_type);

#ifdef USE_NEL
	//xiayu 2005.11.28
	//content->count = 0;
	NEL_REF_INIT (content_type);
#endif
	content_type->ct_type = ct_type;
	content_type->ct_subtype = ct_subtype;
	content_type->ct_parameters = ct_parameters;

	//DEBUG_SMTP(SMTP_DBG, "NEW CONTENT get_content_charset: %s\n", get_content_type_charset(content_type));

	content_type->err_cnt = 0;
	return content_type;
}


struct smtp_mime_content_type *
smtp_mime_get_content_type_of_text (void)
{
	clist *list;
	struct smtp_mime_discrete_type *discrete_type;
	struct smtp_mime_type *mime_type;
	struct smtp_mime_content_type *content_type;
	char *subtype;

	discrete_type =
		smtp_mime_discrete_type_new (SMTP_MIME_DISCRETE_TYPE_TEXT,
					     NULL);
	if (discrete_type == NULL)
		goto err;

	mime_type =
		smtp_mime_type_new (SMTP_MIME_TYPE_DISCRETE_TYPE,
				    discrete_type, NULL);
	if (mime_type == NULL)
		goto free_discrete;
	discrete_type = NULL;

	list = clist_new ();
	if (list == NULL)
		goto free_type;

	subtype = strdup ("plain");
	if (subtype == NULL)
		goto free_list;
	DEBUG_SMTP (SMTP_MEM_1,
		    "STRDUP smtp_mime_get_content_type_of_text: pointer=%p\n",
		    subtype);

	content_type = smtp_mime_content_type_new (mime_type, subtype, list);
	if (content_type == NULL)
		goto free_subtype;

	return content_type;

      free_subtype:
	free (subtype);
	DEBUG_SMTP (SMTP_MEM_1,
		    "smtp_mime_get_content_type_of_text: FREE pointer=%p\n",
		    subtype);
      free_list:
	clist_free (list);
      free_type:
	smtp_mime_type_free (mime_type);
      free_discrete:
	if (discrete_type != NULL)
		smtp_mime_discrete_type_free (discrete_type);
      err:
	return NULL;
}



struct smtp_mime_content_type *
smtp_mime_get_content_type_of_message (void)
{
	clist *list;
	struct smtp_mime_composite_type *composite_type;
	struct smtp_mime_type *mime_type;
	struct smtp_mime_content_type *content_type;
	char *subtype;

	composite_type =
		smtp_mime_composite_type_new
		(SMTP_MIME_COMPOSITE_TYPE_MESSAGE, NULL);
	if (composite_type == NULL)
		goto err;

	mime_type =
		smtp_mime_type_new (SMTP_MIME_TYPE_COMPOSITE_TYPE, NULL,
				    composite_type);
	if (mime_type == NULL)
		goto free_composite;
	composite_type = NULL;

	list = clist_new ();
	if (list == NULL)
		goto free_mime_type;

	subtype = strdup ("rfc822");
	if (subtype == NULL)
		goto free_list;
	DEBUG_SMTP (SMTP_MEM_1,
		    "STRDUP smtp_mime_get_content_type_of_message: pointer=%p\n",
		    subtype);

	content_type = smtp_mime_content_type_new (mime_type, subtype, list);
	if (content_type == NULL)
		goto free_subtype;

	return content_type;

      free_subtype:
	free (subtype);
	DEBUG_SMTP (SMTP_MEM_1,
		    "smtp_mime_get_content_type_of_message: FREE pointer=%p\n",
		    subtype);
      free_list:
	clist_free (list);
      free_mime_type:
	smtp_mime_type_free (mime_type);
      free_composite:
	if (composite_type != NULL)
		smtp_mime_composite_type_free (composite_type);
      err:
	return NULL;
}



/*
x  description := "Content-Description" ":" *text
*/

static int
is_text (char ch)
{
	unsigned char uch = (unsigned char) ch;

	if (uch < 1)
		return FALSE;

	if ((uch == 10) || (uch == 13))
		return FALSE;

	return TRUE;
}


int
smtp_mime_description_parse (const char *message, size_t length,
			     size_t * index, char **result)
{
	return smtp_custom_string_parse (message, length,
					 index, result, is_text);
}

/*
 * x discrete-type := "text" / "image" / "audio" / "video" /
 *                  "application" / extension-token
 */
static int
smtp_mime_discrete_type_parse (const char *message, size_t length,
			       size_t * index,
			       struct smtp_mime_discrete_type **result)
{
	char *extension;
	int type;
	struct smtp_mime_discrete_type *discrete_type;
	size_t cur_token;
	int r;
	int res;

	cur_token = *index;

	extension = NULL;

	type = SMTP_MIME_DISCRETE_TYPE_ERROR;	/* XXX - removes a gcc warning */

	r = smtp_token_case_insensitive_parse (message, length,
					       &cur_token, "text");
	if (r == SMTP_NO_ERROR)
		type = SMTP_MIME_DISCRETE_TYPE_TEXT;

	if (r == SMTP_ERROR_PARSE) {
		r = smtp_token_case_insensitive_parse (message, length,
						       &cur_token, "image");
		if (r == SMTP_NO_ERROR)
			type = SMTP_MIME_DISCRETE_TYPE_IMAGE;
	}

	if (r == SMTP_ERROR_PARSE) {
		r = smtp_token_case_insensitive_parse (message, length,
						       &cur_token, "audio");
		if (r == SMTP_NO_ERROR)
			type = SMTP_MIME_DISCRETE_TYPE_AUDIO;
	}

	if (r == SMTP_ERROR_PARSE) {
		r = smtp_token_case_insensitive_parse (message, length,
						       &cur_token, "video");
		if (r == SMTP_NO_ERROR)
			type = SMTP_MIME_DISCRETE_TYPE_VIDEO;
	}

	if (r == SMTP_ERROR_PARSE) {
		r = smtp_token_case_insensitive_parse (message, length,
						       &cur_token,
						       "application");
		if (r == SMTP_NO_ERROR)
			type = SMTP_MIME_DISCRETE_TYPE_APPLICATION;
	}

	if (r == SMTP_ERROR_PARSE) {
		r = smtp_mime_extension_token_parse (message, length,
						     &cur_token, &extension);
		if (r == SMTP_NO_ERROR)
			type = SMTP_MIME_DISCRETE_TYPE_EXTENSION;
	}

	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	discrete_type = smtp_mime_discrete_type_new (type, extension);
	if (discrete_type == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free;
	}

	*result = discrete_type;
	*index = cur_token;

	return SMTP_NO_ERROR;

      free:
	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	smtp_mime_extension_token_free (extension);
      err:
	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	return res;
}


int
get_body_type (struct smtp_mime_content_type *content_type)
{
	int body_type = SMTP_MIME_NONE;

	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	if (content_type == NULL) {
		return body_type;
	}

	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	if (content_type->ct_type->tp_type == SMTP_MIME_TYPE_COMPOSITE_TYPE) {

		switch (content_type->ct_type->tp_data.tp_composite_type->
			ct_type) {
		case SMTP_MIME_COMPOSITE_TYPE_MULTIPART:
			DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__,
				    __FUNCTION__, __LINE__);
			body_type = SMTP_MIME_MULTIPLE;
			break;

		case SMTP_MIME_COMPOSITE_TYPE_MESSAGE:
			DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__,
				    __FUNCTION__, __LINE__);
			if (strcasecmp (content_type->ct_subtype, "rfc822") ==
			    0) {
				DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__,
					    __FUNCTION__, __LINE__);
				body_type = SMTP_MIME_MESSAGE;
			}
			else {
				DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__,
					    __FUNCTION__, __LINE__);
				body_type = SMTP_MIME_SINGLE;
			}
			break;

		default:
			DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__,
				    __FUNCTION__, __LINE__);
			body_type = SMTP_MIME_NONE;
		}
	}

	else {			/* SMTP_MIME_TYPE_DISCRETE_TYPE */
		DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
			    __LINE__);
		body_type = SMTP_MIME_SINGLE;
	}

	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	return body_type;

}

int
single_body_type (struct smtp_mime_content_type *content_type)
{
	return (get_body_type (content_type) == SMTP_MIME_SINGLE);
}

int
multiple_body_type (struct smtp_mime_content_type *content_type)
{
	return (get_body_type (content_type) == SMTP_MIME_MULTIPLE);
}

int
message_body_type (struct smtp_mime_content_type *content_type)
{
	return (get_body_type (content_type) == SMTP_MIME_MESSAGE);
}



/*
 * x composite-type := "message" / "multipart" / extension-token
 */
static int
smtp_mime_composite_type_parse (const char *message, size_t length,
				size_t * index,
				struct smtp_mime_composite_type **result)
{
	char *extension_token;
	int type;
	struct smtp_mime_composite_type *ct;
	size_t cur_token;
	int r;
	int res;

	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	cur_token = *index;
	extension_token = NULL;
	type = SMTP_MIME_COMPOSITE_TYPE_ERROR;	/* XXX - removes a gcc warning */

	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	r = smtp_token_case_insensitive_parse (message, length, &cur_token,
					       "message");
	if (r == SMTP_NO_ERROR) {
		DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
			    __LINE__);
		type = SMTP_MIME_COMPOSITE_TYPE_MESSAGE;
	}

	if (r == SMTP_ERROR_PARSE) {
		DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
			    __LINE__);
		r = smtp_token_case_insensitive_parse (message, length,
						       &cur_token,
						       "multipart");
		if (r == SMTP_NO_ERROR) {
			DEBUG_SMTP (SMTP_DBG,
				    "SMTP_MIME_COMPOSITE_TYPE_MULTIPART\n");
			type = SMTP_MIME_COMPOSITE_TYPE_MULTIPART;
		}
	}

	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	if (r != SMTP_NO_ERROR) {
		DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
			    __LINE__);
		res = r;
		goto err;
	}

	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	ct = smtp_mime_composite_type_new (type, extension_token);
	if (ct == NULL) {
		DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
			    __LINE__);
		res = SMTP_ERROR_MEMORY;
		goto free_extension;
	}

	*result = ct;
	*index = cur_token;
	return SMTP_NO_ERROR;

      free_extension:
	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	if (extension_token != NULL)
		smtp_mime_extension_token_free (extension_token);

      err:
	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	return res;

}


/*
x  subtype := extension-token / iana-token
*/

int
smtp_mime_subtype_parse (const char *message, size_t length,
			 size_t * index, char **result)
{
	return smtp_mime_extension_token_parse (message, length, index,
						result);
}



/*
x  type := discrete-type / composite-type
*/

//xiayu 2005.10.28
//static int smtp_mime_type_parse(const char * message, size_t length,
static int
smtp_mime_type_parse (const char *message, size_t length,
		      size_t * index, struct smtp_mime_type **result)
{
	struct smtp_mime_discrete_type *discrete_type;
	struct smtp_mime_composite_type *composite_type;
	size_t cur_token;
	struct smtp_mime_type *mime_type;
	int type;
	int res;
	int r;

	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	cur_token = *index;

	discrete_type = NULL;
	composite_type = NULL;

	type = SMTP_MIME_TYPE_ERROR;	/* XXX - removes a gcc warning */

	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	r = smtp_mime_composite_type_parse (message, length, &cur_token,
					    &composite_type);
	if (r == SMTP_NO_ERROR) {
		DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
			    __LINE__);
		type = SMTP_MIME_TYPE_COMPOSITE_TYPE;
	}

	if (r == SMTP_ERROR_PARSE) {
		DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
			    __LINE__);
		r = smtp_mime_discrete_type_parse (message, length,
						   &cur_token,
						   &discrete_type);
		if (r == SMTP_NO_ERROR)
			type = SMTP_MIME_TYPE_DISCRETE_TYPE;
	}

	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	mime_type = smtp_mime_type_new (type, discrete_type, composite_type);
	if (mime_type == NULL) {
		res = r;
		goto free;
	}

	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	*result = mime_type;
	*index = cur_token;
	return SMTP_NO_ERROR;

      free:
	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	if (discrete_type != NULL)
		smtp_mime_discrete_type_free (discrete_type);
	if (composite_type != NULL)
		smtp_mime_composite_type_free (composite_type);

      err:
	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	return res;

}



/*
 * x content := "Content-Type" ":" type "/" subtype *(";" parameter)
 *            ; Matching of media type and subtype
 *            ; is ALWAYS case-insensitive.
 */
int
smtp_mime_content_type_parse (struct smtp_info *psmtp, const char *message,
			      size_t length, size_t * index,
			      struct smtp_mime_content_type **result)
{
	size_t cur_token;
	struct smtp_mime_type *type;
	char *subtype;
	clist *parameters_list;
	struct smtp_mime_content_type *content_type;
	int r;
	int res;

	struct smtp_data *body;
	struct mailmime *old_parent;
	//char * boundary = *boundary;
	//struct smtp_mime_content * content_type;
	int encoding;
	int body_type;
	struct smtp_part *part = &psmtp->part_stack[psmtp->part_stack_top];

	DEBUG_SMTP (SMTP_DBG, "smtp_mime_content_type_parse\n");
	DEBUG_SMTP (SMTP_DBG, "CT part_stack_top =%d, part = %p\n",
		    psmtp->part_stack_top, part);

	cur_token = *index;

	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	smtp_cfws_parse (message, length, &cur_token);

	type = NULL;
	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	r = smtp_mime_type_parse (message, length, &cur_token, &type);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	r = smtp_unstrict_char_parse (message, length, &cur_token, '/');
	switch (r) {
	case SMTP_NO_ERROR:
		DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
			    __LINE__);
		r = smtp_cfws_parse (message, length, &cur_token);
		if ((r != SMTP_NO_ERROR) && (r != SMTP_ERROR_PARSE)) {
			res = r;
			goto free_type;
		}

		DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
			    __LINE__);
		r = smtp_mime_subtype_parse (message, length, &cur_token,
					     &subtype);
		if (r != SMTP_NO_ERROR) {
			DEBUG_SMTP (SMTP_DBG, "subtype parse failed\n");
			res = r;
			goto free_type;
		}
		DEBUG_SMTP (SMTP_DBG, "subtype parse succeed: %s\n", subtype);
		break;

	case SMTP_ERROR_PARSE:
		DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
			    __LINE__);
		subtype = strdup ("unknown");
		DEBUG_SMTP (SMTP_MEM_1,
			    "STRDUP smtp_mime_content_type_parse: pointer=%p\n",
			    subtype);
		break;

	default:
		DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
			    __LINE__);
		res = r;
		goto free_type;
	}

	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	parameters_list = clist_new ();
	if (parameters_list == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free_type;
	}

	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	while (1) {
		size_t final_token;
		struct smtp_mime_parameter *parameter;

		final_token = cur_token;
		DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
			    __LINE__);
		r = smtp_unstrict_char_parse (message, length, &cur_token,
					      ';');
		if (r != SMTP_NO_ERROR) {
			cur_token = final_token;
			break;
		}

		DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
			    __LINE__);
		r = smtp_cfws_parse (message, length, &cur_token);
		if ((r != SMTP_NO_ERROR) && (r != SMTP_ERROR_PARSE)) {
			res = r;
			goto free_type;
		}

		DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
			    __LINE__);
		r = smtp_mime_parameter_parse (message, length, &cur_token,
					       &parameter);
		if (r == SMTP_NO_ERROR) {
			DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__,
				    __FUNCTION__, __LINE__);
			/* do nothing */
		}
		else if (r == SMTP_ERROR_PARSE) {
			DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__,
				    __FUNCTION__, __LINE__);
			cur_token = final_token;
			break;
		}
		else {
			DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__,
				    __FUNCTION__, __LINE__);
			res = r;
			goto err;
		}

		DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
			    __LINE__);
		r = clist_append (parameters_list, parameter);
		if (r < 0) {
			DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__,
				    __FUNCTION__, __LINE__);
			smtp_mime_parameter_free (parameter);
			res = SMTP_ERROR_MEMORY;
			goto free_parameters;
		}

	}

	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	DEBUG_SMTP (SMTP_DBG, "message+cur_token = %s\n",
		    message + cur_token);
	r = smtp_unstrict_crlf_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		return r;
	}

	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	content_type =
		smtp_mime_content_type_new (type, subtype, parameters_list);
	if (content_type == NULL) {
		DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
			    __LINE__);
		res = SMTP_ERROR_MEMORY;
		goto free_parameters;
	}

	DEBUG_SMTP (SMTP_DBG,
		    "%s_%s[%d], content_type = %p,  content_type->ct_type=%p, content_type->ct_type->tp_type = %s \n",
		    __FILE__, __FUNCTION__, __LINE__, content_type,
		    content_type->ct_type,
		    content_type->ct_type->tp_type ==
		    SMTP_MIME_TYPE_ERROR ? "ERROR" : (content_type->ct_type->
						      tp_type ==
						      SMTP_MIME_TYPE_DISCRETE_TYPE
						      ? "DISCRETE"
						      : (content_type->
							 ct_type->tp_type ==
							 SMTP_MIME_TYPE_COMPOSITE_TYPE
							 ? "COMPSTITE" :
							 "")));


	/* set default type if no content type */
	if (content_type == NULL) {
		/*content_type is detached,in any case,we will have to free it */
		DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
			    __LINE__);
		content_type = smtp_mime_get_content_type_of_text ();
		if (content_type == NULL) {
			DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__,
				    __FUNCTION__, __LINE__);
			res = SMTP_ERROR_MEMORY;
			goto err;
		}
	}

#define NO_CASE                 1
#define CASE_SENS               0

	if (content_type->ct_type->tp_type == SMTP_MIME_TYPE_COMPOSITE_TYPE) {

		switch (content_type->ct_type->tp_data.tp_composite_type->
			ct_type) {
		case SMTP_MIME_COMPOSITE_TYPE_MULTIPART:
			DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__,
				    __FUNCTION__, __LINE__);
			part->content_type = content_type;
			part->boundary =
				smtp_mime_extract_boundary (content_type);
			DEBUG_SMTP (SMTP_DBG, "part = %p\n", part);
			DEBUG_SMTP (SMTP_DBG, "part->boundary = %p\n",
				    part->boundary);
			DEBUG_SMTP (SMTP_DBG, "%s_%s[%d], boundary = %s\n",
				    __FILE__, __FUNCTION__, __LINE__,
				    part->boundary);

			if (part->boundary == NULL) {
				part->body_type = SMTP_MIME_SINGLE;
				DEBUG_SMTP (SMTP_DBG, "part = %p\n", part);
			}
			else {
				char bound_buf[129];
				int len = strlen (part->boundary);
				if (len > 128) {
					res = SMTP_ERROR_BUFFER;
					goto err;
				}
				//xiayu 2005.12.14 bugfix for Content-Composition not found in mail message composed by Foxmail-6.0
				//memcpy(bound_buf, "\r\n--", strlen("\r\n--"));
				//memcpy(bound_buf+strlen("\r\n--"), part->boundary, len);
				//len += strlen("\r\n--");
				memcpy (bound_buf, "--", strlen ("--"));
				memcpy (bound_buf + strlen ("--"),
					part->boundary, len);
				len += strlen ("--");
				//xiayu 2005.12.14 bugfix end

				bound_buf[len] = '\0';

				part->body_type = SMTP_MIME_MULTIPLE;
				DEBUG_SMTP (SMTP_DBG, "part = %p\n", part);

				//wyong, 20231017 
				//part->boundary_tree = compile_single_keyword_suffix(bound_buf, len, -1, CASE_SENS);
				part->boundary = strdup (bound_buf);
				//DEBUG_SMTP(SMTP_MEM_1, "smtp_content_type_parse: CREATE trieobj=%p\n", part->boundary_tree);
			}
			break;

		case SMTP_MIME_COMPOSITE_TYPE_MESSAGE:
			DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__,
				    __FUNCTION__, __LINE__);
			part->content_type = content_type;
			if (strcasecmp
			    (part->content_type->ct_subtype, "rfc822") == 0) {
				part->body_type = SMTP_MIME_MESSAGE;
			}
			else {
				part->body_type = SMTP_MIME_SINGLE;
			}
			break;

		default:
			DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__,
				    __FUNCTION__, __LINE__);
			res = SMTP_ERROR_INVAL;
			goto err;
		}
	}

	else {			/* SMTP_MIME_TYPE_DISCRETE_TYPE */
		DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
			    __LINE__);
		part->body_type = SMTP_MIME_SINGLE;
		DEBUG_SMTP (SMTP_DBG, "part = %p\n", part);
	}

	/* get the body type */
	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	//part->content_type = content;
	//xiayu 2005.10.28
	//part->boundary = NULL; /* XXX - removes a gcc warning */

#ifdef USE_NEL
	if ((r =
	     nel_env_analysis (eng, &(psmtp->env), content_type_id,
			       (struct smtp_simple_event *) content_type)) <
	    0) {
		DEBUG_SMTP (SMTP_DBG,
			    "smtp_content_type_parse: nel_env_analysis error\n");
		res = SMTP_ERROR_ENGINE;
		smtp_mime_content_type_free (content_type);
		goto err;
	}
#endif

	if (psmtp->permit & SMTP_PERMIT_DENY) {
		//fprintf(stderr, "found a deny event\n");
		//smtp_close_connection(ptcp, psmtp);
		res = SMTP_ERROR_POLICY;
		goto err;

	}
	else if (psmtp->permit & SMTP_PERMIT_DROP) {
		fprintf (stderr,
			 "found a drop event, no drop for message, denied.\n");
		//smtp_close_connection(ptcp, psmtp);
		res = SMTP_ERROR_POLICY;
		goto err;
	}

	DEBUG_SMTP (SMTP_DBG, "\n");
	*result = content_type;
	*index = cur_token;

	return SMTP_NO_ERROR;

      free_parameters:
	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	clist_foreach (parameters_list, (clist_func) smtp_mime_parameter_free,
		       NULL);
	clist_free (parameters_list);

	smtp_mime_subtype_free (subtype);

      free_type:
	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	smtp_mime_type_free (type);

      err:
	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	return res;

}


int
get_body_type_from_content_type (struct smtp_mime_content_type *content_type)
{
	int body_type = SMTP_MIME_NONE;

	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	if (content_type == NULL) {
		return body_type;
	}

	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	if (content_type->ct_type->tp_type == SMTP_MIME_TYPE_COMPOSITE_TYPE) {

		switch (content_type->ct_type->tp_data.tp_composite_type->
			ct_type) {
		case SMTP_MIME_COMPOSITE_TYPE_MULTIPART:
			DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__,
				    __FUNCTION__, __LINE__);
			body_type = SMTP_MIME_MULTIPLE;
			break;

		case SMTP_MIME_COMPOSITE_TYPE_MESSAGE:
			DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__,
				    __FUNCTION__, __LINE__);
			if (strcasecmp (content_type->ct_subtype, "rfc822") ==
			    0) {
				DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__,
					    __FUNCTION__, __LINE__);
				body_type = SMTP_MIME_MESSAGE;
			}
			else {
				DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__,
					    __FUNCTION__, __LINE__);
				body_type = SMTP_MIME_SINGLE;
			}
			break;

		default:
			DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__,
				    __FUNCTION__, __LINE__);
			body_type = SMTP_MIME_NONE;
		}
	}

	else {			/* SMTP_MIME_TYPE_DISCRETE_TYPE */
		DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
			    __LINE__);
		body_type = SMTP_MIME_SINGLE;
	}

	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	return body_type;

}


struct smtp_mime_content_type *
get_cur_content_type (struct smtp_info *psmtp)
{
	//return psmtp->part_stack[psmtp->part_stack_top].content_type;
	//wyong, 20231009 
	struct smtp_part *cur_part =
		psmtp->part_stack + psmtp->part_stack_top;
	return cur_part->content_type;
}

int
get_cur_body_type (struct smtp_info *psmtp)
{
	//return psmtp->part_stack[psmtp->part_stack_top].body_type;
	//wyong, 20231009 
	struct smtp_part *cur_part =
		psmtp->part_stack + psmtp->part_stack_top;
	return cur_part->body_type;
}



int
is_discrete_text (struct smtp_mime_content_type *content_type)
{
	printf ("is_discrete_text: content_type->ct_type->tp_type = %d \n",
		content_type->ct_type->tp_type);
	if (content_type->ct_type->tp_type == SMTP_MIME_TYPE_DISCRETE_TYPE) {
		printf ("is_discrete_text: content_type->ct_type->tp_data.tp_discrete_type->dt_type = %d\n", content_type->ct_type->tp_data.tp_discrete_type->dt_type);
	}

	return ((content_type->ct_type->tp_type ==
		 SMTP_MIME_TYPE_DISCRETE_TYPE)
		&& (content_type->ct_type->tp_data.tp_discrete_type->
		    dt_type == SMTP_MIME_DISCRETE_TYPE_TEXT));
}

int
null_charset_content_type (struct smtp_mime_content_type *content_type)
{
	char *charset;
	if ((content_type->ct_type->tp_type == SMTP_MIME_TYPE_DISCRETE_TYPE)
	    && (content_type->ct_type->tp_data.tp_discrete_type->dt_type ==
		SMTP_MIME_DISCRETE_TYPE_TEXT)) {
		printf ("null_charset_content_type: charset = %s\n",
			get_charset_of_content_type (content_type));
		if ((charset =
		     get_charset_of_content_type (content_type)) == NULL
		    || *charset == '\0') {
			return 1;
		}

	}
	return 0;
}
