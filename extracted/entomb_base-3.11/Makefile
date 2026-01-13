# $Id: Makefile.meta,v 3.11 2010/08/16 16:07:00 ksb Exp $
# msrc recipe for file entombing as a package
RELEASE=3.11
INTO=	/usr/src/Pkgs/entomb-${RELEASE}
SUBDIR= local/lib/libtomb local/lib/entomb local/sbin/preend \
	local/bin/unrm local/bin/untmp local/bin/rmfile
GEN=
SOURCE=	Makefile Makefile.host Msrc.hxmd INSTALL ITO.spec LICENSE README
MAKE=make


quit:
	echo "You are not in the right place to make all, for sure" 1>&2
	false

all: source FRC
	${LOOP}

# Loop-back from Msrc.hxmd, for local or remote updates, at PRE_CMD.
# Please find DOWN_CFG, THOST, TINTO, and LINTO defined in Msrc.hxmd.  --ksb
# set to -x to trace the xapply recursion
X_OPT=
remote_descend:
	xapply ${X_OPT} 'cd %1 && msrc ${DOWN_CFG} -E HOST=${THOST} -y INTO=${TINTO}/%1 -- make source' ${SUBDIR}

local_descend:
	mkdir -p ${LINTO}/lib ${LINTO}/sbin ${LINTO}/bin
	xapply ${X_OPT} 'cd %1 && mmsrc ${DOWN_CFG} -E HOST=${THOST} -y INTO=${LINTO}/%1 -- make source' ${SUBDIR}

clean: FRC
	rm -f Makefile.bak a.out core errs lint.out tags
	${LOOP}

source: ${SOURCE} ${GEN}

${SOURCE}:
	co -q $@

FRC:

# the common hook
__msrc: source
