#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "smtp.h"
#include "address.h"
#include "mime.h"
#include "mem_pool.h"

extern ObjPool_t smtp_mailbox_addr_pool;

struct smtp_address *
smtp_address_new (int ad_type, struct smtp_mailbox *ad_mailbox,
		  struct smtp_group *ad_group)
{
	struct smtp_address *address;

	address = malloc (sizeof (*address));
	if (address == NULL)
		return NULL;
	DEBUG_SMTP (SMTP_MEM, "smtp_address_new: MALLOC pointer=%p\n",
		    address);

	address->ad_type = ad_type;
	switch (ad_type) {
	case SMTP_ADDRESS_MAILBOX:
		address->ad_data.ad_mailbox = ad_mailbox;
		break;
	case SMTP_ADDRESS_GROUP:
		address->ad_data.ad_group = ad_group;
		break;
	}

	return address;
}


void
smtp_address_free (struct smtp_address *address)
{
	DEBUG_SMTP (SMTP_DBG, "smtp_address_free: address = %p\n", address);
	switch (address->ad_type) {
	case SMTP_ADDRESS_MAILBOX:
		smtp_mailbox_free (address->ad_data.ad_mailbox);
		break;
	case SMTP_ADDRESS_GROUP:
		smtp_group_free (address->ad_data.ad_group);
	}
	free (address);
	DEBUG_SMTP (SMTP_MEM, "smtp_address_free: FREE pointer=%p\n",
		    address);
}


struct smtp_mailbox *
smtp_mailbox_new (char *mb_display_name, char *mb_addr_spec)
{
	struct smtp_mailbox *mb;

	mb = malloc (sizeof (*mb));
	if (mb == NULL)
		return NULL;
	DEBUG_SMTP (SMTP_MEM, "smtp_mailbox_new: MALLOC pointer=%p\n", mb);

	mb->mb_display_name = mb_display_name;
	mb->mb_addr_spec = mb_addr_spec;

	return mb;
}


void
smtp_mailbox_free (struct smtp_mailbox *mailbox)
{
	if (mailbox->mb_display_name != NULL) {
		DEBUG_SMTP (SMTP_MEM, "BEFORE smtp_display_name_free\n");
		smtp_display_name_free (mailbox->mb_display_name);
	}

	if (mailbox->mb_addr_spec != NULL) {
		smtp_addr_spec_free (mailbox->mb_addr_spec);
	}

	free (mailbox);
	DEBUG_SMTP (SMTP_MEM, "smtp_mailbox_free: FREE pointer=%p\n",
		    mailbox);
}

void
smtp_angle_addr_free (char *angle_addr)
{
	free (angle_addr);
	DEBUG_SMTP (SMTP_MEM, "smtp_angle_addr_free: FREE pointer=%p\n",
		    angle_addr);
}


struct smtp_group *
smtp_group_new (char *grp_display_name, struct smtp_mailbox_list *grp_mb_list)
{
	struct smtp_group *group;

	group = malloc (sizeof (*group));
	if (group == NULL)
		return NULL;
	DEBUG_SMTP (SMTP_MEM, "smtp_group_new: MALLOC pointer=%p\n", group);

	group->grp_display_name = grp_display_name;
	group->grp_mb_list = grp_mb_list;

	return group;
}


void
smtp_group_free (struct smtp_group *group)
{
	if (group->grp_mb_list)
		smtp_mailbox_list_free (group->grp_mb_list);
	DEBUG_SMTP (SMTP_MEM, "BEFORE smtp_display_name_free\n");
	smtp_display_name_free (group->grp_display_name);
	free (group);
	DEBUG_SMTP (SMTP_MEM, "smtp_group_free: FREE pointer=%p\n", group);
}

void
smtp_display_name_free (char *display_name)
{
	DEBUG_SMTP (SMTP_MEM, "BEFORE smtp_phrase_free\n");
	smtp_phrase_free (display_name);
}


struct smtp_mailbox_list *
smtp_mailbox_list_new (clist * mb_list)
{
	struct smtp_mailbox_list *mbl;

	mbl = malloc (sizeof (*mbl));
	if (mbl == NULL)
		return NULL;
	DEBUG_SMTP (SMTP_MEM, "smtp_mailbox_list_new: MALLOC pointer=%p\n",
		    mbl);

	mbl->mb_list = mb_list;

	return mbl;
}


