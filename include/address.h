
#ifndef ADDRESS_H
#define ADDRESS_H

#include "clist.h"

/* this is the type of address */
enum
{
	SMTP_ADDRESS_ERROR,	/* on parse error */
	SMTP_ADDRESS_MAILBOX,	/* if this is a mailbox (mailbox@domain) */
	SMTP_ADDRESS_GROUP,	/* if this is a group
				   (group_name: address1@domain1, address2@domain2; ) */
};

/*
  smtp_address is an address
  - type can be SMTP_ADDRESS_MAILBOX or SMTP_ADDRESS_GROUP
  - mailbox is a mailbox if type is SMTP_ADDRESS_MAILBOX
  - group is a group if type is SMTP_ADDRESS_GROUP
*/
struct smtp_address
{
	int ad_type;
	union
	{
		struct smtp_mailbox *ad_mailbox;	/* can be NULL */
		struct smtp_group *ad_group;	/* can be NULL */
	} ad_data;
};

/*
  smtp_address_parse will parse the given address
  
  @param message this is a string containing the address
  @param length this is the size of the given string
  @param index this is a pointer to the start of the address in
    the given string, (* index) is modified to point at the end
    of the parsed data
  @param result the result of the parse operation is stored in
    (* result)

  @return SMTP_NO_ERROR on success, SMTP_ERROR_XXX on error
*/

int smtp_address_parse (const char *message, size_t length,
			size_t * index, struct smtp_address **result);

struct smtp_address *smtp_address_new (int ad_type,
				       struct smtp_mailbox *ad_mailbox,
				       struct smtp_group *ad_group);
void smtp_address_free (struct smtp_address *address);


/*
  smtp_mailbox is a mailbox
  - display_name is the name that will be displayed for this mailbox,
    for example 'name' in '"name" <mailbox@domain>,
    should be allocated with malloc()
  
  - addr_spec is the mailbox, for example 'mailbox@domain'
    in '"name" <mailbox@domain>, should be allocated with malloc()
*/
struct smtp_mailbox
{
	char *mb_display_name;	/* can be NULL */
	char *mb_addr_spec;	/* != NULL */
};

/*
  smtp_mailbox_parse will parse the given address
  
  @param message this is a string containing the mailbox
  @param length this is the size of the given string
  @param index this is a pointer to the start of the mailbox in
    the given string, (* index) is modified to point at the end
    of the parsed data
  @param result the result of the parse operation is stored in
    (* result)

  @return SMTP_NO_ERROR on success, SMTP_ERROR_XXX on error
*/

int smtp_mailbox_parse (const char *message, size_t length,
			size_t * index, struct smtp_mailbox **result);
struct smtp_mailbox *smtp_mailbox_new (char *mb_display_name,
				       char *mb_addr_spec);
void smtp_mailbox_free (struct smtp_mailbox *mailbox);



/*
  smtp_group is a group
  - display_name is the name that will be displayed for this group,
    for example 'group_name' in
    'group_name: address1@domain1, address2@domain2;', should be allocated
    with malloc()
  - mb_list is a list of mailboxes
*/
struct smtp_group
{
	char *grp_display_name;	/* != NULL */
	struct smtp_mailbox_list *grp_mb_list;	/* can be NULL */
};

struct smtp_group *smtp_group_new (char *grp_display_name,
				   struct smtp_mailbox_list *grp_mb_list);
void smtp_group_free (struct smtp_group *group);


/*
  smtp_mailbox_list is a list of mailboxes
  - list is a list of mailboxes
*/
struct smtp_mailbox_list
{
	clist *mb_list;		/* list of (struct smtp_mailbox *), != NULL */
};

/*
  smtp_mailbox_list_parse will parse the given mailbox list
  
  @param message this is a string containing the mailbox list
  @param length this is the size of the given string
  @param index this is a pointer to the start of the mailbox list in
    the given string, (* index) is modified to point at the end
    of the parsed data
  @param result the result of the parse operation is stored in
    (* result)

  @return SMTP_NO_ERROR on success, SMTP_ERROR_XXX on error
*/

int
smtp_mailbox_list_parse (const char *message, size_t length,
			 size_t * index, struct smtp_mailbox_list **result);
struct smtp_mailbox_list *smtp_mailbox_list_new (clist * mb_list);
void smtp_mailbox_list_free (struct smtp_mailbox_list *mb_list);


/*
  smtp_address_list is a list of addresses
  - list is a list of addresses
*/
struct smtp_address_list
{
	clist *ad_list;		/* list of (struct smtp_address *), != NULL */
};


/*
  smtp_address_list_parse will parse the given address list
  
  @param message this is a string containing the address list
  @param length this is the size of the given string
  @param index this is a pointer to the start of the address list in
    the given string, (* index) is modified to point at the end
    of the parsed data
  @param result the result of the parse operation is stored in
    (* result)

  @return SMTP_NO_ERROR on success, SMTP_ERROR_XXX on error
*/

