#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
# Thanks to and contributed by my brother at itap.purdue.edu.
# $Id: configure.ac,v 1.5 2009/11/18 19:33:00 ksb Exp $
# $Compile: ${autoreconf:-autoreconf} %f && rm -rf autom4te.cache

AC_PREREQ(2.61)
AC_INIT(mmsrc, 1.0.0, ksb@sac.fedex.com)
AC_CONFIG_SRCDIR([mmsrc.m])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_SEARCH_LIBS([socket], [socket])

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([fcntl.h malloc.h netdb.h stdlib.h string.h strings.h sys/param.h sys/socket.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_FORK
AC_FUNC_LSTAT
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_STAT
AC_CHECK_FUNCS([arc4random atexit dup2 fgetln ftruncate getcwd gethostname memmove mkdir mkdtemp rmdir snprintf strcasecmp strchr strdup strerror strncasecmp strrchr strstr])

# Make it so
AC_OUTPUT([Makefile.local])
