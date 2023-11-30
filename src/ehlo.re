
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mem_pool.h"

#include "smtp.h"
#include "auth.h"
#include "mime.h"
#include "address.h"
#include "ehlo.h"
#include "command.h"
#include "helo.h" 
#include "rcpt-to.h"
#include "mail-from.h" 

#ifdef USE_NEL
#include "engine.h"

extern struct nel_eng *eng;
int ehlo_id;
int ack_ehlo_id;
#endif

extern int max_smtp_ack_multiline_count;
extern int max_smtp_ack_len;


int var_disable_ehlo_cmd = 0;
ObjPool_t smtp_cmd_ehlo_pool;
//ObjPool_t smtp_ack_ehlo_pool;

void
smtp_cmd_ehlo_init (
#ifdef USE_NEL
			   struct nel_eng *eng
#endif
	)
{
	create_mem_pool (&smtp_cmd_ehlo_pool,
			 sizeof (struct smtp_cmd_ehlo), SMTP_MEM_STACK_DEPTH);
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
smtp_ack_ehlo_param_auth_parse (struct smtp_info *psmtp, char *message,
				int length, size_t *index)
{
	char *p1, *p2, *p3;
	int is_finished = 0;

	p1 = message + *index;
	p2 = message + length;

	while (!is_finished) {

		/* *INDENT-OFF* */
		/*!re2c
		"CRAM-MD5"
		{
			DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_auth_parse: CRAM-MD5\n");
			psmtp->auth_allow[SMTP_AUTH_CRAM_MD5] = 1;
			continue;
		}

		"DIGEST-MD5"
		{
			DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_auth_parse: DIGEST-MD5\n");
			psmtp->auth_allow[SMTP_AUTH_DIGEST_MD5] = 1;
			continue;
		}

		"FOOBAR"
		{
			DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_auth_parse: FOOBAR\n");
			psmtp->auth_allow[SMTP_AUTH_FOOBAR] = 1;
			continue;
		}

		"GSSAPI"
		{
			DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_auth_parse: GSSAPI\n");
			psmtp->auth_allow[SMTP_AUTH_GSSAPI] = 1;
			continue;
		}

		"LOGIN"
		{
			DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_auth_parse: LOGIN\n");
			psmtp->auth_allow[SMTP_AUTH_LOGIN] = 1;
			continue;
		}

		"NTLM"
		{
			DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_auth_parse: NTLM\n");
			psmtp->auth_allow[SMTP_AUTH_NTLM] = 1;
			continue;
		}

		"PLAIN"
		{
			DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_auth_parse: PLAIN\n");
			psmtp->auth_allow[SMTP_AUTH_PLAIN] = 1;
			continue;
		}	
	
		space +
		{
			DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_auth_parse: spaces\n");
			DEBUG_SMTP(SMTP_DBG, "p1: %s\n", p1);
			//do nothing
			continue;
		}

		"="
		{
			DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_auth_parse: equal sign \'=\'\n");
			DEBUG_SMTP(SMTP_DBG, "p1: %s\n", p1);
			//do nothing
			continue;
		}
		
		"\r\n"
		{
			DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_auth_parse: CRLF\n");
			is_finished = 1;
			continue;
		}

		print +
		{
			DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_auth_parse: any other auth keyword in ehlo ack\n");
			//Fixme: we temperory skip unrecognized auth keywords
			DEBUG_SMTP(SMTP_DBG, "p1: %s\n", p1);
			continue;
		}
		*/
		/* *INDENT-ON* */
	}

	if (!is_finished)
		return SMTP_ERROR_PARSE;

	*index = p1 - message;

	return SMTP_NO_ERROR;

}


int
smtp_ack_ehlo_param_parse (struct smtp_info *psmtp, char *message, int length,
			   size_t *index)
{
	int r, offset;
	char *p1, *p2, *p3;
	size_t cur_token = *index;

	DEBUG_SMTP (SMTP_DBG, "smtp_ack_ehlo_param_parse... \n");

	p1 = message + cur_token;
	p2 = message + length;

	/* *INDENT-OFF* */
	/*!re2c
	[8][Bb][Ii][Tt][Mm][Ii][Mm][Ee] "\r\n"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_parse: 8BITMIME\n");
		cur_token += strlen("8BITMIME\r\n");

		psmtp->cmd_allow[SMTP_ALLOW_8BITMIME] = 1;
		goto succ;
	}

	[7][Bb][Ii][Tt]
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_parse: 7BIT\n");
		cur_token += strlen("7BIT");
		psmtp->cmd_allow[SMTP_ALLOW_7BIT] = 1;
		r = smtp_str_crlf_parse(message, length, &cur_token);
		if (r != SMTP_NO_ERROR)
			return SMTP_ERROR_PARSE;
		goto succ;
	}

	[Aa][Tt][Rr][Nn]
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_parse: ATRN\n");
		cur_token += strlen("ATRN");
		psmtp->cmd_allow[SMTP_ALLOW_ATRN] = 1;

		r = smtp_str_crlf_parse(message, length, &cur_token);
		if (r != SMTP_NO_ERROR)
			return SMTP_ERROR_PARSE;
		goto succ;
	}

	[Aa][Uu][Tt][Hh]
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_parse: AUTH\n");
		cur_token += strlen("AUTH");

		r = smtp_ack_ehlo_param_auth_parse(psmtp, message, length, &cur_token);
		if (r != SMTP_NO_ERROR)
			return r;
		goto succ;
	}

	[Bb][Ii][Nn][Aa][Rr][Yy][Mm][Ii][Mm][Ee] "\r\n"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_parse: BINARYMIME\n");
		cur_token += strlen("BINARYMIME\r\n");

		psmtp->cmd_allow[SMTP_ALLOW_BINARYMIME] = 1;
		goto succ;
	}

	[Cc][Hh][Ee][Cc][Kk][Pp][Oo][Ii][Nn][Tt] "\r\n"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_parse: CHECKPOINT\n");
		cur_token += strlen("CHECKPOINT\r\n");

		psmtp->cmd_allow[SMTP_ALLOW_CHECKPOINT] = 1;
		goto succ;
	}

	[Cc][Hh][Uu][Nn][Kk][Ii][Nn][Gg] "\r\n"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_parse: CHUNKING\n");
		cur_token += strlen("CHUNKING\r\n");

		psmtp->cmd_allow[SMTP_ALLOW_CHUNKING] = 1;
		goto succ;
	}

	[Dd][Ee][Ll][Ii][Vv][Ee][Rr][Bb][Yy]
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_parse: DELIVERBY\n");
		cur_token += strlen("DELIVERBY");

		//deliverby-param = min-by-time *( ',' extension-token )
		//min-by-time     = [1*9DIGIT]
		//extension-token = 1*<any CHAR excluding SP, COMMA  and all
		//	   control  characters (US ASCII 0-31 inclusive)>
		//SP  = <the space character (ASCII decimal code 32)>
		//COMMA    = <the comma character (ASCII decimal code 44)>

		psmtp->cmd_allow[SMTP_ALLOW_DELIVERBY] = 1;

		r = smtp_str_crlf_parse(message, length, &cur_token);
		if (r != SMTP_NO_ERROR)
			return SMTP_ERROR_PARSE;

		goto succ;
	}

	[Dd][Ss][Nn] "\r\n"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_parse: DSN\n");
		cur_token += strlen("DNS\r\n");
		psmtp->cmd_allow[SMTP_ALLOW_DSN] = 1;
		goto succ;
	}

	[Ee][Nn][Hh][Aa][Nn][Cc][Ee][Dd][Ss][Tt][Aa][Tt][Uu][Ss][Cc][Oo][Dd][Ee][Ss] "\r\n"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_parse: ENHANCEDSTATUSCODES\n");
		cur_token += strlen("ENHANCEDSTATUSCODES\r\n");
		psmtp->cmd_allow[SMTP_ALLOW_ENHANCEDSTATUSCODES] = 1;
		goto succ;
	}

	[Ee][Tt][Rr][Nn] "\r\n"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_parse: ETRN\n");
		cur_token += strlen("ETRN\r\n");
		psmtp->cmd_allow[SMTP_ALLOW_ETRN] = 1;
		goto succ;
	}

	[Ee][Xx][Pp][Nn] "\r\n"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_parse: EXPN\n");
		cur_token += strlen("EXPN\r\n");
		psmtp->cmd_allow[SMTP_ALLOW_EXPN] = 1;
		goto succ;
	}

	[Hh][Ee][Ll][Pp] "\r\n"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_parse: HELP\n");
		cur_token += strlen("HELP\r\n");
		psmtp->cmd_allow[SMTP_ALLOW_HELP] = 1;
		goto succ;
	}

	[Oo][Nn][Ee][Xx]
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_parse: ONEX\n");
		cur_token += strlen("ONEX");
		psmtp->cmd_allow[SMTP_ALLOW_ONEX] = 1;
		r = smtp_str_crlf_parse(message, length, &cur_token);
		if (r != SMTP_NO_ERROR)
			return SMTP_ERROR_PARSE;
		goto succ;
	}


	[Nn][Oo][-][Ss][Oo][Ll][Ii][Cc][Ii][Tt][Ii][Nn][Gg]
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_parse: NO-SOLICITING\n");
		//No-Soliciting-Service = "NO-SOLICITING" [ SP Solicitation-keywords ]
		//Solicitation-keywords = word
           	//			0*("," word) 	; length of this string is limited
		//					; to <= 1000 characters
		//word = ALPHA 0*(wordchar)
		//wordchar=("." / "-" / "_" / ":" / ALPHA / DIGIT)
		//wordchar=("." /
		cur_token += strlen("NO-SOLICITING");

		psmtp->cmd_allow[SMTP_ALLOW_NO_SOLICITING] = 1;

		r = smtp_str_crlf_parse(message, length, &cur_token);
		if (r != SMTP_NO_ERROR)
			return SMTP_ERROR_PARSE;

		goto succ;
	}

	[Mm][Tt][Rr][Kk] "\r\n"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_parse: MTRK\n");
		cur_token += strlen("MTRK\r\n");
		psmtp->cmd_allow[SMTP_ALLOW_MTRK] = 1;
		goto succ;
	}

	[Pp][Ii][Pp][Ee][Ll][Ii][Nn][Ii][Nn][Gg] "\r\n"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_parse: PIPELINING\n");
		cur_token += strlen("PIPELINING\r\n");
		psmtp->cmd_allow[SMTP_ALLOW_PIPELINING] = 1;
		goto succ;
	}

	[Rr][Ss][Ee][Tt]
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_parse: RSET\n");
		cur_token += strlen("RSET");
		psmtp->cmd_allow[SMTP_ALLOW_RSET] = 1;
		r = smtp_str_crlf_parse(message, length, &cur_token);
		if (r != SMTP_NO_ERROR)
			return SMTP_ERROR_PARSE;
		goto succ;
	}

	[Ss][Aa][Mm][Ll]
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_parse: SAML\n");
		cur_token += strlen("SAML");
		psmtp->cmd_allow[SMTP_ALLOW_SAML] = 1;

		r = smtp_str_crlf_parse(message, length, &cur_token);
		if (r != SMTP_NO_ERROR)
			return SMTP_ERROR_PARSE;
		goto succ;
	}

	[Ss][Ee][Nn][Dd]
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_parse: SEND\n");
		cur_token += strlen("SEND");
		psmtp->cmd_allow[SMTP_ALLOW_SEND] = 1;

		r = smtp_str_crlf_parse(message, length, &cur_token);
		if (r != SMTP_NO_ERROR)
			return SMTP_ERROR_PARSE;
		goto succ;
	}

	[Ss][Ii][Zz][Ee]
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_parse: SIZE\n");
		cur_token += strlen("SIZE");
		// size-param ::= [1*DIGIT], max length of mail that can be received. 
		psmtp->cmd_allow[SMTP_ALLOW_SIZE] = 1;

		r = smtp_str_crlf_parse(message, length, &cur_token);
		if (r != SMTP_NO_ERROR)
			return SMTP_ERROR_PARSE;

		goto succ;
	}

	[Ss][Oo][Mm][Ll]
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_parse: SOML\n");
		cur_token += strlen("SOML");
		psmtp->cmd_allow[SMTP_ALLOW_SOML] = 1;

		r = smtp_str_crlf_parse(message, length, &cur_token);
		if (r != SMTP_NO_ERROR)
			return SMTP_ERROR_PARSE;
		goto succ;
	}

	[Ss][Tt][Aa][Rr][Tt][Tt][Ll][Ss] "\r\n"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_parse: STARTTLS\n");
		cur_token += strlen("STARTTLS\r\n");
		psmtp->cmd_allow[SMTP_ALLOW_STARTTLS] = 1;
		goto succ;
	}

	[Rr][Ee][Ll][Aa][Yy] "\r\n"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_parse: RELAY\n");
		cur_token += strlen("RELAY\r\n");
		psmtp->cmd_allow[SMTP_ALLOW_RELAY] = 1;
		goto succ;
	}

	[Tt][Ii][Mm][Ee]
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_parse: TIME\n");
		cur_token += strlen("TIME");
		psmtp->cmd_allow[SMTP_ALLOW_TIME] = 1;

		r = smtp_str_crlf_parse(message, length, &cur_token);
		if (r != SMTP_NO_ERROR)
			return SMTP_ERROR_PARSE;
		goto succ;
	}

	[Tt][Ll][Ss]
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_parse: TLS\n");
		cur_token += strlen("TLS");
		psmtp->cmd_allow[SMTP_ALLOW_TLS] = 1;

		r = smtp_str_crlf_parse(message, length, &cur_token);
		if (r != SMTP_NO_ERROR)
			return SMTP_ERROR_PARSE;
		goto succ;
	}

	[Tt][Uu][Rr][Nn] "\r\n"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_parse: TRUN\n");
		cur_token += strlen("TRUN\r\n");
		psmtp->cmd_allow[SMTP_ALLOW_TURN] = 1;
		goto succ;
	}

	[Vv][Ee][Rr][Bb] "\r\n"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_parse: VERB\n");
		cur_token += strlen("VERB\r\n");
		psmtp->cmd_allow[SMTP_ALLOW_VERB] = 1;
		goto succ;
	}

	[Vv][Rr][Ff][Yy]
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_parse: VRFY\n");
		cur_token += strlen("VRFY");
		psmtp->cmd_allow[SMTP_ALLOW_VRFY] = 1;

		r = smtp_str_crlf_parse(message, length, &cur_token);
		if (r != SMTP_NO_ERROR)
			return SMTP_ERROR_PARSE;
		goto succ;
	}

	[Xx][-][Ee][Xx][Pp][Ss]
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_parse: X-EXPS\n");
		cur_token += strlen("X-EXPS");
		psmtp->cmd_allow[SMTP_ALLOW_X_EXPS] = 1;

		//X-EXPS=LOGIN
		r = smtp_str_crlf_parse(message, length, &cur_token);
		if (r != SMTP_NO_ERROR)
			return SMTP_ERROR_PARSE;
		goto succ;
	}

	[Xx][-][Ll][Ii][Nn][Kk][2][Ss][Tt][Aa][Tt][Ee]
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_parse: X-LINK2STATE\n");
		cur_token += strlen("X-LINK2STATE");
		psmtp->cmd_allow[SMTP_ALLOW_X_LINK2STATE] = 1;

		r = smtp_str_crlf_parse(message, length, &cur_token);
		if (r != SMTP_NO_ERROR)
			return SMTP_ERROR_PARSE;
		goto succ;
	}

	[Xx][-][Rr][Cc][Pp][Tt][Ll][Ii][Mm][Ii][Tt]
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_parse: X-RCPTLIMIT\n");
		cur_token += strlen("X-RCPTLIMIT");
		psmtp->cmd_allow[SMTP_ALLOW_X_RCPTLIMIT] = 1;

		r = smtp_str_crlf_parse(message, length, &cur_token);
		if (r != SMTP_NO_ERROR)
			return SMTP_ERROR_PARSE;
		goto succ;
	}

	[Xx][-][Tt][Uu][Rr][Nn][Mm][Ee]
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_parse: X-TURNME\n");
		cur_token += strlen("X-TURNME");
		psmtp->cmd_allow[SMTP_ALLOW_X_TURNME] = 1;

		r = smtp_str_crlf_parse(message, length, &cur_token);
		if (r != SMTP_NO_ERROR)
			return SMTP_ERROR_PARSE;
		goto succ;
	}

	[Xx][Aa][Dd][Rr] "\r\n"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_parse: XADR\n");
		cur_token += strlen("XADR\r\n");
		psmtp->cmd_allow[SMTP_ALLOW_XADR] = 1;
		goto succ;
	}

	[Xx][Aa][Uu][Dd]
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_parse: XAUD\n");
		cur_token += strlen("XAUD");
		psmtp->cmd_allow[SMTP_ALLOW_XAUD] = 1;

		r = smtp_str_crlf_parse(message, length, &cur_token);
		if (r != SMTP_NO_ERROR)
			return SMTP_ERROR_PARSE;
		goto succ;
	}

	[Xx][Dd][Ss][Nn]
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_parse: XDSN\n");
		cur_token += strlen("XDSN");
		psmtp->cmd_allow[SMTP_ALLOW_XDSN] = 1;

		r = smtp_str_crlf_parse(message, length, &cur_token);
		if (r != SMTP_NO_ERROR)
			return SMTP_ERROR_PARSE;
		goto succ;
	}

	[Xx][Ee][Xx][Cc][Hh] "50"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_parse: XEXCH50\n");
		cur_token += strlen("XEXCH50");
		psmtp->cmd_allow[SMTP_ALLOW_XEXCH50] = 1;

		r = smtp_str_crlf_parse(message, length, &cur_token);
		if (r != SMTP_NO_ERROR)
			return SMTP_ERROR_PARSE;
		goto succ;
	}

	[Xx][Gg][Ee][Nn]
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_parse: XGEN\n");
		cur_token += strlen("XGEN");
		psmtp->cmd_allow[SMTP_ALLOW_XGEN] = 1;

		r = smtp_str_crlf_parse(message, length, &cur_token);
		if (r != SMTP_NO_ERROR)
			return SMTP_ERROR_PARSE;
		goto succ;
	}

	[Xx][Ll][Oo][Oo][Pp]
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_parse: XLOOP\n");
		cur_token += strlen("XLOOP");
		psmtp->cmd_allow[SMTP_ALLOW_XLOOP] = 1;

		r = smtp_str_crlf_parse(message, length, &cur_token);
		if (r != SMTP_NO_ERROR)
			return SMTP_ERROR_PARSE;
		goto succ;
	}

	[Xx][Oo][Nn][Ee]
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_parse: XONE\n");
		cur_token += strlen("XONE");
		psmtp->cmd_allow[SMTP_ALLOW_XONE] = 1;

		r = smtp_str_crlf_parse(message, length, &cur_token);
		if (r != SMTP_NO_ERROR)
			return SMTP_ERROR_PARSE;
		goto succ;
	}

	[Xx][Qq][Uu][Ee]
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_parse: XQUE\n");
		cur_token += strlen("XQUE");
		psmtp->cmd_allow[SMTP_ALLOW_XQUE] = 1;

		r = smtp_str_crlf_parse(message, length, &cur_token);
		if (r != SMTP_NO_ERROR)
			return SMTP_ERROR_PARSE;
		goto succ;
	}

	[Xx][Rr][Ee][Mm][Oo][Tt][Ee][Qq][Uu][Ee][Uu][Ee]
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_parse: XREMOTEQUEUE\n");
		cur_token += strlen("XREMOTEQUEUE");
		psmtp->cmd_allow[SMTP_ALLOW_XREMOTEQUEUE] = 1;

		r = smtp_str_crlf_parse(message, length, &cur_token);
		if (r != SMTP_NO_ERROR)
			return SMTP_ERROR_PARSE;
		goto succ;
	}

	[Xx][Ss][Tt][Aa]
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_parse: XSTA\n");
		cur_token += strlen("XSTA");
		psmtp->cmd_allow[SMTP_ALLOW_XSTA] = 1;

		r = smtp_str_crlf_parse(message, length, &cur_token);
		if (r != SMTP_NO_ERROR)
			return SMTP_ERROR_PARSE;
		goto succ;
	}

	[Xx][Tt][Rr][Nn]
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_parse: XTRN\n");
		cur_token += strlen("XTRN");
		psmtp->cmd_allow[SMTP_ALLOW_XTRN] = 1;

		r = smtp_str_crlf_parse(message, length, &cur_token);
		if (r != SMTP_NO_ERROR)
			return SMTP_ERROR_PARSE;
		goto succ;
	}

	[Xx][Uu][Ss][Rr]
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_parse: XUSR\n");
		cur_token += strlen("XUSR");
		psmtp->cmd_allow[SMTP_ALLOW_XUSR] = 1;

		r = smtp_str_crlf_parse(message, length, &cur_token);
		if (r != SMTP_NO_ERROR)
			return SMTP_ERROR_PARSE;
		goto succ;
	}

	[Xx][Vv][Rr][Bb]
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_parse: XVRB\n");
		cur_token += strlen("XVRB");
		psmtp->cmd_allow[SMTP_ALLOW_XVRB] = 1;

		r = smtp_str_crlf_parse(message, length, &cur_token);
		if (r != SMTP_NO_ERROR)
			return SMTP_ERROR_PARSE;
		goto succ;
	}

	any
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_param_parse: any\n");
		DEBUG_SMTP(SMTP_DBG, "p1 = %s\n", p1);

		/* Fixme: skip unknown param keywords */
		r = smtp_str_crlf_parse(message, length, &cur_token);
		if (r != SMTP_NO_ERROR)
			return SMTP_ERROR_PARSE;
		goto succ;
	}
	*/
	/* *INDENT-ON* */

      succ:

	*index = cur_token;
	return SMTP_NO_ERROR;
}


