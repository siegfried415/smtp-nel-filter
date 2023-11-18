/*
 * auth.re
 * $Id: auth.re,v 1.16 2005/11/29 06:29:26 xiay Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mem_pool.h"

#include "smtp.h"
#include "auth.h"
#include "command.h"
#include "mime.h"

/*
SMTP Authentication is a scheme which was introduced in 1999 by J. Meyers of Netscape 
Communications and finally released as RFC 2554("SMTP Service Extension for 
Authentication"). It is partly based on the SMTP Service Extensions as defined in 
RFC 1869. Most modern SMTP implementations support SMTP Authentication, whereas Qmail
1.03 does not (without a patch). On the other hand, a lot of Mail User Agents (MUAs) 
- which include a SMTP Client - make SMTP Authentication available (e.g. Outlook, 
Eudora, Netscape, Mozilla, The Bat! ....).

SMTP Authentication is advertised by the SMTP Authentication server, requires a client 
to authenticate, while finally both parties have to mutually accept and support the 
chosen authentication procedure. Originally invented as a Host-to-Host protocol, with
SMTP Authentication, a User has to identify itself and after successful authentication, 
reception/transmission of his/her emails is granted.

RFC 2554 does not explicitly state, what advantages/benefits a user has being SMTP 
authenticated, except that optionally a "security layer"; for subsequent protocol 
interactions may be chosen. However, in common sense, an authenticated user is allowed 
for email transmission not only to the target system (the SMTP server) but rather 
anywhere. In Qmail terminology, this is equivalent to a 'relayclient'.

SMTP Authentication takes some ideas of the Simple Authentication and Security Layer
(SASL) and does not fit well into the SMTP scheme, as will be outlined in this document.

In order to understand SMTP Authentication, one has to work through several RFC, 
which seem to be unrelated in the first place.  On the other hand, the most recent SMTP 
RFC 2821(by John Klensin) does not even mention any SMTP Extension, though written by 
the same author - and - requiring the 'EHLO' command introducing a SMTP transaction. 
It really would be time, to have more coherent SMTP RFC; see also the comments of Dan
Bernstein about the "Klensin RFC".

* RFC 1869 defines a protocol extension (ESMTP) for the SMTP dialog, in order to 
indicate extended capabilities by the SMTP Server and/or to transmit additional
SMTP protocol information by the SMTP client. SMTP servers, supporting ESMTP, 
have to use the keyword 'EHLO' in the SMTP greeting. 
The SMTP client may only use those extensions the server offers.  By construction, 
the server may send the offered extensions as ESMTP verb anywhere in the SMTP dialog 
or as part of the 'MAIL FROM: ' or 'RCPT TO: ' command. 
A typical use is 'MAIL FROM: <foobar@example.com> SIZE=1512'. In this sample, 'SIZE' 
is the ESMTP keyword, '1512' is the ESMTP value and the whole term 'SIZE=1512' is the 
ESMTP parameter (RFC 1870 "SMTP Service Extension for Message Size Declaration").
RFC 1869 employes two different schemes to promote the ESMTP value: 
 - As ESMTP verb, it uses "SIZE xxxxx", 
 - whereas in the 'MAIL FROM: <foobar@example.com> SIZE=1512' command, the ESMTP 
   keyword and it's value are joined by a "=" equal sign.
Every ESMTP keyword has to be registered at the IANA.

* In this scope, RFC 2554 describes SMTP Authentication with the particular ESMTP 
keyword 'AUTH'. 
In the text passages and samples of RFC 2554, the ESMTP Auth values 'CRAM-MD5', 
'DIGEST-MD5', and 'PLAIN' are mentioned (which correspond to particular authentication
methods or mechanisms) but no reference to any of those is provided.

* The attempt, to find the meaning of the above mentioned ESMTP AUTH mechanisms in 
RFC 2222 "Simple Authentication and Security Layer (SASL)" fails. 
In this RFC (also published by John Myers), only the overall SASL mechanism is 
outlined and how to register a new "SASL mechanism name". However, the SASL 
mechanisms 'KERBEROS_V4', 'GSSAPI', and 'SKEY' are defined.

* In order to succeed, one has to dig out RFC 1731 "IMAP4 Authentication Mechanisms"
and RFC 2195 "IMAP/POP Authorize Extension for Simple Challenge/Response".
Here, some practical samples for authentication are given based upon the POP3 and 
IMAP4 protocol. Those RFC are originated as well by John Myers. RFC 2060 "INTERNET 
MESSAGE ACCESS PROTOCOL - VERSION 4rev1" (John Myers, sic) tells about the IMAP4 
'LOGIN' command. Too bad; this has nothing in common with the ESMTP 'AUTH LOGIN' 
method.

* The way the actual ESMTP Auth values are en-/decoded, corresponds to the BASE64
scheme, which was first described in RFC 1113"Privacy Enhancement for Internet 
Electronic Mail: Part I -- Message Encipherment and Authentication Procedures";
though not explicitly declared as BASE64 here (but later called it that). 
RFC 2045 "Multipurpose Internet Mail Extensions (MIME) Part One: Format of Internet
Message Bodies" gives an identical outline of the BASE64 "alphabet"in section 6.8.

*If in addition the Challenge/Response authentication mechanism is used, one has to 
become familiar with the so-called HMAC procedure from RFC 2104 "HMAC: 
Keyed-Hashing for Message Authentication" and in addition according to RFC 1321 with 
"The MD5 Message-Digest Algorithm" as an en-/decryption scheme.

* Until recently, there was no common understanding, how to propagate the SMTP 
Authentication information in the email's header. With RFC 3848 however, there exist 
at least a minimal scheme, including a particular keyword ESMTPA in the last 
included "Received:" header line in case of an authenticated SMTP session.
</UL>

It seems to be clear by know, that SMTP Authentication depends upon a patchwork of 
mechanisms/methods/procedures scattered over a wide range of RFC. Now, we have to go on 
and discuss the SMTP Authentication framework and will realize, that things are even
more complicated.
*/