void
smtp_mailbox_list_free (struct smtp_mailbox_list *mb_list)
{
	clist_foreach (mb_list->mb_list, (clist_func) smtp_mailbox_free,
		       NULL);
	clist_free (mb_list->mb_list);
	free (mb_list);
	DEBUG_SMTP (SMTP_MEM, "smtp_mailbox_list_free: FREE pointer=%p\n",
		    mb_list);
}



struct smtp_address_list *
smtp_address_list_new (clist * ad_list)
{
	struct smtp_address_list *addr_list;

	addr_list = malloc (sizeof (*addr_list));
	if (addr_list == NULL)
		return NULL;
	DEBUG_SMTP (SMTP_MEM, "smtp_address_list_new: MALLOC pointer=%p\n",
		    addr_list);

	addr_list->ad_list = ad_list;

	return addr_list;
}


void
smtp_address_list_free (struct smtp_address_list *addr_list)
{
	DEBUG_SMTP (SMTP_DBG, "smtp_address_list_free: addr_list = %p\n",
		    addr_list);
	clist_foreach (addr_list->ad_list, (clist_func) smtp_address_free,
		       NULL);
	clist_free (addr_list->ad_list);
	free (addr_list);
	DEBUG_SMTP (SMTP_MEM, "smtp_address_list_free: FREE pointer=%p\n",
		    addr_list);
}


void
smtp_addr_spec_free (char *addr_spec)
{
	free (addr_spec);
	DEBUG_SMTP (SMTP_MEM, "smtp_add_spec_free: FREE pointer=%p\n",
		    addr_spec);
}

void
smtp_local_part_free (char *local_part)
{
	free (local_part);
	DEBUG_SMTP (SMTP_MEM, "smtp_local_part_free: FREE pointer=%p\n",
		    local_part);
}

void
smtp_domain_free (char *domain)
{
	free (domain);
	DEBUG_SMTP (SMTP_MEM, "smtp_domain_free: FREE pointer=%p\n",
		    domain);
}

void
smtp_domain_literal_free (char *domain_literal)
{
	free (domain_literal);
	DEBUG_SMTP (SMTP_MEM, "smtp_domain_literal_free: FREE pointer=%p\n",
		    domain_literal);
}


struct smtp_mailbox_list *
smtp_mailbox_list_new_empty ()
{
	clist *list;
	struct smtp_mailbox_list *mb_list;

	list = clist_new ();
	if (list == NULL)
		return NULL;

	mb_list = smtp_mailbox_list_new (list);
	if (mb_list == NULL)
		return NULL;

	return mb_list;
}

int
smtp_mailbox_list_add (struct smtp_mailbox_list *mailbox_list,
		       struct smtp_mailbox *mb)
{
	int r;

	r = clist_append (mailbox_list->mb_list, mb);
	if (r < 0)
		return SMTP_ERROR_MEMORY;

	return SMTP_NO_ERROR;
}

int
smtp_mailbox_list_add_parse (struct smtp_mailbox_list *mailbox_list,
			     char *mb_str)
{
	int r;
	size_t cur_token;
	struct smtp_mailbox *mb;
	int res;

	cur_token = 0;
	r = smtp_mailbox_parse (mb_str, strlen (mb_str), &cur_token, &mb);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	r = smtp_mailbox_list_add (mailbox_list, mb);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free;
	}

	return SMTP_NO_ERROR;

      free:
	smtp_mailbox_free (mb);
      err:
	return res;
}

int
smtp_mailbox_list_add_mb (struct smtp_mailbox_list *mailbox_list,
			  char *display_name, char *address)
{
	int r;
	struct smtp_mailbox *mb;
	int res;

	mb = smtp_mailbox_new (display_name, address);
	if (mb == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto err;
	}

	r = smtp_mailbox_list_add (mailbox_list, mb);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free;
	}

	return SMTP_NO_ERROR;

      free:
	smtp_mailbox_free (mb);
      err:
	return res;
}



struct smtp_address_list *
smtp_address_list_new_empty ()
{
	clist *list;
	struct smtp_address_list *addr_list;

	list = clist_new ();
	if (list == NULL)
		return NULL;

	addr_list = smtp_address_list_new (list);
	if (addr_list == NULL)
		return NULL;

	return addr_list;
}

int
smtp_address_list_add (struct smtp_address_list *address_list,
		       struct smtp_address *addr)
{
	int r;

	r = clist_append (address_list->ad_list, addr);
	if (r < 0)
		return SMTP_ERROR_MEMORY;

	return SMTP_NO_ERROR;
}

