# $Id: Make.meta,v 1.1 1997/11/24 01:41:03 ksb Exp $
#
#	Meta Makefile for rm/mv/cp BSD

GEN=	
SUBDIR= bin/rm bin/mv bin/cp
OTHER=	README INSTALL
SOURCE=	Makefile ${OTHER}

INTO=/usr/src/Pkgs/rm
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