extern int max_smtp_ack_len;

ObjPool_t smtp_cmd_auth_pool;
extern struct nel_eng *eng;
extern int ack_id;

int max_fail_auth_cnt;
int var_disable_auth_cmd;
int auth_id;

#ifdef USE_NEL
void
smtp_cmd_auth_init (struct nel_eng *eng)
{
	create_mem_pool (&smtp_cmd_auth_pool,
			 sizeof (struct smtp_cmd_auth), SMTP_MEM_STACK_DEPTH);

	//todo, move this to nel, wyong, 20231022 
	//nel_func_name_call(eng, (char *)&auth_id, "nel_id_of", "auth_req");
}
#else
void
smtp_cmd_auth_init ()
{
	create_mem_pool (&smtp_cmd_auth_pool,
			 sizeof (struct smtp_cmd_auth), SMTP_MEM_STACK_DEPTH);
}
#endif


#if 0
/* smtpd_sasl_auth_reset - clean up after AUTH command */
void
smtpd_sasl_auth_reset (SMTPD_STATE * state)
{
	smtpd_sasl_logout (state);
}


#ifdef USE_TLS
/* tls_reset - undo STARTTLS */
static void
tls_reset (SMTPD_STATE * state)
{
	int failure = 0;

	/*
	 * Don't waste time when we lost contact.
	 */
	if (state->tls_context) {
		if (vstream_feof (state->client)
		    || vstream_ferror (state->client))
			failure = 1;
		vstream_fflush (state->client);	/* NOT: smtp_flush() */
		tls_server_stop (smtpd_tls_ctx, state->client,
				 var_smtpd_starttls_tmout, failure,
				 &(state->tls_info));
		state->tls_context = 0;
	}
}
#endif

#endif



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
smtp_ack_auth_parse (struct smtp_info *psmtp)
{
	struct smtp_ack *ack;
	int r, res;
	int code;
	size_t cur_token;

	char *buf = psmtp->svr_data;
	int len = psmtp->svr_data_len;

	char *p1, *p2, *p3;

	DEBUG_SMTP (SMTP_DBG, "smtp_ack_auth_parse... \n");

	p1 = buf;
	p2 = buf + len;


	/* *INDENT-OFF* */
	/*!re2c
	"334"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_auth_parse: 334\n");
		code = 334;
		cur_token = 3;
		//goto ack_new;
		goto crlf;
	}

	"235"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_auth_parse: 235\n");
		code = 235;
		cur_token = 3;
		//goto ack_new;
		goto crlf;
	}

	[45] digit{2,2}
	{
		code = atoi(p1-3);
		cur_token = 3;
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_auth_parse: %d\n", code);

		psmtp->last_cli_event_type = SMTP_EVENT_UNCERTAIN;
		//goto ack_new;
		goto crlf;
	}

	any 
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_auth_parse: any\n");
		DEBUG_SMTP(SMTP_DBG, "server response unrecognizable data %s\n", psmtp->svr_data);
		res = SMTP_ERROR_PARSE;
		goto err;
	}
	*/

	/* *INDENT-ON* */
      crlf:
	DEBUG_SMTP (SMTP_DBG, "p1 : %s\n", p1);
	r = smtp_str_crlf_parse (buf, len, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = SMTP_ERROR_PARSE;
		goto free;
	}

      ack_new:
	ack = smtp_ack_new (len, code);
	if (!ack) {
		DEBUG_SMTP (SMTP_DBG, "\n");
		res = SMTP_ERROR_MEMORY;
		goto err;
	}

	DEBUG_SMTP (SMTP_DBG, "ack->code = %d\n", ack->code);

#ifdef USE_NEL
	if ((r = nel_env_analysis (eng, &(psmtp->env), ack_id,
				   (struct smtp_simple_event *) ack)) < 0) {
		DEBUG_SMTP (SMTP_DBG,
			    "smtp_ack_auth_parse: nel_env_analysis error\n");
		res = SMTP_ERROR_ENGINE;
		goto err;
	}
#endif
	/* Fixme: how to deal with the client? */
	if (psmtp->permit & SMTP_PERMIT_DENY) {
		//smtp_close_connection(ptcp, psmtp);
		res = SMTP_ERROR_POLICY;
		goto err;
	}
	else if (psmtp->permit & SMTP_PERMIT_DROP) {
		res = SMTP_ERROR_POLICY;
		goto err;
	}

	DEBUG_SMTP (SMTP_DBG, "len = %d,  cur_token = %d\n", len, cur_token);

	//wyong, 20231003
	//psmtp->svr_data += cur_token;
	//psmtp->svr_data_len -= cur_token;
	//r = write_to_client(ptcp, psmtp);

	//bugfix, wyong, 20231016 
	res = sync_server_data (psmtp, cur_token);
	if (res < 0) {
		goto err;
	}

	return SMTP_NO_ERROR;

      free:
	DEBUG_SMTP (SMTP_DBG, "smtp_ack_auth_parse: Free\n");
	smtp_ack_free (ack);
      err:
	DEBUG_SMTP (SMTP_DBG, "smtp_ack_auth_parse: Error\n");
	reset_server_buf (psmtp, &cur_token);
	return res;

}




void
smtp_cmd_auth_free (struct smtp_cmd_auth *auth)
{
	DEBUG_SMTP (SMTP_MEM, "smtp_cmd_auth_free\n");
	free_mem (&smtp_cmd_auth_pool, auth);
	DEBUG_SMTP (SMTP_MEM, "smtp_cmd_auth_free: pointer=%p, elm=%p\n",
		    &smtp_cmd_auth_pool, auth);
}

struct smtp_cmd_auth *
smtp_cmd_auth_new (int len, int mech)
{
	struct smtp_cmd_auth *auth;
	auth = (struct smtp_cmd_auth *) alloc_mem (&smtp_cmd_auth_pool);
	if (auth == NULL) {
		return NULL;
	}
	DEBUG_SMTP (SMTP_MEM, "smtp_cmd_auth_new: pointer=%p, elm=%p\n",
		    &smtp_cmd_auth_pool, auth);

	//auth->event_type = SMTP_CMD_AUTH;
	//auth->nel_id = auth_id;

#ifdef USE_NEL
	//auth->count = 0;
	NEL_REF_INIT (auth);
#endif

	auth->len = len;
	auth->mech = mech;

	return auth;
}


int
smtp_cmd_auth_parse (struct smtp_info *psmtp, char *message, size_t length,
		     size_t * index)
{
	struct smtp_cmd_auth *auth = NULL;
	char *mech;
	size_t cur_token = *index;
	int r;
	int res;
	char *p1, *p2, *p3;
	int mechanism;

	DEBUG_SMTP (SMTP_DBG, "smtp_cmd_auth_parse\n");

#if 0

	if (var_helo_required && state->helo_name == 0) {
		state->error_mask |= MAIL_ERROR_POLICY;
		smtpd_chat_reply (state, "503 Error: send HELO/EHLO first");
		return (-1);
	}
	if (SMTPD_STAND_ALONE (state) || !var_smtpd_sasl_enable) {
		state->error_mask |= MAIL_ERROR_PROTOCOL;
		smtpd_chat_reply (state,
				  "503 Error: authentication not enabled");
		return (-1);
	}
#ifdef USE_TLS
	if (state->tls_auth_only && !state->tls_context) {
		state->error_mask |= MAIL_ERROR_PROTOCOL;
		smtpd_chat_reply (state,
				  "538 Encryption required for requested authentication mechanism");
		return (-1);
	}
#endif
	if (state->sasl_username) {
		state->error_mask |= MAIL_ERROR_PROTOCOL;
		smtpd_chat_reply (state, "503 Error: already authenticated");
		return (-1);
	}
	if (argc < 2 || argc > 3) {
		state->error_mask |= MAIL_ERROR_PROTOCOL;
		smtpd_chat_reply (state, "501 Syntax: AUTH mechanism");
		return (-1);
	}

#endif

	r = smtp_wsp_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		//if (res == SMTP_ERROR_PARSE || res == SMTP_ERROR_CONTINUE) {
		//      r = reply_to_client(ptcp, "501 AUTH Syntax error.\r\n");
		//      if (r != SMTP_NO_ERROR) {
		//              res = r;
		//              goto err;
		//      }
		//      res = SMTP_ERROR_PARSE;
		//}
		goto err;
	}

	p1 = message + cur_token;
	p2 = message + length;

	/* *INDENT-OFF* */
	/*!re2c
	"CRAM-MD5"
	{
		//While for AUTH PLAIN and LOGIN clear user names and password
		//are transmitted, things go a little more secure with the CRAM-MD5
		//authentication mechanism. As already mentioned in it's name, CRAM-MD5
		//combines a Challenge/Response mechanism to exchange information
		//and a Message Digest 5 algorithm to encrypt important information.

		//I use an example based on a posting of Markus Stumpf (SpaceNet)
		//to the Qmail mailing list. A typical ESMTP AUTH CRAM-MD5 dialog 
		//starts like this:

		//S: 220 popmail.space.net ESMTP
		//C: ehlo client.example.com
		//S: 250-popmail.space.net
		//S: 250-PIPELINING
		//S: 250-8BITMIME
		//S: 250-SIZE 0
		//S: 250 AUTH CRAM-MD5
		//C: auth cram-md5
		//S: 334 PDI0NjA5LjEwNDc5MTQwNDZAcG9wbWFpbC5TcGFjZS5OZXQ+

		//Unlike AUTH LOGIN, the server's response is now a one-time BASE64 
		//encoded 'challenge'. The challenge 
		//'PDI0NjA5LjEwNDc5MTQwNDZAcG9wbWFpbC5TcGFjZS5OZXQ+' translates to 
		//'<24609.1047914046@popmail.Space.Net>'.  The leading and trailing 
		//brackets ('<', '>') are mandatory, as well the portion of the 
		//challenge which provides the hostname after the '@'. 
		//'24609.1047914046' is a random string, typically build from the 
		//'pid' and the current time stamp to make that challenge unique.

		//While the user name is transmitted in clear text (but of course
		//BASE64 encoded), the servers's challenge is used by the client
		//to generate a 'digest' from the challenge and the password (which
		//is commonly called 'secret' or 'shared secret' in this context)
		//employing the following MD5 hashing algorithm:

		//digest = MD5(('secret' XOR opad), MD5(('secret' XOR ipad),
		//	challenge))

		//If both the ESMTP server and the client 'share' the same challenge
		//and secret, the user may now be authenticated successfully by
		//means of the transmitted and BASE 64 encoded 'user name' and 'digest'.

		DEBUG_SMTP(SMTP_DBG, "smtp_cmd_auth_parse: CRAM-MD5\n");
		mechanism = SMTP_AUTH_CRAM_MD5;
		goto auth_new;
	}

	[Ll][Oo][Gg][Ii][Nn]
	{
		//The most common 'AUTH LOGIN' mechanism looks like this:
		//S: 220 esmtp.example.com ESMTP
		//C: ehlo client.example.com
		//S: 250-esmtp.example.com
		//S: 250-PIPELINING
		//S: 250-8BITMIME
		//S: 250-SIZE 255555555
		//S: 250 AUTH LOGIN PLAIN CRAM-MD5
		//C: auth login
		//S: 334 VXNlcm5hbWU6
		//C: avlsdkfj
		//S: 334 UGFzc3dvcmQ6
		//C: lkajsdfvlj
		//S: 535 authentication failed (#5.7.1)

		//From all the ESMTP Authentication mechanisms the offered, 
		//the client selects 'auth login'. The ESMTP server issues
		//then a '334 VXNlcm5hbWU6' where 'VXNlcm5hbWU6' is a BASE64 
		//encoded string 'Username:'. The client provides the BASE64 
		//encoded user name and the sever responses with the request 
		//for the 'Password:' ('334 UGFzc3dvcmQ6').
		//In the sample above, random input is given and the server 
		//finally rejects the authentication request.

		DEBUG_SMTP(SMTP_DBG, "smtp_cmd_auth_parse: LOGIN\n");
		mechanism = SMTP_AUTH_LOGIN;
		goto auth_new;
	}

	"PLAIN"
	{
		//According to IANA, the PLAIN Authentication is defined in 
		//rfc2245 "Anonymous SASL Mechanism",  the correct form of 
		//the AUTH PLAIN value is authid\0userid\0passwd' where '\0' 
		//is the null byte.
		
		//Some ESMTP AUTH PLAIN implementations don't follow that 
		//procedure completely. We see that in the trace using Netscape's 
		//4.8 MUA connecting to a modified Qmail 1.03 to do PLAIN 
		//authentication:

		//C: ehlo client.example.com<BR>
		//S: 220-esmtp.example.com<BR>
		//C: AUTH PLAIN dGVzdAB0ZXN0AHRlc3RwYXNz<BR>
		//S: 235 ok, go ahead (#2.0.0)<BR>
		//C: RCPT TO:&lt;....&gt;</TT></P>

		//In this sample, the user name was 'test' and the password 
		//'testpass'.Here, the Netscape client immediately 
		//blasts the authentication information to the server 
		//(including the artificial authorization identity 'test')
		//without waiting for the server to announce his SMTP Auth 
		//capabilites.

		DEBUG_SMTP(SMTP_DBG, "smtp_cmd_auth_parse: PLAIN\n");
		mechanism = SMTP_AUTH_PLAIN;
		goto auth_new;
	}

	any
	{
		DEBUG_SMTP(SMTP_DBG, "\n");
		//r = smtp_cfws_parse(message, length, &cur_token);
		//if ((r != SMTP_NO_ERROR) && (r != SMTP_ERROR_PARSE)) {
		//	res = r;
		//	goto err;
		//}

		//xiayu 2005.11.15 Fixme: let anything just pass through
		DEBUG_SMTP(SMTP_DBG, "smtp_cmd_auth_parse: AUTH UNCERTAIN\n");
		mechanism = SMTP_AUTH_UNCERTAIN;
		goto auth_new;
		
	}
	*/	

	/* *INDENT-ON* */

      auth_new:

