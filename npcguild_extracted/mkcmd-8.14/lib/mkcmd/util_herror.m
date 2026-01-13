#!mkcmd
# $Id: util_herror.m,v 8.30 1999/09/27 01:30:37 ksb Exp $
# replay herror on systems where it is missing

%i
char *h_errlist[] = {
	"Unknown 0",
	"Host not found",
	"Try again",
	"Non recoverable implementaion error",
	"No data of requested type",
	"Unknown 5"
};
%%
%h
extern char *h_errlist[6];
%%
