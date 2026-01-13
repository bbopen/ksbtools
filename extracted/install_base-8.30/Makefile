# $Id: Makefile.meta,v 8.30 2010/08/13 19:59:30 ksb Exp $
# Makefile for install distriburtion, msrc 2008
RELEASE=8.30
# if you can't write in /usr/src change INTO to /tmp/$something
INTO=	/usr/src/Pkgs/install-${RELEASE}
# Listing bin and sbin allow the second level directories to be instanced here.
# Pkgs is not a directory we should look at for this process. --ksb
SUBDIR= bin/install.d lib/install.cf sbin/purge sbin/instck sbin/installus \
	bin/vinst bin/mk lib/mk bin/op
DESTDIR=
MAKE=make
#MFLAGS from command-line

GEN=
SOURCE=	Makefile Makefile.host README ITO.spec access.cf

all install:
	mmsrc -Cauto.cf make ${MFLAGS} DESTDIR=${DESTDIR} $@

clean: FRC
	mmsrc -Cauto.cf make ${MFLAGS} DESTDIR=${DESTDIR} $@
	rm -f ${GEN}

# Loop-back from Msrc.hxmd, for local or remote updates, at PRE_CMD.
# Please find DOWN_CFG, THOST, TINTO, and LINTO defined in Msrc.hxmd.  --ksb
# set to -x to trace the xapply recursion
X_OPT=
remote_descend:
	xapply ${X_OPT} 'cd %1 && msrc ${DOWN_CFG} -E HOST=${THOST} -y INTO=${TINTO}/%1 -- make source' ${SUBDIR}

local_descend:
	mkdir -p ${LINTO}/bin ${LINTO}/lib
	xapply ${X_OPT} 'cd %1 && mmsrc ${DOWN_CFG} -E HOST=${THOST} -y INTO=${LINTO}/%1 -- make source' ${SUBDIR}

source: ${SOURCE} ${GEN}

${SOURCE}:
	co -q $@

FRC:

# msrc patch to get up-to-date and checked-out sources
__msrc: source