#if 1				//xiayu 2005.11.24 Fixme: we only support AUTH LOGIN now.
	if (mechanism != SMTP_AUTH_LOGIN) {
		//r = reply_to_client(ptcp, "502 support AUTH LOGIN only.\r\n");
		//if (r != SMTP_NO_ERROR) {
		//      res = r;
		//      goto err;
		//}
		res = SMTP_ERROR_PARSE;	/* should be SMTP_ERROR_POLICY ? */
		goto err;
	}
#endif

	r = smtp_str_crlf_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		//if (res == SMTP_ERROR_PARSE || res == SMTP_ERROR_CONTINUE) {
		//      r = reply_to_client(ptcp, "501 AUTH Syntax: no CRLF.\r\n");
		//      if (r != SMTP_NO_ERROR) {
		//              res = r;
		//              goto err;
		//      }
		//      res = SMTP_ERROR_PARSE;
		//}
		goto err;
	}

	auth = smtp_cmd_auth_new (length, mechanism);
	if (auth == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto err;
	}

#ifdef USE_NEL
	DEBUG_SMTP (SMTP_DBG, "\n");
	/* put NEL Engine checking here */
	if ((r = nel_env_analysis (eng, &(psmtp->env), auth_id,
				   (struct smtp_simple_event *) auth)) < 0) {
		DEBUG_SMTP (SMTP_DBG,
			    "smtp_cmd_auth_parse: nel_env_analysis error\n");
		res = SMTP_ERROR_POLICY;
		goto err;
	}
