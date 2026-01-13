# $Id: Makefile,v 8.11 2008/08/12 21:17:21 ksb Exp $
#
# Meta-Makefile for mkcmd
INTO=	/usr/src/local/bin/mkcmd

GEN=
SOURCE=	Makefile Makefile.host README TODO Changes \
	mkcmd.m scan.m mkcmd.mh \
	atoc.c atoc.h check.c check.h emit.c emit.h \
	key.c key.h list.c list.h machine.h \
	main.c main.h mkcmd.man mkcmd.c mkcmd.h \
	mkman.c mkusage.h mkusage.c \
	option.c option.h parser.c parser.h routine.c routine.h \
	type.c type.h scan.h scan.c stracc.c stracc.h

clean: FRC
	rm -f ${GEN}

source: ${SOURCE} ${GEN}

${SOURCE}:
	co -q $@

FRC:

# msrc patch to get up-to-date and checked-out sources
__msrc: source
