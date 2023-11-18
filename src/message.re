/*
 * $Id: message.c,v 1.30 2005/12/20 08:30:43 xiay Exp $
  RFC 2045, RFC 2046, RFC 2047, RFC 2048, RFC 2049, RFC 2231, RFC 2387
  RFC 2424, RFC 2557, RFC 2183 Content-Disposition, RFC 1766  Language
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "mmapstring.h"

#include "smtp.h"
#include "message.h"
#include "mime.h"
#include "fields.h"
#include "body.h"
#include "encoding.h"
#include "command.h"


#ifdef USE_NEL
#include "analysis.h"

extern struct nel_eng *eng;
int text_id;
int eot_id, eob_id, eom_id;
extern int ack_id;
#endif

#define YYCURSOR  p1
#define YYCTYPE   char
#define YYLIMIT   p2
#define YYMARKER  p3
#define YYRESCAN  yy2
#define YYFILL(n)


/*!re2c
any     = [\000-\377];
print   = [\040-\176];
digit   = [0-9];
letter  = [a-zA-Z];
space   = [\040];
*/



int
smtp_ack_msg_parse (struct smtp_info *psmtp)
{
	struct smtp_ack *ack;
	int r, res;
	int code;
	size_t cur_token = 0;

	char *buf = psmtp->svr_data;
	int len = psmtp->svr_data_len;

	char *p1, *p2, *p3;

	DEBUG_SMTP (SMTP_DBG, "smtp_ack_msg_parse... \n");
	DEBUG_SMTP (SMTP_DBG, "smtp_ack_msg_parse,buf=%s \n", buf);

	p1 = buf;
	p2 = buf + len;

	cur_token = 0;

	/* *INDENT-OFF* */
	/*!re2c
	"250"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_msg_parse: 354\n");
		code = 354;
		cur_token = 3;
		
		//xiayu 2005.12.3 command state dfa
		smtp_rcpt_reset(psmtp);
		smtp_mail_reset(psmtp);
		psmtp->mail_state = SMTP_MAIL_STATE_HELO;
		psmtp->curr_parse_state = SMTP_STATE_PARSE_COMMAND;

		//goto ack_new;
		goto crlf;
	}

	"421"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_msg_parse: 421\n");
		code = 421;
		cur_token = 3;
		//psmtp->curr_parse_state = SMTP_STATE_PARSE_COMMAND;
		//psmtp->sender = NULL;
		//goto ack_new;
		goto crlf;
	}

	"451"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_msg_parse: 451\n");
		code = 451;
		cur_token = 3;
		//psmtp->curr_parse_state = SMTP_STATE_PARSE_COMMAND;
		//psmtp->sender = NULL;
		//goto ack_new;
		goto crlf;
	}

	"452"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_msg_parse: 452\n");
		code = 452;
		cur_token = 3;
		//psmtp->curr_parse_state = SMTP_STATE_PARSE_COMMAND;
		//psmtp->sender = NULL;
		//goto ack_new;
		goto crlf;
	}

	"500"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_msg_parse: 500\n");
		code = 500;
		cur_token = 3;
		//psmtp->curr_parse_state = SMTP_STATE_PARSE_COMMAND;
		//psmtp->sender = NULL;
		//goto ack_new;
		goto crlf;
	}

	"501"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_msg_parse: 501\n");
		code = 501;
		cur_token = 3;
		//psmtp->curr_parse_state = SMTP_STATE_PARSE_COMMAND;
		//psmtp->sender = NULL;
		//goto ack_new;
		goto crlf;
	}

	"503"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_msg_parse: 503\n");
		code = 503;
		cur_token = 3;
		//psmtp->curr_parse_state = SMTP_STATE_PARSE_COMMAND;
		//psmtp->sender = NULL;
		//goto ack_new;
		goto crlf;
	}

	"554"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_msg_parse: 554\n");
		code = 554;
		cur_token = 3;
		//psmtp->curr_parse_state = SMTP_STATE_PARSE_COMMAND;
		//psmtp->sender = NULL;
		//goto ack_new;
		goto crlf;
	}

	digit{3,3}
	{
		code = atoi(p1-6);
		cur_token = 3;
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_msg_parse: %d\n", code);

		//if (code >= 300) {
		//	psmtp->curr_parse_state = SMTP_STATE_PARSE_COMMAND;
		//	psmtp->sender = NULL;
		//}

		//goto ack_new;
		goto crlf;
	}

	any	
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_msg_parse: any\n");
		DEBUG_SMTP(SMTP_DBG, "server response unrecognizable data %s\n", psmtp->svr_data);
		res = SMTP_ERROR_PARSE;
		goto err;
	}
	*/

	/* *INDENT-ON* */

      crlf:
	r = smtp_str_crlf_parse (buf, len, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = SMTP_ERROR_PARSE;
		goto err;
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
			    "smtp_ack_msg_parse: nel_env_analysis error\n");
		res = SMTP_ERROR_ENGINE;
		goto free;
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
	psmtp->svr_data += cur_token;
	psmtp->svr_data_len -= cur_token;
	//r = write_to_client(ptcp, psmtp);
	//if (r < 0) {
	//      res = SMTP_ERROR_TCPAPI_WRITE;
	//      goto err;
	//}

	return SMTP_NO_ERROR;

      free:
	smtp_ack_free (ack);

      err:
	reset_server_buf (psmtp, &cur_token);
	return res;

}



/*
     boundary := 0*69<bchars> bcharsnospace
*/

/*
     bchars := bcharsnospace / " "
*/

/*
     bcharsnospace := DIGIT / ALPHA / "'" / "(" / ")" /
                      "+" / "_" / "," / "-" / "." /
                      "/" / ":" / "=" / "?"
*/

/*
     body-part := <"message" as defined in RFC 822, with all
                   header fields optional, not starting with the
                   specified dash-boundary, and with the
                   delimiter not occurring anywhere in the
                   body part.  Note that the semantics of a
                   part differ from the semantics of a message,
                   as described in the text.>
*/

/*
     close-delimiter := delimiter "--"
*/

/*
     dash-boundary := "--" boundary
                      ; boundary taken from the value of
                      ; boundary parameter of the
                      ; Content-Type field.
*/

/*
     delimiter := CRLF dash-boundary
*/

/*
     discard-text := *(*text CRLF)
                     ; May be ignored or discarded.
*/

/*
     encapsulation := delimiter transport-padding
                      CRLF body-part
*/

/*
     epilogue := discard-text
*/

/*
     multipart-body := [preamble CRLF]
                       dash-boundary transport-padding CRLF
                       body-part *encapsulation
                       close-delimiter transport-padding
                       [CRLF epilogue]
*/

/*
     preamble := discard-text
*/

/*
     transport-padding := *LWSP-char
                          ; Composers MUST NOT generate
                          ; non-zero length transport
                          ; padding, but receivers MUST
                          ; be able to handle padding
                          ; added by message transports.
*/


/*
  ACCESS-TYPE
  EXPIRATION
  SIZE
  PERMISSION
*/

/*
  5.2.3.2.  The 'ftp' and 'tftp' Access-Types
  NAME
  SITE
  
      (3)   Before any data are retrieved, using FTP, the user will
          generally need to be asked to provide a login id and a
          password for the machine named by the site parameter.
          For security reasons, such an id and password are not
          specified as content-type parameters, but must be
          obtained from the user.

  optional :
  DIRECTORY
  MODE
*/

/*
5.2.3.3.  The 'anon-ftp' Access-Type
*/

/*
5.2.3.4.  The 'local-file' Access-Type
NAME
SITE
*/

/*
5.2.3.5.  The 'mail-server' Access-Type
SERVER
SUBJECT
*/

#if 0
enum
{
	PREAMBLE_STATE_A0,
	PREAMBLE_STATE_A,
	PREAMBLE_STATE_A1,
	PREAMBLE_STATE_B,
	PREAMBLE_STATE_C,
	PREAMBLE_STATE_D,
	PREAMBLE_STATE_E
};

static int
smtp_mime_preamble_parse (const char *message, size_t length,
			  size_t * index, int beol)
{
	int state;
	size_t cur_token;

	cur_token = *index;
	if (beol)
		state = PREAMBLE_STATE_A0;
	else
		state = PREAMBLE_STATE_A;

	while (state != PREAMBLE_STATE_E) {
		//xiayu 2005.11.16
		if (cur_token >= length)
			return SMTP_ERROR_CONTINUE;
		if (cur_token >= length)
			return SMTP_ERROR_PARSE;

		switch (state) {
		case PREAMBLE_STATE_A0:
			switch (message[cur_token]) {
			case '-':
				state = PREAMBLE_STATE_A1;
				break;
			case '\r':
				state = PREAMBLE_STATE_B;
				break;
			case '\n':
				state = PREAMBLE_STATE_C;
				break;
			default:
				state = PREAMBLE_STATE_A;
				break;
			}
			break;

		case PREAMBLE_STATE_A:
			switch (message[cur_token]) {
			case '\r':
				state = PREAMBLE_STATE_B;
				break;
			case '\n':
				state = PREAMBLE_STATE_C;
				break;
			default:
				state = PREAMBLE_STATE_A;
				break;
			}
			break;

		case PREAMBLE_STATE_A1:
			switch (message[cur_token]) {
			case '-':
				state = PREAMBLE_STATE_E;
				break;
			case '\r':
				state = PREAMBLE_STATE_B;
				break;
			case '\n':
				state = PREAMBLE_STATE_C;
				break;
			default:
				state = PREAMBLE_STATE_A;
				break;
			}
			break;

		case PREAMBLE_STATE_B:
			switch (message[cur_token]) {
			case '\r':
				state = PREAMBLE_STATE_B;
				break;
			case '\n':
				state = PREAMBLE_STATE_C;
				break;
			case '-':
				state = PREAMBLE_STATE_D;
				break;
			default:
				state = PREAMBLE_STATE_A0;
				break;
			}
			break;

		case PREAMBLE_STATE_C:
			switch (message[cur_token]) {
			case '-':
				state = PREAMBLE_STATE_D;
				break;
			case '\r':
				state = PREAMBLE_STATE_B;
				break;
			case '\n':
				state = PREAMBLE_STATE_C;
				break;
			default:
				state = PREAMBLE_STATE_A0;
				break;
			}
			break;

		case PREAMBLE_STATE_D:
			switch (message[cur_token]) {
			case '-':
				state = PREAMBLE_STATE_E;
				break;
			default:
				state = PREAMBLE_STATE_A;
				break;
			}
			break;
		}

		cur_token++;
	}

	*index = cur_token;

	return SMTP_NO_ERROR;
}

#endif


/*
gboolean smtp_crlf_parse(gchar * message, guint32 length, guint32 * index)
*/


#define NO_CASE         1
#define CASE_SENS       0

