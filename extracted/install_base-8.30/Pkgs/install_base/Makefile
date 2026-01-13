# $Id: Makefile,v 8.37 2010/08/13 19:59:30 ksb Exp $
# pkg level makefile for install and friends to complete chain2
INTO= _This _is _a _package _makefile, _it _cannot _be _pushed.

MSRC=/usr/msrc
TMP=/tmp
VERSION=8.30
STAGE=${TMP}/install_base-${VERSION}
VGOPTS=

# Keep msync happy at this level
GEN=
SOURCE= Makefile Makefile.host Makefile.meta Msrc.hxmd ITO.spec \
	README access.cf

quit:
	echo "You are not in the right place to make no target, for sure" 1>&2
	false

all: stage

# level3 package builder requires this interface, do not mess with it
${TMP}:
	echo "Cannot find ${TMP}, which we will not build."
	false

${TMP}/install_base-${VERSION}.tgz: ${TMP} stage
	cd ${TMP} && tar zcf install_base-${VERSION}.tgz install_base-${VERSION}

${TMP}/install_base-${VERSION}.tbz: ${TMP} stage
	cd ${TMP} && tar cf - install_base-${VERSION} |bzip2 -9 >install_base-${VERSION}.tbz


${STAGE}:
	rcsvg ${VGOPTS} -S ${STAGE} Eight
	rm -f ${STAGE}/Makefile
	mv ${STAGE}/Makefile.meta ${STAGE}/Makefile

${STAGE}/lib ${STAGE}/bin ${STAGE}/sbin ${STAGE}/Pkgs: ${STAGE}
	mkdir $@

${STAGE}/lib/install.cf: ${STAGE}/lib
	cd ${MSRC}/local/lib/install.cf && rcsvg ${VGOPTS} -S $@ Eight

${STAGE}/lib/mk: ${STAGE}/lib
	cd ${MSRC}/local/lib/mk && rcsvg ${VGOPTS} -S $@ Five

${STAGE}/bin/mk: ${STAGE}/bin
	cd ${MSRC}/local/bin/mk && rcsvg ${VGOPTS} -S $@ Five

${STAGE}/bin/op: ${STAGE}/bin
	cd ${MSRC}/local/bin/op && rcsvg ${VGOPTS} -S $@ $${OP_REV:-Two}

${STAGE}/bin/install.d: ${STAGE}/bin
	cd ${MSRC}/local/bin/install.d && rcsvg ${VGOPTS} -S $@ Eight

${STAGE}/bin/vinst: ${STAGE}/bin
	cd ${MSRC}/local/bin/vinst && rcsvg ${VGOPTS} -S $@ One

${STAGE}/sbin/instck: ${STAGE}/sbin
	cd ${MSRC}/local/sbin/instck && rcsvg ${VGOPTS} -S $@ Eight

${STAGE}/sbin/purge: ${STAGE}/sbin
	cd ${MSRC}/local/sbin/purge && rcsvg ${VGOPTS} -S $@ Eight

${STAGE}/sbin/installus: ${STAGE}/sbin
	cd ${MSRC}/local/sbin/installus && rcsvg ${VGOPTS} -S $@ Two

# Include the package source for this, so you can make a better version.
${STAGE}/Pkgs/install_base: ${STAGE}/Pkgs
	rcsvg ${VGOPTS} -S $@ `verof`

clean: FRC
	op -u $${USER:=$$LOGNAME} level2s-chown ${STAGE}
	rm -rf ${STAGE}

stage: ${STAGE} ${STAGE}/lib/install.cf ${STAGE}/bin/install.d \
	${STAGE}/sbin/instck ${STAGE}/sbin/purge \
	${STAGE}/bin/vinst ${STAGE}/sbin/installus \
	${STAGE}/bin/op ${STAGE}/bin/mk ${STAGE}/lib/mk \
	${STAGE}/Pkgs/install_base
	addlic ${STAGE}
	op -u source level2s-chown ${STAGE}

source: ${SOURCE}

${SOURCE}:
	co -q $@

sanity: FRC
	cd ${STAGE} && ( find [!P]*/ -type f -print |xapply -f 'diff -u2 %1 /usr/msrc/local/%1' - ) 2>&1
	cd ${STAGE} && ( find Pkgs -type f -print |xapply -f 'diff -u2 %1 /usr/msrc/%1' - ) 2>&1


FRC:
