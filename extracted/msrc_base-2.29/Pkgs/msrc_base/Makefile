# $Id: Makefile,v 2.35 2010/08/14 18:43:18 ksb Exp $
# build the base tar-blob that all of my tools need --ksb
# Stop msrc from trying to push is
INTO= _This _is _a _package _makefile, _it _cannot _be _pushed.

MSRC=/usr/msrc
TMP=/tmp
VERSION=2.29
STAGE=${TMP}/msrc_base-${VERSION}
VGOPTS=-m
SOURCE= Makefile Makefile.meta Makefile.host Msrc.hxmd ITO.spec README \
	options.html \
	patch-client.c patch-setargs.c

quit:
	echo "You are not in the right place to make all, for sure" 1>&2
	false

all: msource

# This is the level3 package builder interface I used to make the tar files
${TMP}:
	echo "Cannot find ${TMP}, which we will not build."
	false

${TMP}/msrc_base-${VERSION}.tgz: ${TMP} stage
	cd ${TMP} && tar zcf msrc_base-${VERSION}.tgz msrc_base-${VERSION}

${TMP}/msrc_base-${VERSION}.tbz: ${TMP} stage
	cd ${TMP} && tar cf - msrc_base-${VERSION} |bzip2 -9 >msrc_base-${VERSION}.tbz


${STAGE}:
	rcsvg ${VGOPTS} -S ${STAGE} Two
	rm -f ${STAGE}/Makefile
	mv ${STAGE}/Makefile.meta ${STAGE}/Makefile

${STAGE}/local ${STAGE}/Pkgs: ${STAGE}
	mkdir $@

${STAGE}/cgi-bin: ${STAGE}
	cd ${MSRC}/local/cgi-bin/manpage && rcsvg ${VGOPTS} -S $@ One

${STAGE}/local/bin ${STAGE}/local/sbin ${STAGE}/local/lib: ${STAGE}/local
	mkdir $@

${STAGE}/css:
	cd ${MSRC}/css && rcsvg ${VGOPTS} -S $@ One

${STAGE}/local/bin/mkcmd: ${STAGE}/local/bin
	cd ${MSRC}/local/bin/mkcmd && rcsvg ${VGOPTS} -S $@ Eight

${STAGE}/local/lib/mkcmd: ${STAGE}/local/lib
	cd ${MSRC}/local/lib/mkcmd && rcsvg ${VGOPTS} -S $@ Eight

${STAGE}/local/bin/explode: ${STAGE}/local/bin
	cd ${MSRC}/local/bin/explode && rcsvg ${VGOPTS} -S $@ Six

${STAGE}/local/lib/explode: ${STAGE}/local/lib
	cd ${MSRC}/local/lib/explode && rcsvg ${VGOPTS} -S $@ Six

${STAGE}/local/bin/ptbw: ${STAGE}/local/bin
	cd ${MSRC}/local/bin/ptbw && rcsvg ${VGOPTS} -S $@ One

${STAGE}/local/bin/xclate: ${STAGE}/local/bin
	cd ${MSRC}/local/bin/xclate && rcsvg ${VGOPTS} -S $@ Two

${STAGE}/local/bin/xapply: ${STAGE}/local/bin
	cd ${MSRC}/local/bin/xapply && rcsvg ${VGOPTS} -S $@ Three

${STAGE}/local/sbin/hxmd: ${STAGE}/local/sbin
	cd ${MSRC}/local/sbin/hxmd && rcsvg ${VGOPTS} -S $@ One

${STAGE}/local/lib/hxmd: ${STAGE}/local/lib
	cd ${MSRC}/local/lib/hxmd && rcsvg ${VGOPTS} -S $@ One

${STAGE}/local/sbin/msrc: ${STAGE}/local/sbin
	cd ${MSRC}/local/sbin/msrc && rcsvg ${VGOPTS} -S $@ One
	cd ${STAGE}/local/sbin/msrc && make source

# Include the package source and and op rule to change the package ownership.
# We could use verof to find the release version, but version Zero is the
# example version.
${STAGE}/Pkgs/msrc_base: ${STAGE}/Pkgs
	rcsvg ${VGOPTS} -S $@ `verof`
	co -p -rZero /usr/msrc/local/lib/op/remote/level2s.cf.host >$@/level2s.cf.host

# sleep to make our timestamp gt than msrc's, and our depends gt that.
${STAGE}/local/sbin/mmsrc: ${STAGE}/local/sbin
	sleep 1
	cd ${MSRC}/local/sbin/mmsrc && rcsvg ${VGOPTS} -S $@ One
	sleep 1
	cd ${STAGE}/local/sbin/mmsrc && make restart __msrc
	cd ${STAGE}/local/sbin/mmsrc && chmod u+w mmsrc.[ch]

${STAGE}/local/bin/wrapw: ${STAGE}/local/bin
	cd ${MSRC}/local/bin/wrapw && rcsvg ${VGOPTS} -S $@ Zero

stage: ${STAGE} ${STAGE}/local/bin/mkcmd ${STAGE}/local/lib/mkcmd \
	${STAGE}/local/bin/explode ${STAGE}/local/lib/explode \
	${STAGE}/local/bin/ptbw ${STAGE}/local/bin/xclate \
	${STAGE}/local/bin/xapply ${STAGE}/local/sbin/hxmd \
	${STAGE}/local/lib/hxmd ${STAGE}/local/sbin/msrc \
	${STAGE}/local/sbin/mmsrc ${STAGE}/local/bin/wrapw ${STAGE}/css \
	${STAGE}/cgi-bin ${STAGE}/Pkgs/msrc_base
	addlic ${STAGE}
	op -u source level2s-chown ${STAGE}

check: FRC
	cd ${STAGE} && \
	find local css Pkgs -type f |grep -v level2s.cf.host |\
		xapply -f 'diff %1 /usr/msrc/%1' -

clean: FRC
	-[ -d ${STAGE} ] && op -u $${USER:-$$LOGNAME} level2s-chown ${STAGE}
	rm -rf ${STAGE}

${SOURCE}:
	co -q $@

sanity: FRC
	cd ${STAGE} &&  ( find * -type f -print |grep -v "^README" |xapply -f 'diff -u2 %1 /usr/msrc/%1' - ) 2>&1

source: ${SOURCE}

FRC:
