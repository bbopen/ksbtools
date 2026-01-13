# $Id: Makefile,v 1.7 2010/12/06 15:54:55 ksb Exp $
# msrc 2008 support for the perl module that reads HXMD's config files
INTO=/usr/src/local/lib/HXMD.pm
IGNORE= Makefile
SUBDIR= lib t

GEN=
SOURCE=	Makefile Makefile.PL Makefile.host Msrc.hxmd ITO.spec \
	README MANIFEST Changes \
	HXMD.t test1.cf

clean: FRC
	rm -f ${GEN}

# set to -x to trace the xapply recursion
X_OPT=

# Loop-back from Msrc.hxmd, for local or remote updates, at PRE_CMD.
# Please find DOWN_CFG, THOST, TINTO, and LINTO defined in Msrc.hxmd.  --ksb
remote_descend:
	xapply ${X_OPT} 'cd %1 && msrc ${DOWN_CFG} -E HOST=${THOST} -y INTO=${TINTO}/%1 -- :' ${SUBDIR}

local_descend:
	xapply ${X_OPT} 'cd %1 && mmsrc ${DOWN_CFG} -E HOST=${THOST} -y INTO=${LINTO}/%1 -- :' ${SUBDIR}

source: ${SOURCE} ${GEN}

${SOURCE}:
	co -q $@

FRC:

# msrc patch to get up-to-date and checked-out sources
__msrc: source
