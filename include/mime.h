
#ifndef MIME_H
#define MIME_H

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "clist.h"
#include "analysis.h"
#include "nlib/stream.h"

#include "fields.h"
#include "content-type.h"


struct mailmime_list
{
#ifdef USE_NEL
	NEL_REF_DEF
#endif
	clist * mm_list;
};


//wyong, 20231023 
struct smtp_mime_text
{
#ifdef USE_NEL
	NEL_REF_DEF
#endif
	int encoding;
	int encoded;
	char *data;
	unsigned int length;
};

void smtp_mime_text_free (struct smtp_mime_text *mime_text);
struct smtp_mime_text *smtp_mime_text_new (int encoding, int encoded,
					   char *text, unsigned int length);


enum
{
	SMTP_MIME_DATA_TEXT,
	SMTP_MIME_DATA_FILE,
};

struct smtp_mime_data
{
#ifdef USE_NEL
	NEL_REF_DEF
#endif
	int dt_type;
	int dt_encoding;
	int dt_encoded;
	union
	{
		struct
		{
			const char *dt_data;
			unsigned int dt_length;
		} dt_text;
		char *dt_filename;
	} dt_data;
};


enum smtp_mime_body_type
{
	SMTP_MIME_NONE,
	SMTP_MIME_SINGLE,
	SMTP_MIME_MULTIPLE,
	SMTP_MIME_MESSAGE,
};

struct mailmime
{
#ifdef USE_NEL
	NEL_REF_DEF
#endif
		/* parsing state */
	int mm_state;

	/* parent information */
	int mm_parent_type;
	struct mailmime *mm_parent;
	clistiter *mm_multipart_pos;

	int mm_type;
	const char *mm_mime_start;
	size_t mm_length;

	struct smtp_mime_fields *mm_mime_fields;
	struct smtp_mime_content_type *mm_content_type;

	char *mm_boundary;
	//struct trieobj *mm_boundary_tree;

	union
	{
		/* single part */
		struct smtp_mime_data *mm_single;	/* XXX - was body */

		/* multi-part */
		struct
		{
			struct smtp_mime_data *mm_preamble;
			struct smtp_mime_data *mm_epilogue;

			//wyong, 20231021 
			/* clist */ struct mailmime_list *mm_mp_list;
		} mm_multipart;

		/* message */
		struct
		{
			//struct smtp_fields * mm_fields;
			struct mailmime *mm_msg_mime;
		} mm_message;

	} mm_data;
};




struct smtp_mime_parameter
{
	char *pa_name;
	char *pa_value;
};

struct mailmime_list *mailmime_list_new (void);
void mailmime_list_free (struct mailmime_list *mm_list);
int add_to_mailmime_list (struct mailmime_list *list, struct mailmime *mm);

int smtp_mime_value_parse (const char *message, size_t length,
			   size_t * index, char **result);

struct smtp_mime_parameter *smtp_mime_parameter_new (char *pa_name,
						     char *pa_value);

void smtp_mime_parameter_free (struct smtp_mime_parameter *parameter);

void smtp_mime_attribute_free (char *attribute);

void smtp_mime_description_free (char *description);

void smtp_mime_extension_token_free (char *extension);

void smtp_mime_subtype_free (char *subtype);

void smtp_mime_value_free (char *value);


typedef int smtp_struct_parser (const char *message, size_t length,
				size_t * index, void *result);

typedef int smtp_struct_destructor (void *result);

/* internal use, exported for MIME */

int smtp_fws_parse (const char *message, size_t length, size_t * index);

int smtp_cfws_parse (const char *message, size_t length, size_t * index);

int smtp_mime_lwsp_parse (const char *message, size_t length, size_t * index);

int smtp_char_parse (const char *message, size_t length,
		     size_t * index, char token);

int smtp_unstrict_char_parse (const char *message, size_t length,
			      size_t * index, char token);

int smtp_crlf_parse (const char *message, size_t length, size_t * index);

int
smtp_custom_string_parse (const char *message, size_t length,
			  size_t * index, char **result,
			  int (*is_custom_char) (char));

int
smtp_token_case_insensitive_len_parse (const char *message, size_t length,
				       size_t * index, char *token,
				       size_t token_length);

#define smtp_token_case_insensitive_parse(message, length, index, token) \
    smtp_token_case_insensitive_len_parse(message, length, index, token, \
					     sizeof(token) - 1)

int smtp_quoted_string_parse (const char *message, size_t length,
			      size_t * index, char **result);

int
smtp_number_parse (const char *message, size_t length,
		   size_t * index, uint32_t * result);

int smtp_msg_id_parse (const char *message, size_t length,
		       size_t * index, char **result);

int smtp_msg_id_list_parse (const char *message, size_t length,
			    size_t * index, clist ** result);

int smtp_word_parse (const char *message, size_t length,
		     size_t * index, char **result);

int smtp_atom_parse (const char *message, size_t length,
		     size_t * index, char **result);

int smtp_fws_atom_parse (const char *message, size_t length,
			 size_t * index, char **result);

int smtp_fws_word_parse (const char *message, size_t length,
			 size_t * index, char **result);

int smtp_fws_quoted_string_parse (const char *message, size_t length,
				  size_t * index, char **result);

int smtp_mime_parameter_parse (const char *message, size_t length,
			       size_t * index,
			       struct smtp_mime_parameter **result);

