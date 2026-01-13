# $Id: Makefile,v 3.13 2010/08/16 16:14:14 ksb Exp $
# Pkg level makefile for file entombing
INTO= _This _is _a _package _makefile, _it _cannot _be _pushed.

MSRC=/usr/msrc
TMP=/tmp
VERSION=3.11
STAGE=${TMP}/entomb_base-${VERSION}
VGOPTS=
SOURCE= Makefile Makefile.meta Makefile.host Msrc.hxmd ITO.spec \
	README INSTALL

quit:
	echo "You are not in the right place to make all, for sure" 1>&2
	false

all: msource

${TMP}:
	echo "Cannot find ${TMP}, which we will not build."
	false

${TMP}/entomb_base-${VERSION}.tgz: ${TMP} stage
	cd ${TMP} && tar zcf entomb_base-${VERSION}.tgz entomb_base-${VERSION}

${TMP}/entomb_base-${VERSION}.tbz: ${TMP} stage
	cd ${TMP} && tar cf - entomb_base-${VERSION} |bzip2 -9 >entomb_base-${VERSION}.tbz

${STAGE}:
	rcsvg ${VGOPTS} -S ${STAGE} Three
	rm -f ${STAGE}/Makefile
	mv ${STAGE}/Makefile.meta ${STAGE}/Makefile

${STAGE}/local ${STAGE}/Pkgs: ${STAGE}
	mkdir $@

${STAGE}/local/bin ${STAGE}/local/sbin ${STAGE}/local/lib: ${STAGE}/local
	mkdir $@

${STAGE}/local/lib/libtomb: ${STAGE}/local/lib
	cd ${MSRC}/local/lib/libtomb && rcsvg ${VGOPTS} -S $@ Three

${STAGE}/local/lib/entomb: ${STAGE}/local/lib
	cd ${MSRC}/local/lib/entomb && rcsvg ${VGOPTS} -S $@ Three

${STAGE}/local/bin/unrm: ${STAGE}/local/bin
	cd ${MSRC}/local/bin/unrm && rcsvg ${VGOPTS} -S $@ Three

${STAGE}/local/sbin/preend: ${STAGE}/local/sbin
	cd ${MSRC}/local/sbin/preend && rcsvg -m ${VGOPTS} -S $@ Three

${STAGE}/local/bin/untmp: ${STAGE}/local/bin
	cd ${MSRC}/local/bin/untmp && rcsvg ${VGOPTS} -S $@ One

${STAGE}/local/bin/rmfile: ${STAGE}/local/bin
	cd ${MSRC}/local/bin/rmfile && rcsvg ${VGOPTS} -S $@ Two

# Include the package source to show off our strage loops.
${STAGE}/Pkgs/entomb_base: ${STAGE}/Pkgs
	rcsvg ${VGOPTS} -S $@ `verof`

stage: ${STAGE} ${STAGE}/local/lib/libtomb ${STAGE}/local/lib/entomb \
	${STAGE}/local/bin/unrm ${STAGE}/local/sbin/preend \
	${STAGE}/local/bin/untmp ${STAGE}/local/bin/rmfile \
	${STAGE}/Pkgs/entomb_base
	addlic ${STAGE}

check: FRC
	cd ${STAGE} && \
	find local Pkgs -type f |\
		xapply -f 'diff %1 /usr/msrc/%1' -

clean: FRC
	-[ -d ${STAGE} ] && op -u $${USER:-$$LOGNAME} level2s-chown ${STAGE}
	rm -rf ${STAGE}

${SOURCE}:
	co -q $@

source: ${SOURCE}

FRC:
