# $Id: Makefile,v 1.12 2008/11/19 21:56:24 anderson Exp $
#
#	Meta-Makefile for untmp
INTO=	/usr/src/local/bin/untmp

GEN=	
SOURCE=	Makefile Makefile.host ITO.spec README untmp.man \
	untmp.m machine.h

clean: FRC
	rm -f ${GEN}

source: ${SOURCE} ${GEN}

${SOURCE}:
	co -q $@

FRC:

# msrc patch to get up-to-date and checked-out sources
__msrc: source
