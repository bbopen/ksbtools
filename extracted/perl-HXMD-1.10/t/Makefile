# $Id: Makefile,v 1.2 2008/09/29 15:35:36 ksb Exp $
# msrc support for a plain file directory
INTO=/usr/src/local/lib/HXMD.pm/t
IGNORE= Makefile

GEN=
SOURCE=	Makefile 00-load.t boilerplate.t pod-coverage.t pod.t

clean: FRC
	rm -f ${GEN}

source: ${SOURCE} ${GEN}

${SOURCE}:
	co -q $@

FRC:

# msrc patch to get up-to-date and checked-out sources
__msrc: source
