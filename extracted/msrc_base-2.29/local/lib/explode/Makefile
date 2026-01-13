# $Id: Makefile,v 6.12 2008/11/17 16:02:47 ksb Exp $
# meta makefile for explode lib, msrc version 2008
INTO=	/usr/src/local/lib/explode

GEN=
SOURCE=	Makefile Makefile.host README TODO index.html explode.man \
	avl.html avl.c avl.h \
	binpack.html binpack.c binpack.h \
	ckpass.html ckpass.c ckpass.h \
	dicer.html dicer.c dicer.h dicer.man \
	fdrights.html fdrights.c fdrights.h \
	identd.html identd.c identd.h \
	pipering.html pipering.c pipering.h \
	scandir.html scandir.c scandir.h \
	setenv.html setenv.c setenv.h \
	sparse.html sparse.c sparse.h \
	strcasecmp.html strcasecmp.c strcasecmp.h \
	srtunq.html srtunq.c srtunq.h \
	mk.html mk.c mk.h \
	wise.html wise.c wise.h \
	cred.html cred.c cred.h \
	unctime.html unctime.c unctime.h

clean: FRC
	rm -f ${GEN}

source: ${SOURCE} ${GEN}

${SOURCE}:
	co -q $@

FRC:

# msrc patch to get up-to-date and checked-out sources
__msrc: source
