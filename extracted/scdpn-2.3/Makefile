# $Id: Makefile,v 2.1 2008/11/19 21:28:26 ksb Exp $
# master source Makefile for scdpn, msrc 2008

INTO=	/usr/src/local/sbin/scdpn

GEN=
SOURCE=	Makefile Makefile.host README TODO ITO.spec \
	machine.h pcap-hpux.h scdpn.m scdpn.man

clean: FRC
	rm -f ${GEN}

source: ${SOURCE} ${GEN}

${SOURCE}:
	co -q $@

FRC:

# msrc patch to get up-to-date and checked-out sources
__msrc: source
