#ifndef SMTP_H
#define SMTP_H


#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <sys/types.h>
#include <inttypes.h>


#include "nids.h"
#include "clist.h"
#include "analysis.h"

/* error message */
#define NETI_ERR_NO_MEMORY		"³õÊ¼»¯´íÎó,ÄÚ´æ²»×ã\n"
#define NETI_ERR_STREAM_TIMEOUT	"Á¬½Ó³¬Ê±\n"
#define NETI_ERR_STEAM_CLOSED	"¹Ø±ÕÁ¬½Ó\n"
#define NETI_ERR_REFUSED		"ÄÚ´æ²»×ã,¾Ü¾øÁ¬½Ó\n"
#define NETI_ERR_CMD_TOO_LONG	"SMTP ÃüÁîÐÐ³¬³¤\n"
#define NETI_ERR_CMD_UNRECOGNIZED	"SMTP ÃüÁîÎÞ·¨Ê¶±ð\n"
#define NETI_ERR_CMD_BAD_SEQUENCE	"SMTP ÃüÁîÐòÁÐ´íÎó\n"
#define NETI_ERR_SYNTAX			"SMTP ÃüÁîÓï·¨´íÎó\n"
#define NETI_ERR_NO_RULE		"Ã»ÓÐÆ¥ÅäµÄ¹æÔò,¹Ø±ÕÁ¬½Ó\n"
#define NETI_ERR_SIP_BOMB		"Ô´IPÕ¨µ¯,¹Ø±ÕÁ¬½Ó\n"
#define NETI_ERR_CLOSED_BY_CLIENT	"¿Í»§¶Ë¶Ï¿ªÁ¬½Ó\n"
#define NETI_ERR_CLOSED_BY_SERVER	"·þÎñÆ÷¶Ë¶Ï¿ªÁ¬½Ó\n"

/* debug message */
#define CF_SMTP_WAIT	"SMTP waitting..\n"
#define CF_SMTP_RUN		"SMTP run..\n"
#define CF_SMTP_TIMEOUT	"SMTP stream timeout..\n"
#define CF_SMTP_CLOSED	"SMTP stream closed..\n"
#define CF_SMTP_REFUSED	"SMTP connection refused by no memory.\n"
#define CF_SMTP_ACCEPT	"SMTP accept new stream\n"
#define CF_SMTP_PARSE_CMD	"SMTP parse smtp command..\n"

#define SMTP_BUF	1
#define SMTP_DBG	1
#define SMTP_MEM	1	/* memory debuging */

