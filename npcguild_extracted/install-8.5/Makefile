#	$Id: Make.meta,v 8.5 1999/03/28 18:53:08 ksb Exp $
#
#	Makefile for install distriburtion
#

SUBDIR= lib/install.cf bin/install.d etc/purge etc/installus etc/instck
OTHER= README Make.host
SOURCE=	Makefile ${OTHER}

INTO=/usr/src/Pkgs/install
HOSTS=-S
MDEFS=	
DDEFS=	-dINTO=${INTO} ${MDEFS} ${HOSTS}

LOOP=	for i in ${SUBDIR}; do\
		echo $$i:;\
		( cd $$i;\
		[ -f Makefile ] || co -q Makefile;\
		make ${MFLAGS} DEBUG="${DEBUG}" DESTDIR="${DESTDIR}" INTO="${INTO}/$$i" HOSTS="${HOSTS}" P="$P" $@; \
		)\
	done
DEBUG=	-O

quit:
	echo "You are not in the right place to make all, for sure" 1>&2
	false

all: source FRC
	${LOOP}

calls: FRC
	echo "Too much output and CPU usage."

clean: FRC
	rm -f Makefile.bak a.out core errs lint.out tags
	${LOOP}

deinstall: FRC
	echo "Unsafe to deinstall a whole binary directory."
	false

depend: FRC
	${LOOP}

dirs: FRC
	${LOOP}

distrib: FRC
	${LOOP}

install: FRC
	${LOOP}

lint: FRC
	echo "Too much CPU usage."

mkcat: FRC
	${LOOP}

print: FRC
	echo "Too much output."

source: rsource
	${LOOP}

rsource: msource
	distrib -o nodescend ${DDEFS}
	${LOOP}

msource: ${SOURCE}
	${LOOP}

spotless: FRC
	${LOOP}

tags: FRC
	${LOOP}

${SOURCE}:
	co -q $@

FRC:
