# $Id: Makefile,v 2.14 2008/11/19 23:33:39 anderson Exp $
# Meta-Makefile for cdecl

INTO=	/usr/src/local/bin/cdecl

GEN=
SOURCE=	Makefile Makefile.host ITO.spec \
	cdecl.man gram.y machine.h scan.l support.c

clean: FRC
	rm -f ${GEN}

source: ${SOURCE} ${GEN}

${SOURCE}:
	co -q $@

FRC:

# msrc patch to get up-to-date and checked-out sources
__msrc: source
