# $Id: Makefile,v 3.12 2008/11/17 16:02:47 ksb Exp $
# meta makefile for libtomb, msrc 2008
INTO=/usr/src/local/lib/libtomb

GEN=	
SOURCE=	Makefile Makefile.host README TODO libtomb.man \
	libsys_r.c libsys_r.exp libtomb.exp \
	debug.c debug.h ecreat.c eopen.c erename.c etruncate.c eunlink.c \
	libtomb.c libtomb.h

clean: FRC
	rm -f ${GEN}

source: ${SOURCE} ${GEN}

${SOURCE}:
	co -q $@

FRC:

# msrc patch to get up-to-date and checked-out sources
__msrc: source
