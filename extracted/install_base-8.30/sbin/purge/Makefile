# $Id: Makefile,v 8.10 2009/12/10 17:16:10 ksb Exp $
# meta makefile for purge, msrc 2008
INTO=	/usr/src/local/sbin/purge

GEN=
SOURCE=	Makefile Makefile.host README TODO purge.man \
	filedup.c filedup.h purge.c purge.h purge.m

clean: FRC
	rm -f ${GEN}

source: ${SOURCE} ${GEN}

${SOURCE}:
	co -q $@

FRC:

# msrc patch to get up-to-date and checked-out sources
__msrc: source
