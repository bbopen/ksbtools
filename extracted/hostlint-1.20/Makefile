# $Id: Makefile,v 1.9 2012/03/10 19:07:07 ksb Exp $
# Master-Makefile for hostlint
#
# See the documents on msrc (or mmsrc) for usage information.

# msrc macros, let msrc auto generate the file lists (SEND, MAP, IGNORE)
INTO=	/usr/src/local/libexec/hostlint

SOURCE=	Makefile Makefile.host ITO.spec README TODO hostlint.man hostlint.html \
	hostlint.ksh.host repo.m4

quit: FRC
	echo 1>&2 "Use the msrc program to select remote hosts, chain to make"

${SOURCE}:
	co -q $@

FRC:

__msrc: ${SOURCE}
