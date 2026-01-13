# $Id: Makefile,v 2.9 2009/12/10 17:19:07 ksb Exp $
# meta makefile for installus
INTO=	/usr/src/local/sbin/installus

GEN=
SOURCE=	Makefile Makefile.host INSTALL README TODO \
	Installus.pl mkowner.ksh Owners owners.man installus.man \
	argvutil.c argvutil.h control.c control.h glob.c glob.h \
	installus.m machine.c machine.h sendfd.c sendfd.h \

clean: FRC
	rm -f ${GEN}

source: ${SOURCE} ${GEN}

${SOURCE}:
	co -q $@

FRC:

# msrc patch to get up-to-date and checked-out sources
__msrc: source
