# $Id: Makefile,v 1.8 2010/08/08 19:03:42 ksb Exp $
# master makefile for hxmd, msrc version 2008

# msrc support
INTO=/usr/src/local/sbin/hxmd

GEN=
SOURCE=	Makefile Makefile.host README TODO hreg.ksh posse.sh \
	hostdb.m hxmd.m slot.m evector.m machine.c machine.h \
	hxmd.fig hxmd.html hxmd.man

clean: FRC
	rm -f ${GEN}

source: ${SOURCE} ${GEN}

${SOURCE}:
	co -q $@

FRC:

__msrc: source
