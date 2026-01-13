# $Id: Makefile,v 2.3 2008/12/22 18:41:18 ksb Exp $
# meta makefile for op, msrc version 2008
INTO=	/usr/src/local/bin/op

GEN=
SOURCE=	Makefile Makefile.host TODO README ITO.spec \
	op.man op.html refs.html showme.sh jacket.pl \
	op.m machine.h

clean: FRC
	rm -f ${GEN}

source: ${SOURCE} ${GEN}

${SOURCE}:
	co -q $@

FRC:

# msrc patch to get up-to-date and checked-out sources
__msrc: source