int
smtp_ack_ehlo_parse (struct smtp_info *psmtp)
{
	struct smtp_ack *ack = NULL;
	int multiline_count;
	int code; 
	size_t cur_token;
	int r, res;

	char *buf = psmtp->svr_data;
	int len = psmtp->svr_data_len;

	char *p1, *p2, *p3;

	DEBUG_SMTP (SMTP_DBG, "smtp_ack_ehlo_parse... \n");

	p1 = buf;
	p2 = buf + len;

	code = 0;
	multiline_count = 0;
	cur_token = 0;


	while (1) {

		p1 = buf + cur_token;
		DEBUG_SMTP (SMTP_DBG, "p1: %s\n", p1);

		/* *INDENT-OFF* */
		/*!re2c
		"250-"
		{
			DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_parse: 250-\n");
			multiline_count ++;

			cur_token += strlen("250-");
			
			if (0 == code) {
				code = 250;
				r = smtp_str_crlf_parse(buf, len, &cur_token);
				if (r != SMTP_NO_ERROR) {
					res = SMTP_ERROR_PARSE;
					goto free;
				}

			} else {

				if (250 != code) {
					DEBUG_SMTP(SMTP_DBG, "Multi-line ack of EHLO does not have consistent codes: %s\n", psmtp->svr_data);
					res = SMTP_ERROR_PARSE;
					goto free;
				}

				r = smtp_ack_ehlo_param_parse(psmtp, buf, len, &cur_token);	
				if (r != SMTP_NO_ERROR) {
					res = r;
					goto free;
				}
			}

			DEBUG_SMTP(SMTP_DBG, "p1: %s\n", p1);
			continue;
		}

		"250 "
		{
			DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_parse: 250 space\n");
			multiline_count ++;
			cur_token += strlen("250 ");

			if (0 == code) {
				code = 250;

			} else if (250 != code ) {
				DEBUG_SMTP(SMTP_DBG, "Multi-line ack of EHLO does not has consistent codes: %s\n", psmtp->svr_data);
				res = SMTP_ERROR_PARSE;
				goto free;
			}

			if (multiline_count > max_smtp_ack_multiline_count) {
				DEBUG_SMTP(SMTP_DBG, "smtp multi-line count of ack are too many\n");
				res = SMTP_ERROR_PROTO;
				goto free;
			}

			r = smtp_ack_ehlo_param_parse(psmtp, buf, len, &cur_token);
			if (r != SMTP_NO_ERROR) {
				res = r;
				goto free;
			}

			if (psmtp->mail_state != SMTP_MAIL_STATE_HELO
				/*&& psmtp->mail_state != SMTP_MAIL_STATE_RSET*/)
			{
				smtp_rcpt_reset(psmtp);
				smtp_mail_reset(psmtp);
			}
			psmtp->mail_state = SMTP_MAIL_STATE_HELO;

			goto ack_new;
		}

		"421"
		{
			DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_parse: 421\n");
			code = 421;
			cur_token = 3;
			smtp_helo_reset(psmtp);
			goto crlf;
		}

		"500"
		{
			DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_parse: 500\n");
			code = 500;
			cur_token = 3;
			smtp_helo_reset(psmtp);
			goto crlf;
		}

		"501"
		{
			DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_parse: 501\n");
			code = 501;
			cur_token = 3;
			smtp_helo_reset(psmtp);
			goto crlf;
		}

		"501"
		{
			DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_parse: 501\n");
			code = 501;
			cur_token = 3;
			smtp_helo_reset(psmtp);
			goto crlf;
		}

		"504"
		{
			DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_parse: 504\n");
			code = 504;
			cur_token = 3;
			smtp_helo_reset(psmtp);
			goto crlf;
		}

		"550"
		{
			DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_parse: 550\n");
			code = 550;
			cur_token = 3;
			smtp_helo_reset(psmtp);
			goto crlf;
		}
		

		digit{3,3}
		{
			code = atoi(p1-6);
			cur_token = 3;
			DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_parse: %d\n", code);
			if (code >= 300) {
				smtp_helo_reset(psmtp);
			}

			goto crlf;
		}

		any 
		{
			DEBUG_SMTP(SMTP_DBG, "smtp_ack_ehlo_parse: any\n");
			DEBUG_SMTP(SMTP_DBG, "p1: %s\n", p1);
			DEBUG_SMTP(SMTP_DBG, "server response unrecognizable data %s\n", psmtp->svr_data);
			res = SMTP_ERROR_PARSE;
			goto free;
		}
		*/
		/* *INDENT-ON* */
	}

      crlf:
	DEBUG_SMTP (SMTP_DBG, "p1 : %s\n", p1);
	r = smtp_str_crlf_parse (buf, len, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = SMTP_ERROR_PARSE;
		goto err;
	}

      ack_new:
	ack = smtp_ack_new (len, code);
	if (!ack) {
		res = SMTP_ERROR_MEMORY;
		goto free;
	}
	DEBUG_SMTP (SMTP_DBG, "ack->code = %d-\n", ack->code);

#ifdef USE_NEL
	if ((r = nel_env_analysis (eng, &(psmtp->env), ack_ehlo_id,
				   (struct smtp_simple_event *) ack)) < 0) {
		DEBUG_SMTP (SMTP_DBG,
			    "smtp_ack_ehlo_parse: nel_env_analysis error\n");
		res = SMTP_ERROR_ENGINE;
		goto free;
	}
#endif

	/* Fixme: how to deal with the client? */
	if (psmtp->permit & SMTP_PERMIT_DENY) {
		res = SMTP_ERROR_POLICY;
		goto err;
	}
	else if (psmtp->permit & SMTP_PERMIT_DROP) {
		res = SMTP_ERROR_POLICY;
		goto err;
	}

	res = sync_server_data (psmtp, cur_token);
	if (res < 0) {
		goto err;
	}

	return SMTP_NO_ERROR;

      free:
	DEBUG_SMTP (SMTP_DBG, "smtp_ack_ehlo_parse: Free\n");
	if (psmtp->domain) {
		smtp_domain_free (psmtp->domain);
		psmtp->domain = NULL;
	}
	if (ack)
		smtp_ack_free (ack);

      err:
	DEBUG_SMTP (SMTP_DBG, "smtp_ack_ehlo_parse: Error\n");
	reset_server_buf (psmtp, &cur_token);
	return res;
}

