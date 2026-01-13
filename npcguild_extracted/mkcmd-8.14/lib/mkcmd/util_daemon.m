#!mkcmd
# $Id: util_daemon.m,v 8.9 1998/11/28 17:24:18 ksb Exp $
#
from '<sys/types.h>'
from '<sys/time.h>'
from '<sys/param.h>'
from '<fcntl.h>'
from '<signal.h>'
from '<stdio.h>'

require "util_sigret.m"
require "util_daemon.mc" "util_daemon.mh"

%i
#if POSIX
#include <limits.h>
#include <unistd.h>
#endif

#if NEED_GNU_TYPES
/* ugly LINUX hack */
#include <gnu/types.h>
#endif

#if HAVE_SELECT_H
#include <sys/select.h>
#endif
%%