static int
smtp_mime_body_part_dash2_boundary_parse (const char *message, size_t length, size_t * index, char *boundary,	//struct trieobj *boundary_tree, wyong, 20231017 
					  const char **result,
					  size_t * result_size)
{

	size_t cur_token = *index;
	int keyword_type, ret, len;
	size_t size;
	size_t begin_text = cur_token;
	size_t end_text = length;
	size_t offset;

	//wyong, 20231018 
	int boundary_len = strlen (boundary);

	DEBUG_SMTP (SMTP_DBG, "cur_token = %d\n", cur_token);
	DEBUG_SMTP (SMTP_DBG, "message = %p\n", message);
	//DEBUG_SMTP(SMTP_DBG, "boundary_tree = %p\n", boundary_tree);

	//wyong, 20231017 
	//ret = search_keyword_suffix (message + cur_token, length - cur_token , 0, boundary_tree, 
	//                              &offset, &keyword_type);
	char *p = strstr (message + cur_token, boundary);

	DEBUG_SMTP (SMTP_DBG, "\n");
	//if( ret == 1 ) {
	if (p) {

		//wyong, 20231017 
		char *start = message + cur_token;
		offset = (int) (p - start);

		DEBUG_SMTP (SMTP_DBG,
			    "%s_%s[%d], search_keyword_suffix successed!, offset = %d\n",
			    __FILE__, __FUNCTION__, __LINE__, offset);

		cur_token += offset;
		end_text = cur_token;
		size = end_text - begin_text;

		//wyong, 20231017 
		//cur_token += boundary_tree->shortest;
		cur_token += boundary_len;

		if (size >= 1) {
			if (message[end_text - 1] == '\r') {
				end_text--;
				size--;
			}
			else if (size >= 1) {
				if (message[end_text - 1] == '\n') {
					end_text--;
					size--;
					if (size >= 1) {
						if (message[end_text - 1] ==
						    '\r') {
							end_text--;
							size--;
						}
					}
				}
			}
		}

		size = end_text - begin_text;
		*result = message + begin_text;
		*result_size = size;
		*index = cur_token;

		DEBUG_SMTP (SMTP_DBG,
			    "return SMTP_NO_ERROR, cur_token = %d\n",
			    cur_token);
		return SMTP_NO_ERROR;

	}
	else {

		//wyong, 20231017
		offset = length - cur_token;

		//if boundary across two packets, hold boundary_len -1 data at least.  
		//wyong, 20231018
		if (offset > (boundary_len - 1)) {
			offset -= boundary_len - 1;
		}

		//xiayu 2005.10.27
		//cur_token = length;

		DEBUG_SMTP (SMTP_DBG,
			    "cur_token = %d, offset = %d, length = %d\n",
			    cur_token, offset, length);
		cur_token += offset;
		DEBUG_SMTP (SMTP_DBG, "message+cur_token: %s\n",
			    message + cur_token);

		end_text = cur_token;

		size = end_text - begin_text;
		*result = message + begin_text;
		*result_size = size;
		*index = cur_token;
		DEBUG_SMTP (SMTP_DBG,
			    "%s_%s[%d], return SMTP_ERROR_CONTINUE, cur_token = %d\n",
			    __FILE__, __FUNCTION__, __LINE__, cur_token);
		return SMTP_ERROR_CONTINUE;
	}

	return -1;

}

static int
smtp_mime_body_part_dash2_boundary_transport_crlf_parse (const char *message, size_t length, size_t * index, char *boundary,	//struct trieobj * boundary_tree, wyong, 20231017 
							 const char **result,
							 size_t * result_size)
{
	size_t cur_token;
	int r;
	const char *data_str;
	size_t data_size;
	const char *begin_text;
	const char *end_text;

	cur_token = *index;

	begin_text = message + cur_token;
	end_text = message + cur_token;

	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	while (1) {
		DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
			    __LINE__);
		r = smtp_mime_body_part_dash2_boundary_parse (message, length,
							      &cur_token,
							      /*boundary_tree, */
							      boundary,
							      &data_str,
							      &data_size);
		DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
			    __LINE__);

		if (r == SMTP_NO_ERROR) {
			DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__,
				    __FUNCTION__, __LINE__);
			end_text = data_str + data_size;
		}
		else {
			DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__,
				    __FUNCTION__, __LINE__);
			return r;
		}

		/* parse transport-padding */
		while (1) {
			DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__,
				    __FUNCTION__, __LINE__);
			r = smtp_mime_lwsp_parse (message, length,
						  &cur_token);
			if (r == SMTP_NO_ERROR) {
				DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__,
					    __FUNCTION__, __LINE__);
				/* do nothing */
			}
			else if (r == SMTP_ERROR_PARSE) {
				DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__,
					    __FUNCTION__, __LINE__);
				break;
			}
			else {
				DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__,
					    __FUNCTION__, __LINE__);
				return r;
			}
		}

		DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
			    __LINE__);
		r = smtp_crlf_parse (message, length, &cur_token);
		if (r == SMTP_NO_ERROR) {
			DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__,
				    __FUNCTION__, __LINE__);
			break;
		}
		else if (r == SMTP_ERROR_PARSE) {
			DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__,
				    __FUNCTION__, __LINE__);
			/* do nothing */
		}
		else {
			DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__,
				    __FUNCTION__, __LINE__);
			return r;
		}
	}

	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	*index = cur_token;
	*result = begin_text;
	*result_size = end_text - begin_text;
	return SMTP_NO_ERROR;
}

static int
smtp_mime_body_part_dash2_boundary_transport_parse (const char *message, size_t length, size_t * index, char *boundary,	//struct trieobj * boundary_tree, wyong, 20231017 
						    const char **result,
						    size_t * result_size)
{
	size_t cur_token;
	const char *data_str;
	size_t data_size;
	const char *begin_text;
	const char *end_text;
	int r = SMTP_NO_ERROR;

	cur_token = *index;

	begin_text = message + cur_token;
	end_text = message + cur_token;

	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d],length=%d, cur_token = %d\n",
		    __FILE__, __FUNCTION__, __LINE__, length, cur_token);


	//DEBUG_SMTP(SMTP_DBG, "boundary_tree = %p\n", boundary_tree);
	r = smtp_mime_body_part_dash2_boundary_parse (message, length,
						      &cur_token,
						      /*boundary_tree, */
						      boundary, &data_str,
						      &data_size);
	if (r == SMTP_NO_ERROR || SMTP_ERROR_CONTINUE) {
		DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
			    __LINE__);
		end_text = data_str + data_size;

	}
	else {
		DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
			    __LINE__);
		return r;
	}



	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d], cur_token =%d\n", __FILE__,
		    __FUNCTION__, __LINE__, cur_token);
	*index = cur_token;
	*result = begin_text;
	*result_size = end_text - begin_text;
	return r;
}


static int smtp_mime_multipart_close_parse (const char *message,
					    size_t length, size_t * index);

static int
smtp_mime_body_part_dash2_boundary_close_parse (const char *message, size_t length, size_t * index, char *boundary,	//struct trieobj *boundary_tree, wyong, 20231017 
						const char **result,
						size_t * result_size)
{
	size_t cur_token;
	int r;
	const char *data_str;
	size_t data_size;
	const char *begin_text;
	const char *end_text;

	cur_token = *index;

	begin_text = message + cur_token;
	end_text = message + cur_token;

	while (1) {
		r = smtp_mime_body_part_dash2_boundary_parse (message, length,
							      &cur_token,
							      /* boundary_tree, */
							      boundary,
							      &data_str,
							      &data_size);
		if (r == SMTP_NO_ERROR) {
			end_text = data_str + data_size;
		}
		else {
			return r;
		}

		r = smtp_mime_multipart_close_parse (message, length,
						     &cur_token);
		if (r == SMTP_NO_ERROR) {
			break;
		}
		else if (r == SMTP_ERROR_PARSE) {
			/* do nothing */
		}
		else {
			return r;
		}
	}

	*index = cur_token;
	*result = data_str;
	*result_size = data_size;

	return SMTP_NO_ERROR;
}

enum
{
	MULTIPART_CLOSE_STATE_0,
	MULTIPART_CLOSE_STATE_1,
	MULTIPART_CLOSE_STATE_2,
	MULTIPART_CLOSE_STATE_3,
	MULTIPART_CLOSE_STATE_4
};

static int
smtp_mime_multipart_close_parse (const char *message, size_t length,
				 size_t * index)
{
	int state;
	size_t cur_token;

	cur_token = *index;
	state = MULTIPART_CLOSE_STATE_0;

	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d], length = %d, *index= %d \n",
		    __FILE__, __FUNCTION__, __LINE__, length, *index);
	while (state != MULTIPART_CLOSE_STATE_4) {

		switch (state) {

		case MULTIPART_CLOSE_STATE_0:
			DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__,
				    __FUNCTION__, __LINE__);
			//xiayu 2005.11.16
			if (cur_token >= length)
				return SMTP_ERROR_CONTINUE;
			if (cur_token >= length)
				return SMTP_ERROR_PARSE;

			DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__,
				    __FUNCTION__, __LINE__);
			switch (message[cur_token]) {
			case '-':
				state = MULTIPART_CLOSE_STATE_1;
				break;
			default:
				return SMTP_ERROR_PARSE;
			}
			break;

		case MULTIPART_CLOSE_STATE_1:
			DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__,
				    __FUNCTION__, __LINE__);
			//xiayu 2005.11.16
			if (cur_token >= length)
				return SMTP_ERROR_CONTINUE;
			if (cur_token >= length)
				return SMTP_ERROR_PARSE;

			switch (message[cur_token]) {
			case '-':
				state = MULTIPART_CLOSE_STATE_2;
				break;
			default:
				return SMTP_ERROR_PARSE;
			}
			break;

		case MULTIPART_CLOSE_STATE_2:
			DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__,
				    __FUNCTION__, __LINE__);
			if (cur_token >= length) {
				state = MULTIPART_CLOSE_STATE_4;
				break;
			}

			switch (message[cur_token]) {
			case ' ':
				state = MULTIPART_CLOSE_STATE_2;
				break;
			case '\t':
				state = MULTIPART_CLOSE_STATE_2;
				break;
			case '\r':
				state = MULTIPART_CLOSE_STATE_3;
				break;
			case '\n':
				state = MULTIPART_CLOSE_STATE_4;
				break;
			default:
				state = MULTIPART_CLOSE_STATE_4;
				break;
			}
			break;

		case MULTIPART_CLOSE_STATE_3:
			DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__,
				    __FUNCTION__, __LINE__);
			if (cur_token >= length) {
				state = MULTIPART_CLOSE_STATE_4;
				break;
			}

			switch (message[cur_token]) {
			case '\n':
				state = MULTIPART_CLOSE_STATE_4;
				break;
			default:
				state = MULTIPART_CLOSE_STATE_4;
				break;
			}
			break;
		}

		cur_token++;
	}

	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	*index = cur_token;

	return SMTP_NO_ERROR;
}

enum
{
	MULTIPART_NEXT_STATE_0,
	MULTIPART_NEXT_STATE_1,
	MULTIPART_NEXT_STATE_2
};

int
smtp_mime_multipart_next_parse (const char *message, size_t length,
				size_t * index)
{
	int state;
	size_t cur_token;

	cur_token = *index;
	state = MULTIPART_NEXT_STATE_0;

	while (state != MULTIPART_NEXT_STATE_2) {

		//xiayu 2005.11.16
		if (cur_token >= length)
			return SMTP_ERROR_CONTINUE;
		if (cur_token >= length)
			return SMTP_ERROR_PARSE;

		switch (state) {

		case MULTIPART_NEXT_STATE_0:
			switch (message[cur_token]) {
			case ' ':
				state = MULTIPART_NEXT_STATE_0;
				break;
			case '\t':
				state = MULTIPART_NEXT_STATE_0;
				break;
			case '\r':
				state = MULTIPART_NEXT_STATE_1;
				break;
			case '\n':
				state = MULTIPART_NEXT_STATE_2;
				break;
			default:
				return SMTP_ERROR_PARSE;
			}
			break;

		case MULTIPART_NEXT_STATE_1:
			switch (message[cur_token]) {
			case '\n':
				state = MULTIPART_NEXT_STATE_2;
				break;
			default:
				return SMTP_ERROR_PARSE;
			}
			break;
		}

		cur_token++;
	}

	*index = cur_token;

	return SMTP_NO_ERROR;
}

enum
{
	SMTP_MIME_DEFAULT_TYPE_TEXT_PLAIN,
	SMTP_MIME_DEFAULT_TYPE_MESSAGE
};


#if 0

static void
remove_unparsed_mime_headers (struct smtp_fields *fields)
{
	clistiter *cur;

	cur = clist_begin (fields->fld_list);
	while (cur != NULL) {
		struct smtp_field *field;
		int delete;

		field = clist_content (cur);

		switch (field->fld_type) {
		case SMTP_FIELD_OPTIONAL_FIELD:
			delete = 0;
			if (strncasecmp
			    (field->fld_data.fld_optional_field->fld_name,
			     "Content-", 8) == 0) {
				char *name;

				name = field->fld_data.fld_optional_field->
					fld_name + 8;
				if ((strcasecmp (name, "Type") == 0)
				    || (strcasecmp (name, "Transfer-Encoding")
					== 0)
				    || (strcasecmp (name, "ID") == 0)
				    || (strcasecmp (name, "Description") == 0)
				    || (strcasecmp (name, "Disposition") == 0)
				    || (strcasecmp (name, "Language") == 0)) {
					delete = 1;
				}
			}
			else if (strcasecmp
				 (field->fld_data.fld_optional_field->
				  fld_name, "MIME-Version") == 0) {
				delete = 1;
			}

			if (delete) {
				cur = clist_delete (fields->fld_list, cur);
				smtp_field_free (field);
			}
			else {
				cur = clist_next (cur);
			}
			break;

		default:
			cur = clist_next (cur);
		}
	}
}


/* ************************************************************************* */
/* MIME part decoding */

static signed char
get_base64_value (char ch)
{
	if ((ch >= 'A') && (ch <= 'Z'))
		return ch - 'A';
	if ((ch >= 'a') && (ch <= 'z'))
		return ch - 'a' + 26;
	if ((ch >= '0') && (ch <= '9'))
		return ch - '0' + 52;
	switch (ch) {
	case '+':
		return 62;
	case '/':
		return 63;
	case '=':		/* base64 padding */
		return -1;
	default:
		return -1;
	}
}



static int
hexa_to_char (char hexdigit)
{
	if ((hexdigit >= '0') && (hexdigit <= '9'))
		return hexdigit - '0';
	if ((hexdigit >= 'a') && (hexdigit <= 'f'))
		return hexdigit - 'a' + 10;
	if ((hexdigit >= 'A') && (hexdigit <= 'F'))
		return hexdigit - 'A' + 10;
	return 0;
}

static char
to_char (const char *hexa)
{
	return (hexa_to_char (hexa[0]) << 4) | hexa_to_char (hexa[1]);
}

enum
{
	STATE_NORMAL,
	STATE_CODED,
	STATE_OUT,
	STATE_CR,
};


static int
write_decoded_qp (MMAPString * mmapstr, const char *start, size_t count)
{
	if (mmap_string_append_len (mmapstr, start, count) == NULL)
		return SMTP_ERROR_MEMORY;

	return SMTP_NO_ERROR;
}



int
smtp_mime_part_parse (const char *message, size_t length,
		      size_t * index,
		      int encoding, char **result, size_t * result_len)
{
	switch (encoding) {
	case SMTP_MIME_MECHANISM_BASE64:
		return smtp_mime_base64_body_parse (message, length, index,
						    result, result_len);

	case SMTP_MIME_MECHANISM_QUOTED_PRINTABLE:
		return smtp_mime_quoted_printable_body_parse (message, length,
							      index, result,
							      result_len,
							      FALSE);

	case SMTP_MIME_MECHANISM_7BIT:
	case SMTP_MIME_MECHANISM_8BIT:
	case SMTP_MIME_MECHANISM_BINARY:
	default:
		return smtp_mime_binary_body_parse (message, length, index,
						    result, result_len);
	}
}

#endif

int
push_part_stack (struct smtp_info *psmtp)
{
	struct smtp_part *part;
	DEBUG_SMTP (SMTP_DBG, "push_part_stack\n");

	if (psmtp->part_stack_top == SMTP_MSG_STACK_MAX_DEPTH - 1) {
		DEBUG_SMTP (SMTP_DBG, "smtp part stack PUSH overflow\n");
		DEBUG_SMTP (SMTP_DBG, "smtp part stack PUSH overflow\n");
		return SMTP_ERROR_STACK_PUSH;
	}

	psmtp->part_stack_top++;

	/* init the current body part before being used */
	part = &psmtp->part_stack[psmtp->part_stack_top];
	part->state = 0;
	part->body_type = 0;
	part->content_type = NULL;
	part->encode_type = 0;
	part->boundary = NULL;
	//part->boundary_tree = NULL;

	DEBUG_SMTP (SMTP_DBG, "STACK_TOP psmtp->part_stack_top = %d\n",
		    psmtp->part_stack_top);

	DEBUG_SMTP (SMTP_DBG, "STACK_TOP psmtp->part_stack_top = %d\n",
		    psmtp->part_stack_top);
	DEBUG_SMTP (SMTP_DBG,
		    "&psmtp->part_stack[psmtp->part_stack_top] = %p\n",
		    &psmtp->part_stack[psmtp->part_stack_top]);

	return SMTP_NO_ERROR;
}

int
pop_part_stack (struct smtp_info *psmtp)
{
	struct smtp_part *part;

	DEBUG_SMTP (SMTP_DBG, "pop_part_stack\n");

	if (psmtp->part_stack_top == 0) {
		DEBUG_SMTP (SMTP_DBG, "smtp part stack POP overflow\n");
		DEBUG_SMTP (SMTP_DBG, "smtp part stack POP overflow\n");
		return SMTP_ERROR_STACK_POP;
	}

	part = &psmtp->part_stack[psmtp->part_stack_top];
	part->body_type = 0;
	part->state = 0;
	part->body_type = 0;
	part->content_type = NULL;	//has been given to nel engine
	part->encode_type = 0;	//Fixeme

	if (part->boundary) {
		free (part->boundary);
		DEBUG_SMTP (SMTP_MEM_1, "pop_part_stack: FREE pointer=%p\n",
			    part->boundary);
		part->boundary = NULL;
	}


	//if (part->boundary_tree) {
	//      free_trieobj(part->boundary_tree);
	//      DEBUG_SMTP(SMTP_MEM_1, "pop_part_stack: FREE trieobj=%p\n", part->boundary_tree);
	//      part->boundary_tree = NULL;
	//}

	psmtp->part_stack_top--;

	DEBUG_SMTP (SMTP_DBG, ": STACK_TOP psmtp->part_stack_top = %d\n",
		    psmtp->part_stack_top);

	DEBUG_SMTP (SMTP_DBG, "STACK_TOP psmtp->part_stack_top = %d\n",
		    psmtp->part_stack_top);
	DEBUG_SMTP (SMTP_DBG,
		    "&psmtp->part_stack[psmtp->part_stack_top] = %p\n",
		    &psmtp->part_stack[psmtp->part_stack_top]);

	return SMTP_NO_ERROR;
}

int
smtp_msg_fields_parse (struct smtp_info *psmtp, char *message, int length,
		       int *index)
{
	struct smtp_part *stack;
	int stack_top;
	size_t cur_token;
	struct smtp_part *cur_part;
	int body_type;
	int r, res;
	struct smtp_mime_field *field;
	int fld_type;

	DEBUG_SMTP (SMTP_DBG, "smtp_msg_fields_parse\n");
	DEBUG_SMTP (SMTP_DBG, "smtp_msg_fields_parse\n");

	cur_token = *index;

	DEBUG_SMTP (SMTP_DBG, "length = %d, cur_token: %d\n", length,
		    cur_token);
	DEBUG_SMTP (SMTP_DBG, "message+cur_token: %s\n", message + cur_token);
	while (1) {

		r = smtp_envelope_or_optional_field_parse (psmtp, message,
							   length, &cur_token,
							   &field, &fld_type);
		if (r != SMTP_NO_ERROR) {
			if (r == SMTP_ERROR_CONTINUE) {
				*index = cur_token;
				return r;
			}
			res = r;
			goto err;
		}

		DEBUG_SMTP (SMTP_DBG, "\n");
		if (fld_type == SMTP_FIELD_EOH) {

			DEBUG_SMTP (SMTP_DBG, "psmtp->part_stack_top = %d\n",
				    psmtp->part_stack_top);
			cur_part = &psmtp->part_stack[psmtp->part_stack_top];
			DEBUG_SMTP (SMTP_DBG, "cur_part = %p\n", cur_part);

			switch (cur_part->body_type) {
			case SMTP_MIME_MESSAGE:
				DEBUG_SMTP (SMTP_DBG, "\n");
				//stack_top--;
				cur_part->state = SMTP_MSG_FIELDS_PARSING;
				break;

			case SMTP_MIME_MULTIPLE:
				DEBUG_SMTP (SMTP_DBG, "\n");
				cur_part->state = SMTP_MSG_PREAMBLE_PARSING;
				break;

			case SMTP_MIME_SINGLE:
				DEBUG_SMTP (SMTP_DBG, "\n");
				cur_part->state = SMTP_MSG_BODY_PARSING;
				break;

			default:
				//xiayu 2005.12.5 bugfix for single text mail message without "Content-Type" header.
				if (cur_part->body_type == SMTP_MIME_NONE
				    && psmtp->part_stack_top == 1) {
					DEBUG_SMTP (SMTP_DBG, "\n");
					cur_part->state =
						SMTP_MSG_BODY_PARSING;

				}
				else {
					DEBUG_SMTP (SMTP_DBG,
						    "Error body type,  body_type = %d\n",
						    cur_part->body_type);
					return SMTP_ERROR_PARSE;
				}
				break;

			}

			*index = cur_token;
			DEBUG_SMTP (SMTP_DBG,
				    "SMTP_NO_ERROR: length = %d, cur_token: %d\n",
				    length, cur_token);
			return SMTP_NO_ERROR;
		}

		DEBUG_SMTP (SMTP_DBG, "\n");

	}


	//DEBUG_SMTP(SMTP_DBG, "\n");
	// *index = cur_token;
	//return SMTP_NO_ERROR;

      err:
	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	/* xiayu 2005.11.16  left the *index unchanged */
	DEBUG_SMTP (SMTP_DBG, "SMTP_ERROR = %d: length = %d, *index: %d\n",
		    res, length, *index);
	return res;

}


int
smtp_msg_body_parse (struct smtp_info *psmtp, char *message, int length,
		     int *index)
{
	const char *data_str = NULL;
	size_t data_size = 0;
	size_t cur_token;
	struct smtp_part *cur_part;
	struct smtp_part *parent_part;
	int left;
	int r, res;

	//bugfix, smtp_mime_data -> smtp_mime_text, wyong, 20231024 
	struct smtp_mime_data *text;


	DEBUG_SMTP (SMTP_DBG, "smtp_msg_body_parse, message=%s\n", message);

	cur_token = *index;
	DEBUG_SMTP (SMTP_DBG, "length = %d, cur_token = %d\n", length,
		    cur_token);
	cur_part = &psmtp->part_stack[psmtp->part_stack_top];
	parent_part = &psmtp->part_stack[psmtp->part_stack_top - 1];

	left = length - cur_token;
	if (left <= 0) {
		res = SMTP_ERROR_CONTINUE;
		goto succ;
	}

	DEBUG_SMTP (SMTP_DBG, "psmtp->part_stack_top = %d\n",
		    psmtp->part_stack_top);
	if (psmtp->part_stack_top == 1) {
		DEBUG_SMTP (SMTP_DBG, "message+cur_token: %s\n",
			    message + cur_token);
		if (left >= 5 && message[length - 5] == '\r'
		    && message[length - 4] == '\n'
		    && message[length - 3] == '.'
		    && message[length - 2] == '\r'
		    && message[length - 1] == '\n') {
			/* have got an SINGLE message */
			DEBUG_SMTP (SMTP_DBG, "\n");
			DEBUG_SMTP (SMTP_DBG, "left = %d\n", left);

			if (left > 5) {
				DEBUG_SMTP (SMTP_DBG, "\n");
				/* got some data, create an body */
				DEBUG_SMTP (SMTP_DBG, "TEXT BRANCH 1\n");

				//wyong, 20231023 
				//if( (text = smtp_mime_data_new(SMTP_MIME_DATA_TEXT, cur_part->encode_type, 1, message + cur_token, left - 5, NULL)) == NULL){
				if ((text =
				     smtp_mime_text_new (cur_part->
							 encode_type, 1,
							 message + cur_token,
							 left - 5)) == NULL) {
					res = SMTP_ERROR_MEMORY;
					goto err;	//free_content;
				}

			}
			else {
				DEBUG_SMTP (SMTP_DBG, "\n");
				/* when text is null */
				DEBUG_SMTP (SMTP_DBG, "TEXT BRANCH 1.5\n");

				//wyong, 20231023 
				//if( (text = smtp_mime_data_new(SMTP_MIME_DATA_TEXT, cur_part->encode_type, 1, NULL, 0, NULL)) == NULL){
				if ((text =
				     smtp_mime_text_new (cur_part->
							 encode_type, 1, NULL,
							 0)) == NULL) {
					res = SMTP_ERROR_MEMORY;
					goto err;	//free_content;
				}
			}

#ifdef USE_NEL
			DEBUG_SMTP (SMTP_DBG, "\n");
			if ((r =
			     nel_env_analysis (eng, &(psmtp->env), text_id,
					       (struct smtp_simple_event *)
					       text)) < 0) {
				DEBUG_SMTP (SMTP_DBG,
					    "smtp_msg_body_parse: nel_env_analysis error\n");
				res = SMTP_ERROR_POLICY;
				goto err;
			}
#endif

			DEBUG_SMTP (SMTP_DBG, "\n");
			if (psmtp->permit & SMTP_PERMIT_DENY) {
				//fprintf(stderr, "found a deny event\n");
				//smtp_close_connection(psmtp);
				res = SMTP_ERROR_POLICY;
				goto err;

			}
			else if (psmtp->permit & SMTP_PERMIT_DROP) {
				psmtp->permit |= SMTP_PERMIT_DENY;
				fprintf (stderr,
					 "found a drop event, no drop for message, denied.\n");
				//smtp_close_connection(psmtp);
				res = SMTP_ERROR_POLICY;
				goto err;
			}

#ifdef USE_NEL
			DEBUG_SMTP (SMTP_DBG, "\n");
			if ((r = nel_env_analysis (eng, &(psmtp->env), eot_id,
						   (struct smtp_simple_event
						    *) 0)) < 0) {
				DEBUG_SMTP (SMTP_DBG,
					    "smtp_msg_body_parse: nel_env_analysis error\n");
				res = SMTP_ERROR_POLICY;
				goto err;
			}

			DEBUG_SMTP (SMTP_DBG, "\n");
			if ((r = nel_env_analysis (eng, &(psmtp->env), eom_id,
						   (struct smtp_simple_event
						    *) 0)) < 0) {
				DEBUG_SMTP (SMTP_DBG,
					    "smtp_msg_body_parse: nel_env_analysis error\n");
				res = SMTP_ERROR_POLICY;
				goto err;
			}
#endif

			DEBUG_SMTP (SMTP_DBG, "\n");
			psmtp->last_cli_event_type = SMTP_MSG_EOM;
			//xiayu 2005.12.4 command dfa
			//psmtp->curr_parse_state = SMTP_STATE_PARSE_COMMAND;

			/* all data has been processed */
			DEBUG_SMTP (SMTP_DBG, "left = %d\n", left);
			DEBUG_SMTP (SMTP_DBG, "length = %d, cur_token = %d\n",
				    length, cur_token);
			cur_token += left;
			res = SMTP_NO_ERROR;
			goto succ;

		}
		else {
			/* continue to process the SINGLE mesg */
			DEBUG_SMTP (SMTP_DBG, "left = %d\n", left);
			if (left > 5) {
				DEBUG_SMTP (SMTP_DBG, "\n");
				/* got some data, create an body */

				DEBUG_SMTP (SMTP_DBG, "TEXT BRANCH 2\n");
				//wyong, 20231023 
				//if( (text = smtp_mime_data_new(SMTP_MIME_DATA_TEXT, cur_part->encode_type, 1, message + cur_token, left - 5, NULL)) == NULL){
				if ((text =
				     smtp_mime_text_new (cur_part->
							 encode_type, 1,
							 message + cur_token,
							 left - 5)) == NULL) {
					res = SMTP_ERROR_MEMORY;
					goto err;	//free_content;
				}

				/* xiayu 2005.11.30 
				 * reserve the last 5 chars for next parsing,
				 * in case there are incompeted "CRLF.CRLF"
				 * in them.
				 */
				cur_token += left - 5;

			}
			else {
				/* when text is null */
				DEBUG_SMTP (SMTP_DBG, "TEXT BRANCH 2.5\n");

				//wyong, 20231023 
				//if( (text = smtp_mime_data_new(SMTP_MIME_DATA_TEXT, cur_part->encode_type, 1, NULL, 0, NULL)) == NULL){
				if ((text =
				     smtp_mime_text_new (cur_part->
							 encode_type, 1, NULL,
							 0)) == NULL) {
					res = SMTP_ERROR_MEMORY;
					goto err;	//free_content;
				}

				/* since left <= 5,
				 * so we reserve all the left chars
				 * for next parsing
				 * which is: "cur_token += 0;"
				 */

			}


#ifdef USE_NEL
			DEBUG_SMTP (SMTP_DBG, "\n");
			if ((r =
			     nel_env_analysis (eng, &(psmtp->env), text_id,
					       (struct smtp_simple_event *)
					       text)) < 0) {
				DEBUG_SMTP (SMTP_DBG,
					    "smtp_msg_body_parse: nel_env_analysis error\n");
				res = SMTP_ERROR_POLICY;
				goto err;
			}
#endif

			DEBUG_SMTP (SMTP_DBG, "\n");
			if (psmtp->permit & SMTP_PERMIT_DENY) {
				//fprintf(stderr, "found a deny event\n");
				//smtp_close_connection(psmtp);
				res = SMTP_ERROR_POLICY;
				goto err;

			}
			else if (psmtp->permit & SMTP_PERMIT_DROP) {
				psmtp->permit |= SMTP_PERMIT_DENY;
				fprintf (stderr,
					 "found a drop event, no drop for message, denied.\n");
				//smtp_close_connection(psmtp);
				res = SMTP_ERROR_POLICY;
				goto err;
			}

			DEBUG_SMTP (SMTP_DBG, "left = %d\n", left);
			DEBUG_SMTP (SMTP_DBG, "length = %d, cur_token = %d\n",
				    length, cur_token);

			res = SMTP_ERROR_CONTINUE;
			goto succ;
		}

	}

	//xiayu 2005.11.29 debug
	//if (parent_part->boundary_tree == NULL) {

	//wyong, 20231017 
	if (parent_part->boundary == NULL) {
		res = SMTP_ERROR_PARSE;
		goto err;
	}

	/* if cur_mime has mm_parent, then we must get 
	   message 's end position by parent 's boundary */
	r = smtp_mime_body_part_dash2_boundary_transport_parse (message, length, &cur_token, parent_part->boundary,	//parent_part->boundary_tree, wyong, 20231017 
								&data_str,
								&data_size);
	if (r != SMTP_NO_ERROR) {

		DEBUG_SMTP (SMTP_DBG, "\n");
		if (r == SMTP_ERROR_CONTINUE) {

			DEBUG_SMTP (SMTP_DBG, "\n");
			/* boundary not found, check in next data segment */
			DEBUG_SMTP (SMTP_DBG, "data_size = %d\n", data_size);
			if (data_size > 0) {

				DEBUG_SMTP (SMTP_DBG, "\n");
				/* got some data, create an body */

				DEBUG_SMTP (SMTP_DBG, "TEXT BRANCH 3\n");

				//wyong, 20231023 
				//if( (text = smtp_mime_data_new(SMTP_MIME_DATA_TEXT, cur_part->encode_type, 1, data_str, data_size, NULL)) == NULL)
				if ((text =
				     smtp_mime_text_new (cur_part->
							 encode_type, 1,
							 data_str,
							 data_size)) ==
				    NULL) {
					res = SMTP_ERROR_MEMORY;
					goto err;	//free_content;
				}

			}
			else {
				/* when text is null */
				DEBUG_SMTP (SMTP_DBG, "TEXT BRANCH 3.5\n");

				//wyong, 20231023 
				//if( (text = smtp_mime_data_new(SMTP_MIME_DATA_TEXT, cur_part->encode_type, 1, NULL, 0, NULL)) == NULL)
				if ((text =
				     smtp_mime_text_new (cur_part->
							 encode_type, 1, NULL,
							 0)) == NULL) {
					res = SMTP_ERROR_MEMORY;
					goto err;	//free_content;
				}
			}

#ifdef USE_NEL
			DEBUG_SMTP (SMTP_DBG, "\n");
			if ((r =
			     nel_env_analysis (eng, &(psmtp->env), text_id,
					       (struct smtp_simple_event *)
					       text)) < 0) {
				DEBUG_SMTP (SMTP_DBG,
					    "smtp_msg_body_parse: nel_env_analysis error\n");
				res = SMTP_ERROR_POLICY;
				goto err;
			}
#endif

			DEBUG_SMTP (SMTP_DBG, "\n");
			if (psmtp->permit & SMTP_PERMIT_DENY) {
				//fprintf(stderr, "found a deny event\n");
				//smtp_close_connection(psmtp);
				res = SMTP_ERROR_POLICY;
				goto err;

			}
			else if (psmtp->permit & SMTP_PERMIT_DROP) {
				psmtp->permit |= SMTP_PERMIT_DENY;
				fprintf (stderr,
					 "found a drop event, no drop for message, denied.\n");
				//smtp_close_connection(psmtp);
				res = SMTP_ERROR_POLICY;
				goto err;
			}

			DEBUG_SMTP (SMTP_DBG, "\n");
			res = SMTP_ERROR_CONTINUE;
			goto succ;
		}

		DEBUG_SMTP (SMTP_DBG, "\n");
		res = r;
		goto err;

	}

	/* --boundary found */
	DEBUG_SMTP (SMTP_DBG, "\n");
	DEBUG_SMTP (SMTP_DBG, "data_size = %d\n", data_size);

	if (data_size > 0) {
		DEBUG_SMTP (SMTP_DBG, "\n");
		/* got some data, create an body */
		DEBUG_SMTP (SMTP_DBG, "TEXT BRANCH 4\n");

		//wyong, 20231023 
		//if( (text = smtp_mime_data_new(SMTP_MIME_DATA_TEXT, cur_part->encode_type, 1, data_str, data_size, NULL)) == NULL)
		if ((text =
		     smtp_mime_text_new (cur_part->encode_type, 1, data_str,
					 data_size)) == NULL) {
			res = SMTP_ERROR_MEMORY;
			goto err;	//free_content;
		}

	}
	else {
		DEBUG_SMTP (SMTP_DBG, "\n");
		/* when text is null */
		DEBUG_SMTP (SMTP_DBG, "TEXT BRANCH 4.5\n");

		//wyong, 20231023 
		//if( (text = smtp_mime_data_new(SMTP_MIME_DATA_TEXT, cur_part->encode_type, 1, NULL, 0, NULL)) == NULL)
		if ((text =
		     smtp_mime_text_new (cur_part->encode_type, 1, NULL,
					 0)) == NULL) {
			res = SMTP_ERROR_MEMORY;
			goto err;	//free_content;
		}
	}

#ifdef USE_NEL
	DEBUG_SMTP (SMTP_DBG, "\n");
	if ((r = nel_env_analysis (eng, &(psmtp->env), text_id,
				   (struct smtp_simple_event *) text)) < 0) {
		DEBUG_SMTP (SMTP_DBG,
			    "smtp_msg_body_parse: nel_env_analysis error\n");
		res = SMTP_ERROR_POLICY;
		goto err;
	}
#endif

	DEBUG_SMTP (SMTP_DBG, "\n");
	if (psmtp->permit & SMTP_PERMIT_DENY) {
		//fprintf(stderr, "found a deny event\n");
		//smtp_close_connection(psmtp);
		res = SMTP_ERROR_POLICY;
		goto err;

	}
	else if (psmtp->permit & SMTP_PERMIT_DROP) {
		psmtp->permit |= SMTP_PERMIT_DENY;
		fprintf (stderr,
			 "found a drop event, no drop for message, denied.\n");
		//smtp_close_connection(psmtp);
		res = SMTP_ERROR_POLICY;
		goto err;
	}


	DEBUG_SMTP (SMTP_DBG, "\n");
	r = smtp_mime_multipart_close_parse (message, length, &cur_token);
	if (r == SMTP_NO_ERROR) {
		DEBUG_SMTP (SMTP_DBG, "\n");
		/* found --boundary-- */

#ifdef USE_NEL
		DEBUG_SMTP (SMTP_DBG, "\n");
		if ((r = nel_env_analysis (eng, &(psmtp->env), eot_id,
					   (struct smtp_simple_event *) 0)) <
		    0) {
			DEBUG_SMTP (SMTP_DBG,
				    "smtp_msg_body_parse: nel_env_analysis error\n");
			res = SMTP_ERROR_POLICY;
			goto err;
		}
		DEBUG_SMTP (SMTP_DBG, "\n");
		if ((r = nel_env_analysis (eng, &(psmtp->env), eob_id,
					   (struct smtp_simple_event *) 0)) <
		    0) {
			DEBUG_SMTP (SMTP_DBG,
				    "smtp_msg_body_parse: nel_env_analysis error\n");
			res = SMTP_ERROR_POLICY;
			goto err;
		}
#endif

		/* process the epilogue when continue process the old_parent */

		//cur_part = &psmtp->part_stack[psmtp->part_stack_top];
		//cur_part->state = SMTP_MSG_EPILOGUE_PARSING;
		//DEBUG_SMTP(SMTP_DBG, "smtp_msg_body_parse: part_stack_top = %d\n", psmtp->part_stack_top);
		//DEBUG_SMTP(SMTP_DBG, "smtp_msg_body_parse: cur_part = %p\n", cur_part);
		psmtp->last_cli_event_type = SMTP_MSG_EOB;
		res = SMTP_NO_ERROR;

	}
	else if (r == SMTP_ERROR_CONTINUE) {
		DEBUG_SMTP (SMTP_DBG, "\n");

		//wyong, 20231017 
		//cur_token -= parent_part->boundary_tree->shortest;
		cur_token -= strlen (parent_part->boundary);

		res = SMTP_ERROR_CONTINUE;

	}
	else if (r == SMTP_ERROR_PARSE) {
		DEBUG_SMTP (SMTP_DBG, "\n");
		/* found --boundary only, now the "\r\n" after that */
		r = smtp_crlf_parse (message, length, &cur_token);
		if (r != SMTP_NO_ERROR) {
			DEBUG_SMTP (SMTP_DBG, "\n");
			if (r == SMTP_ERROR_CONTINUE) {
				DEBUG_SMTP (SMTP_DBG, "\n");

				//wyong, 20231017 
				//cur_token -= parent_part->boundary_tree->shortest;
				cur_token -= strlen (parent_part->boundary);

				res = r;
				goto succ;
			}
			res = r;
			goto err;
		}

#ifdef USE_NEL
		DEBUG_SMTP (SMTP_DBG, "\n");
		if ((r = nel_env_analysis (eng, &(psmtp->env), eot_id,
					   (struct smtp_simple_event *) 0)) <
		    0) {
			DEBUG_SMTP (SMTP_DBG,
				    "smtp_msg_body_parse: nel_env_analysis error\n");
			res = SMTP_ERROR_POLICY;
			goto err;
		}
#endif
		DEBUG_SMTP (SMTP_DBG, "\n");
		//cur_part = &psmtp->part_stack[psmtp->part_stack_top];
		//cur_part->state = SMTP_MSG_FIELDS_PARSING;
		psmtp->last_cli_event_type = SMTP_MSG_EOT;
		res = SMTP_NO_ERROR;

	}
	else {
		/* error occured */
		DEBUG_SMTP (SMTP_DBG, "\n");
		res = r;
		goto err;
	}

      succ:
	DEBUG_SMTP (SMTP_DBG, "\n");
	*index = cur_token;
	DEBUG_SMTP (SMTP_DBG, "SUCC = %d: length = %d, cur_token = %d\n", res,
		    length, cur_token);
	return res;

      err:
	DEBUG_SMTP (SMTP_DBG, "\n");
	reset_client_buf (psmtp, index);
	DEBUG_SMTP (SMTP_DBG, "ERR = %d: length = %d, *index= %d\n", res,
		    length, *index);
	return res;

}


