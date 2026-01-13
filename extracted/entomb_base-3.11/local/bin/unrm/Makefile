# $Id: Makefile,v 3.16 2008/10/22 23:49:20 ksb Exp $
# meta makefile for unrm, msrc 2008
INTO=	/usr/src/local/bin/unrm

GEN=
SOURCE=	Makefile Makefile.host TODO breakargs.c breakargs.h \
	changedir.c changedir.h findfiles.c findfiles.h glob.c glob.h \
	init.c init.h lists.c lists.h listtombs.c listtombs.h ls.c ls.h \
	machine.h purge.c purge.h restore.c restore.h show.c show.h \
	unrm.man unrm.m

clean: FRC
	rm -f ${GEN}

source: ${SOURCE} ${GEN}

${SOURCE}:
	co -q $@

FRC:

# msrc patch to get up-to-date and checked-out sources
__msrc: source
