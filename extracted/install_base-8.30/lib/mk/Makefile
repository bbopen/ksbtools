# $Id: Makefile,v 5.6 2008/11/09 00:26:19 anderson Exp $
#
#	Makefile for mk-lib distriburtion, msrc 2008
INTO=/usr/src/local/lib/mk
SUBDIR= bin dot file is m map pre type
HXINCLUDE=Msrc.hxmd

OTHER=
SOURCE=	Makefile Makefile.host Msrc.hxmd README TODO ITO.spec \
	Machine.m4 mk.man
# set to -x to trace the xapply recursion
X_OPT=


# Loop-back from Msrc.hxmd, for local or remote updates, at PRE_CMD.
# Please find DOWN_CFG, THOST, TINTO, and LINTO defined in Msrc.hxmd.  --ksb
remote_descend:
	xapply ${X_OPT} 'cd %1 && msrc ${DOWN_CFG} -E HOST=${THOST} -y INTO=${TINTO}/%1 -- make source' ${SUBDIR}

local_descend:
	xapply ${X_OPT} 'cd %1 && mmsrc ${DOWN_CFG} -E HOST=${THOST} -y INTO=${LINTO}/%1 -- make source' ${SUBDIR}

${SOURCE}:
	co -u $@

__msrc: ${SOURCE}