int
smtp_msg_preamble_parse (struct smtp_info *psmtp, char *message, int length,
			 int *index)
{
	const char *data_str = NULL;
	size_t data_size = 0;
	size_t cur_token;
	struct smtp_part *cur_part;
	struct smtp_part *parent_part;
	int r, res;
	int left;

	//smtp_mime_data -> smtp_mime_text, wyong, 20231024 
	struct smtp_mime_text *text;

	DEBUG_SMTP (SMTP_DBG, "smtp_msg_preamble_parse\n");
	DEBUG_SMTP (SMTP_DBG, "smtp_msg_preamble_parse\n");

	cur_token = *index;
	DEBUG_SMTP (SMTP_DBG, "length = %d, cur_token = %d\n", length,
		    cur_token);
	DEBUG_SMTP (SMTP_DBG, "part_stack_top = %d\n", psmtp->part_stack_top);
	cur_part = &psmtp->part_stack[psmtp->part_stack_top];
	DEBUG_SMTP (SMTP_DBG, "cur_part = %p\n", cur_part);
	parent_part = &psmtp->part_stack[psmtp->part_stack_top - 1];

	if (cur_part->boundary == NULL) {
		DEBUG_SMTP (SMTP_DBG, "\n");
		res = SMTP_ERROR_PARSE;
		goto err;

	}

	DEBUG_SMTP (SMTP_DBG, "\n");
	left = length - cur_token;
	if (left <= 0) {
		DEBUG_SMTP (SMTP_DBG, "\n");
		res = SMTP_ERROR_CONTINUE;
		goto succ;
	}

	DEBUG_SMTP (SMTP_DBG, "\n");
	DEBUG_SMTP (SMTP_DBG,
		    "BEFORE smtp_mime_body_part_dash2_boundary_transport_parse: message+cur_token: %s\n",
		    message + cur_token);
	r = smtp_mime_body_part_dash2_boundary_transport_parse (message, length, &cur_token, cur_part->boundary,	//cur_part->boundary_tree, wyong, 20231017 
								&data_str,
								&data_size);

	if (r != SMTP_NO_ERROR) {
		DEBUG_SMTP (SMTP_DBG, "\n");
		/* boundary not found, check in next data segment */
		if (r == SMTP_ERROR_CONTINUE) {

			DEBUG_SMTP (SMTP_DBG, "\n");
			DEBUG_SMTP (SMTP_DBG, "data_size = %d\n", data_size);
			if (data_size > 0) {
				DEBUG_SMTP (SMTP_DBG, "\n");
				/* got some data, create an body */
				DEBUG_SMTP (SMTP_DBG, "TEXT BRANCH 5\n");


				//wyong, 20231023 
				//if( (text = smtp_mime_data_new(SMTP_MIME_DATA_TEXT, cur_part->encode_type, 1, data_str, data_size, NULL)) == NULL)
				if ((text =
				     smtp_mime_text_new (cur_part->
							 encode_type, 1,
							 data_str,
							 data_size)) ==
				    NULL) {
					res = SMTP_ERROR_MEMORY;
					goto err;	//free_content;
				}

			}
			else {
				DEBUG_SMTP (SMTP_DBG, "\n");
				/* when text is null */
				DEBUG_SMTP (SMTP_DBG, "TEXT BRANCH 5.5\n");

				//wyong, 20231023
				//if( (text = smtp_mime_data_new(SMTP_MIME_DATA_TEXT, cur_part->encode_type, 1, NULL, 0, NULL)) == NULL)
				if ((text =
				     smtp_mime_text_new (cur_part->
							 encode_type, 1, NULL,
							 0)) == NULL) {
					res = SMTP_ERROR_MEMORY;
					goto err;	//free_content;
				}
			}

#ifdef USE_NEL
			DEBUG_SMTP (SMTP_DBG, "\n");
			if ((r =
			     nel_env_analysis (eng, &(psmtp->env), text_id,
					       (struct smtp_simple_event *)
					       text)) < 0) {
				DEBUG_SMTP (SMTP_DBG,
					    "smtp_msg_preamble_parse: nel_env_analysis error\n");
				res = SMTP_ERROR_POLICY;
				goto err;
			}
#endif

			DEBUG_SMTP (SMTP_DBG, "\n");
			if (psmtp->permit & SMTP_PERMIT_DENY) {
				//fprintf(stderr, "found a deny event\n");
				//smtp_close_connection(psmtp);
				res = SMTP_ERROR_POLICY;
				goto err;

			}
			else if (psmtp->permit & SMTP_PERMIT_DROP) {
				psmtp->permit |= SMTP_PERMIT_DENY;
				fprintf (stderr,
					 "found a drop event, no drop for message, denied.\n");
				//smtp_close_connection(psmtp);
				res = SMTP_ERROR_POLICY;
				goto err;
			}

			DEBUG_SMTP (SMTP_DBG, "\n");
			res = SMTP_ERROR_CONTINUE;
			goto succ;
		}

		DEBUG_SMTP (SMTP_DBG, "\n");
		res = r;
		goto err;
	}

	/* found --boundary */
	DEBUG_SMTP (SMTP_DBG, "smtp_msg_preamble_parse: found --boundary\n");
	DEBUG_SMTP (SMTP_DBG, "data_size = %d\n", data_size);
	if (data_size > 0) {
		DEBUG_SMTP (SMTP_DBG, "\n");
		/* got some data, create an body */
		DEBUG_SMTP (SMTP_DBG, "TEXT BRANCH 6\n");

		//wyong, 20231023 
		//if( (text = smtp_mime_data_new(SMTP_MIME_DATA_TEXT, cur_part->encode_type, 1, data_str, data_size, NULL)) == NULL){
		if ((text =
		     smtp_mime_text_new (cur_part->encode_type, 1, data_str,
					 data_size)) == NULL) {
			res = SMTP_ERROR_MEMORY;
			goto err;	//free_content;
		}

	}
	else {
		DEBUG_SMTP (SMTP_DBG, "\n");
		/* when text is null */
		DEBUG_SMTP (SMTP_DBG, "TEXT BRANCH 6.5\n");

		//wyong, 20231023 
		//if( (text = smtp_mime_data_new(SMTP_MIME_DATA_TEXT, cur_part->encode_type, 1, NULL, 0, NULL)) == NULL){
		if ((text =
		     smtp_mime_text_new (cur_part->encode_type, 1, NULL,
					 0)) == NULL) {
			res = SMTP_ERROR_MEMORY;
			goto err;	//free_content;
		}
	}

#ifdef USE_NEL
	DEBUG_SMTP (SMTP_DBG, "\n");
	if ((r = nel_env_analysis (eng, &(psmtp->env), text_id,
				   (struct smtp_simple_event *) text)) < 0) {
		DEBUG_SMTP (SMTP_DBG,
			    "smtp_msg_preamble_parse: nel_env_analysis error\n");
		res = SMTP_ERROR_POLICY;
		goto err;
	}
#endif

	DEBUG_SMTP (SMTP_DBG, "\n");
	if (psmtp->permit & SMTP_PERMIT_DENY) {
		//fprintf(stderr, "found a deny event\n");
		//smtp_close_connection(psmtp);
		res = SMTP_ERROR_POLICY;
		goto err;

	}
	else if (psmtp->permit & SMTP_PERMIT_DROP) {
		psmtp->permit |= SMTP_PERMIT_DENY;
		fprintf (stderr,
			 "found a drop event, no drop for message, denied.\n");
		//smtp_close_connection(psmtp);
		res = SMTP_ERROR_POLICY;
		goto err;
	}

	left = length - cur_token;
	r = smtp_crlf_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		DEBUG_SMTP (SMTP_DBG, "\n");
		if (r == SMTP_ERROR_CONTINUE) {
			DEBUG_SMTP (SMTP_DBG, "\n");

			//wyong, 20231017 
			//cur_token -= cur_part->boundary_tree->shortest;
			cur_token -= strlen (cur_part->boundary);

			res = r;
			goto succ;
		}
		res = r;
		goto err;
	}

