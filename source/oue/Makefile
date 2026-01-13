# $Id: Makefile,v 2.8 2008/11/16 04:52:53 ksb Exp $
# master source Makefile for oue, msrc 2008
INTO=	/usr/src/local/bin/oue

GEN=
SOURCE=	Makefile Makefile.host README TODO ITO.spec oue.html ouereg.sh \
	oue.man machine.h oue.m

clean: FRC
	rm -f ${GEN}

source: ${SOURCE} ${GEN}

${SOURCE}:
	co -q $@

FRC:

# msrc patch to get up-to-date and checked-out sources
__msrc: source