int
smtp_address_list_add_parse (struct smtp_address_list *address_list,
			     char *addr_str)
{
	int r;
	size_t cur_token;
	struct smtp_address *addr;
	int res;

	cur_token = 0;
	r = smtp_address_parse (addr_str, strlen (addr_str), &cur_token,
				&addr);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	r = smtp_address_list_add (address_list, addr);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free;
	}

	return SMTP_NO_ERROR;

      free:
	smtp_address_free (addr);
      err:
	return res;
}

int
smtp_address_list_add_mb (struct smtp_address_list *address_list,
			  char *display_name, char *address)
{
	int r;
	struct smtp_mailbox *mb;
	struct smtp_address *addr;
	int res;

	mb = smtp_mailbox_new (display_name, address);
	if (mb == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto err;
	}

	addr = smtp_address_new (SMTP_ADDRESS_MAILBOX, mb, NULL);
	if (addr == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free_mb;
	}

	r = smtp_address_list_add (address_list, addr);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free_addr;
	}

	return SMTP_NO_ERROR;

      free_addr:
	smtp_address_free (addr);
      free_mb:
	smtp_mailbox_free (mb);
      err:
	return res;
}


/*
address         =       mailbox / group
*/

int
smtp_address_parse (const char *message, size_t length,
		    size_t * index, struct smtp_address **result)
{
	int type;
	size_t cur_token;
	struct smtp_mailbox *mailbox;
	struct smtp_group *group;
	struct smtp_address *address;
	int r;
	int res;

	cur_token = *index;

	mailbox = NULL;
	group = NULL;

	type = SMTP_ADDRESS_ERROR;	/* XXX - removes a gcc warning */
	DEBUG_SMTP (SMTP_DBG, "\n");
	r = smtp_group_parse (message, length, &cur_token, &group);
	if (r == SMTP_NO_ERROR) {
		DEBUG_SMTP (SMTP_DBG, "\n");
		type = SMTP_ADDRESS_GROUP;
	}
	DEBUG_SMTP (SMTP_DBG, "\n");

	if (r == SMTP_ERROR_PARSE) {
		DEBUG_SMTP (SMTP_DBG, "\n");
		r = smtp_mailbox_parse (message, length, &cur_token,
					&mailbox);
		if (r == SMTP_NO_ERROR) {
			DEBUG_SMTP (SMTP_DBG, "\n");
			type = SMTP_ADDRESS_MAILBOX;
			DEBUG_SMTP (SMTP_DBG, "\n");
		}
		DEBUG_SMTP (SMTP_DBG, "\n");
	}

	DEBUG_SMTP (SMTP_DBG, "\n");
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	DEBUG_SMTP (SMTP_DBG, "\n");
	address = smtp_address_new (type, mailbox, group);
	if (address == NULL) {
		DEBUG_SMTP (SMTP_DBG, "\n");
		res = SMTP_ERROR_MEMORY;
		goto free;
	}

	DEBUG_SMTP (SMTP_DBG, "address = %p\n", address);
	*result = address;
	*index = cur_token;

	return SMTP_NO_ERROR;

      free:
	if (mailbox != NULL)
		smtp_mailbox_free (mailbox);
	if (group != NULL)
		smtp_group_free (group);
      err:
	return res;
}


/*
mailbox         =       name-addr / addr-spec
*/


int
smtp_mailbox_parse (const char *message, size_t length,
		    size_t * index, struct smtp_mailbox **result)
{
	size_t cur_token;
	char *display_name;
	struct smtp_mailbox *mailbox;
	char *addr_spec;
	int r;
	int res;

	cur_token = *index;
	display_name = NULL;
	addr_spec = NULL;

	r = smtp_name_addr_parse (message, length, &cur_token,
				  &display_name, &addr_spec);
	if (r == SMTP_ERROR_PARSE) {
		r = smtp_addr_spec_parse (message, length, &cur_token,
					  &addr_spec);
	}

	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	mailbox = smtp_mailbox_new (display_name, addr_spec);
	if (mailbox == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free;
	}

	*result = mailbox;
	*index = cur_token;

	return SMTP_NO_ERROR;

      free:
	if (display_name != NULL) {
		DEBUG_SMTP (SMTP_MEM, "BEFORE smtp_display_name_free\n");
		smtp_display_name_free (display_name);
	}
	if (addr_spec != NULL)
		smtp_addr_spec_free (addr_spec);
      err:
	return res;
}