#ifdef USE_NEL
	DEBUG_SMTP (SMTP_DBG, "\n");
	if ((r = nel_env_analysis (eng, &(psmtp->env), eot_id,
				   (struct smtp_simple_event *) 0)) < 0) {
		DEBUG_SMTP (SMTP_DBG,
			    "smtp_msg_preamble_parse: nel_env_analysis error\n");
		res = SMTP_ERROR_POLICY;
		goto err;
	}
#endif

	DEBUG_SMTP (SMTP_DBG, "\n");
	//cur_part->state = SMTP_MSG_FIELDS_PARSING;
	psmtp->last_cli_event_type = SMTP_MSG_EOT;
	res = SMTP_NO_ERROR;

      succ:
	DEBUG_SMTP (SMTP_DBG, "\n");
	*index = cur_token;
	DEBUG_SMTP (SMTP_DBG, "SUCC = %d: length = %d, cur_token = %d\n", res,
		    length, cur_token);
	return res;

      err:
	DEBUG_SMTP (SMTP_DBG, "\n");
	reset_client_buf (psmtp, index);
	DEBUG_SMTP (SMTP_DBG, "ERR = %d: length = %d, *index= %d\n", res,
		    length, *index);
	return res;

}

int
smtp_msg_epilogue_parse (struct smtp_info *psmtp, char *message, int length,
			 int *index)
{
	const char *data_str = NULL;
	size_t data_size = 0;
	size_t cur_token;
	struct smtp_part *cur_part;
	//struct smtp_part *parent_part;
	int left;
	int r, res;

	//smtp_mime_data -> smtp_mime_text, wyong, 20231024 
	struct smtp_mime_data *text;


	DEBUG_SMTP (SMTP_DBG, "smtp_msg_epilogue_parse\n");

	cur_token = *index;
	DEBUG_SMTP (SMTP_DBG, "length = %d, cur_token = %d\n", length,
		    cur_token);
	cur_part = &psmtp->part_stack[psmtp->part_stack_top];
	//parent_part = &psmtp->part_stack[psmtp->part_stack_top - 1];

	DEBUG_SMTP (SMTP_DBG, "\n");
	left = length - cur_token;
	if (left <= 0) {
		res = SMTP_ERROR_CONTINUE;
		goto succ;
	}

	if (psmtp->part_stack_top == 0) {
		DEBUG_SMTP (SMTP_DBG, "message+cur_token: %s\n",
			    message + cur_token);
		if (left >= 5 && message[length - 5] == '\r'
		    && message[length - 4] == '\n'
		    && message[length - 3] == '.'
		    && message[length - 2] == '\r'
		    && message[length - 1] == '\n') {
			/* have got an SINGLE message */
			DEBUG_SMTP (SMTP_DBG, "\n");
			DEBUG_SMTP (SMTP_DBG, "left = %d\n", left);
			if (left > 5) {
				DEBUG_SMTP (SMTP_DBG, "\n");
				/* got some data, create an body */
				DEBUG_SMTP (SMTP_DBG, "TEXT BRANCH 7\n");

				//wyong, 20231023 
				//if( (text = smtp_mime_data_new(SMTP_MIME_DATA_TEXT, cur_part->encode_type, 1, message + cur_token, left - 5 , NULL)) == NULL){
				if ((text =
				     smtp_mime_text_new (cur_part->
							 encode_type, 1,
							 message + cur_token,
							 left - 5)) == NULL) {
					res = SMTP_ERROR_MEMORY;
					goto err;	//free_content;
				}

			}
			else {
				DEBUG_SMTP (SMTP_DBG, "\n");
				/* when text is null */
				DEBUG_SMTP (SMTP_DBG, "TEXT BRANCH 7.5\n");

				//wyong, 20231023 
				//if( (text = smtp_mime_data_new(SMTP_MIME_DATA_TEXT, cur_part->encode_type, 1, NULL, 0 , NULL)) == NULL){
				if ((text =
				     smtp_mime_text_new (cur_part->
							 encode_type, 1, NULL,
							 0)) == NULL) {
					res = SMTP_ERROR_MEMORY;
					goto err;	//free_content;
				}
			}

#ifdef USE_NEL
			DEBUG_SMTP (SMTP_DBG, "\n");
			if ((r =
			     nel_env_analysis (eng, &(psmtp->env), text_id,
					       (struct smtp_simple_event *)
					       text)) < 0) {
				DEBUG_SMTP (SMTP_DBG,
					    "smtp_msg_epilogue_parse: nel_env_analysis error\n");
				res = SMTP_ERROR_POLICY;
				goto err;
			}
#endif

			DEBUG_SMTP (SMTP_DBG, "\n");
			if (psmtp->permit & SMTP_PERMIT_DENY) {
				//fprintf(stderr, "found a deny event\n");
				//smtp_close_connection(psmtp);
				res = SMTP_ERROR_POLICY;
				goto err;

			}
			else if (psmtp->permit & SMTP_PERMIT_DROP) {
				psmtp->permit |= SMTP_PERMIT_DENY;
				fprintf (stderr,
					 "found a drop event, no drop for message, denied.\n");
				//smtp_close_connection(psmtp);
				res = SMTP_ERROR_POLICY;
				goto err;
			}
#ifdef USE_NEL
			DEBUG_SMTP (SMTP_DBG, "\n");
			if ((r =
			     nel_env_analysis (eng, &(psmtp->env), eot_id,
					       (struct smtp_simple_event *)
					       0)) < 0) {
				DEBUG_SMTP (SMTP_DBG,
					    "smtp_msg_epilogue_parse: nel_env_analysis error\n");
				res = SMTP_ERROR_POLICY;
				goto err;
			}


			DEBUG_SMTP (SMTP_DBG, "\n");
			if ((r = nel_env_analysis (eng, &(psmtp->env), eom_id,
						   (struct smtp_simple_event
						    *) 0)) < 0) {
				DEBUG_SMTP (SMTP_DBG,
					    "smtp_msg_epilogue_parse: nel_env_analysis error\n");
				res = SMTP_ERROR_POLICY;
				goto err;
			}
#endif

			DEBUG_SMTP (SMTP_DBG, "\n");
			psmtp->last_cli_event_type = SMTP_MSG_EOM;
			//xiayu 2005.12.4 command dfa
			//psmtp->curr_parse_state = SMTP_STATE_PARSE_COMMAND;

			/* all data has been processed */
			DEBUG_SMTP (SMTP_DBG, "left = %d\n", left);
			DEBUG_SMTP (SMTP_DBG, "length = %d, cur_token = %d\n",
				    length, cur_token);
			cur_token += left;
			res = SMTP_NO_ERROR;
			goto succ;

		}
		else {
			// continue to process the SINGLE mesg 
			DEBUG_SMTP (SMTP_DBG, "\n");

			DEBUG_SMTP (SMTP_DBG, "left = %d\n", left);
			if (left > 5) {
				DEBUG_SMTP (SMTP_DBG, "\n");
				/* got some data, create an body */
				DEBUG_SMTP (SMTP_DBG, "TEXT BRANCH 8\n");

				//wyong, 20231023 
				//if( (text = smtp_mime_data_new(SMTP_MIME_DATA_TEXT, cur_part->encode_type, 1, message + cur_token, left - 5, NULL)) == NULL){
				if ((text =
				     smtp_mime_text_new (cur_part->
							 encode_type, 1,
							 message + cur_token,
							 left - 5)) == NULL) {
					res = SMTP_ERROR_MEMORY;
					goto err;	//free_content;
				}

				/* xiayu 2005.11.30 
				 * reserve the last 5 chars for next parsing,
				 * in case there are incompeted "CRLF.CRLF"
				 * in them.
				 */
				DEBUG_SMTP (SMTP_DBG, "\n");
				cur_token += left - 5;

			}
			else {
				DEBUG_SMTP (SMTP_DBG, "\n");
				/* when text is null */
				DEBUG_SMTP (SMTP_DBG, "TEXT BRANCH 8.5\n");

				//wyong, 20231023 
				//if( (text = smtp_mime_data_new(SMTP_MIME_DATA_TEXT, cur_part->encode_type, 1, NULL, 0, NULL)) == NULL){
				if ((text =
				     smtp_mime_text_new (cur_part->
							 encode_type, 1, NULL,
							 0)) == NULL) {
					res = SMTP_ERROR_MEMORY;
					goto err;	//free_content;
				}

				/* since left <= 5,
				 * so we reserve all the left chars
				 * for next parsing
				 * which is: "cur_token += 0;"
				 */
			}

#ifdef USE_NEL
			DEBUG_SMTP (SMTP_DBG, "\n");
			if ((r =
			     nel_env_analysis (eng, &(psmtp->env), text_id,
					       (struct smtp_simple_event *)
					       text)) < 0) {
				DEBUG_SMTP (SMTP_DBG,
					    "smtp_msg_epilogue_parse: nel_env_analysis error\n");
				res = SMTP_ERROR_POLICY;
				goto err;
			}
#endif
			DEBUG_SMTP (SMTP_DBG, "\n");
			if (psmtp->permit & SMTP_PERMIT_DENY) {
				//fprintf(stderr, "found a deny event\n");
				//smtp_close_connection(psmtp);
				res = SMTP_ERROR_POLICY;
				goto err;

			}
			else if (psmtp->permit & SMTP_PERMIT_DROP) {
				psmtp->permit |= SMTP_PERMIT_DENY;
				fprintf (stderr,
					 "found a drop event, no drop for message, denied.\n");
				//smtp_close_connection(psmtp);
				res = SMTP_ERROR_POLICY;
				goto err;
			}

			DEBUG_SMTP (SMTP_DBG, "left = %d\n", left);
			DEBUG_SMTP (SMTP_DBG, "length = %d, cur_token = %d\n",
				    length, cur_token);
			res = SMTP_ERROR_CONTINUE;
			goto succ;
		}

	}

	//check if "\r\n" right after previous boundary, wyong, 20231018
	//DEBUG_SMTP(SMTP_DBG, "\n");
	//r = smtp_crlf_parse(message, length, &cur_token);
	//if(r == SMTP_NO_ERROR ) {
	//      DEBUG_SMTP(SMTP_DBG, "\n");
	//      psmtp->last_cli_event_type = SMTP_MSG_EOT;
	//      res = SMTP_NO_ERROR;
	//      goto succ; 
	//}

	//xiayu 2005.11.29 debug
	//if (cur_part->boundary_tree == NULL) {

	//wyong, 20231017 
	if (cur_part->boundary == NULL) {
		res = SMTP_ERROR_PARSE;
		goto err;
	}

	DEBUG_SMTP (SMTP_DBG, "\n");
	//DEBUG_SMTP(SMTP_DBG, "parent_part->boundary_tree = %p\n", parent_part->boundary_tree );
	/* if cur_mime has mm_parent, then we must get 
	   message 's end position by parent 's boundary */
	r = smtp_mime_body_part_dash2_boundary_transport_parse (message,
								length,
								&cur_token,
								//parent_part->boundary_tree, 
								cur_part->boundary,	//cur_part->boundary_tree, wyong, 20231017 
								&data_str,
								&data_size);

	DEBUG_SMTP (SMTP_DBG, "cur_token = %d\n", cur_token);
	if (r != SMTP_NO_ERROR) {

		DEBUG_SMTP (SMTP_DBG, "\n");
		/* boundary not found, check in next data segment */
		if (r == SMTP_ERROR_CONTINUE) {

			DEBUG_SMTP (SMTP_DBG, "\n");
			DEBUG_SMTP (SMTP_DBG, "data_size = %d\n", data_size);

			if (data_size > 0) {
				DEBUG_SMTP (SMTP_DBG, "\n");
				/* got some data, create an body */
				DEBUG_SMTP (SMTP_DBG, "TEXT BRANCH 9\n");

				//wyong, 20231023 
				//if( (text = smtp_mime_data_new(SMTP_MIME_DATA_TEXT, cur_part->encode_type, 1, data_str, data_size, NULL)) == NULL){
				if ((text =
				     smtp_mime_text_new (cur_part->
							 encode_type, 1,
							 data_str,
							 data_size)) ==
				    NULL) {
					res = SMTP_ERROR_MEMORY;
					goto err;	//free_content;
				}

			}
			else {
				DEBUG_SMTP (SMTP_DBG, "\n");
				/* when text is null */
				DEBUG_SMTP (SMTP_DBG, "TEXT BRANCH 9.5\n");

				//wyong, 20231023 
				//if( (text = smtp_mime_data_new(SMTP_MIME_DATA_TEXT, cur_part->encode_type, 1, NULL, 0, NULL)) == NULL){
				if ((text =
				     smtp_mime_text_new (cur_part->
							 encode_type, 1, NULL,
							 0)) == NULL) {
					res = SMTP_ERROR_MEMORY;
					goto err;	//free_content;
				}
			}

#ifdef USE_NEL
			DEBUG_SMTP (SMTP_DBG, "\n");
			if ((r =
			     nel_env_analysis (eng, &(psmtp->env), text_id,
					       (struct smtp_simple_event *)
					       text)) < 0) {
				DEBUG_SMTP (SMTP_DBG,
					    "smtp_msg_epilogue_parse: nel_env_analysis error\n");
				res = SMTP_ERROR_POLICY;
				goto err;
			}
#endif

			DEBUG_SMTP (SMTP_DBG, "\n");
			if (psmtp->permit & SMTP_PERMIT_DENY) {
				//fprintf(stderr, "found a deny event\n");
				//smtp_close_connection(psmtp);
				res = SMTP_ERROR_POLICY;
				goto err;

			}
			else if (psmtp->permit & SMTP_PERMIT_DROP) {
				psmtp->permit |= SMTP_PERMIT_DENY;
				fprintf (stderr,
					 "found a drop event, no drop for message, denied.\n");
				//smtp_close_connection(psmtp);
				res = SMTP_ERROR_POLICY;
				goto err;
			}

			DEBUG_SMTP (SMTP_DBG, "\n");
			res = SMTP_ERROR_CONTINUE;
			goto succ;
		}

		DEBUG_SMTP (SMTP_DBG, "\n");
		res = r;
		goto err;

	}

	/* --boundary found */
	DEBUG_SMTP (SMTP_DBG, "\n");
	DEBUG_SMTP (SMTP_DBG, "data_size = %d\n", data_size);
	if (data_size > 0) {
		DEBUG_SMTP (SMTP_DBG, "data_size = %d\n", data_size);
		/* got some data, create an body */
		DEBUG_SMTP (SMTP_DBG, "TEXT BRANCH 10\n");

		//wyong, 20231023 
		//if( (text = smtp_mime_data_new(SMTP_MIME_DATA_TEXT, cur_part->encode_type, 1, data_str, data_size, NULL)) == NULL){
		if ((text =
		     smtp_mime_text_new (cur_part->encode_type, 1, data_str,
					 data_size)) == NULL) {
			res = SMTP_ERROR_MEMORY;
			goto err;	//free_content;
		}

	}
	else {
		DEBUG_SMTP (SMTP_DBG, "data_size = %d\n", data_size);
		/* when text is null */
		DEBUG_SMTP (SMTP_DBG, "TEXT BRANCH 10.5\n");

		//wyong, 20231023 
		//if( (text = smtp_mime_data_new(SMTP_MIME_DATA_TEXT, cur_part->encode_type, 1, NULL, 0, NULL)) == NULL){
		if ((text =
		     smtp_mime_text_new (cur_part->encode_type, 1, NULL,
					 0)) == NULL) {
			res = SMTP_ERROR_MEMORY;
			goto err;	//free_content;
		}
	}

#ifdef USE_NEL
	DEBUG_SMTP (SMTP_DBG, "\n");
	if ((r = nel_env_analysis (eng, &(psmtp->env), text_id,
				   (struct smtp_simple_event *) text)) < 0) {
		DEBUG_SMTP (SMTP_DBG,
			    "smtp_msg_epilogue_parse: nel_env_analysis error\n");
		res = SMTP_ERROR_POLICY;
		goto err;
	}
#endif

	DEBUG_SMTP (SMTP_DBG, "\n");
	if (psmtp->permit & SMTP_PERMIT_DENY) {
		//fprintf(stderr, "found a deny event\n");
		//smtp_close_connection(psmtp);
		res = SMTP_ERROR_POLICY;
		goto err;

	}
	else if (psmtp->permit & SMTP_PERMIT_DROP) {
		psmtp->permit |= SMTP_PERMIT_DENY;
		fprintf (stderr,
			 "found a drop event, no drop for message, denied.\n");
		//smtp_close_connection(psmtp);
		res = SMTP_ERROR_POLICY;
		goto err;
	}


	DEBUG_SMTP (SMTP_DBG, "\n");
	r = smtp_mime_multipart_close_parse (message, length, &cur_token);
	if (r == SMTP_NO_ERROR) {
		/* found --boundary-- */

#ifdef USE_NEL
		DEBUG_SMTP (SMTP_DBG, "\n");
		if ((r = nel_env_analysis (eng, &(psmtp->env), eot_id,
					   (struct smtp_simple_event *) 0)) <
		    0) {
			DEBUG_SMTP (SMTP_DBG,
				    "smtp_msg_epilogue_parse: nel_env_analysis error\n");
			res = SMTP_ERROR_POLICY;
			goto err;
		}

		if ((r = nel_env_analysis (eng, &(psmtp->env), eob_id,
					   (struct smtp_simple_event *) 0)) <
		    0) {
			DEBUG_SMTP (SMTP_DBG,
				    "smtp_msg_epilogue_parse: nel_env_analysis error\n");
			res = SMTP_ERROR_POLICY;
			goto err;
		}
#endif

		DEBUG_SMTP (SMTP_DBG, "\n");
		/* process the epilogue when continue process the old_parent */
		//cur_part = &psmtp->part_stack[psmtp->part_stack_top];
		//cur_part->state = SMTP_MSG_EPILOGUE_PARSING;
		psmtp->last_cli_event_type = SMTP_MSG_EOB;
		res = SMTP_NO_ERROR;

	}
	else if (r == SMTP_ERROR_CONTINUE) {

		DEBUG_SMTP (SMTP_DBG,
			    "multi close parse in epilogue parsing\n");

		//wyong, 20231017
		//cur_token -= cur_part->boundary_tree->shortest;
		cur_token -= strlen (cur_part->boundary);

		res = SMTP_ERROR_CONTINUE;

	}
	else if (r == SMTP_ERROR_PARSE) {

		DEBUG_SMTP (SMTP_DBG, "\n");
		/* found --boundary */
		/* the start of sub mime  */
		r = smtp_crlf_parse (message, length, &cur_token);
		if (r != SMTP_NO_ERROR) {
			DEBUG_SMTP (SMTP_DBG, "\n");
			if (r == SMTP_ERROR_CONTINUE) {

				//wyong, 20231017 
				//cur_token -= cur_part->boundary_tree->shortest;
				cur_token -= strlen (cur_part->boundary);

				res = r;
				goto succ;
			}
			res = r;
			goto err;
		}

#ifdef USE_NEL
		DEBUG_SMTP (SMTP_DBG, "\n");
		if ((r = nel_env_analysis (eng, &(psmtp->env), eot_id,
					   (struct smtp_simple_event *) 0)) <
		    0) {
			DEBUG_SMTP (SMTP_DBG,
				    "smtp_msg_epilogue_parse: nel_env_analysis error\n");
			res = SMTP_ERROR_POLICY;
			goto err;
		}
#endif

		DEBUG_SMTP (SMTP_DBG, "\n");
		//cur_part = &psmtp->part_stack[psmtp->part_stack_top];
		//cur_part->state = SMTP_MSG_FIELDS_PARSING;
		psmtp->last_cli_event_type = SMTP_MSG_EOT;
		res = SMTP_NO_ERROR;

	}
	else {
		/* error occured */
		res = r;
		goto err;
	}

      succ:
	DEBUG_SMTP (SMTP_DBG, "\n");
	*index = cur_token;
	DEBUG_SMTP (SMTP_DBG, "SUCC = %d: length = %d, cur_token = %d\n", res,
		    length, cur_token);
	return res;

      err:
	DEBUG_SMTP (SMTP_DBG, "\n");
	reset_client_buf (psmtp, index);
	DEBUG_SMTP (SMTP_DBG, "ERR = %d: length = %d, *index= %d\n", res,
		    length, *index);
	return res;

}


