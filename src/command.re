/*
 * $Id: command.re,v 1.13 2005/12/06 10:32:14 xiay Exp $
 */


#include "mem_pool.h"
#include "mmapstring.h"


#include "smtp.h"
#include "command.h"
#include "mime.h"
#include "address.h"
#include "auth.h"
#include "helo.h"
#include "ehlo.h"
#include "user.h"
#include "password.h"
#include "mail-from.h"
#include "send-from.h"
#include "soml-from.h"
#include "saml-from.h"
#include "rcpt-to.h"
#include "data.h"
#include "quit.h"
#include "rset.h"
#include "expn.h"
#include "vrfy.h"
#include "turn.h"
#include "message.h"


#ifdef USE_NEL
#include "engine.h"
int ack_id;
extern struct nel_eng *eng;
#endif

extern ObjPool_t smtp_ack_pool;
extern short max_smtp_ack_len;


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


void
smtp_ack_free (struct smtp_ack *ack)
{
	free_mem (&smtp_ack_pool, (void *) ack);
	DEBUG_SMTP (SMTP_MEM, "smtp_ack_free: pointer=%p, elm=%p\n",
		    &smtp_ack_pool, (void *) ack);
}

struct smtp_ack *
smtp_ack_new (int len, int code /*, int str_len, char *str */ )
{
	struct smtp_ack *ack;

	ack = (struct smtp_ack *) alloc_mem (&smtp_ack_pool);
	if (!ack) {
		return NULL;
	}
	DEBUG_SMTP (SMTP_MEM, "smtp_ack_new: pointer=%p, elm=%p\n",
		    &smtp_ack_pool, ack);

	//ack->event_type = SMTP_ACK;
	//ack->nel_id = ack_id;

#ifdef USE_NEL
	//ack->count = 0;
	NEL_REF_INIT (ack);
#endif
	ack->len = len;		/* the total length of this ack line */
	ack->code = code;

	//ack->str_len = str_len;
	//ack->str = str

	return ack;
}