/*
name-addr       =       [display-name] angle-addr
*/

int
smtp_name_addr_parse (const char *message, size_t length,
		      size_t * index,
		      char **pdisplay_name, char **pangle_addr)
{
	char *display_name;
	char *angle_addr;
	size_t cur_token;
	int r;
	int res;

	cur_token = *index;

	display_name = NULL;
	angle_addr = NULL;

	r = smtp_display_name_parse (message, length, &cur_token,
				     &display_name);
	if ((r != SMTP_NO_ERROR) && (r != SMTP_ERROR_PARSE)) {
		res = r;
		goto err;
	}

	r = smtp_angle_addr_parse (message, length, &cur_token, &angle_addr);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free_display_name;
	}

	*pdisplay_name = display_name;
	*pangle_addr = angle_addr;
	*index = cur_token;

	return SMTP_NO_ERROR;

      free_display_name:
	if (display_name != NULL) {
		DEBUG_SMTP (SMTP_MEM, "BEFORE smtp_display_name_free\n");
		smtp_display_name_free (display_name);
	}
      err:
	return res;
}

/*
angle-addr      =       [CFWS] "<" addr-spec ">" [CFWS] / obs-angle-addr
*/

int
smtp_angle_addr_parse (const char *message, size_t length,
		       size_t * index, char **result)
{
	size_t cur_token;
	char *addr_spec;
	int r;

	cur_token = *index;

	r = smtp_cfws_parse (message, length, &cur_token);
	if ((r != SMTP_NO_ERROR) && (r != SMTP_ERROR_PARSE))
		return r;

	r = smtp_lower_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR)
		return r;

	r = smtp_addr_spec_parse (message, length, &cur_token, &addr_spec);
	if (r != SMTP_NO_ERROR)
		return r;

	r = smtp_greater_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		free (addr_spec);
		DEBUG_SMTP (SMTP_MEM,
			    "smtp_angle_addr_parse: FREE pointer=%p\n",
			    addr_spec);
		return r;
	}

	*result = addr_spec;
	*index = cur_token;

	return SMTP_NO_ERROR;
}

/*
group           =       display-name ":" [mailbox-list / CFWS] ";"
                        [CFWS]
*/

int
smtp_group_parse (const char *message, size_t length,
		  size_t * index, struct smtp_group **result)
{
	size_t cur_token;
	char *display_name;
	struct smtp_mailbox_list *mailbox_list;
	struct smtp_group *group;
	int r;
	int res;

	cur_token = *index;

	mailbox_list = NULL;

	r = smtp_display_name_parse (message, length, &cur_token,
				     &display_name);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	r = smtp_colon_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free_display_name;
	}

	r = smtp_mailbox_list_parse (message, length, &cur_token,
				     &mailbox_list);
	switch (r) {
	case SMTP_NO_ERROR:
		break;
	case SMTP_ERROR_PARSE:
		r = smtp_cfws_parse (message, length, &cur_token);
		if ((r != SMTP_NO_ERROR) && (r != SMTP_ERROR_PARSE))
			return r;
		break;
	default:
		return r;
	}

	r = smtp_semi_colon_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free_mailbox_list;
	}

	group = smtp_group_new (display_name, mailbox_list);
	if (group == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free_mailbox_list;
	}

	*index = cur_token;
	*result = group;

	return SMTP_NO_ERROR;

      free_mailbox_list:
	smtp_mailbox_list_free (mailbox_list);
      free_display_name:
	DEBUG_SMTP (SMTP_MEM, "BEFORE smtp_display_name_free\n");
	smtp_display_name_free (display_name);
      err:
	return res;
}

/*
display-name    =       phrase
*/

int
smtp_display_name_parse (const char *message, size_t length,
			 size_t * index, char **result)
{
	return smtp_phrase_parse (message, length, index, result);
}

/*
mailbox-list    =       (mailbox *("," mailbox)) / obs-mbox-list
*/

int
smtp_mailbox_list_parse (const char *message, size_t length,
			 size_t * index, struct smtp_mailbox_list **result)
{
	size_t cur_token;
	clist *list;
	struct smtp_mailbox_list *mailbox_list;
	int r;
	int res;

	cur_token = *index;

	r = smtp_struct_list_parse (message, length,
				    &cur_token, &list, ',',
				    (smtp_struct_parser *)
				    smtp_mailbox_parse,
				    (smtp_struct_destructor *)
				    smtp_mailbox_free);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	mailbox_list = smtp_mailbox_list_new (list);
	if (mailbox_list == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free_list;
	}

	*result = mailbox_list;
	*index = cur_token;

	return SMTP_NO_ERROR;

free_list:
	clist_foreach (list, (clist_func) smtp_mailbox_free, NULL);
	clist_free (list);
err:
	return res;
}