int
parse_smtp_message (struct smtp_info *psmtp)
{
	struct smtp_part *cur_part;
	size_t cur_token;
	char *message;
	int length;
	int r, res;

	DEBUG_SMTP (SMTP_DBG, "parse_smtp_message \n");
	DEBUG_SMTP (SMTP_DBG, "parse_smtp_message \n");

	res = SMTP_NO_ERROR;
	message = psmtp->cli_data;
	length = psmtp->cli_data_len;

	//cur_token = psmtp->cli_parse_len;
	cur_token = 0;
	DEBUG_SMTP (SMTP_DBG, "length=%d, cur_token=%d\n", length, cur_token);
	DEBUG_SMTP (SMTP_DBG, "cur_token=%d, length=%d\n", cur_token, length);

	while (cur_token < length) {

		DEBUG_SMTP (SMTP_DBG, "cur_token = %d, length = %d\n",
			    cur_token, length);
		DEBUG_SMTP (SMTP_DBG, "top = %d\n", psmtp->part_stack_top);

		if (psmtp->new_part_flag) {
			/* a new smtp message part start */
			res = push_part_stack (psmtp);
			if (res != SMTP_NO_ERROR) {
				DEBUG_SMTP (SMTP_DBG, "\n");
				goto err;
			}
			psmtp->new_part_flag = 0;
			DEBUG_SMTP (SMTP_DBG, "new part (PUSH) TOP = %d\n",
				    psmtp->part_stack_top);
		}

		cur_part = &psmtp->part_stack[psmtp->part_stack_top];
		switch (psmtp->part_stack[psmtp->part_stack_top].state) {
		case SMTP_MSG_FIELDS_PARSING:
			DEBUG_SMTP (SMTP_DBG, "\n");
			r = smtp_msg_fields_parse (psmtp, message, length,
						   &cur_token);
			if (r != SMTP_NO_ERROR) {
				if (r == SMTP_ERROR_CONTINUE) {
					DEBUG_SMTP (SMTP_DBG, "\n");
					//psmtp->cli_parse_len = cur_token;
					res = r;
					//goto succ;
					goto cont;
				}
				res = r;
				goto err;
			}
			cur_part = &psmtp->part_stack[psmtp->part_stack_top];
			if (cur_part->body_type == SMTP_MIME_MESSAGE) {
				res = pop_part_stack (psmtp);
				if (res != SMTP_NO_ERROR) {
					DEBUG_SMTP (SMTP_DBG, "\n");
					goto err;
				}
				DEBUG_SMTP (SMTP_DBG,
					    "RFC FIELDS (POP) TOP = %d\n",
					    psmtp->part_stack_top);
				psmtp->new_part_flag = 1;
			}
			break;

		case SMTP_MSG_BODY_PARSING:
			DEBUG_SMTP (SMTP_DBG, "\n");
			r = smtp_msg_body_parse (psmtp, message, length,
						 &cur_token);
			if (r != SMTP_NO_ERROR) {
				DEBUG_SMTP (SMTP_DBG, "\n");
				if (r == SMTP_ERROR_CONTINUE) {
					DEBUG_SMTP (SMTP_DBG, "\n");
					//psmtp->cli_parse_len = cur_token;
					res = r;
					//goto succ;
					goto cont;
				}
				res = r;
				goto err;
			}

			if (psmtp->last_cli_event_type == SMTP_MSG_EOT) {
				res = pop_part_stack (psmtp);
				if (res != SMTP_NO_ERROR) {
					DEBUG_SMTP (SMTP_DBG, "\n");
					goto err;
				}
				DEBUG_SMTP (SMTP_DBG,
					    "eot BODY (POP) TOP = %d\n",
					    psmtp->part_stack_top);
				cur_part =
					&psmtp->part_stack[psmtp->
							   part_stack_top];
				cur_part->state = SMTP_MSG_FIELDS_PARSING;
				psmtp->new_part_flag = 1;

			}
			else if (psmtp->last_cli_event_type == SMTP_MSG_EOB) {
				struct smtp_part *parent_part =
					&psmtp->part_stack[psmtp->
							   part_stack_top -
							   1];
				if (parent_part->body_type ==
				    SMTP_MIME_MULTIPLE) {
					res = pop_part_stack (psmtp);
					if (res != SMTP_NO_ERROR)
						goto err;

					res = pop_part_stack (psmtp);
					if (res != SMTP_NO_ERROR)
						goto err;
					DEBUG_SMTP (SMTP_DBG,
						    "eob multi BODY (POP*2) TOP = %d\n",
						    psmtp->part_stack_top);

				}
				else {
					/* SMTP_MIME_SINGLE */
					res = pop_part_stack (psmtp);
					if (res != SMTP_NO_ERROR)
						goto err;
					DEBUG_SMTP (SMTP_DBG,
						    "eob single BODY (POP) TOP = %d\n",
						    psmtp->part_stack_top);
				}

				cur_part =
					&psmtp->part_stack[psmtp->
							   part_stack_top];
				cur_part->state = SMTP_MSG_EPILOGUE_PARSING;

				//wyong, 20231017 
				//r = smtp_crlf_parse(message, length, &cur_token);
				//if(r != SMTP_NO_ERROR ) {
				//      cur_part->state = SMTP_MSG_EPILOGUE_PARSING;
				//} else {
				//      cur_part->state = SMTP_MSG_FIELDS_PARSING;
				//      psmtp->new_part_flag = 1; 
				//}

			}
			else if (psmtp->last_cli_event_type == SMTP_MSG_EOM) {
				psmtp->new_part_flag = 1;

				DEBUG_SMTP (SMTP_DBG, "eom BODY RAIN\n",
					    psmtp->part_stack_top);
#if 1
				//xiayu 2005.12.9 bugfix the partial mails
				res = pop_part_stack (psmtp);
				if (res != SMTP_NO_ERROR)
					goto err;
				DEBUG_SMTP (SMTP_DBG,
					    "eom BODY (POP) TOP = %d\n",
					    psmtp->part_stack_top);
#endif
				goto succ;
			}
			break;

		case SMTP_MSG_PREAMBLE_PARSING:
			DEBUG_SMTP (SMTP_DBG, "\n");
			r = smtp_msg_preamble_parse (psmtp, message, length,
						     &cur_token);
			if (r != SMTP_NO_ERROR) {
				DEBUG_SMTP (SMTP_DBG, "\n");
				if (r == SMTP_ERROR_CONTINUE) {
					DEBUG_SMTP (SMTP_DBG, "\n");
					//psmtp->cli_parse_len = cur_token;
					res = r;
					//goto succ;
					goto cont;
				}
				res = r;
				goto err;
			}
			cur_part = &psmtp->part_stack[psmtp->part_stack_top];
			cur_part->state = SMTP_MSG_FIELDS_PARSING;
			psmtp->new_part_flag = 1;
			break;

		case SMTP_MSG_EPILOGUE_PARSING:
			DEBUG_SMTP (SMTP_DBG, "\n");
			r = smtp_msg_epilogue_parse (psmtp, message, length,
						     &cur_token);
			if (r != SMTP_NO_ERROR) {
				DEBUG_SMTP (SMTP_DBG, "\n");
				if (r == SMTP_ERROR_CONTINUE) {
					DEBUG_SMTP (SMTP_DBG, "\n");
					//psmtp->cli_parse_len = cur_token;
					res = r;
					//goto succ;
					goto cont;
				}
				res = r;
				goto err;
			}

			if (psmtp->last_cli_event_type == SMTP_MSG_EOB) {
				res = pop_part_stack (psmtp);
				if (res != SMTP_NO_ERROR)
					goto err;
				//res = pop_part_stack(psmtp);
				//if (res != SMTP_NO_ERROR)
				//      goto err;
				DEBUG_SMTP (SMTP_DBG,
					    "eob EPILOGUE (POP) TOP = %d\n",
					    psmtp->part_stack_top);
				cur_part =
					&psmtp->part_stack[psmtp->
							   part_stack_top];
				cur_part->state = SMTP_MSG_EPILOGUE_PARSING;

			}
			else if (psmtp->last_cli_event_type == SMTP_MSG_EOT) {
				DEBUG_SMTP (SMTP_DBG,
					    "eot EPILOGUE (POP) TOP = %d\n",
					    psmtp->part_stack_top);
				cur_part =
					&psmtp->part_stack[psmtp->
							   part_stack_top];
				cur_part->state = SMTP_MSG_FIELDS_PARSING;
				psmtp->new_part_flag = 1;

			}
			else if (psmtp->last_cli_event_type == SMTP_MSG_EOM) {
				psmtp->new_part_flag = 1;

				//bugfix, wyong, 20231025 
				psmtp->curr_parse_state =
					SMTP_STATE_PARSE_COMMAND;
				goto succ;
			}
			break;

		default:
			DEBUG_SMTP (SMTP_DBG, "part_stack state Error\n");
			res = r;
			goto err;
		}

	}

      cont:
	/* dealing with corrupt mail messages */
	DEBUG_SMTP (SMTP_DBG, "Check CRLF\\.CRLF when continue\n");
	if (message[length - 5] == '\r' &&
	    message[length - 4] == '\n' &&
	    message[length - 3] == '.' &&
	    message[length - 2] == '\r' && message[length - 1] == '\n') {

		DEBUG_SMTP (SMTP_DBG, "Error: Corrupt SMTP Mail message\n");
		res = SMTP_ERROR_PARSE;
		goto err;
	}

	//xiayu 2005.12.9 bugfix for long headers
	cur_part = &psmtp->part_stack[psmtp->part_stack_top];
	if (cur_part->state == SMTP_MSG_FIELDS_PARSING
	    && psmtp->cli_data == psmtp->cli_buf
	    && psmtp->cli_data_len == psmtp->cli_buf_len - 1) {
		/* enlarge the client parsing buffer */
		DEBUG_SMTP (SMTP_DBG, "Enlarge the client parsing buffer\n");
		if (psmtp->cli_buf_len >= 10 * SMTP_BUF_LEN + 1) {
			//r = reply_to_client(ptcp, "554 Transaction failed\r\n");
			//if (r != SMTP_NO_ERROR) {
			//      res = r;
			//}
			printf ("The message has a too much long header.\n");
			//smtp_close_connection(psmtp);
			printf ("The connection was closed by IPS.\n");
			return SMTP_ERROR_PARSE;
		}

		psmtp->cli_buf_len += SMTP_BUF_LEN;
		psmtp->cli_buf = (unsigned char *) realloc (psmtp->cli_buf,
							    psmtp->
							    cli_buf_len);
		if (psmtp->cli_buf == NULL) {
			DEBUG_SMTP (SMTP_DBG, "realloc error\n");
			return SMTP_ERROR_MEMORY;
		}
		psmtp->cli_data = psmtp->cli_buf;
	}

      succ:

	DEBUG_SMTP (SMTP_DBG, "cli_data = %p, cli_buf = %p\n",
		    psmtp->cli_data, psmtp->cli_buf);
	DEBUG_SMTP (SMTP_DBG, "cur_token = %d\n", cur_token);
	DEBUG_SMTP (SMTP_DBG, "cli_data_len = %d\n", psmtp->cli_data_len);

	//wyong, 20231003 
	res = sync_client_data (psmtp, cur_token);
	//psmtp->cli_data += cur_token;
	//psmtp->cli_data_len -= cur_token;

	//xiayu 2005.12.9 bugfix for long headers
	//change "SMTP_BUF_LEN" into "(psmtp->cli_buf_len-1)"
	if (psmtp->cli_data > psmtp->cli_buf + (psmtp->cli_buf_len - 1)
	    || psmtp->cli_data_len < 0) {
		DEBUG_SMTP (SMTP_DBG, "buffer overflow: cli_data_len = %d\n",
			    psmtp->cli_data_len);
		DEBUG_SMTP (SMTP_DBG,
			    "buffer overflow: cli_data - cli_buf = %d\n",
			    psmtp->cli_data - psmtp->cli_buf);
		return SMTP_ERROR_BUFFER;
	}

	//DEBUG_SMTP(SMTP_DBG, "cli_data_len = %d\n", psmtp->cli_data_len);
	//DEBUG_SMTP(SMTP_DBG, "before write_to_server\n");
	//r = write_to_server(ptcp, psmtp);
	//if (r < 0) {
	//      res = SMTP_ERROR_TCPAPI_WRITE;
	//      goto err;
	//}
	//DEBUG_SMTP(SMTP_DBG, "SUCC = %d: length = %d, cur_token = %d\n", res, length, cur_token);
	return res;

      err:
	DEBUG_SMTP (SMTP_DBG, "\n");
	if (res == SMTP_ERROR_PARSE) {
		//r = reply_to_client(ptcp, "554 Transaction failed\r\n");
		//if (r != SMTP_NO_ERROR) {
		//      res = r;
		//}
		printf ("The message was not using RFC822 headers, or it's headers has been corrupted.\n");
		//smtp_close_connection(psmtp);
		printf ("The connection was closed by IPS.\n");
	}

	//xiayu 2005.12.9 bugfix BUG44
	if (res == SMTP_ERROR_STACK_PUSH) {
		res = SMTP_ERROR_PARSE;
		//r = reply_to_client(ptcp, "554 Transaction failed\r\n");
		//if (r != SMTP_NO_ERROR) {
		//      res = r;
		//}
		printf ("The RFC822 format messages had been embeded in to one mail for too many times.\r\n");
		//smtp_close_connection(psmtp);
		printf ("The connection was closed by IPS.\n");
	}

	DEBUG_SMTP (SMTP_DBG, "res = %d\n", res);
	DEBUG_SMTP (SMTP_DBG, "ERR = %d: length = %d, *index= %d\n", res,
		    length, *index);
	return res;

}
