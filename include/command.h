#ifndef COMMAND_H
#define COMMAND_H

/* type of commands */
enum
{
	SMTP_EVENT_UNCERTAIN,

	SMTP_CMD_RSET,
	SMTP_CMD_HELO,
	SMTP_CMD_EHLO,
	SMTP_CMD_AUTH,
	SMTP_CMD_USER,
	SMTP_CMD_PASSWD,
	SMTP_CMD_MAIL,
	SMTP_CMD_RCPT,
	SMTP_CMD_DATA,
	SMTP_CMD_BDAT,
	SMTP_CMD_MESG,
	SMTP_CMD_QUIT,
	SMTP_CMD_TURN,
	SMTP_CMD_ATRN,
	SMTP_CMD_VRFY,
	SMTP_CMD_EXPN,
	SMTP_CMD_HELP,
	SMTP_CMD_NOOP,
	SMTP_CMD_SEND,
	SMTP_CMD_SOML,
	SMTP_CMD_SAML,
	SMTP_CMD_ETRN,
	SMTP_CMD_STARTTLS,

	SMTP_MSG_EOH,		/* end of message header */
	SMTP_MSG_EOT,		/* end of text ("CRLF.CRLF" or 
				   "boundary" or "boundary--") */
	SMTP_MSG_EOB,		/* end of a message nested body ("boundary--") */
	SMTP_MSG_EOM,		/* end of message ("CRLF.CRLF") */
	SMTP_MSG_TEXT,

	SMTP_CMD_NUM,
};

enum smtp_allow_type
{
	SMTP_ALLOW_UNCERTAIN,
	SMTP_ALLOW_8BITMIME,
	SMTP_ALLOW_7BIT,
	SMTP_ALLOW_ATRN,
	SMTP_ALLOW_AUTH,	// AUTH=LOGIN 
	SMTP_ALLOW_BINARYMIME,
	SMTP_ALLOW_CHECKPOINT,
	SMTP_ALLOW_CHUNKING,
	SMTP_ALLOW_DELIVERBY,
	SMTP_ALLOW_DSN,
	SMTP_ALLOW_ENHANCEDSTATUSCODES,	// ENHANGEDSTATUSCODES [sic] 
	SMTP_ALLOW_ETRN,
	SMTP_ALLOW_EXPN,
	SMTP_ALLOW_HELP,
	SMTP_ALLOW_ONEX,
	SMTP_ALLOW_NO_SOLICITING,
	SMTP_ALLOW_MTRK,
	SMTP_ALLOW_PIPELINING,
	SMTP_ALLOW_RSET,
	SMTP_ALLOW_SAML,
	SMTP_ALLOW_SEND,
	SMTP_ALLOW_SIZE,
	SMTP_ALLOW_SOML,
	SMTP_ALLOW_STARTTLS,
	SMTP_ALLOW_RELAY,
	SMTP_ALLOW_TIME,
	SMTP_ALLOW_TLS,
	SMTP_ALLOW_TURN,
	SMTP_ALLOW_VERB,	//xiayu 2005.10.10
	SMTP_ALLOW_VRFY,
	SMTP_ALLOW_X_EXPS,	// X-EXPS, X-EXPS=LOGIN 
	SMTP_ALLOW_X_LINK2STATE,	// X-LINK2STATE
	SMTP_ALLOW_X_RCPTLIMIT,	// X-RCPTLIMIT
	SMTP_ALLOW_X_TURNME,	// X-TURNME
	SMTP_ALLOW_XADR,
	SMTP_ALLOW_XAUD,
	SMTP_ALLOW_XDSN,
	SMTP_ALLOW_XEXCH50,
	SMTP_ALLOW_XGEN,
	SMTP_ALLOW_XLOOP,
	SMTP_ALLOW_XONE,
	SMTP_ALLOW_XQUE,
	SMTP_ALLOW_XREMOTEQUEUE,
	SMTP_ALLOW_XSTA,
	SMTP_ALLOW_XTRN,
	SMTP_ALLOW_XUSR,
	SMTP_ALLOW_XVRB,

	SMTP_ALLOW_TYPE_NUM
};

/* ack events */
enum
{
	SMTP_ACK,
	SMTP_ACK_VRFY,
	SMTP_ACK_EXPN,
};




/*** ack event types ***/
struct smtp_ack
{
#ifdef USE_NEL
	NEL_REF_DEF
#endif
	int len;
	short code;
	short str_len;
	char *str;
};

void smtp_ack_free (struct smtp_ack *ack);

struct smtp_ack *smtp_ack_new (int len, int code);

//int parse_smtp_ack( struct smtp_info *psmtp);
//int parse_smtp_command ( struct smtp_info *psmtp);

#endif