int
parse_smtp_ack (struct smtp_info *psmtp)
{
	char *p1, *p2, *p3;
	struct smtp_ack *ack;
	int r, res;
	char *buf = psmtp->svr_data;
	int len = psmtp->svr_data_len;
	int cur_token = 0;

	DEBUG_SMTP (SMTP_DBG, "parse_smtp_ack... \n");

	DEBUG_SMTP (SMTP_DBG, "\n");
	if (len < 5) {		/* 3 digits, "\r\n", add up to 5 */
		DEBUG_SMTP (SMTP_DBG, "\n");
		//psmtp->curr_svr_event = NULL;
		res = SMTP_ERROR_PARSE;	//Fixme
		goto err;
	}

	DEBUG_SMTP (SMTP_DBG, "\n");
	switch (psmtp->last_cli_event_type) {

	case SMTP_CMD_HELO:
		DEBUG_SMTP (SMTP_DBG, "\n");
		return smtp_ack_helo_parse ( /* ptcp, */ psmtp);

	case SMTP_CMD_EHLO:
		DEBUG_SMTP (SMTP_DBG, "\n");
		return smtp_ack_ehlo_parse ( /* ptcp, */ psmtp);

	case SMTP_CMD_AUTH:
		DEBUG_SMTP (SMTP_DBG, "\n");
		return smtp_ack_auth_parse ( /* ptcp, */ psmtp);

	case SMTP_CMD_USER:
		DEBUG_SMTP (SMTP_DBG, "\n");
		return smtp_ack_user_parse ( /* ptcp, */ psmtp);

	case SMTP_CMD_PASSWD:
		DEBUG_SMTP (SMTP_DBG, "\n");
		return smtp_ack_passwd_parse ( /* ptcp, */ psmtp);

	case SMTP_CMD_MAIL:
		DEBUG_SMTP (SMTP_DBG, "\n");
		return smtp_ack_mail_parse ( /* ptcp, */ psmtp);

	case SMTP_CMD_SEND:
		DEBUG_SMTP (SMTP_DBG, "\n");
		return smtp_ack_send_parse ( /* ptcp, */ psmtp);

	case SMTP_CMD_SOML:
		DEBUG_SMTP (SMTP_DBG, "\n");
		return smtp_ack_soml_parse ( /*ptcp, */ psmtp);

	case SMTP_CMD_SAML:
		DEBUG_SMTP (SMTP_DBG, "\n");
		return smtp_ack_saml_parse ( /* ptcp, */ psmtp);

	case SMTP_CMD_RCPT:
		DEBUG_SMTP (SMTP_DBG, "\n");
		return smtp_ack_rcpt_parse ( /* ptcp, */ psmtp);

	case SMTP_CMD_VRFY:
		DEBUG_SMTP (SMTP_DBG, "\n");
		return smtp_ack_vrfy_parse ( /* ptcp, */ psmtp);

	case SMTP_CMD_EXPN:
		DEBUG_SMTP (SMTP_DBG, "\n");
		return smtp_ack_expn_parse ( /* ptcp, */ psmtp);

	case SMTP_CMD_DATA:
		DEBUG_SMTP (SMTP_DBG, "\n");
		return smtp_ack_data_parse ( /* ptcp, */ psmtp);

	case SMTP_CMD_RSET:
		DEBUG_SMTP (SMTP_DBG, "\n");
		return smtp_ack_rset_parse ( /*ptcp, */ psmtp);

	case SMTP_CMD_QUIT:
		DEBUG_SMTP (SMTP_DBG, "\n");
		return smtp_ack_quit_parse ( /*ptcp, */ psmtp);

	case SMTP_MSG_EOM:
		DEBUG_SMTP (SMTP_DBG, "\n");
		return smtp_ack_msg_parse ( /*ptcp, */ psmtp);

	default:
		break;
	}

#if 0				//xiayu 2005.11.22 let engine do the checking
	DEBUG_SMTP (SMTP_DBG, "\n");
	if (len > max_smtp_ack_len) {
		DEBUG_SMTP (SMTP_DBG, "smtp server ack line is too long\n");
		return SMTP_ERROR_PROTO;
	}
#endif

	p1 = buf;
	p2 = buf + len;

	/* *INDENT-OFF* */
	/*!re2c
	digit{3,3}
	{
		DEBUG_SMTP(SMTP_DBG, "\n");

		//xiayu Fixme
		//str = (char *)alloc_mem (&smtp_string_pool);
		//str_len = p1 - p2  + len - 2; 
		//memcpy(str, buf+4, str_len);
		//str[str_len] = '\0';

		ack = smtp_ack_new(len, atoi(buf) /*, str_len, str */);
		if (!ack) {
			res = SMTP_ERROR_MEMORY;
			goto err;
		}

#ifdef USE_NEL
		if ( ( r = nel_env_analysis(eng, &(psmtp->env), ack_id,  
				(struct smtp_simple_event *)ack) ) < 0 ) {
			DEBUG_SMTP(SMTP_DBG, "parse_smtp_ack: nel_env_analysis error\n");
			res = SMTP_ERROR_ENGINE; 
			goto free;
		}