#endif

	if (psmtp->permit & SMTP_PERMIT_DENY) {
		//smtp_close_connection(ptcp, psmtp);
		res = SMTP_ERROR_POLICY;
		goto err;

	}
	else if (psmtp->permit & SMTP_PERMIT_DROP) {
		//r = reply_to_client(ptcp, "550 AUTH cannot be implemented.\r\n");
		//if (r != SMTP_NO_ERROR) {
		//      res = r;
		//      goto err;
		//}
		res = SMTP_ERROR_POLICY;
		goto err;
	}


	DEBUG_SMTP (SMTP_DBG, "\n");
	psmtp->last_cli_event_type = SMTP_CMD_AUTH;

	//psmtp->cli_data += cur_token;
	//psmtp->cli_data_len -= cur_token;
	res = sync_client_data (psmtp, cur_token);
	if (res < 0) {
		goto err;
	}

	*index = cur_token;

	return SMTP_NO_ERROR;

      free:
	DEBUG_SMTP (SMTP_DBG, "smtp_cmd_auth_parse: Free\n");
	if (auth) {
		free_mem (&smtp_cmd_auth_pool, (void *) auth);
		DEBUG_SMTP (SMTP_MEM,
			    "smtp_cmd_auth_free: pointer=%p, elm=%p\n",
			    &smtp_cmd_auth_pool, auth);
	}

      err:
	DEBUG_SMTP (SMTP_DBG, "smtp_cmd_auth_parse: Error\n");
	reset_client_buf (psmtp, index);
	return res;
}


#if 0				//xiayu will use the following funcs in the future
#ifdef USE_SASL
#include <sasl/sasl.h>
#include <sasl/saslutil.h>
#endif
#include "mailsasl.h"


int
auth_map_errors (int err)
{
	switch (err) {
	case 235:
		return MAILSMTP_NO_ERROR;	/* AUTH successfull */
	case 334:
		return MAILSMTP_NO_ERROR;	/* AUTH in progress */
	case 432:
		return MAILSMTP_ERROR_AUTH_TRANSITION_NEEDED;
	case 454:
		return MAILSMTP_ERROR_AUTH_TEMPORARY_FAILTURE;
	case 504:
		return MAILSMTP_ERROR_AUTH_NOT_SUPPORTED;
	case 530:
		return MAILSMTP_ERROR_AUTH_REQUIRED;
	case 534:
		return MAILSMTP_ERROR_AUTH_TOO_WEAK;
	case 538:
		return MAILSMTP_ERROR_AUTH_ENCRYPTION_REQUIRED;
	default:
		/* opportunistic approach ;) */
		return MAILSMTP_NO_ERROR;
	}
}

