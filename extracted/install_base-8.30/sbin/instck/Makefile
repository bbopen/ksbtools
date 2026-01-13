# $Id: Makefile,v 8.11 2009/12/10 18:41:30 ksb Exp $
# Meta-Makefile for instck
INTO=	/usr/src/local/sbin/instck

GEN=
SOURCE=	Makefile Makefile.host README INSTALL TODO mktOLD \
	owners2instck.pl fixother.pl override.m \
	backups.c backups.h gen.c gen.h instck.man instck.c instck.h \
	instck.m instck.mh links.c links.h magic.c magic.h \
	maxfreq.c maxfreq.h old.c old.h path.c path.h

clean: FRC
	rm -f ${GEN}

source: ${SOURCE} ${GEN}

${SOURCE}:
	co -q $@

FRC:

# msrc patch to get up-to-date and checked-out sources
__msrc: source
