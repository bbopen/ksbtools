#!mkcmd
# $Id: util_rr.m,v 8.10 2008/11/15 14:18:14 ksb Exp $
# Look up A records in a round-robin return a (struct hosten *) which	(ksb)
# represents the current state, refresh only after the TTL expires.
# reference: http://www.faqs.org/rfcs/rfc1035.html
# reference: sendmail/contrib/bitdomain.c
from '<sys/types.h>'
from '<netinet/in.h>'
from '<arpa/nameser.h>'
from '<sys/socket.h>'
from '<netdb.h>'
from '<time.h>'
from '<resolv.h>'
from '<string.h>'
from '<stdio.h>'

require "util_ppm.m"
require "util_rr.mc"

%hi
extern void *util_rrwatch(char *pcRR);
extern struct hostent *util_rrpoll(void *pvHandle);
extern void util_rrrelease(void *pvHeld);
%%

%c
/* I wouldn't tune these unless I had to --ksb
 */
#if !defined(MAXKEEPADDR)
#define MAXKEEPADDR	32		/* max size on an INET address */
#endif

#if !defined(LONGKEEPTTL)
#define LONGKEEPTTL	(60*60)		/* first time-to-live we'll believe */
#endif

extern int h_errno;
%%
