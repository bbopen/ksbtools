# $Id: Makefile,v 2.6 2008/08/11 13:21:30 ksb Exp $
#	Meta-Makefile for rmfile
INTO=	/usr/src/local/bin/rmfile

GEN=	
SOURCE=	Makefile Makefile.host machine.h rmfile.m rmfile.man

clean: FRC
	rm -f ${GEN}

source: ${SOURCE} ${GEN}

${SOURCE}:
	co -q $@

FRC:

# msrc patch to get up-to-date and checked-out sources
__msrc: source