static int
mailsmtp_auth_login (mailsmtp * session, const char *user, const char *pass)
{
	int err;
	char command[SMTP_STRING_SIZE];
	char *user64, *pass64;

	user64 = NULL;
	pass64 = NULL;

	user64 = encode_base64 (user, strlen (user));
	if (user64 == NULL) {
		err = MAILSMTP_ERROR_MEMORY;
		goto err_free;
	}

	pass64 = encode_base64 (pass, strlen (pass));
	if (pass64 == NULL) {
		err = MAILSMTP_ERROR_MEMORY;
		goto err_free;
	}

	snprintf (command, SMTP_STRING_SIZE, "%s\r\n", user64);
	err = send_command (session, command);
	if (err == -1) {
		err = MAILSMTP_ERROR_STREAM;
		goto err_free;
	}
	err = read_response (session);
	err = auth_map_errors (err);
	if (err != MAILSMTP_NO_ERROR)
		goto err_free;

	snprintf (command, SMTP_STRING_SIZE, "%s\r\n", pass64);
	err = send_command (session, command);
	if (err == -1) {
		err = MAILSMTP_ERROR_STREAM;
		goto err_free;
	}
	err = read_response (session);
	err = auth_map_errors (err);

      err_free:
	free (user64);
	free (pass64);

	return err;
}

static int
mailsmtp_auth_plain (mailsmtp * session, const char *user, const char *pass)
{
	int err, len;
	char command[SMTP_STRING_SIZE];
	char *plain, *plain64;

	len = strlen (user) + strlen (pass) + 3;
	plain = (char *) malloc (len);
	if (plain == NULL) {
		err = MAILSMTP_ERROR_MEMORY;
		goto err;
	}

	snprintf (plain, len, "%c%s%c%s", '\0', user, '\0', pass);
	plain64 = encode_base64 (plain, len - 1);

	snprintf (command, SMTP_STRING_SIZE, "%s\r\n", plain64);
	err = send_command (session, command);
	if (err == -1) {
		err = MAILSMTP_ERROR_STREAM;
		goto err_free;
	}

	err = read_response (session);
	err = auth_map_errors (err);

      err_free:
	free (plain64);
	free (plain);

      err:
	return err;
}

static char *
convert_hex (unsigned char *in, int len)
{
	static char hex[] = "0123456789abcdef";
	char *out;
	int i;

	out = (char *) malloc (len * 2 + 1);
	if (out == NULL)
		return NULL;

	for (i = 0; i < len; i++) {
		out[i * 2] = hex[in[i] >> 4];
		out[i * 2 + 1] = hex[in[i] & 15];
	}

	out[i * 2] = 0;

	return out;
}

static char *
hash_md5 (const char *sec_key, const char *data, int len)
{
	char key[65], digest[24];
	char *hash_hex;

	int sec_len, i;

	sec_len = strlen (sec_key);

	if (sec_len < 64) {
		memcpy (key, sec_key, sec_len);
		for (i = sec_len; i < 64; i++) {
			key[i] = 0;
		}
	}
	else {
		memcpy (key, sec_key, 64);
	}

	hmac_md5 (data, len, key, 64, digest);
	hash_hex = convert_hex ((unsigned char *) digest, 16);

	return hash_hex;
}

static int
mailsmtp_auth_cram_md5 (mailsmtp * session,
			const char *user, const char *pass)
{
	int err;
	char command[SMTP_STRING_SIZE];
	char *response, *auth_hex, *auth;

	response =
		decode_base64 (session->response, strlen (session->response));
	if (response == NULL)
		return MAILSMTP_ERROR_MEMORY;

	auth_hex = hash_md5 (pass, response, strlen (response));
	if (auth_hex == NULL) {
		err = MAILSMTP_ERROR_MEMORY;
		goto err_free_response;
	}

	snprintf (command, SMTP_STRING_SIZE, "%s %s", user, auth_hex);

	auth = encode_base64 (command, strlen (command));
	if (auth == NULL) {
		err = MAILSMTP_ERROR_MEMORY;
		goto err_free_auth_hex;
	}

	snprintf (command, SMTP_STRING_SIZE, "%s\r\n", auth);
	err = send_command (session, command);
	if (err == -1) {
		err = MAILSMTP_ERROR_STREAM;
		goto err_free;
	}

	err = read_response (session);
	err = auth_map_errors (err);

      err_free:
	free (auth);
      err_free_auth_hex:
	free (auth_hex);
      err_free_response:
	free (response);
	return err;
}

#ifdef USE_SASL
static int
sasl_getsimple (void *context, int id, const char **result, unsigned *len)
{
	mailsmtp *session;

	session = context;

	switch (id) {
	case SASL_CB_USER:
		if (result != NULL)
			*result = session->smtp_sasl.sasl_login;
		if (len != NULL)
			*len = strlen (session->smtp_sasl.sasl_login);
		return SASL_OK;

	case SASL_CB_AUTHNAME:
		if (result != NULL)
			*result = session->smtp_sasl.sasl_auth_name;
		if (len != NULL)
			*len = strlen (session->smtp_sasl.sasl_auth_name);
		return SASL_OK;
	}

	return SASL_FAIL;
}