void
smtp_cmd_ehlo_free (struct smtp_cmd_ehlo *ehlo)
{
	if (ehlo->host_name) {
		//should use the proper free func
		//smtp_string_free(ehlo->host_name);
		smtp_atom_free (ehlo->host_name);
	}
	free_mem (&smtp_cmd_ehlo_pool, (void *) ehlo);
	DEBUG_SMTP (SMTP_MEM, "smtp_cmd_ehlo_free: pointer=%p, elm=%p\n",
		    &smtp_cmd_ehlo_pool, (void *) ehlo);
}

struct smtp_cmd_ehlo *
smtp_cmd_ehlo_new (int len, char *host_name)
{
	struct smtp_cmd_ehlo *ehlo;
	ehlo = (struct smtp_cmd_ehlo *) alloc_mem (&smtp_cmd_ehlo_pool);
	if (ehlo == NULL) {
		return NULL;
	}
	DEBUG_SMTP (SMTP_MEM, "smtp_cmd_ehlo_new: pointer=%p, elm=%p\n",
		    &smtp_cmd_ehlo_pool, (void *) ehlo);

#ifdef USE_NEL
	NEL_REF_INIT (ehlo);
#endif
	ehlo->len = len;

	if (host_name) {
		ehlo->host_name = host_name;
		ehlo->host_name_len = strlen (host_name);
	}
	else {
		ehlo->host_name = NULL;
		ehlo->host_name_len = 0;
	}

	return ehlo;
}


