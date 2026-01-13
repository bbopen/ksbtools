#!mkcmd
# $Id: util_errno.m,v 8.12 1998/09/25 14:46:11 ksb Exp $
# Provide "extern int errno;" and include <errno.h> -- ksb
# This is _wrong_ on some PC compilers, fix it here once.
from '<errno.h>'

%i
extern int errno;
%%