#if 1
#define DEBUG_SMTP(tag,fmt...)
#else
#define DEBUG_SMTP(tag,fmt, ...)\
do {\
	if(tag) {\
		fprintf(stderr, "%s [%d]: ", __FILE__, __LINE__);\
		fprintf(stderr, fmt, ##__VA_ARGS__ );\
	}\
}while(0)
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif


#define MIME_VERSION (1 << 16)



/* these are the possible returned error codes */
enum
{

	SMTP_ERROR_CONTINUE = 1,
	SMTP_NO_ERROR = 0,
	SMTP_ERROR_MEMORY = -1,	/* can't alloc memory */
	SMTP_ERROR_FILE = -2,	/* can't open file */
	SMTP_ERROR_ENGINE = -3,
	SMTP_ERROR_TCPAPI_READ = -4,
	SMTP_ERROR_TCPAPI_WRITE = -5,
	SMTP_ERROR_INVAL = -6,	/* */
	SMTP_ERROR_STACK_POP = -7,	/* part stack overflow */
	SMTP_ERROR_STACK_PUSH = -8,	/* part stack overflow */
	SMTP_ERROR_BUFFER = -9,	/* buffer overflow */

	SMTP_ERROR_PROTO = -10,
	SMTP_ERROR_POLICY = -11,
	SMTP_ERROR_PARSE = -12,
};


/* memory stack size */
#define SMTP_INFO_STACK_DEPTH 20
#define SMTP_INFO_STACK_LIMIT 0

#define MAIL_ATTR_ENCODING      "encoding"	/* internal encoding */
#define MAIL_ATTR_ENC_BINARY	"BINARYMIME"	/* BINARY equivalent */
#define MAIL_ATTR_ENC_8BIT      "8bit"	/* 8BITMIME equivalent */
#define MAIL_ATTR_ENC_7BIT      "7bit"	/* 7BIT equivalent */
#define MAIL_ATTR_ENC_NONE      ""	/* encoding unknown */


/*
 * Define smtp closed flags
 */
#define	CLIENT_CLOSED	0x01
#define	SERVER_CLOSED	0x02

#define	SMTP_MAILBOX_ADDR_BUF_LEN	256
#define	SMTP_STRING_BUF_LEN	256

#define SMTP_BUF_LEN		2048


#define SMTP_MEM_STACK_DEPTH	200
#define	SMTP_MEM_STACK_LIMIT	0




enum smtp_encode_type
{
	SMTP_ENCODE_UNCERTAIN = 0,
	SMTP_ENCODE_UTF_8,
	SMTP_ENCODE_BASE64,
	SMTP_ENCODE_QUOTED_PRINTABLE,
	SMTP_ENCODE_UUENCODE,
	SMTP_ENCODE_HZ,
	SMTP_ENCODE_GZIP,
	SMTP_ENCODE_7BIT,
	SMTP_ENCODE_8BIT,
	SMTP_ENCODE_BINARY,
	SMTP_ENCODE_IETF_TOKEN,
	SMTP_ENCODE_X_TOKEN
};

enum smtp_charset_type
{

	SMTP_CHARSET_UNCERTAIN,
	/* "Charset" form ShiYan's charset lib */
	SMTP_CHARSET_GB2312,
	SMTP_CHARSET_BIG5,
	SMTP_CHARSET_UNICODE,
	SMTP_CHARSET_US_ASCII,
	SMTP_CHARSET_ISO_8859_1,
	SMTP_CHARSET_ISO_8859_2,
	SMTP_CHARSET_ISO_8859_3,
	SMTP_CHARSET_ISO_8859_4,
	SMTP_CHARSET_ISO_8859_5,
	SMTP_CHARSET_ISO_8859_6,
	SMTP_CHARSET_ISO_8859_7,
	SMTP_CHARSET_ISO_8859_8,
	SMTP_CHARSET_ISO_8859_9,
	SMTP_CHARSET_ISO_8859_10
};

/* enum type of the parameter "differences",
   of Content-Type Multipart/Alternative */
enum smtp_difference_type
{
	SMTP_DIFF_UNCERTAIN,
	SMTP_DIFF_CONTENT_TYPE,	/* Content-Type */
	SMTP_DIFF_CONTENT_LANGUAGE	/* Content-Language¡ */
};

/******************* for outputing to NEL Engine **************************/
struct smtp_simple_event
{
	NEL_REF_DEF int len;
	int str_len;
	char *str;
};

struct smtp_atk
{
#ifdef USE_NEL
	NEL_REF_DEF
#endif
	int len;
};


/************************** parser struct ******************************/
struct smtp_keyword
{
	char *kw_str;
	int kw_type;
};


enum smtp_permit_type
{
	SMTP_PERMIT_FORWARD,	/* 0x00 forward */
	SMTP_PERMIT_ALERT,	/* 0x01 alert */
	SMTP_PERMIT_DENY,	/* 0x02 deny */
	SMTP_PERMIT_DROP	/* 0x04 drop */
};


/* smtp parser type */
enum smtp_parse_state
{

	SMTP_STATE_UNCERTAIN,
	SMTP_STATE_PARSE_COMMAND,
	SMTP_STATE_PARSE_HEADER,
	SMTP_STATE_PARSE_CONTENT,
	SMTP_STATE_PARSE_MESSAGE,
	SMTP_STATE_PARSE_RESPONSE
};

struct smtp_keyword_tree_item
{
	struct trieobj *tree;
	int refer_flag;		/* "tree" ÊÇ·ñÊÇ±»ÒýÓÃµÄÖ¸Õë£¬ÒÔÃâÄÚ´æ±»ÖØ¸´ÊÍ·Å */
};

struct count_item
{
	short total;		/* including both succeeded ones and failed ones */
	short fail;		/* only count of failed ones */
};



enum smtp_mail_state
{
	SMTP_MAIL_STATE_ORIGN,
	SMTP_MAIL_STATE_HELO,
	SMTP_MAIL_STATE_MAIL,
	SMTP_MAIL_STATE_RCPT,
	SMTP_MAIL_STATE_DATA,
};


#define MAX_SMTPS 	1040 * 100


/*
 * information of a mail's transfer and content.
 */
struct smtp_info
{				/* connection infomation and state */
#ifdef USE_NEL
	NEL_REF_DEF
#endif
	int errors_count;
	u_int32_t saddr;
	u_int32_t daddr;
	u_short sport;
	u_short dport;
	int hash_index;
	int opened;

	struct smtp_info *prev_node;
	struct smtp_info *next_node;
	struct smtp_info *next_free;

	struct smtp_info *next;
	int completed;
	short connect_state;	/* 0x01 :CLIENT_CLOSED  */
				/* 0x02 :SERVER_CLOSED  */

#ifdef USE_NEL
	struct nel_env env;	/* for engine analysis */
#endif

	short last_cli_event_type;
	short curr_parse_state;	/* SMTP_STATE_PARSE_ENVELOP 
				   or SMTP_STATE_PARSE_CONTENT */

	/*** data buffer ***/
	/* server */
	unsigned char svr_buf[SMTP_BUF_LEN + 1];	/* buffer, save the data that
							 * server return to client, according
							 * to a SMTP command. */
	unsigned char *svr_data;	/* pointer to the beging of data
					   that hasn't been sent to client yet */
	short svr_data_len;	/* length of data that server return */


	/* client */
	unsigned char *cli_buf;	/* buffer,save data from client
				 * including commands, parameters,
				 * and mails to be transfered. */
	int cli_buf_len;

	unsigned char *cli_data;	/* pointer to the beging of data
					   that hasn't been sent to server yet */
	int cli_data_len;	/* length of data that has NOT been parsed yet
				   calculated from "cli_data + cli_parse_len" */

	char *auth_allow;
	int *cmd_allow;

	struct smtp_part *part_stack;	/*** for content parsing ***/
	int part_stack_top;
	int new_part_flag;

	char *domain;		/* the domain of SMTP server */
	int msg_size;		/* MAIL FROM message size */
	int act_size;		/* message's actural size */
	char *helo_name;	/* client HELO/EHLO argument */
	char *sender;		/* sender address */
	char *recipient;	/* reciever address */
	int rcpt_count;		/* reciever counter */
	int encoding;		/* owned by mail_cmd() */

	int mail_state;
	short permit;		/* whether the mail need drop or deny */

	//struct        mailmime *mime;
};


//int smtp_init (struct nel_eng *, char *);
//void smtp_close_connection (struct smtp_info *);

//void process_smtp (void);
//void smtp_cleanup(void);

//struct smtp_ack *smtp_ack_new(int len, int code/*, int str_len, char *str */);

//int process_smtp(struct tcp_packet *tp);
int process_smtp (struct tcp_stream *a_tcp, size_t size, int from_client,
		  int conn_type);

int smtp_init (void);
void smtp_cleanup (void);
int sync_client_data (struct smtp_info *psmtp, int processed_data_len);
int sync_server_data (struct smtp_info *psmtp, int processed_data_len);
void reset_server_buf (struct smtp_info *sp, size_t *index);
void reset_client_buf (struct smtp_info *sp, size_t *index);


void free_smtp_info (struct smtp_info *psmtp);



#endif