static int
sasl_getsecret (sasl_conn_t * conn, void *context, int id,
		sasl_secret_t ** psecret)
{
	mailsmtp *session;

	session = context;

	switch (id) {
	case SASL_CB_PASS:
		if (psecret != NULL)
			*psecret = session->smtp_sasl.sasl_secret;
		return SASL_OK;
	}

	return SASL_FAIL;
}

static int
sasl_getrealm (void *context, int id,
	       const char **availrealms, const char **result)
{
	mailsmtp *session;

	session = context;

	switch (id) {
	case SASL_CB_GETREALM:
		if (result != NULL)
			*result = session->smtp_sasl.sasl_realm;
		return SASL_OK;
	}

	return SASL_FAIL;
}
#endif

int
mailesmtp_auth_sasl (mailsmtp * session, const char *auth_type,
		     const char *server_fqdn,
		     const char *local_ip_port,
		     const char *remote_ip_port,
		     const char *login, const char *auth_name,
		     const char *password, const char *realm)
{
#ifdef USE_SASL
	int r;
	char command[SMTP_STRING_SIZE];
	sasl_callback_t sasl_callback[5];
	const char *sasl_out;
	unsigned sasl_out_len;
	const char *mechusing;
	sasl_secret_t *secret;
	int res;
	size_t len;
	char *encoded;
	unsigned int encoded_len;
	unsigned int max_encoded;

	sasl_callback[0].id = SASL_CB_GETREALM;
	sasl_callback[0].proc = sasl_getrealm;
	sasl_callback[0].context = session;
	sasl_callback[1].id = SASL_CB_USER;
	sasl_callback[1].proc = sasl_getsimple;
	sasl_callback[1].context = session;
	sasl_callback[2].id = SASL_CB_AUTHNAME;
	sasl_callback[2].proc = sasl_getsimple;
	sasl_callback[2].context = session;
	sasl_callback[3].id = SASL_CB_PASS;
	sasl_callback[3].proc = sasl_getsecret;
	sasl_callback[3].context = session;
	sasl_callback[4].id = SASL_CB_LIST_END;
	sasl_callback[4].proc = NULL;
	sasl_callback[4].context = NULL;

	len = strlen (password);
	secret = malloc (sizeof (*secret) + len);
	if (secret == NULL) {
		res = MAILSMTP_ERROR_MEMORY;
		goto err;
	}
	secret->len = len;
	memcpy (secret->data, password, len + 1);

	session->smtp_sasl.sasl_server_fqdn = server_fqdn;
	session->smtp_sasl.sasl_login = login;
	session->smtp_sasl.sasl_auth_name = auth_name;
	session->smtp_sasl.sasl_password = password;
	session->smtp_sasl.sasl_realm = realm;
	session->smtp_sasl.sasl_secret = secret;

	/* init SASL */
	mailsasl_ref ();

	r = sasl_client_new ("smtp", server_fqdn,
			     local_ip_port, remote_ip_port, sasl_callback, 0,
			     (sasl_conn_t **) & session->smtp_sasl.sasl_conn);
	if (r != SASL_OK) {
		res = MAILSMTP_ERROR_AUTH_LOGIN;
		goto free_secret;
	}

	r = sasl_client_start (session->smtp_sasl.sasl_conn,
			       auth_type, NULL, &sasl_out, &sasl_out_len,
			       &mechusing);
	if ((r != SASL_CONTINUE) && (r != SASL_OK)) {
		res = MAILSMTP_ERROR_AUTH_LOGIN;
		goto free_secret;
	}

	if (sasl_out_len != 0) {
		max_encoded = ((sasl_out_len + 2) / 3) * 4;
		encoded = malloc (max_encoded + 1);
		if (encoded == NULL) {
			res = MAILSMTP_ERROR_MEMORY;
			goto free_secret;
		}

		r = sasl_encode64 (sasl_out, sasl_out_len,
				   encoded, max_encoded + 1, &encoded_len);
		if (r != SASL_OK) {
			free (encoded);
			res = MAILSMTP_ERROR_MEMORY;
			goto free_secret;
		}

		snprintf (command, SMTP_STRING_SIZE, "AUTH %s %s\r\n",
			  auth_type, encoded);

		free (encoded);
	}
	else {
		snprintf (command, SMTP_STRING_SIZE, "AUTH %s\r\n",
			  auth_type);
	}

	r = send_command (session, command);
	if (r == -1) {
		res = MAILSMTP_ERROR_STREAM;
		goto free_secret;
	}

	while (1) {
		r = read_response (session);
		switch (r) {
		case 220:
		case 235:
			free (session->smtp_sasl.sasl_secret);
			return MAILSMTP_NO_ERROR;

		case 334:
			{
				size_t response_len;
				char *decoded;
				unsigned int decoded_len;
				unsigned int max_decoded;

				response_len = strlen (session->response);
				max_decoded = response_len * 3 / 4;
				decoded = malloc (max_decoded + 1);
				if (decoded == NULL) {
					res = MAILSMTP_ERROR_MEMORY;
					goto free_secret;
				}

				r = sasl_decode64 (session->response,
						   response_len, decoded,
						   max_decoded + 1,
						   &decoded_len);

				if (r != SASL_OK) {
					free (decoded);
					res = MAILSMTP_ERROR_MEMORY;
					goto free_secret;
				}

				r = sasl_client_step (session->smtp_sasl.
						      sasl_conn, decoded,
						      decoded_len, NULL,
						      &sasl_out,
						      &sasl_out_len);

				free (decoded);

				if ((r != SASL_CONTINUE) && (r != SASL_OK)) {
					res = MAILSMTP_ERROR_AUTH_LOGIN;
					goto free_secret;
				}

				max_encoded = ((sasl_out_len + 2) / 3) * 4;
				encoded = malloc (max_encoded + 1);
				if (encoded == NULL) {
					res = MAILSMTP_ERROR_MEMORY;
					goto free_secret;
				}

				r = sasl_encode64 (sasl_out, sasl_out_len,
						   encoded, max_encoded + 1,
						   &encoded_len);
				if (r != SASL_OK) {
					free (encoded);
					res = MAILSMTP_ERROR_MEMORY;
					goto free_secret;
				}

				snprintf (command, SMTP_STRING_SIZE, "%s\r\n",
					  encoded);
				r = send_command (session, command);

				free (encoded);

				if (r == -1) {
					res = MAILSMTP_ERROR_STREAM;
					goto free_secret;
				}
			}
			break;

		default:
			res = auth_map_errors (r);
			goto free_secret;
		}
	}

	return MAILSMTP_NO_ERROR;

      free_secret:
	free (session->smtp_sasl.sasl_secret);
      err:
	return res;
#else
	return MAILSMTP_ERROR_NOT_IMPLEMENTED;
#endif
}


