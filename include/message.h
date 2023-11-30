#ifndef MESSAGE_H
#define MESSAGE_H

#include "smtp.h"
#include "content-type.h"

//struct smtp_mime_content_type;

enum
{
	SMTP_MSG_FIELDS_PARSING,
	SMTP_MSG_BODY_PARSING,
	SMTP_MSG_PREAMBLE_PARSING,
	SMTP_MSG_EPILOGUE_PARSING,
};


/*
  smtp_message is the content of the message
  - msg_fields is the header fields of the message
  - msg_body is the text part of the message
*/
struct smtp_message
{
	struct smtp_fields *msg_fields;	/* != NULL */
	struct smtp_body *msg_body;	/* != NULL */
};

//struct smtp_message *
//smtp_message_new(struct smtp_fields * msg_fields, struct smtp_body * msg_body);
//void smtp_message_free(struct smtp_message * message);

struct smtp_part
{
	int state;
	int body_type;
	struct smtp_mime_content_type *content_type;
	int encode_type;
	char *boundary;	
	//struct trieobj *boundary_tree;	
	int flag;		
};

#define SMTP_MSG_STACK_MAX_DEPTH	16

int smtp_mime_part_parse(char *message, 
			size_t length,
			size_t * index,
			int encoding,
			char **result, 
			size_t * result_len);
/*
  smtp_message_parse will parse the given message
  
  @param message this is a string containing the message content
  @param length this is the size of the given string
  @param index this is a pointer to the start of the message in
    the given string, (* index) is modified to point at the end
    of the parsed data
  @param result the result of the parse operation is stored in
    (* result)

  @return SMTP_NO_ERROR on success, SMTP_ERROR_XXX on error
*/

//int smtp_message_parse(char * message, size_t length,
//                        size_t * index,
//                        struct smtp_message ** result);

int parse_smtp_message (struct smtp_info *psmtp);



//int smtp_mime_add_part(struct mailmime * build_info,
//                    struct mailmime * part);

//void smtp_mime_remove_part(struct mailmime * mime);

//int smtp_mime_smart_add_part(struct mailmime * mime,
//    struct mailmime * mime_sub);

//int smtp_mime_smart_remove_part(struct mailmime * mime);

//struct mailmime * smtp_mime_multiple_new(const char * type);

int smtp_ack_msg_parse (struct smtp_info *psmtp);

static int
smtp_mime_multipart_close_parse(char *message,
                                size_t length,
                                size_t * index);


static int
smtp_mime_body_part_dash2_boundary_parse(char *message,
                                        size_t length,
                                        size_t * index,
                                        char *boundary, //struct trieobj *boundary_tree, 
                                        char **result,
                                        size_t * result_size) ;


int
pop_part_stack (struct smtp_info *psmtp); 

#endif
