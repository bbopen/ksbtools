`# GNU Makefile for the generic GNU auto configured program		(ab)
'divert(-1)dnl
dnl $Id: gnumake.m4,v 1.5 2009/10/14 20:16:30 ksb Exp $
dnl from <ab@purdue.edu> with stuff from ksb mixed in.
dnl
dnl Include this file from Makefile.host with some optional defines
dnl for the m4 markup and some make macro definitions for the key
dnl macros documented below the diversion.
dnl
dnl The m4 macros {PRE,POST}_{ALL,CLEAN,CONFIGURE,INSTALL} are used to
dnl hook a shell command into the make recipe.  Usually to run patch, or
dnl some local policy tune.  The make environment is used to tune other
dnl options ar run-time.  If you want to use the tar file to set the
dnl make macros just pull it apart with string operations.
dnl
dnl You may need to modify the settings below, but the defaults usually work:
dnl Override them by defining each before you include this file.
dnl
dnl Build the archive name from the run-time make macros, by default
ifdef(`DEFAULTARCHIVE',`',
	`define(`DEFAULTARCHIVE',`${PRODUCT}-${VERSION}.${ARCHIVER}${COMPRESSION}')')dnl
dnl
dnl Set some common default configure options, could be site policy from hxmd
ifdef(`DEFAULTCONFIGUREOPTIONS',`',
	`define(`DEFAULTCONFIGUREOPTIONS',`--prefix=/usr/local --sharedstatedir=/com --localstatedir=/var')')dnl
dnl
dnl The local directory that unpacking the archive builds.
ifdef(`DEFAULTBASE',`',
	`define(`DEFAULTBASE',`${PRODUCT}-${VERSION}')')dnl
dnl
dnl The archive target could fetch a missing archive, we fail by default.
ifdef(`PRE_ARCHIVE',`',
	`define(`PRE_ARCHIVE',`echo 1>&2 "missing `$'@"; ${false:-false}')')dnl
dnl
dnl We leave you the body in diversion 4, you can prefix what you like
dnl before or after it.  The make macros are in diversion 2.
dnl Build a "targets.m4" file that diverts to -1 to remove the wildcard
dnl target, if you like.
dnl					-- ksb and ab, Dec 2008
divert(2)dnl
`# Set these before including this file (use the example values for clues):
#PRODUCT=grep		# product name
#VERSION=2.5.3		# version we unpack
#ARCHIVER=tar		# format of the archive
#COMPRESSION=.gz	# compression extension
#CONFIGUREOPTIONS=	# passed to configure
#CONFIGUREENV=		# NAME=Values for the configure environment execution

BASE='defn(`DEFAULTBASE')`
ARCHIVE='defn(`DEFAULTARCHIVE')`
'divert(4)
`# Check these - sometimes you need to add more than they give you, or
# you might need to set hxmd macros to tune pre/post commands.
all: ${BASE}/Makefile
'ifdef(`PRE_ALL',`	PRE_ALL
')dnl
`	${MAKE} -C ${BASE} $@
'ifdef(`POST_ALL',`	POST_ALL
')dnl
`
${BASE}/Makefile: ${BASE}/configure | ${BASE}
'ifdef(`PRE_CONFIGURE',`	PRE_CONFIGURE
')dnl
`	cd ${BASE} && ${CONFIGUREENV} ./configure 'defn(`DEFAULTCONFIGUREOPTIONS')` ${CONFIGUREOPTIONS}
'ifdef(`POST_CONFIGURE',`	POST_CONFIGURE
')dnl
`

${BASE}/configure: | ${BASE}

${BASE} ${BASE}/configure: | ${ARCHIVE}
	${ARCHIVER} -xaf ${ARCHIVE}

clean: ${BASE}/Makefile
'ifdef(`PRE_CLEAN',`	PRE_CLEAN
')dnl
`	${MAKE} -C ${BASE} $@
'ifdef(`POST_CLEAN',`	POST_CLEAN
')dnl
`
install: all
'ifdef(`PRE_INSTALL',`	PRE_INSTALL
')dnl
	ifdef(`SUDO',`SUDO') `${MAKE} -C ${BASE} $@
'ifdef(`POST_INSTALL',`	POST_INSTALL
')dnl
`
distrib:
'ifdef(`PRE_DISTRIB',`	PRE_DISTRIB
')dnl
	ifdef(`SUDO',`SUDO') `distrib ${HOSTS}
'ifdef(`POST_DISTRIB',`	POST_DISTRIB
')dnl
`
'sinclude(targets.m4)dnl
`
# We remove the wildcard for the archive with this target
${ARCHIVE}:
'ifdef(`PRE_ARCHIVE',`	PRE_ARCHIVE
')dnl
`
# Anything else we just pass through, after configure is done.
# N.B.: This makes a circular depend which is then ignored safely --ab
%: | ${BASE}/Makefile
	${MAKE} -C ${BASE} $@

.PHONY: all clean install distrib
'dnl
