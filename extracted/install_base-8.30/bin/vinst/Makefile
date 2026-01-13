# $Id: Makefile,v 1.9 2008/11/18 00:03:12 ksb Exp $
#
#	master source Makefile for vinst
INTO=	/usr/src/local/bin/vinst

GEN=	
SOURCE=	Makefile Makefile.host README TODO override.m \
	vmachine.h vinst.m vinst.mc vinst.mh vinst.man

clean: FRC
	rm -f ${GEN}

source: ${SOURCE} ${GEN}

${SOURCE}:
	co -q $@

FRC:

# msrc patch to get up-to-date and checked-out sources
__msrc: source