int
mailsmtp_auth_type (mailsmtp * session,
		    const char *user, const char *pass, int type)
{
	int err;
	char command[SMTP_STRING_SIZE];

	if (session->auth == MAILSMTP_AUTH_NOT_CHECKED)
		return MAILSMTP_ERROR_BAD_SEQUENCE_OF_COMMAND;

	if (!(session->auth & type))
		return MAILSMTP_ERROR_AUTH_NOT_SUPPORTED;

	switch (type) {
	case MAILSMTP_AUTH_LOGIN:
		{
			snprintf (command, SMTP_STRING_SIZE,
				  "AUTH LOGIN\r\n");
			err = send_command (session, command);
			if (err == -1)
				return MAILSMTP_ERROR_STREAM;

			err = read_response (session);
			err = auth_map_errors (err);
			if (err != MAILSMTP_NO_ERROR)
				return err;

			return mailsmtp_auth_login (session, user, pass);
		}
	case MAILSMTP_AUTH_PLAIN:
		return mailesmtp_auth_sasl (session, "PLAIN",
					    NULL, NULL, NULL, user, user,
					    pass, NULL);

	case MAILSMTP_AUTH_CRAM_MD5:
		return mailesmtp_auth_sasl (session, "CRAM-MD5",
					    NULL, NULL, NULL, user, user,
					    pass, NULL);

	case MAILSMTP_AUTH_DIGEST_MD5:
		return mailesmtp_auth_sasl (session, "DIGEST-MD5",
					    NULL, NULL, NULL, user, user,
					    pass, NULL);

	default:
		return MAILSMTP_ERROR_NOT_IMPLEMENTED;
	}

}


int
mailsmtp_auth (mailsmtp * session, const char *user, const char *pass)
{
	if (session->auth == MAILSMTP_AUTH_NOT_CHECKED)
		return MAILSMTP_ERROR_BAD_SEQUENCE_OF_COMMAND;

	if (session->auth & MAILSMTP_AUTH_DIGEST_MD5) {
		return mailsmtp_auth_type (session, user, pass,
					   MAILSMTP_AUTH_DIGEST_MD5);
	}
	else if (session->auth & MAILSMTP_AUTH_CRAM_MD5) {
		return mailsmtp_auth_type (session, user, pass,
					   MAILSMTP_AUTH_CRAM_MD5);
	}
	else if (session->auth & MAILSMTP_AUTH_PLAIN) {
		return mailsmtp_auth_type (session, user, pass,
					   MAILSMTP_AUTH_PLAIN);
	}
	else if (session->auth & MAILSMTP_AUTH_LOGIN) {
		return mailsmtp_auth_type (session, user, pass,
					   MAILSMTP_AUTH_LOGIN);
	}
	else {
		return MAILSMTP_ERROR_AUTH_NOT_SUPPORTED;
	}
}

#endif
