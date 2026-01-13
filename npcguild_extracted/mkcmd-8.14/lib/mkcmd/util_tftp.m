#!mkcmd
# $Id: util_tftp.m,v 8.16 1998/11/25 01:14:59 ksb Exp $
#
# Some of this code was taken from the BSD tftp implementation, but
# I think I recoded almost all of it. -- ksb
from '<sys/types.h>'
from '<sys/socket.h>'
from '<sys/ioctl.h>'
from '<netinet/in.h>'
from '<arpa/tftp.h>'
from '<sys/time.h>'
from '<syslog.h>'
from '<setjmp.h>'
from '<signal.h>'
from '<netdb.h>'

from '<stdio.h>'
from '<unistd.h>'

require "util_errno.m"
require "util_tftp.mh" "util_tftp.mi" "util_tftp.mc"

%i
#if !defined(LOG_FTP)
#define LOG_FTP LOG_DAEMON
#endif

#if !defined(FIONREAD)
#include <sys/filio.h>
#endif
%%

%hi
#if !defined(TFTP_PROTO)
/* See http://info.internet.isi.edu/in-notes/rfc/files/rfc783.txt,
 * this is ksb's interpretation of that standard.
 */
typedef struct	{
	ushort usopcode;			/* command		*/
	union {
		struct {
			char acname[SEGSIZE];	/* request text		*/
		} req;
		struct {
			ushort usblock;		/* block number		*/
			char acbuf[SEGSIZE];	/* payload		*/
		} data;
		struct {
			ushort usblock;		/* block number		*/
		} ack;
		struct {
			ushort uscode;		/* error code		*/
			char acmsg[SEGSIZE];	/* error msg		*/
		} err;
	} u;
} TFTP_PACKET;

#define TFTP_PROTO	783
#endif
%%
