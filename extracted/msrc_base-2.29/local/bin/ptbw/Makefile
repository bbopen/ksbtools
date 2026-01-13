# $Id: Makefile,v 1.8 2008/08/25 14:28:28 ksb Exp $
#
# Master source Makefile for ptbw (phantom toll booth weapon?), version 2008
INTO=	/usr/src/local/bin/ptbw

GEN=	
SOURCE=	Makefile Makefile.host README TODO \
	preg.ksh ptbc.html ptbw.html ptbw.man \
	ptbc.m ptbw.m machine.c machine.h \
	util_nproc.m util_nproc.mc util_nproc.mh util_nproc.mi

clean: FRC
	rm -f ${GEN}

source: ${SOURCE} ${GEN}

${SOURCE}:
	co -q $@

FRC:

# msrc patch to get up-to-date and checked-out sources
__msrc: source