int
smtp_mime_extension_token_parse (const char *message, size_t length,
				 size_t * index, char **result);

int smtp_mime_id_parse (const char *message, size_t length,
			size_t * index, char **result);

struct smtp_mime_data *smtp_mime_data_new_data (int encoding, int encoded,
						const char *data,
						size_t length);

struct smtp_mime_data *smtp_mime_data_new_file (int encoding, int encoded,
						char *filename);

struct mailmime *smtp_mime_new_message_data (struct mailmime *msg_mime);

struct mailmime *smtp_mime_new_empty (struct smtp_mime_content_type *content,
				      struct smtp_mime_fields *mime_fields);

int smtp_mime_new_with_content (const char *content_type,
				struct smtp_mime_fields *mime_fields,
				struct mailmime **result);


int smtp_mime_set_preamble_file (struct mailmime *build_info, char *filename);

int smtp_mime_set_epilogue_file (struct mailmime *build_info, char *filename);

int smtp_mime_set_preamble_text (struct mailmime *build_info,
				 char *data_str, size_t length);

int smtp_mime_set_epilogue_text (struct mailmime *build_info,
				 char *data_str, size_t length);

int smtp_mime_set_body_file (struct mailmime *build_info, char *filename);

int smtp_mime_set_body_text (struct mailmime *build_info,
			     char *data_str, size_t length);

void smtp_mime_set_imf_fields (struct mailmime *build_info,
			       struct smtp_fields *fields);

void smtp_mime_decoded_part_free (char *part);

void smtp_mime_encoded_text_free (char *text);

void smtp_mime_charset_free (char *charset);

char *smtp_mime_content_type_charset_get (struct smtp_mime_content_type
					  *content_type);

char *smtp_mime_content_type_param_get (struct smtp_mime_content_type
					*content_type, char *name);

char *smtp_mime_extract_boundary (struct smtp_mime_content_type
				  *content_type);

//wyong, 20231021
struct smtp_mime_text_stream
{
#ifdef USE_NEL
	NEL_REF_DEF
#endif
	int ts_encoding;
	int ts_encoded;
	unsigned int ts_length;
	struct stream *ts_stream;
};

void smtp_mime_text_stream_free (struct smtp_mime_text_stream
				 *mime_text_stream);
struct smtp_mime_text_stream *smtp_mime_text_stream_new (int encoding,
							 int encoded);


struct smtp_mime_data *smtp_mime_data_new (int dt_type, int dt_encoding,
					   int dt_encoded,
					   const char *dt_data,
					   unsigned int dt_length,
					   char *dt_filename);

void smtp_mime_data_free (struct smtp_mime_data *mime_data);

void smtp_atom_free (char *atom);

void smtp_dot_atom_free (char *dot_atom);

void smtp_dot_atom_text_free (char *dot_atom);

void smtp_quoted_string_free (char *quoted_string);

void smtp_word_free (char *word);

void smtp_phrase_free (char *phrase);

void smtp_unstructured_free (char *unstructured);

void smtp_no_fold_quote_free (char *nfq);

void smtp_no_fold_literal_free (char *nfl);


/*
phrase          =       1*word / obs-phrase
*/

int smtp_phrase_parse (const char *message, size_t length,
		       size_t * index, char **result);



int smtp_wsp_phrase_parse (const char *message, size_t length,
			   size_t * index, char **result);

int smtp_wsp_greater_parse (const char *message, size_t length,
			    size_t * index);
int smtp_wsp_lower_parse (const char *message, size_t length, size_t * index);

int smtp_obracket_parse (const char *message, size_t length, size_t * index);

int smtp_cbracket_parse (const char *message, size_t length, size_t * index);

int smtp_dcontent_parse (const char *message, size_t length,
			 size_t * index, char *result);

int
smtp_struct_list_parse (const char *message, size_t length,
			size_t * index, clist ** result,
			char symbol,
			smtp_struct_parser * parser,
			smtp_struct_destructor * destructor);

int smtp_semi_colon_parse (const char *message, size_t length,
			   size_t * index);

int smtp_colon_parse (const char *message, size_t length, size_t * index);


int smtp_lower_parse (const char *message, size_t length, size_t * index);

int smtp_greater_parse (const char *message, size_t length, size_t * index);


/*
dot-atom        =       [CFWS] dot-atom-text [CFWS]
*/
int smtp_dot_atom_parse (const char *message, size_t length,
			 size_t * index, char **result);

int smtp_wsp_parse (const char *message, size_t length, size_t * index);


int smtp_str_crlf_parse (char *message, int length, int *index);


int smtp_unstrict_crlf_parse (const char *message,
			      size_t length, size_t * index);

/* mailmime build */
struct mailmime *smtp_mime_new (int mm_type,
				const char *mm_mime_start,
				size_t mm_length,
				struct smtp_mime_fields *mm_mime_fields,
				struct smtp_mime_content_type
				*mm_content_type,
				//for single message 
				struct smtp_mime_data *mm_body,
				//for multipart 
				struct smtp_mime_data *mm_preamble,
				struct smtp_mime_data *mm_epilogue,
				struct mailmime_list *mm_mp_list,
				//for rfc message 
				struct smtp_fields *mm_fields,
				struct mailmime *mm_msg_mime);

void smtp_mime_free (struct mailmime *mime);

#endif
