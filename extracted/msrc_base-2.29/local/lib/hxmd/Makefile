# $Id: Makefile,v 1.17 2010/06/18 17:26:45 ksb Exp $
# master source Makefile for hxmd-lib, msrc 2008
INTO=	/usr/src/local/lib/hxmd

# Add your local files to the macro below, and to DATA in Makefile.host
SOURCE=	Makefile Makefile.host README hxmd.man site.html auto.cf.host \
	msrc.cf class.m4 cronsup.m4 fqdn.m4 gnumake.m4 sendfile.m4

clean: FRC
	rm -f ${GEN}

source: ${SOURCE} ${GEN}

${SOURCE}:
	co -q $@

FRC:

# msrc patch to get up-to-date and checked-out sources
__msrc: source
