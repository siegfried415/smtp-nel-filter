PCAP_INC_DIR	= /usr/include/pcap

NIDS_INC_DIR    = /usr/local/include
NIDS_LIB_DIR	= /usr/local/lib

NEL_INC_DIR 	= /usr/local/include/nel
NEL_LIB_DIR	= /usr/local/lib

INSTALL_DIR 	= ../bin/
TARGET		= smtp-nel-filter 

C_SRCS       = 	main.c tcp.c address.c bcc.c body.c cc.c \
		comments.c content-type.c disposition.c  \
		encoding.c from.c keywords.c language.c \
		message-id.c mime.c \
		msg-id.c path.c received.c reference.c \
		reply-to.c resent-bcc.c resent-cc.c resent-date.c \
		resent-from.c resent-msg-id.c resent-sender.c \
		resent-to.c return.c sender.c smtp.c subject.c _time.c \
		to.c turn.c version.c x-token.c in_reply_to.c \
		./charconv/charconv.c \
		./chash/chash.c \
		./clist/clist.c \
		./mmapstring/mmapstring.c \
		./mem_pool/mem_pool.c 
		
RE2C_SRCS    = 	auth.c data.c expn.c fields.c mail-from.c  \
		quit.c rset.c send-from.c user.c ehlo.c helo.c \
		password.c rcpt-to.c saml-from.c \
		soml-from.c vrfy.c command.c message.c

LNET_CFLAGS  = 	-D_BSD_SOURCE -D__BSD_SOURCE -D__FAVOR_BSD \
		-DHAVE_NET_ETHERNET_H

RE2C_OBJS    =  $(RE2C_SRCS:.c=.o)
%.c	:%.re
	re2c -o $@  $<

C_OBJS       =  $(C_SRCS:.c=.o)
%.o 	:%.c
	$(CC) -g -gstabs+ -DUSE_NEL -c -o $@ \
		-I$(PCAP_INC_DIR) -I$(NIDS_INC_DIR) -I$(NEL_INC_DIR) \
		-I../include $<

${TARGET}: ${C_OBJS} ${RE2C_OBJS} 
	$(CC) -g -o $@ $(C_OBJS) $(RE2C_OBJS) -lnids -lpcap -lnet \
				-lgthread-2.0 -lglib-2.0 \
				-L$(NEL_LIB_DIR) -lnel -lnlib
		
install:${TARGET}
	cp ${TARGET} $(INSTALL_DIR)

all	:${TAEGET} 
clean:
	rm -f ${TARGET} *.o   
	rm -f $(RE2C_SRCS) 
	rm -f core
