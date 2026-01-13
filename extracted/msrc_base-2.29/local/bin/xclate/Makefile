# $Id: Makefile,v 2.6 2008/10/18 23:04:55 ksb Exp $
# master source Makefile for xclate (say ex-c'llate)
INTO=	/usr/src/local/bin/xclate

GEN=	
SOURCE=	Makefile Makefile.host README TODO \
	xclate.html xclreg.ksh machine.h xclate.m xclate.man

clean: FRC
	rm -f ${GEN}

source: ${SOURCE} ${GEN}

${SOURCE}:
	co -q $@

FRC:

# msrc patch to get up-to-date and checked-out sources
__msrc: source