/*
 * called during mailesmtp_ehlo
 * checks EHLO answer for server extensions and sets flags
 * in session->esmtp
 * checks AUTH methods in session->response and sets flags
 * in session->auth
 */
int
smtp_cmd_ehlo_parse (struct smtp_info *psmtp, char *message, size_t length, size_t * index)
{
	struct smtp_cmd_ehlo *ehlo = NULL;
	char *host_name;
	size_t cur_token = *index;
	int r, res;

	DEBUG_SMTP (SMTP_DBG, "smtp_cmd_ehlo_parse\n");

	/* if nel configureable variable var_disable_ehlo_cmd 
	   is set to 1, don't allow the command pass through. */
	/*
	   //var_disable_ehlo_cmd = 1;
	   if (var_disable_ehlo_cmd == 1) {
	   DEBUG_SMTP(SMTP_DBG,"smtp_cmd_ehlo_parse:var_disable_ehlo_cmd\n");
	   r = reply_to_client(ptcp, "550 EHLO cannot be implemented.\r\n");
	   if (r != SMTP_NO_ERROR) {
	   res = r;
	   goto err;
	   }
	   res = SMTP_ERROR_POLICY;
	   goto err;
	   }
	 */

	DEBUG_SMTP (SMTP_DBG, "\n");
	r = smtp_wsp_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	DEBUG_SMTP (SMTP_DBG, "\n");
	r = smtp_wsp_atom_parse (message, length, &cur_token, &host_name);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	DEBUG_SMTP (SMTP_DBG, "\n");
	r = smtp_wsp_unstrict_crlf_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free;
	}

	DEBUG_SMTP (SMTP_DBG, "\n");
	psmtp->helo_name = smtp_string_new (host_name, strlen (host_name));
	if (psmtp->helo_name == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free;
	}

	ehlo = smtp_cmd_ehlo_new (length, host_name);
	if (ehlo == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free;
	}
	host_name = NULL;

