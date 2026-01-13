#!mkcmd
# $Id: util_herror.m,v 8.32 2004/04/21 15:17:28 ksb Exp $
# replay herror on systems where it is missing

%hi
/* this belongs in machine.h, try to figure out if we need emulation
 * for error lists for host entries.
 */
#if !defined(HAVE_H_ERRLIST)
#define HAVE_H_ERRLIST	0
#endif

#if !defined(NEED_HSTRERROR)
#define NEED_HSTRERROR	0
#endif
%%

%i
#if !HAVE_H_ERRLIST
char *h_errlist[] = {
	"Unknown 0",
	"Host not found",
	"Try again",
	"Non recoverable implementaion error",
	"No data of requested type",
	"Unknown 5"
};
#endif

#if NEED_HSTRERROR
#define hstrerror(Me)   (h_errlist[Me])
#endif
%%
