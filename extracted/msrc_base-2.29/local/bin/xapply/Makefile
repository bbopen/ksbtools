# $Id: Makefile,v 3.13 2008/10/18 23:05:26 ksb Exp $
# Meta-Makefile for xapply version 2008
INTO=	/usr/src/local/bin/xapply

GEN=
SOURCE=	Makefile Makefile.host README TODO xapply.html xreg.ksh \
	fullpath.m xapply.m xapply.man xapply.spec

clean: FRC
	rm -f ${GEN}

source: ${SOURCE} ${GEN}

${SOURCE}:
	co -q $@

FRC:

# msrc patch to get up-to-date and checked-out sources
__msrc: source