#endif 
		/* Fixme: how to deal with the client? */
		if (psmtp->permit & SMTP_PERMIT_DENY) {
			//smtp_close_connection(ptcp, psmtp);
			res = SMTP_ERROR_POLICY;
			goto err;
		} else if (psmtp->permit & SMTP_PERMIT_DROP) {
			res = SMTP_ERROR_POLICY;
			goto err;
		}

		psmtp->svr_data += len;
		psmtp->svr_data_len = 0;
		//if (write_to_client(ptcp, psmtp) < 0) {
		//	res = SMTP_ERROR_TCPAPI_WRITE;
		//	goto free;
		//}
		return SMTP_NO_ERROR;
	}

	digit{3,3} "\r\n"
	{
		DEBUG_SMTP(SMTP_DBG, "\n");

		ack = smtp_ack_new(len, atoi(buf) /*, str_len, str */);
		if (!ack) {
			res = SMTP_ERROR_MEMORY;
			goto err;
		}

#ifdef USE_NEL
		if ( ( r = nel_env_analysis(eng, &(psmtp->env), ack_id,  
				(struct smtp_simple_event *)ack) ) < 0 ) {
			DEBUG_SMTP(SMTP_DBG, "parse_smtp_ack: nel_env_analysis error\n");
			res = SMTP_ERROR_ENGINE; 
			goto free;
		}
#endif 
		/* Fixme: how to deal with the client? */
		if (psmtp->permit & SMTP_PERMIT_DENY) {
			//smtp_close_connection(ptcp, psmtp);
			res = SMTP_ERROR_POLICY;
			goto err;
		} else if (psmtp->permit & SMTP_PERMIT_DROP) {
			res = SMTP_ERROR_POLICY;
			goto err;
		}

		psmtp->svr_data += 5;
		psmtp->svr_data_len = 0;
		//if (write_to_client(ptcp, psmtp) < 0) {
		//	res = SMTP_ERROR_TCPAPI_WRITE;
		//	goto free;
		//}
		return SMTP_NO_ERROR;
	}

	any 
	{
		//psmtp->curr_svr_event = NULL;
		DEBUG_SMTP(SMTP_DBG, "server response unrecognizable data %s\n", buf);
		res = SMTP_ERROR_PARSE;
		goto err;
	}

	*/
	/* *INDENT-ON* */

      free:
	DEBUG_SMTP (SMTP_DBG, "parse_smtp_ack: Free\n");
	smtp_ack_free (ack);

      err:
	DEBUG_SMTP (SMTP_DBG, "parse_smtp_ack: Error\n");
	reset_server_buf (psmtp, &cur_token);
	return res;

}





/*
 * Return :	 1 - smtp command line successfully recognized
 *		 0 - smtp command line is incomplete
 *		-1 - smtp command line unrecognized
 *		-2 - error
 */
