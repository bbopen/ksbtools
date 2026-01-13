# $Id: Makefile,v 8.3 2008/11/17 16:02:47 ksb Exp $
# meta makefile for install.cf, msrc 2008
INTO=	/usr/src/local/lib/install.cf

SOURCE=	Makefile Makefile.host README install.cf install.cf.man

clean: FRC
	rm -f ${GEN}

source: ${SOURCE} ${GEN}

${SOURCE}:
	co -q $@

FRC:

# msrc patch to get up-to-date and checked-out sources
__msrc: source
