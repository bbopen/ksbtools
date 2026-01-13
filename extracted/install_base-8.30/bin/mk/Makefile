# $Id: Makefile,v 5.10 2009/02/26 18:51:50 ksb Exp $
# meta makefile for mk, msrc 2008
INTO=	/usr/src/local/bin/mk

GEN=
SOURCE=	Makefile Makefile.host INSTALL README TODO ITO.spec \
	mk.html expand.html mk.man \
	machine.h mk.c mk.h mk.m match.c match.h \
	rcsname.c rlimsys.c rlimsys.h

clean: FRC
	rm -f ${GEN}

source: ${SOURCE} ${GEN}

${SOURCE}:
	co -q $@

FRC:

# msrc patch to get up-to-date and checked-out sources
__msrc: source