int
smtp_address_list_parse (const char *message, size_t length,
			 size_t * index, struct smtp_address_list **result);

struct smtp_address_list *smtp_address_list_new (clist * ad_list);
void smtp_address_list_free (struct smtp_address_list *addr_list);

/*
  smtp_mailbox_list_new_empty creates an empty list of mailboxes
*/
struct smtp_mailbox_list *smtp_mailbox_list_new_empty ();

/*
  smtp_mailbox_list_add adds a mailbox to the list of mailboxes

  @return SMTP_NO_ERROR will be returned on success,
  other code will be returned otherwise
*/
int smtp_mailbox_list_add (struct smtp_mailbox_list *mailbox_list,
			   struct smtp_mailbox *mb);

/*
  smtp_mailbox_list_add_parse parse the given string
  into a smtp_mailbox structure and adds it to the list of mailboxes

  @return SMTP_NO_ERROR will be returned on success,
  other code will be returned otherwise
*/
int smtp_mailbox_list_add_parse (struct smtp_mailbox_list *mailbox_list,
				 char *mb_str);

/*
  smtp_mailbox creates a smtp_mailbox structure with the given
  arguments and adds it to the list of mailboxes

  - display_name is the name that will be displayed for this mailbox,
    for example 'name' in '"name" <mailbox@domain>,
    should be allocated with malloc()
  
  - address is the mailbox, for example 'mailbox@domain'
    in '"name" <mailbox@domain>, should be allocated with malloc()

  @return SMTP_NO_ERROR will be returned on success,
  other code will be returned otherwise
*/
int smtp_mailbox_list_add_mb (struct smtp_mailbox_list *mailbox_list,
			      char *display_name, char *address);

/*
  smtp_address_list_new_empty creates an empty list of addresses
*/
struct smtp_address_list *smtp_address_list_new_empty ();


/*
  smtp_address_list_add adds a mailbox to the list of addresses

  @return SMTP_NO_ERROR will be returned on success,
  other code will be returned otherwise
*/
int smtp_address_list_add (struct smtp_address_list *address_list,
			   struct smtp_address *addr);


/*
  smtp_address_list_add_parse parse the given string
  into a smtp_address structure and adds it to the list of addresses

  @return SMTP_NO_ERROR will be returned on success,
  other code will be returned otherwise
*/
int smtp_address_list_add_parse (struct smtp_address_list *address_list,
				 char *addr_str);


/*
  smtp_address_list_add_mb creates a mailbox smtp_address
  with the given arguments and adds it to the list of addresses

  - display_name is the name that will be displayed for this mailbox,
    for example 'name' in '"name" <mailbox@domain>,
    should be allocated with malloc()
  
  - address is the mailbox, for example 'mailbox@domain'
    in '"name" <mailbox@domain>, should be allocated with malloc()

  @return SMTP_NO_ERROR will be returned on success,
  other code will be returned otherwise
*/
int smtp_address_list_add_mb (struct smtp_address_list *address_list,
			      char *display_name, char *address);


void smtp_angle_addr_free (char *angle_addr);

void smtp_display_name_free (char *display_name);

void smtp_addr_spec_free (char *addr_spec);

void smtp_local_part_free (char *local_part);

void smtp_domain_free (char *domain);

void smtp_domain_literal_free (char *domain);

/*  
domain-literal  =       [CFWS] "[" *([FWS] dcontent) [FWS] "]" [CFWS]
*/
int smtp_domain_literal_parse (const char *message, size_t length,
			       size_t * index, char **result);


/*  
angle-addr      =       [CFWS] "<" addr-spec ">" [CFWS] / obs-angle-addr
*/

int smtp_angle_addr_parse (const char *message, size_t length,
			   size_t * index, char **result);
/*
display-name    =       phrase
*/

int smtp_display_name_parse (const char *message, size_t length,
			     size_t * index, char **result);
/*
addr-spec       =       local-part "@" domain
*/


int smtp_addr_spec_parse (const char *message, size_t length,
			  size_t * index, char **result);
/*
name-addr       =       [display-name] angle-addr
*/

int smtp_name_addr_parse (const char *message, size_t length,
			  size_t * index,
			  char **pdisplay_name, char **pangle_addr);
/*
group           =       display-name ":" [mailbox-list / CFWS] ";"
                        [CFWS]
*/

int smtp_group_parse (const char *message, size_t length,
		      size_t * index, struct smtp_group **result);

//
int smtp_wsp_display_name_parse (const char *message, size_t length,
                             size_t * index, char **result);

//
int smtp_wsp_name_addr_parse (const char *message, size_t length,
                          size_t * index,
                          char **pdisplay_name, char **pangle_addr);


void smtp_mailbox_addr_free (char *addr);

char *mailbox_addr_dup (char *addr_spec);

int
smtp_wsp_mailbox_list_parse (const char *message, size_t length,
                             size_t * index,
                             struct smtp_mailbox_list **result);

#endif