#ifdef USE_NEL
	DEBUG_SMTP (SMTP_DBG, "eng=%p\n", eng);
	if ((r = nel_env_analysis (eng, &(psmtp->env), ehlo_id,
				   (struct smtp_simple_event *) ehlo)) < 0) {
		DEBUG_SMTP (SMTP_DBG,
			    "smtp_cmd_ehlo_parse: nel_env_analysis error\n");
		res = SMTP_ERROR_ENGINE;
		goto free;
	}
#endif

	if (psmtp->permit & SMTP_PERMIT_DENY) {
		res = SMTP_ERROR_POLICY;
		goto err;

	}
	else if (psmtp->permit & SMTP_PERMIT_DROP) {
		res = SMTP_ERROR_POLICY;
		goto err;
	}

	DEBUG_SMTP (SMTP_DBG, "\n");
	psmtp->last_cli_event_type = SMTP_CMD_EHLO;

	res = sync_client_data (psmtp, cur_token);
	if (res < 0) {
		goto err;
	}

	return SMTP_NO_ERROR;

      free:
	DEBUG_SMTP (SMTP_DBG, "smtp_cmd_helo_parse: Free\n");
	if (host_name) {
		//should use the proper free func
		//smtp_string_free(host_name);
		smtp_atom_free (host_name);
	}
	if (psmtp->helo_name) {
		smtp_string_free (psmtp->helo_name);
		psmtp->helo_name = NULL;
	}
	if (ehlo) {
		free_mem (&smtp_cmd_ehlo_pool, (void *) ehlo);
		DEBUG_SMTP (SMTP_MEM,
			    "smtp_cmd_ehlo_new: pointer=%p, elm=%p\n",
			    &smtp_cmd_ehlo_pool, (void *) ehlo);
	}

      err:
	DEBUG_SMTP (SMTP_DBG, "smtp_cmd_helo_parse: Error\n");
	reset_client_buf (psmtp, index);

	return res;

}