/*
address-list    =       (address *("," address)) / obs-addr-list
*/


int
smtp_address_list_parse (const char *message, size_t length,
			 size_t * index, struct smtp_address_list **result)
{
	size_t cur_token;
	clist *list;
	struct smtp_address_list *address_list;
	int r;
	int res;

	cur_token = *index;

	r = smtp_struct_list_parse (message, length,
				    &cur_token, &list, ',',
				    (smtp_struct_parser *)
				    smtp_address_parse,
				    (smtp_struct_destructor *)
				    smtp_address_free);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	address_list = smtp_address_list_new (list);
	if (address_list == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free_list;
	}

	*result = address_list;
	*index = cur_token;

	return SMTP_NO_ERROR;

free_list:
	clist_foreach (list, (clist_func) smtp_address_free, NULL);
	clist_free (list);
err:
	return res;
}

/*
addr-spec       =       local-part "@" domain
*/


int
smtp_addr_spec_parse (const char *message, size_t length,
		      size_t * index, char **result)
{
	size_t cur_token;
	char *addr_spec;
	int r;
	int res;
	size_t begin;
	size_t end;
	int final;
	size_t count;
	const char *src;
	char *dest;
	size_t i;

	cur_token = *index;
	DEBUG_SMTP (SMTP_DBG, "smtp_addr_spec_parse\n");

	DEBUG_SMTP (SMTP_DBG, "message+cur_token: %s\n", message + cur_token);

	r = smtp_cfws_parse (message, length, &cur_token);
	if ((r != SMTP_NO_ERROR) && (r != SMTP_ERROR_PARSE)) {
		DEBUG_SMTP (SMTP_DBG, "\n");
		res = r;
		goto err;
	}

	//mail without recipients
	if (memcmp (message + cur_token,
		    "<Undisclosed-Recipient:;>",
		    strlen ("<Undisclosed-Recipient:;>")) == 0) {
		*result = NULL;
		*index = cur_token + strlen ("<Undisclosed-Recipient:;>");
		return SMTP_NO_ERROR;
	}

	end = cur_token;
	if (end >= length) {
		res = SMTP_ERROR_PARSE;
		goto err;
	}

	begin = cur_token;

	final = FALSE;
	while (1) {
		switch (message[end]) {
		case '>':
		case ',':
		case '\r':
		case '\n':
		case '(':
		case ')':
		case ':':
		case ';':
			final = TRUE;
			break;
		}

		if (final)
			break;

		end++;
		if (end >= length)
			break;
	}

	if (end == begin) {
		res = SMTP_ERROR_PARSE;
		goto err;
	}

	addr_spec = malloc (end - cur_token + 1);
	if (addr_spec == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto err;
	}
	DEBUG_SMTP (SMTP_MEM, "smtp_addr_spec_parse: MALLOC pointer=%p\n",
		    addr_spec);

	count = end - cur_token;
	src = message + cur_token;

	dest = addr_spec;
	for (i = 0; i < count; i++) {
		if ((*src != ' ') && (*src != '\t')) {
			*dest = *src;
			dest++;
		}
		src++;
	}
	*dest = '\0';
	DEBUG_SMTP (SMTP_DBG, "addr_spec = %s\n", addr_spec);
	DEBUG_SMTP (SMTP_DBG, "\n");

#if 0
	strncpy (addr_spec, message + cur_token, end - cur_token);
	addr_spec[end - cur_token] = '\0';
#endif

	cur_token = end;

	DEBUG_SMTP (SMTP_DBG, "\n");
#if 0
	r = smtp_local_part_parse (message, length, &cur_token, &local_part);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	r = smtp_at_sign_parse (message, length, &cur_token);
	switch (r) {
	case SMTP_NO_ERROR:
		r = smtp_domain_parse (message, length, &cur_token, &domain);
		if (r != SMTP_NO_ERROR) {
			res = r;
			goto free_local_part;
		}
		break;

	case SMTP_ERROR_PARSE:
		domain = NULL;
		break;

	default:
		res = r;
		goto free_local_part;
	}

	if (domain) {
		addr_spec =
			malloc (strlen (local_part) + strlen (domain) + 2);
		if (addr_spec == NULL) {
			res = SMTP_ERROR_MEMORY;
			goto free_domain;
		}
		DEBUG_SMTP (SMTP_MEM,
			    "smtp_addr_spec_parse: MALLOC pointer=%p\n",
			    addr_spec);

		strcpy (addr_spec, local_part);
		strcat (addr_spec, "@");
		strcat (addr_spec, domain);

		smtp_domain_free (domain);
		smtp_local_part_free (local_part);
	}
	else {
		addr_spec = local_part;
	}
#endif

	*result = addr_spec;
	*index = cur_token;

	return SMTP_NO_ERROR;

#if 0
      free_domain:
	smtp_domain_free (domain);
      free_local_part:
	smtp_local_part_free (local_part);
#endif
err:
	return res;
}

