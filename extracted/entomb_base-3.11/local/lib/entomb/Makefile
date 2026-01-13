# $Id: Makefile,v 3.11 2008/11/17 16:02:47 ksb Exp $
# Meta makefile for entomb, msrc 2008
INTO=/usr/src/local/lib/entomb

GEN=
SOURCE=	Makefile Makefile.host entomb.man \
	entomb.c entomb.h entomb.m fslist.c fslist.h machine.h

clean: FRC
	rm -f ${GEN}

source: ${SOURCE} ${GEN}

${SOURCE}:
	co -q $@

FRC:

# msrc patch to get up-to-date and checked-out sources
__msrc: source
