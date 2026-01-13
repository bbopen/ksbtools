# $Id: Makefile,v 1.2 2008/12/16 21:25:14 anderson Exp $
# msrc support for a plain file directory
INTO=/usr/src/local/lib/HXMD.pm/lib
IGNORE= Makefile

GEN=
SOURCE=	Makefile HXMD.pm

source: ${SOURCE} ${GEN}

clean: FRC
	rm -f ${GEN}

${SOURCE}:
	co -q $@

FRC:

# msrc patch to get up-to-date and checked-out sources
__msrc: source
