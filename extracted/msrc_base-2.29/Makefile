# $Id: Makefile.meta,v 2.29 2010/08/14 18:43:18 ksb Exp $
# The control recipe to upgrade the msrc_base package, or boot it. --ksb
RELEASE=2.29
INTO=  /usr/src/Pkgs/msrc_base-${RELEASE}

PREP=	local/sbin/mmsrc
BOOTSTRAP= local/lib/hxmd local/lib/mkcmd local/lib/explode \
	local/bin/mkcmd local/bin/explode
ORDER=	local/bin/ptbw local/bin/xclate local/bin/xapply \
	local/sbin/hxmd local/sbin/msrc
SUBDIR=${PREP} ${BOOTSTRAP} ${ORDER}
MAKE=make
#MFLAGS from command-line

GEN=
SOURCE=	Makefile Makefile.host README ITO.spec Msrc.hxmd \
	patch-client.c patch-setargs.c options.html

# Assuming we've been booted we can rebuild with "all", else see "boot" below.
all:
	xapply 'cd %1 && mmsrc -Cauto.cf make ${MFLAGS} $@' ${SUBDIR}

# Loop-back from Msrc.hxmd, for local or remote updates, at PRE_CMD.
# Please find DOWN_CFG, THOST, TINTO, and LINTO defined in Msrc.hxmd.  --ksb
# Set X_OPT to -x to trace the xapply recursion
X_OPT=
remote_descend:
	xapply ${X_OPT} 'cd %1 && msrc ${DOWN_CFG} -E HOST=${THOST} -y INTO=${TINTO}/%1 -- make source' ${SUBDIR}

local_descend:
	mkdir -p ${LINTO}/local/bin ${LINTO}/local/sbin ${LINTO}/local/lib
	xapply ${X_OPT} 'cd %1 && mmsrc ${DOWN_CFG} -E HOST=${THOST} -y INTO=${LINTO}/%1 -- make source' ${SUBDIR}

# If we do NOT have msrc_base already installed we have to work a little
# harder to make get going. -- ksb
#
# Self-boot depends on you setting HOSTTYPE and HOSTOS for us, which we
# record in "auto.cf".  For example for Solaris 10 I might run:
#    make DESTDIR=/tmp/ksb SRCDIR=/tmp/_src HOSTTYPE=SUN5 HOSTOS=210000 boot
#
# DESTDIR might be used to install into a jail, altenate slash or zone.
# SRCDIR helps if you can't write in /usr/src/local.  Remove it when you
#	are done with the boot, as you like.
#
# The defaults are for FreeBSD 8.0 (which I run at home --ksb)
DESTDIR=
SRCDIR=/tmp/_src
HOSTTYPE=FREEBSD
HOSTOS=80000
MMSRC=${DESTDIR}/usr/local/sbin/mmsrc
MSELF=${MMSRC} -C${DESTDIR}/usr/local/lib/hxmd/auto.cf -DHOST=`hostname`

# This should get you up the first time.  If you set SRCDIR, DESTDIR, and
# HOSTTYPE, HOSTOS as specified in local/sbin/msrc/qstart.html.
boot:
	mkdir -p ${DESTDIR}/usr/local/sbin ${DESTDIR}/usr/local/bin \
		${DESTDIR}/usr/local/lib/hxmd ${SRCDIR}/local/lib/hxmd
	${MAKE} -C local/sbin/mmsrc ${MFLAGS} DESTDIR=${DESTDIR} \
		CFLAGS="-D${HOSTTYPE} -DHOSTOS=${HOSTOS}" boot
	install -c -m 755 local/sbin/mmsrc/mmsrc ${MMSRC}
	cd local/lib/hxmd && \
	${MMSRC} -DHOST=`hostname` -DHOSTTYPE=${HOSTTYPE} -DHOSTOS=${HOSTOS} \
		-y INTO=${SRCDIR}/local/lib/hxmd  -- \
		make DESTDIR=${DESTDIR} install clean
	for d in ${BOOTSTRAP} ; do \
		mkdir -p ${SRCDIR}/$$d ;\
		(cd $$d && ${MSELF} -y INTO=${SRCDIR}/$$d -- \
			make ${MFLAGS} DESTDIR=${DESTDIR} boot clean );\
	done
	for d in ${ORDER} ; do \
		mkdir -p ${SRCDIR}/$$d ;\
		(cd $$d && ${MSELF} -y INTO=${SRCDIR}/$$d -- \
			make ${MFLAGS} DESTDIR=${DESTDIR} install clean );\
	done

# To build the RPM use level2s if you have it (downloaded)
#	# level2s rpm msrc_base-2.29.tgz
# else use this in-line version from the directory with the tgz file:
#	# tar zxf msrc_base-2.29.tgz msrc_base-2.29/Makefile
#	# make -f msrc_base-2.29/Makefile rpm-boot
N1=msrc_base
B1=${N1}-${RELEASE}
${B1}/ITO.spec: ${B1}.tgz
	tar xzf ${B1}.tgz ${B1}/ITO.spec

rpm-boot: ${B1}/ITO.spec
	chown root:root ${B1}.tgz ${B1}/ITO.spec
	cp ${B1}/ITO.spec /usr/src/redhat/SPECS/${N1}.spec
	rm -f ${B1}/Makefile ${B1}/ITO.spec
	rm -f /usr/src/redhat/SOURCES/${N1}-*.*
	mv ${B1}.tgz /usr/src/redhat/SOURCES/
	rpmbuild -ba $RPM_ADD /usr/src/redhat/SPECS/${N1}.spec
	rpm -U /usr/src/redhat/RPMS/`uname -i`/${B1}*.rpm
	-rmdir ${B1}

source: ${SOURCE} ${GEN}

${SOURCE}:
	co -q $@

FRC:

# msrc patch to get up-to-date and checked-out sources
__msrc: source
