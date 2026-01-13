# $Id: Make.meta,v 1.2 1997/11/29 23:41:38 ksb Exp $
#
#	Meta Makefile for ptyd

GEN=	
SUBDIR= lib/libptyd etc/ptyd bin/script
OTHER=	README INSTALL
SOURCE=	Makefile ${OTHER}

INTO=/usr/src/Pkgs/ptyd
HOSTS= -S
MDEFS=	
DDEFS=	-dINTO=${INTO} ${MDEFS} ${HOSTS}

P=
LOOP=	for i in ${SUBDIR}; do\
		echo $$i:;\
		( cd $$i;\
		[ -f Makefile ] || co -q Makefile;\
		make ${MFLAGS} DEBUG="${DEBUG}" DESTDIR="${DESTDIR}" P="$P" INTO="${INTO}/$$i" HOSTS="${HOSTS}" MDEFS="${MDEFS}" $@; \
		)\
	done
DEBUG=	-O

quit:
	echo 1>&2 "Use \`make all' to build this release"
	false

all: source FRC
	${LOOP}

calls: FRC
	echo "Too much output and CPU usage."

clean: FRC
	rm -f Makefile.bak ${GEN} a.out core errs lint.out tags
	${LOOP}

deinstall: FRC
	${LOOP}

depend: FRC
	${LOOP}

dirs: FRC
	${LOOP}

distrib: FRC
	${LOOP}

install: rsource FRC
	${LOOP}

lint: FRC
	${LOOP}

mkcat: FRC
	${LOOP}

print: FRC
	echo "Too much output."

source: rsource
	${LOOP}

rsource: msource ${GEN}
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