/*
local-part      =       dot-atom / quoted-string / obs-local-part
*/

int
smtp_local_part_parse (const char *message, size_t length,
		       size_t * index, char **result)
{
	int r;

	r = smtp_dot_atom_parse (message, length, index, result);
	switch (r) {
	case SMTP_NO_ERROR:
		return r;
	case SMTP_ERROR_PARSE:
		break;
	default:
		return r;
	}

	r = smtp_quoted_string_parse (message, length, index, result);
	if (r != SMTP_NO_ERROR)
		return r;

	return SMTP_NO_ERROR;
}

/*
domain          =       dot-atom / domain-literal / obs-domain
*/
int
smtp_domain_parse (const char *message, size_t length,
		   size_t * index, char **result)
{
	int r;

	r = smtp_dot_atom_parse (message, length, index, result);
	switch (r) {
	case SMTP_NO_ERROR:
		return r;
	case SMTP_ERROR_PARSE:
		break;
	default:
		return r;
	}

	r = smtp_domain_literal_parse (message, length, index, result);
	if (r != SMTP_NO_ERROR)
		return r;

	return SMTP_NO_ERROR;
}

/*
[FWS] dcontent
*/

int
smtp_domain_literal_fws_dcontent_parse (const char *message, size_t length,
					size_t * index)
{
	size_t cur_token;
	char ch;
	int r;

	cur_token = *index;

	r = smtp_cfws_parse (message, length, &cur_token);
	if ((r != SMTP_NO_ERROR) && (r != SMTP_ERROR_PARSE))
		return r;

	r = smtp_dcontent_parse (message, length, &cur_token, &ch);
	if (r != SMTP_NO_ERROR)
		return r;

	*index = cur_token;

	return SMTP_NO_ERROR;
}

/*
domain-literal  =       [CFWS] "[" *([FWS] dcontent) [FWS] "]" [CFWS]
*/
int
smtp_domain_literal_parse (const char *message, size_t length,
			   size_t * index, char **result)
{
	size_t cur_token;
	int len;
	int begin;
	char *domain_literal;
	int r;

	cur_token = *index;

	r = smtp_cfws_parse (message, length, &cur_token);
	if ((r != SMTP_NO_ERROR) && (r != SMTP_ERROR_PARSE))
		return r;

	begin = cur_token;
	r = smtp_obracket_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR)
		return r;

	while (1) {
		r = smtp_domain_literal_fws_dcontent_parse (message, length,
							    &cur_token);
		if (r == SMTP_NO_ERROR) {
			/* do nothing */
		}
		else if (r == SMTP_ERROR_PARSE)
			break;
		else
			return r;
	}

	r = smtp_fws_parse (message, length, &cur_token);
	if ((r != SMTP_NO_ERROR) && (r != SMTP_ERROR_PARSE))
		return r;

	r = smtp_cbracket_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR)
		return r;

	len = cur_token - begin;

	domain_literal = malloc (len + 1);
	if (domain_literal == NULL)
		return SMTP_ERROR_MEMORY;
	DEBUG_SMTP (SMTP_MEM,
		    "smtp_domain_literal_parse: MALLOC pointer=%p\n",
		    domain_literal);

	strncpy (domain_literal, message + begin, len);
	domain_literal[len] = '\0';

	*result = domain_literal;
	*index = cur_token;

	return SMTP_NO_ERROR;
}


/* check_relay_domains - OK/FAIL for message relaying */
int
relay_mailbox (char *mailbox)
{
	DEBUG_SMTP (SMTP_DBG, "relay_mailbox, mailbox=%s\n", mailbox);
	printf ("relay_mailbox: mailbox = %s", mailbox);
	return ( /*(*mailbox == '@') && */ (strchr (mailbox, ':') != 0));
}