int
parse_smtp_command (struct smtp_info *psmtp)
{
	char *p1 = psmtp->cli_data;
	char *p2 = psmtp->cli_data + psmtp->cli_data_len;
	char *p3;
	size_t cur_token = 0;
	size_t length = psmtp->cli_data_len;
	int r, res;

	DEBUG_SMTP (SMTP_DBG, "parse_smtp_command\n");

	DEBUG_SMTP (SMTP_DBG, "\n");
	if (psmtp->last_cli_event_type == SMTP_CMD_AUTH) {
		DEBUG_SMTP (SMTP_DBG, "\n");
		return smtp_cmd_user_parse ( /*ptcp, */ psmtp,
					    psmtp->cli_data,
					    psmtp->cli_data_len, &cur_token);
	}

	DEBUG_SMTP (SMTP_DBG, "\n");
	if (psmtp->last_cli_event_type == SMTP_CMD_USER) {
		DEBUG_SMTP (SMTP_DBG, "\n");
		return smtp_cmd_passwd_parse ( /*ptcp, */ psmtp,
					      psmtp->cli_data,
					      psmtp->cli_data_len,
					      &cur_token);

	}

	DEBUG_SMTP (SMTP_DBG, "\n");
	DEBUG_SMTP (SMTP_DBG, "p1 = %s\n", p1);

	/* *INDENT-OFF* */
	/*!re2c
	[Rr][Ss][Ee][Tt]
	{ 
		DEBUG_SMTP(SMTP_DBG, "parse_smtp_command:REST\n");
		cur_token += 4; 
		return smtp_cmd_rset_parse(/*ptcp,*/ psmtp, psmtp->cli_data, psmtp->cli_data_len, &cur_token);
	}
	[Hh][Ee][Ll][Oo] 
	{
		DEBUG_SMTP(SMTP_DBG, "parse_smtp_command:HELO\n");
		cur_token += 4; 
		return smtp_cmd_helo_parse(/*ptcp,*/ psmtp, psmtp->cli_data, psmtp->cli_data_len, &cur_token);
	}
	[Ee][Hh][Ll][Oo] 
	{
		DEBUG_SMTP(SMTP_DBG, "parse_smtp_command:EHLO\n");
		cur_token += 4; 
		return smtp_cmd_ehlo_parse(/*ptcp,*/ psmtp, psmtp->cli_data, psmtp->cli_data_len, &cur_token);
	}
	[Aa][Uu][Tt][Hh] 
	{
		DEBUG_SMTP(SMTP_DBG, "parse_smtp_command:AUTH\n");
		cur_token += 4; 
		psmtp->last_cli_event_type = SMTP_CMD_AUTH;
		return smtp_cmd_auth_parse(/*ptcp,*/ psmtp, psmtp->cli_data, psmtp->cli_data_len, &cur_token);
	}
	[Mm][Aa][Ii][Ll][ ][Ff][Rr][Oo][Mm][:]
	{
		DEBUG_SMTP(SMTP_DBG, "parse_smtp_command:MAIL\n");
		cur_token += 10;
		return smtp_cmd_mail_parse(/*ptcp,*/ psmtp, psmtp->cli_data, psmtp->cli_data_len, &cur_token);
	}
	[Rr][Cc][Pp][Tt][ ][Tt][Oo][:]
	{
		DEBUG_SMTP(SMTP_DBG, "parse_smtp_command:RCPT\n");
		cur_token += 8;
		return smtp_cmd_rcpt_parse(/*ptcp,*/ psmtp, psmtp->cli_data, psmtp->cli_data_len, &cur_token);
		goto succ;
	}
	[Dd][Aa][Tt][Aa]
	{
		DEBUG_SMTP(SMTP_DBG, "parse_smtp_command:DATA\n");
		cur_token += 4;
		return smtp_cmd_data_parse(/*ptcp,*/ psmtp, psmtp->cli_data, psmtp->cli_data_len, &cur_token);
	}
	[Bb][Dd][Aa][Tt]
	{
		DEBUG_SMTP(SMTP_DBG, "parse_smtp_command:BDAT\n");
		psmtp->last_cli_event_type = SMTP_CMD_BDAT;
		cur_token += length;
		goto succ;
	}
	[Qq][Uu][Ii][Tt]
	{
		DEBUG_SMTP(SMTP_DBG, "parse_smtp_command:QUIT\n");
		cur_token += 4;
		return smtp_cmd_quit_parse(/*ptcp,*/ psmtp, psmtp->cli_data, psmtp->cli_data_len, &cur_token);
	}
	[Tt][Uu][Rr][Nn]
	{
		//struct smtp_cmd_ehlo *turn;
		DEBUG_SMTP(SMTP_DBG, "parse_smtp_command:TURN\n");
		psmtp->last_cli_event_type = SMTP_CMD_TURN;
		//r = smtp_turn_parse(psmtp, psmtp->cli_data, psmtp->cli_data,  &cur_token, &turn);
		cur_token += length;
		goto succ;
	}
	[Aa][Tt][Rr][Nn]
	{
		DEBUG_SMTP(SMTP_DBG, "parse_smtp_command:ATRN\n");
		psmtp->last_cli_event_type = SMTP_CMD_ATRN;
		cur_token += length;
		goto succ;
	}
	[Vv][Rr][Ff][Yy]
	{
		DEBUG_SMTP(SMTP_DBG, "parse_smtp_command:VRFY\n");
		cur_token += 4; 
		return smtp_cmd_vrfy_parse(/*ptcp, */ psmtp, psmtp->cli_data, psmtp->cli_data_len, &cur_token);
	}
	[Ee][Xx][Pp][Nn]
	{
		DEBUG_SMTP(SMTP_DBG, "parse_smtp_command:EXPN\n");
		cur_token += 4; 
		return smtp_cmd_expn_parse(/*ptcp,*/ psmtp, psmtp->cli_data, psmtp->cli_data_len, &cur_token);
	}
	[Hh][Ee][Ll][Pp]
	{
		DEBUG_SMTP(SMTP_DBG, "parse_smtp_command:HELP\n");
		psmtp->last_cli_event_type = SMTP_CMD_HELP;
		cur_token += length;
		goto succ;
	}
	[Nn][Oo][Oo][Pp]
	{
		DEBUG_SMTP(SMTP_DBG, "parse_smtp_command:NOOP\n");
		cur_token += length;
		goto succ;
	}
	[Ss][Ee][Nn][Dd][ ][Ff][Rr][Oo][Mm][:]
	{ 
		DEBUG_SMTP(SMTP_DBG, "parse_smtp_command:SEND\n");
		cur_token += 10;
		return smtp_cmd_send_parse(/*ptcp,*/ psmtp, psmtp->cli_data, psmtp->cli_data_len, &cur_token);
	}
	[Ss][Oo][Mm][Ll][ ][Ff][Rr][Oo][Mm][:]
	{
		DEBUG_SMTP(SMTP_DBG, "parse_smtp_command:SOML\n");
		cur_token += 10;
		return smtp_cmd_soml_parse(/*ptcp,*/ psmtp, psmtp->cli_data, psmtp->cli_data_len, &cur_token);
	}
	[Ss][Aa][Mm][Ll][ ][Ff][Rr][Oo][Mm][:]
	{
		DEBUG_SMTP(SMTP_DBG, "parse_smtp_command:SAML\n");
		cur_token += 10;
		return smtp_cmd_soml_parse(/*ptcp,*/ psmtp, psmtp->cli_data, psmtp->cli_data_len, &cur_token);
	}
	[Ee][Tt][Rr][Nn]
	{
		DEBUG_SMTP(SMTP_DBG, "parse_smtp_command:ETRN\n");
		psmtp->last_cli_event_type = SMTP_CMD_ETRN;
		cur_token += length;
		goto succ;
	}
	[Ss][Tt][Aa][Rr][Tt][Tt][Ll][Ss]
	{
		DEBUG_SMTP(SMTP_DBG, "parse_smtp_command: STARTTLS\n");
		psmtp->last_cli_event_type = SMTP_CMD_STARTTLS;
		cur_token += length;
		goto succ;
	}
	"\r\n"
	{
		DEBUG_SMTP(SMTP_DBG, "parse_smtp_command: blank line\n");
		//psmtp->curr_event_type = SMTP_EVENT_UNCERTAIN;
		//r = reply_to_client(ptcp, "500 Command unrecognized\r\n");
		//if (r != SMTP_NO_ERROR) {
		//	res = r;
		//	goto err;
		//}
		res = SMTP_ERROR_PARSE;
		goto err;
	}
	any 
	{
		DEBUG_SMTP(SMTP_DBG, "parse_smtp_command: unrecognized\n");
		//psmtp->curr_event_type = SMTP_EVENT_UNCERTAIN;
		//r = reply_to_client(ptcp, "500 Command unrecognized\r\n");
		//if (r != SMTP_NO_ERROR) {
		//	res = r;
		//	goto err;
		//}
		res = SMTP_ERROR_PARSE;
		goto err;
	}
	*/
	/* *INDENT-ON* */

      succ:

	DEBUG_SMTP (SMTP_DBG, "length = %d\n", length);
	DEBUG_SMTP (SMTP_DBG, "cli_data[%d] = %s\n", psmtp->cli_data_len,
		    psmtp->cli_data);

	//wyong, 20231003
	//psmtp->cli_data += cur_token;
	//psmtp->cli_data_len -= cur_token;
	//r = write_to_server(ptcp, psmtp);

	sync_client_data (psmtp, cur_token);
	if (r < 0) {
		//      res = SMTP_ERROR_TCPAPI_WRITE;
		goto err;
	}

	return SMTP_NO_ERROR;

      err:
	reset_client_buf (psmtp, &cur_token);
	return res;

}
