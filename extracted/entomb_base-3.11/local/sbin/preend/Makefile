# $Id: Makefile,v 3.17 2010/08/16 16:20:19 ksb Exp $
# control makefile for preend, msrc 2009
INTO=	/usr/src/local/sbin/preend

GEN=
SOURCE=	Makefile Makefile.host TODO machine.h preend.sh preend.init \
	preend.man preend.c preend.m preentomb.c preentomb.h

clean: FRC
	rm -f ${GEN}

source: ${SOURCE} ${GEN}

${SOURCE}:
	co -q $@

FRC:

# msrc patch to get up-to-date and checked-out sources
__msrc: source
