# $Id: Makefile,v 6.5 2008/08/12 21:19:49 ksb Exp $
#
# Meta-Makefile for explode version 2008
INTO=	/usr/src/local/bin/explode

GEN=
SOURCE=	Makefile Makefile.host TODO explode.man \
	explode.c explode.h explode.m machine.h test.c

clean: FRC
	rm -f ${GEN} main.c main.h

source: ${SOURCE} ${GEN}

${SOURCE}:
	co -q $@

FRC:

# msrc patch to get up-to-date and checked-out sources
__msrc: source
