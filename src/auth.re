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

void
smtp_cmd_auth_init (
#ifdef USE_NEL
	struct nel_eng *eng
#endif
)
{
	create_mem_pool (&smtp_cmd_auth_pool,
			 sizeof (struct smtp_cmd_auth), SMTP_MEM_STACK_DEPTH);
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

	DEBUG_SMTP (SMTP_DBG, "len = %d,  cur_token = %lu\n", len, cur_token);
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

#ifdef USE_NEL
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

	r = smtp_wsp_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
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
		//Fixme: let anything just pass through
		DEBUG_SMTP(SMTP_DBG, "smtp_cmd_auth_parse: AUTH UNCERTAIN\n");
		mechanism = SMTP_AUTH_UNCERTAIN;
		goto auth_new;
		
	}
	*/	

	/* *INDENT-ON* */

      auth_new:

	//Fixme: we only support AUTH LOGIN now.
	if (mechanism != SMTP_AUTH_LOGIN) {
		res = SMTP_ERROR_PARSE;	/* should be SMTP_ERROR_POLICY ? */
		goto err;
	}

	r = smtp_str_crlf_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	auth = smtp_cmd_auth_new (length, mechanism);
	if (auth == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto err;
	}

#ifdef USE_NEL
	DEBUG_SMTP (SMTP_DBG, "\n");
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
		res = SMTP_ERROR_POLICY;
		goto err;
	}


	DEBUG_SMTP (SMTP_DBG, "\n");
	psmtp->last_cli_event_type = SMTP_CMD_AUTH;

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