#if 0
int
match_in_adlist (struct smtp_address_list *addr_list, const char *pattern)
{
	return 0;
}
#endif 

void
dump_ip_addr (unsigned int sip, unsigned int dip, unsigned int spt,
	      unsigned int dpt)
{
#define DIGIP(ip) \
        (ip) & 0xff,\
        ((ip) >> 8)  & 0xff,\
        ((ip) >> 16) & 0xff,\
        ((ip) >> 24) & 0xff

	printf ("TCP Info: (%d.%d.%d.%d, %d)-->(%d.%d.%d.%d, %d)\n",
		DIGIP (sip), ntohs (spt), DIGIP (dip), ntohs (dpt));

}

int
smtp_wsp_addr_spec_parse (const char *message, size_t length,
			  size_t * index, char **result)
{
	size_t cur_token;
	char *addr_spec;
	int r;
	int res;
	size_t begin;
	size_t end;
	int final;
	size_t count;
	const char *src;
	char *dest;
	size_t i;

	DEBUG_SMTP (SMTP_DBG, "smtp_addr_spec_parse\n");
	cur_token = *index;
	begin = end = cur_token;

	final = FALSE;
	while (1) {
		switch (message[end]) {
		case '>':
		case ',':
		case '\r':
		case '\n':
		case '(':
		case ')':
		case ':':
		case ';':
			final = TRUE;
			break;
		}

		if (final)
			break;

		end++;
		if (end >= length)
			break;
	}

	if (end == begin) {
		res = SMTP_ERROR_PARSE;
		goto err;
	}

	addr_spec = malloc (end - cur_token + 1);
	if (addr_spec == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto err;
	}
	DEBUG_SMTP (SMTP_MEM,
		    "smtp_wsp_addr_spec_parse: MALLOC pointer=%p\n",
		    addr_spec);

	count = end - cur_token;
	src = message + cur_token;
	dest = addr_spec;
	for (i = 0; i < count; i++) {
		if ((*src != ' ') && (*src != '\t')) {
			*dest = *src;
			dest++;
		}
		src++;
	}
	*dest = '\0';
	DEBUG_SMTP (SMTP_DBG, "\n");

	cur_token = end;

	DEBUG_SMTP (SMTP_DBG, "\n");

	*result = addr_spec;
	*index = cur_token;

	return SMTP_NO_ERROR;

err:
	return res;
}


int
smtp_wsp_angle_addr_parse (const char *message, size_t length,
			   size_t * index, char **result)
{
	size_t cur_token;
	char *addr_spec;
	int r;

	cur_token = *index;

	DEBUG_SMTP (SMTP_DBG, "\n");
	r = smtp_wsp_lower_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR)
		return r;

	DEBUG_SMTP (SMTP_DBG, "\n");
	r = smtp_wsp_addr_spec_parse (message, length, &cur_token,
				      &addr_spec);
	if (r != SMTP_NO_ERROR) {
		DEBUG_SMTP (SMTP_DBG, "\n");
		return r;
	}

	DEBUG_SMTP (SMTP_DBG, "\n");
	r = smtp_wsp_greater_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		DEBUG_SMTP (SMTP_DBG, "\n");
		free (addr_spec);
		DEBUG_SMTP (SMTP_MEM,
			    "smtp_wsp_angle_addr_parse: FREE pointer=%p\n",
			    addr_spec);
		return r;
	}

	*result = addr_spec;
	*index = cur_token;

	return SMTP_NO_ERROR;
}

int
smtp_wsp_display_name_parse (const char *message, size_t length,
			     size_t * index, char **result)
{
	return smtp_wsp_phrase_parse (message, length, index, result);
}



int
smtp_wsp_name_addr_parse (const char *message, size_t length,
			  size_t * index,
			  char **pdisplay_name, char **pangle_addr)
{
	char *display_name;
	char *angle_addr;
	size_t cur_token;
	int r;
	int res;

	cur_token = *index;

	display_name = NULL;
	angle_addr = NULL;

	r = smtp_wsp_display_name_parse (message, length, &cur_token,
					 &display_name);
	if ((r != SMTP_NO_ERROR) && (r != SMTP_ERROR_PARSE)) {
		DEBUG_SMTP (SMTP_DBG, "\n");
		display_name = NULL;
		res = r;
		goto err;
	}

	DEBUG_SMTP (SMTP_DBG, "\n");
	r = smtp_wsp_angle_addr_parse (message, length, &cur_token,
				       &angle_addr);
	if (r != SMTP_NO_ERROR) {
		DEBUG_SMTP (SMTP_DBG, "\n");
		res = r;
		goto free_display_name;
	}

	*pdisplay_name = display_name;
	*pangle_addr = angle_addr;
	*index = cur_token;

	return SMTP_NO_ERROR;

free_display_name:
	if (display_name != NULL) {
		DEBUG_SMTP (SMTP_MEM,
			    "BEFORE 635 smtp_display_name_free\n");
		smtp_display_name_free (display_name);
	}
err:
	return res;
}



