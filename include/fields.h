/*
 * $Id: headers.h,v 1.7 2005/11/08 17:14:36 wyong Exp $
 */

#ifndef SMTP_HERDERS_H
#define SMTP_HERDERS_H


#ifdef __cplusplus
extern "C"
{
#endif

#include <inttypes.h>
#include <sys/types.h>
#include "clist.h"

#include "content-type.h"
#include "time.h"
#include "disposition.h"


//xiayu 2005.10.28
//#include "smtp.h"

	enum smtp_mime_field_type
	{

		SMTP_FIELD_NONE = 1,	/* on parse error */
		SMTP_FIELD_RETURN_PATH,	/* Return-Path */
		SMTP_FIELD_RESENT_DATE,	/* Resent-Date */
		SMTP_FIELD_RESENT_FROM,	/* Resent-From */
		SMTP_FIELD_RESENT_SENDER,	/* Resent-Sender */
		SMTP_FIELD_RESENT_TO,	/* Resent-To */
		SMTP_FIELD_RESENT_CC,	/* Resent-Cc */
		SMTP_FIELD_RESENT_BCC,	/* Resent-Bcc */
		SMTP_FIELD_RESENT_MSG_ID,	/* Resent-Message-ID */
		SMTP_FIELD_ORIG_DATE,	/* Date */
		SMTP_FIELD_FROM,	/* From */
		SMTP_FIELD_SENDER,	/* Sender */
		SMTP_FIELD_REPLY_TO,	/* Reply-To */
		SMTP_FIELD_TO,	/* To */
		SMTP_FIELD_CC,	/* Cc */
		SMTP_FIELD_BCC,	/* Bcc */
		SMTP_FIELD_MESSAGE_ID,	/* Message-ID */
		SMTP_FIELD_IN_REPLY_TO,	/* In-Reply-To */
		SMTP_FIELD_REFERENCES,	/* References */
		SMTP_FIELD_SUBJECT,	/* Subject */
		SMTP_FIELD_COMMENTS,	/* Comments */
		SMTP_FIELD_KEYWORDS,	/* Keywords */
		SMTP_FIELD_OPTIONAL_FIELD,	/* other field */
		SMTP_FIELD_RECEIVED,	/* Received */
		SMTP_FIELD_SOLICITATION,
		SMTP_FIELD_RESENT_REPLY_TO,
		SMTP_FIELD_ENCRYPTED,

		SMTP_MIME_FIELD_NONE,
		SMTP_MIME_FIELD_TYPE,
		SMTP_MIME_FIELD_TRANSFER_ENCODING,
		SMTP_MIME_FIELD_ID,
		SMTP_MIME_FIELD_DESCRIPTION,
		SMTP_MIME_FIELD_VERSION,
		SMTP_MIME_FIELD_DISPOSITION,
		SMTP_MIME_FIELD_LANGUAGE,
		SMTP_MIME_FIELD_CONVERSION,
		SMTP_MIME_FIELD_FEATURES,
		SMTP_MIME_FIELD_BASE,
		SMTP_MIME_FIELD_LOCATION,
		SMTP_MIME_FIELD_DURATION,

		SMTP_FIELD_EOH	/* end of headers */
	};

/*
  smtp_fields is a list of header fields
  - fld_list is a list of header fields
*/
	struct smtp_fields
	{
#ifdef USE_NEL
		NEL_REF_DEF
#endif
		clist * fld_list;	/* list of (struct smtp_field *), != NULL */
	};

	struct smtp_fields *smtp_fields_new (clist * fld_list);
	void smtp_fields_free (struct smtp_fields *fields);


/*
  smtp_field is a field

  - fld_type is the type of the field

  - fld_data.fld_return_path is the parsed content of the Return-Path
    field if type is SMTP_FIELD_RETURN_PATH

  - fld_data.fld_resent_date is the parsed content of the Resent-Date field
    if type is SMTP_FIELD_RESENT_DATE

  - fld_data.fld_resent_from is the parsed content of the Resent-From field

  - fld_data.fld_resent_sender is the parsed content of the Resent-Sender field

  - fld_data.fld_resent_to is the parsed content of the Resent-To field

  - fld_data.fld_resent_cc is the parsed content of the Resent-Cc field

  - fld_data.fld_resent_bcc is the parsed content of the Resent-Bcc field

  - fld_data.fld_resent_msg_id is the parsed content of the Resent-Message-ID
    field

  - fld_data.fld_orig_date is the parsed content of the Date field

  - fld_data.fld_from is the parsed content of the From field

  - fld_data.fld_sender is the parsed content of the Sender field

  - fld_data.fld_reply_to is the parsed content of the Reply-To field

  - fld_data.fld_to is the parsed content of the To field

  - fld_data.fld_cc is the parsed content of the Cc field

  - fld_data.fld_bcc is the parsed content of the Bcc field

  - fld_data.fld_message_id is the parsed content of the Message-ID field

  - fld_data.fld_in_reply_to is the parsed content of the In-Reply-To field

  - fld_data.fld_references is the parsed content of the References field

  - fld_data.fld_subject is the content of the Subject field

  - fld_data.fld_comments is the content of the Comments field

  - fld_data.fld_keywords is the parsed content of the Keywords field

  - fld_data.fld_optional_field is an other field and is not parsed
*/

#define LIBETPAN_SMTP_FIELD_UNION

	struct smtp_field
	{
#ifdef USE_NEL
		NEL_REF_DEF
#endif
		int fld_type;
		union
		{
			struct smtp_return *fld_return_path;	/* can be NULL */
			struct smtp_orig_date *fld_resent_date;	/* can be NULL */
			struct smtp_from *fld_resent_from;	/* can be NULL */
			struct smtp_sender *fld_resent_sender;	/* can be NULL */
			struct smtp_to *fld_resent_to;	/* can be NULL */
			struct smtp_cc *fld_resent_cc;	/* can be NULL */
			struct smtp_bcc *fld_resent_bcc;	/* can be NULL */
			struct smtp_message_id *fld_resent_msg_id;	/* can be NULL */
			struct smtp_orig_date *fld_orig_date;	/* can be NULL */
			struct smtp_from *fld_from;	/* can be NULL */
			struct smtp_sender *fld_sender;	/* can be NULL */
			struct smtp_reply_to *fld_reply_to;	/* can be NULL */
			struct smtp_to *fld_to;	/* can be NULL */
			struct smtp_cc *fld_cc;	/* can be NULL */
			struct smtp_bcc *fld_bcc;	/* can be NULL */
			struct smtp_message_id *fld_message_id;	/* can be NULL */
			struct smtp_in_reply_to *fld_in_reply_to;	/* can be NULL */
			struct smtp_references *fld_references;	/* can be NULL */
			struct smtp_subject *fld_subject;	/* can be NULL */
			struct smtp_comments *fld_comments;	/* can be NULL */
			struct smtp_keywords *fld_keywords;	/* can be NULL */
			struct smtp_received *fld_received;	/* wyong, 2005.8.17 */
			struct smtp_optional_field *fld_optional_field;	/* can be NULL */
			struct smtp_eoh *fld_eoh;	/* wyong, 2005.8.18 */
		} fld_data;
	};

	struct smtp_field *smtp_field_new (int fld_type, void *value);
	void smtp_field_free (struct smtp_field *field);

	struct smtp_mime_field
	{
#ifdef USE_NEL
		NEL_REF_DEF
#endif
		int fld_type;
		union
		{
			struct smtp_msg_from *fld_from;

			struct smtp_msg_to *fld_to;

			struct smtp_mime_content_type *fld_content_type;
			struct smtp_mime_mechanism *fld_encoding;
			char *fld_id;
			char *fld_description;
			uint32_t fld_version;
			struct smtp_mime_disposition *fld_disposition;
			struct smtp_mime_language *fld_language;
		} fld_data;
	};

	struct smtp_mime_field *smtp_mime_field_new (int fld_type,
						     void *value);
	void smtp_mime_field_free (struct smtp_mime_field *field);

	struct smtp_mime_fields
	{
#ifdef USE_NEL
		NEL_REF_DEF
#endif
		clist * fld_list;	/* list of (struct smtp_mime_field *) */
		int body_type;
	};

	struct smtp_mime_fields *smtp_mime_fields_new (clist * fld_list);
	void smtp_mime_fields_free (struct smtp_mime_fields *fields);

/*
  smtp_optional_field is a non-parsed field
  - fld_name is the name of the field
  - fld_value is the value of the field
*/
	struct smtp_optional_field
	{
		char *fld_name;	/* != NULL */
		char *fld_value;	/* != NULL */
	};

	struct smtp_optional_field *smtp_optional_field_new (char *fld_name,
							     char *fld_value);

	void smtp_optional_field_free (struct smtp_optional_field *opt_field);


/*
  smtp_fields is the native structure that IMF module will use,
  this module will provide an easier structure to use when parsing fields.

  smtp_single_fields is an easier structure to get parsed fields,
  rather than iteration over the list of fields

  - fld_orig_date is the parsed "Date" field

  - fld_from is the parsed "From" field
  
  - fld_sender is the parsed "Sender "field

  - fld_reply_to is the parsed "Reply-To" field
  
  - fld_to is the parsed "To" field

  - fld_cc is the parsed "Cc" field

  - fld_bcc is the parsed "Bcc" field

  - fld_message_id is the parsed "Message-ID" field

  - fld_in_reply_to is the parsed "In-Reply-To" field

  - fld_references is the parsed "References" field

  - fld_subject is the parsed "Subject" field
  
  - fld_comments is the parsed "Comments" field

  - fld_keywords is the parsed "Keywords" field
*/
	struct smtp_single_fields
	{
		struct smtp_orig_date *fld_orig_date;	/* can be NULL */
		struct smtp_from *fld_from;	/* can be NULL */
		struct smtp_sender *fld_sender;	/* can be NULL */
		struct smtp_reply_to *fld_reply_to;	/* can be NULL */
		struct smtp_to *fld_to;	/* can be NULL */
		struct smtp_cc *fld_cc;	/* can be NULL */
		struct smtp_bcc *fld_bcc;	/* can be NULL */
		struct smtp_message_id *fld_message_id;	/* can be NULL */
		struct smtp_in_reply_to *fld_in_reply_to;	/* can be NULL */
		struct smtp_references *fld_references;	/* can be NULL */
		struct smtp_subject *fld_subject;	/* can be NULL */
		struct smtp_comments *fld_comments;	/* can be NULL */
		struct smtp_received *fld_received;	/* wyong, 2005.8.17 */
		struct smtp_keywords *fld_keywords;	/* can be NULL */
	};

/*
  smtp_single_fields_init fills a smtp_single_fields structure
  with the content of a smtp_fields structure
*/
	void smtp_single_fields_init (struct smtp_single_fields
				      *single_fields,
				      struct smtp_fields *fields);


/*
  smtp_single_fields_new creates a new smtp_single_fields and
  fills the structure with smtp_fields
*/
	struct smtp_single_fields *smtp_single_fields_new (struct smtp_fields
							   *fields);

	void smtp_single_fields_free (struct smtp_single_fields
				      *single_fields);


	struct smtp_mime_single_fields
	{
		struct smtp_mime_content_type *fld_content_type;
		char *fld_content_charset;
		char *fld_content_boundary;
		char *fld_content_name;
		struct smtp_mime_mechanism *fld_encoding;
		char *fld_id;
		char *fld_description;
		uint32_t fld_version;
		struct smtp_mime_disposition *fld_disposition;
		char *fld_disposition_filename;
		char *fld_disposition_creation_date;
		char *fld_disposition_modification_date;
		char *fld_disposition_read_date;
		size_t fld_disposition_size;
		struct smtp_mime_language *fld_language;
	};

	void smtp_mime_single_fields_init (struct smtp_mime_single_fields
					   *single_fields,
					   struct smtp_mime_fields
					   *fld_fields,
					   struct smtp_mime_content_type
					   *fld_content_type);

	struct smtp_mime_single_fields *smtp_mime_single_fields_new (struct
								     smtp_mime_fields
								     *fld_fields,
								     struct
								     smtp_mime_content_type
								     *fld_content_type);

	void smtp_mime_single_fields_free (struct smtp_mime_single_fields
					   *single_fields);


/*
  smtp_field_new_custom creates a new field of type optional

  @param name should be allocated with malloc()
  @param value should be allocated with malloc()
*/
	struct smtp_field *smtp_field_new_custom (char *name, char *value);

	struct smtp_mime_fields *smtp_mime_fields_new_empty (void);

	int smtp_mime_fields_add (struct smtp_mime_fields *fields,
				  struct smtp_mime_field *field);

	struct smtp_mime_fields *smtp_mime_fields_new_with_data (struct
								 smtp_mime_mechanism
								 *encoding,
								 char *id,
								 char
								 *description,
								 struct
								 smtp_mime_disposition
								 *disposition,
								 struct
								 smtp_mime_language
								 *language);

	struct smtp_mime_fields *smtp_mime_fields_new_with_version (struct
								    smtp_mime_mechanism
								    *encoding,
								    char *id,
								    char
								    *description,
								    struct
								    smtp_mime_disposition
								    *disposition,
								    struct
								    smtp_mime_language
								    *language);

/*
  smtp_fields_parse will parse the given header fields
  
  @param message this is a string containing the header fields
  @param length this is the size of the given string
  @param index this is a pointer to the start of the header fields in
    the given string, (* index) is modified to point at the end
    of the parsed data
  @param result the result of the parse operation is stored in
    (* result)

  @return SMTP_NO_ERROR on success, SMTP_ERROR_XXX on error
*/

	int smtp_fields_parse (const char *message, size_t length,
			       size_t * index, struct smtp_fields **result);


/*
  smtp_envelope_fields_parse will parse the given fields (Date,
  From, Sender, Reply-To, To, Cc, Bcc, Message-ID, In-Reply-To,
  References and Subject)
  
  @param message this is a string containing the header fields
  @param length this is the size of the given string
  @param index this is a pointer to the start of the header fields in
    the given string, (* index) is modified to point at the end
    of the parsed data
  @param result the result of the parse operation is stored in
    (* result)

  @return SMTP_NO_ERROR on success, SMTP_ERROR_XXX on error
*/

	int smtp_envelope_fields_parse (const char *message, size_t length,
					size_t * index,
					struct smtp_fields **result);

/*
  smtp_ignore_field_parse will skip the given field
  
  @param message this is a string containing the header field
  @param length this is the size of the given string
  @param index this is a pointer to the start of the header field in
    the given string, (* index) is modified to point at the end
    of the parsed data

  @return SMTP_NO_ERROR on success, SMTP_ERROR_XXX on error
*/


	int smtp_ignore_field_parse (const char *message, size_t length,
				     size_t * index);

/*
  smtp_envelope_fields will parse the given fields (Date,
  From, Sender, Reply-To, To, Cc, Bcc, Message-ID, In-Reply-To,
  References and Subject), other fields will be added as optional
  fields.
  
  @param message this is a string containing the header fields
  @param length this is the size of the given string
  @param index this is a pointer to the start of the header fields in
    the given string, (* index) is modified to point at the end
    of the parsed data
  @param result the result of the parse operation is stored in
    (* result)

  @return SMTP_NO_ERROR on success, SMTP_ERROR_XXX on error
*/


	int smtp_envelope_and_optional_fields_parse (const char *message,
						     size_t length,
						     size_t * index,
						     struct smtp_fields
						     **result);

/*
  smtp_envelope_fields will parse the given fields as optional
  fields.
  
  @param message this is a string containing the header fields
  @param length this is the size of the given string
  @param index this is a pointer to the start of the header fields in
    the given string, (* index) is modified to point at the end
    of the parsed data
  @param result the result of the parse operation is stored in
    (* result)

  @return SMTP_NO_ERROR on success, SMTP_ERROR_XXX on error
*/

	int smtp_optional_fields_parse (const char *message, size_t length,
					size_t * index,
					struct smtp_fields **result);


//xiayu 2005.10.28
	int smtp_envelope_or_optional_field_parse (	/*struct neti_tcp_stream *ptcp, */
							  struct smtp_info
							  *psmtp,
							  const char *message,
							  size_t length,
							  size_t * index,
							  struct
							  smtp_mime_field
							  **result,
							  int *fld_type);

/*
  smtp_resent_fields_add_data adds a set of resent fields in the
  given smtp_fields structure.
  
  if you don't want a given field in the set to be added in the list
  of fields, you can give NULL as argument

  @param resent_msg_id sould be allocated with malloc()

  @return SMTP_NO_ERROR will be returned on success,
  other code will be returned otherwise
*/
//int smtp_resent_fields_add_data(struct smtp_fields * fields,
//    struct smtp_date_time * resent_date,
//    struct smtp_mailbox_list * resent_from,
//    struct smtp_mailbox * resent_sender,
//    struct smtp_address_list * resent_to,
//    struct smtp_address_list * resent_cc,
//    struct smtp_address_list * resent_bcc,
//    char * resent_msg_id);
//

/*
  smtp_resent_fields_new_with_data_all creates a new smtp_fields
  structure with a set of resent fields

  if you don't want a given field in the set to be added in the list
  of fields, you can give NULL as argument

  @param resent_msg_id sould be allocated with malloc()

  @return SMTP_NO_ERROR will be returned on success,
  other code will be returned otherwise
*/
//struct smtp_fields *
//smtp_resent_fields_new_with_data_all(struct smtp_date_time *
//    resent_date, struct smtp_mailbox_list * resent_from,
//    struct smtp_mailbox * resent_sender,
//    struct smtp_address_list * resent_to,
//    struct smtp_address_list * resent_cc,
//    struct smtp_address_list * resent_bcc,
//    char * resent_msg_id);
//

/*
  smtp_resent_fields_new_with_data_all creates a new smtp_fields
  structure with a set of resent fields.
  Resent-Date and Resent-Message-ID fields will be generated for you.

  if you don't want a given field in the set to be added in the list
  of fields, you can give NULL as argument

  @return SMTP_NO_ERROR will be returned on success,
  other code will be returned otherwise
*/
//struct smtp_fields *
//smtp_resent_fields_new_with_data(struct smtp_mailbox_list * from,
//    struct smtp_mailbox * sender,
//    struct smtp_address_list * to,
//    struct smtp_address_list * cc,
//    struct smtp_address_list * bcc);
//

/*
  this function creates a new smtp_fields structure with no fields
*/
	struct smtp_fields *smtp_fields_new_empty (void);


/*
  this function adds a field to the smtp_fields structure

  @return SMTP_NO_ERROR will be returned on success,
  other code will be returned otherwise
*/
	int smtp_fields_add (struct smtp_fields *fields,
			     struct smtp_field *field);


/*
  smtp_fields_add_data adds a set of fields in the
  given smtp_fields structure.
  
  if you don't want a given field in the set to be added in the list
  of fields, you can give NULL as argument

  @param msg_id sould be allocated with malloc()
  @param subject should be allocated with malloc()
  @param in_reply_to each elements of this list should be allocated
    with malloc()
  @param references each elements of this list should be allocated
    with malloc()

  @return SMTP_NO_ERROR will be returned on success,
  other code will be returned otherwise
*/
//int smtp_fields_add_data(struct smtp_fields * fields,
//                          struct smtp_date_time * date,
//                          struct smtp_mailbox_list * from,
//                          struct smtp_mailbox * sender,
//                          struct smtp_address_list * reply_to,
//                          struct smtp_address_list * to,
//                          struct smtp_address_list * cc,
//                          struct smtp_address_list * bcc,
//                          char * msg_id,
//                          clist * in_reply_to,
//                          clist * references,
//                          char * subject);


/*
  smtp_fields_new_with_data_all creates a new smtp_fields
  structure with a set of fields

  if you don't want a given field in the set to be added in the list
  of fields, you can give NULL as argument

  @param message_id sould be allocated with malloc()
  @param subject should be allocated with malloc()
  @param in_reply_to each elements of this list should be allocated
    with malloc()
  @param references each elements of this list should be allocated
    with malloc()

  @return SMTP_NO_ERROR will be returned on success,
  other code will be returned otherwise
*/

//struct smtp_fields *
//smtp_fields_new_with_data_all(struct smtp_date_time * date,
//                               struct smtp_mailbox_list * from,
//                               struct smtp_mailbox * sender,
//                               struct smtp_address_list * reply_to,
//                               struct smtp_address_list * to,
//                               struct smtp_address_list * cc,
//                               struct smtp_address_list * bcc,
//                               char * message_id,
//                               clist * in_reply_to,
//                               clist * references,
//                               char * subject);
//

/*
  smtp_fields_new_with_data creates a new smtp_fields
  structure with a set of fields
  Date and Message-ID fields will be generated for you.

  if you don't want a given field in the set to be added in the list
  of fields, you can give NULL as argument

  @param subject should be allocated with malloc()
  @param in_reply_to each elements of this list should be allocated
    with malloc()
  @param references each elements of this list should be allocated
    with malloc()

  @return SMTP_NO_ERROR will be returned on success,
  other code will be returned otherwise
*/
//struct smtp_fields *
//smtp_fields_new_with_data(struct smtp_mailbox_list * from,
//                           struct smtp_mailbox * sender,
//                           struct smtp_address_list * reply_to,
//                           struct smtp_address_list * to,
//                           struct smtp_address_list * cc,
//                           struct smtp_address_list * bcc,
//                           clist * in_reply_to,
//                           clist * references,
//                           char * subject);


/*
  this function returns a smtp_date_time structure to
  use in a Date or Resent-Date field
*/
	struct smtp_date_time *smtp_get_current_date (void);

	struct smtp_mime_fields *smtp_mime_fields_new_filename (int dsp_type,
								char
								*filename,
								int
								encoding_type);

	struct smtp_mime_fields *smtp_mime_fields_new_encoding (int type);

	void smtp_field_name_free (char *field_name);

	struct smtp_mime_content_type *get_content_type_from_fields (struct
								     smtp_mime_fields
								     *fields);

	int get_body_type_from_fields (struct smtp_mime_fields *fields);

#ifdef __cplusplus
}
#endif

#endif