int
smtp_wsp_mailbox_parse (const char *message, size_t length,
			size_t * index, struct smtp_mailbox **result)
{
	size_t cur_token;
	char *display_name;
	struct smtp_mailbox *mailbox;
	char *addr_spec;
	int r;
	int res;

	cur_token = *index;
	display_name = NULL;
	addr_spec = NULL;

	r = smtp_wsp_name_addr_parse (message, length, &cur_token,
				      &display_name, &addr_spec);
	if (r == SMTP_ERROR_PARSE) {
		r = smtp_wsp_addr_spec_parse (message, length, &cur_token,
					      &addr_spec);
	}

	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	mailbox = smtp_mailbox_new (display_name, addr_spec);
	if (mailbox == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free;
	}

	*result = mailbox;
	*index = cur_token;

	return SMTP_NO_ERROR;

free:
	if (display_name != NULL) {
		DEBUG_SMTP (SMTP_MEM,
			    "BEFORE 683 smtp_display_name_free\n");
		smtp_display_name_free (display_name);
	}
	if (addr_spec != NULL)
		smtp_addr_spec_free (addr_spec);
err:
	return res;
}


int
smtp_wsp_mailbox_list_parse (const char *message, size_t length,
			     size_t * index,
			     struct smtp_mailbox_list **result)
{
	size_t cur_token;
	clist *list;
	struct smtp_mailbox_list *mailbox_list;
	int r;
	int res;

	cur_token = *index;

	r = smtp_struct_list_parse (message, length,
				    &cur_token, &list, ',',
				    (smtp_struct_parser *)
				    smtp_wsp_mailbox_parse,
				    (smtp_struct_destructor *)
				    smtp_mailbox_free);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	mailbox_list = smtp_mailbox_list_new (list);
	if (mailbox_list == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free_list;
	}

	*result = mailbox_list;
	*index = cur_token;

	return SMTP_NO_ERROR;

free_list:
	clist_foreach (list, (clist_func) smtp_mailbox_free, NULL);
	clist_free (list);
err:
	return res;
}


void
smtp_mailbox_addr_free (char *addr)
{
	free_mem (&smtp_mailbox_addr_pool, addr);
	DEBUG_SMTP (SMTP_MEM, "smtp_mailbox_addr_free: pointer=%p, elm=%p\n",
		    &smtp_mailbox_addr_pool, addr);
}


char *
mailbox_addr_dup (char *addr_spec)
{

	int len;
	DEBUG_SMTP (SMTP_DBG, "mailbox_addr_dup\n");
	DEBUG_SMTP (SMTP_DBG, "addr_spec = %p\n", addr_spec);
	DEBUG_SMTP (SMTP_DBG, "addr_spec = %s\n", addr_spec);
	DEBUG_SMTP (SMTP_DBG, "addr_spec[11] = %c\n", addr_spec[11]);
	DEBUG_SMTP (SMTP_DBG, "addr_spec[12] = %c\n", addr_spec[12]);
	DEBUG_SMTP (SMTP_DBG, "addr_spec[13] = %c\n", addr_spec[13]);
	len = strlen (addr_spec);
	DEBUG_SMTP (SMTP_DBG, "after strlen\n");

	char *addr = (char *) alloc_mem (&smtp_mailbox_addr_pool);
	DEBUG_SMTP (SMTP_DBG, "after alloc_mem\n");
	if (!addr) {
		DEBUG_SMTP (SMTP_DBG, "alloc_mem error\n");
		return NULL;
	}
	DEBUG_SMTP (SMTP_MEM, "mailbox_addr_dup: pointer=%p, elm=%p\n",
		    &smtp_mailbox_addr_pool, addr);
	memcpy (addr, addr_spec, len);
	addr[len] = '\0';
	return addr;
}
