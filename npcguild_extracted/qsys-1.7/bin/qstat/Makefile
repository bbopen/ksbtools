#	$Id: Makefile,v 1.2 2000/06/21 19:42:18 ksb Exp $
#
#	Meta-Makefile for qstat

GEN=	main.c main.h
SEND=	Make.host README qstat.m qstat.man
SOURCE=	Makefile Distfile ${SEND}
INTO=	/usr/src/local/bin/qstat

HOSTS=	-S
MDEFS=	
DDEFS=	-d"SEND=( ${SEND} )" -d"INTO=${INTO}" ${MDEFS} ${HOSTS}
DEBUG=	-O

LOOP=	for i in `distrib -H ${HOSTS}` ; do \
		echo $$i: ;\
		rsh $$i -n sh -c '". /usr/local/lib/distrib/local.defs && cd ${INTO} && ${MAKE} DESTDIR="${DESTDIR}" DEBUG="${DEBUG}" $@"' ;\
	done
HERE=	distrib -E -f Make.host -m `hostname` | ${MAKE} -f - $@


quit: FRC
	echo 1>&2 "You are in the wrong place to just do a make"

all: rsource FRC
	${LOOP}

${GEN}:
	${HERE}

calls: msource FRC
	${HERE}

clean: FRC
	${LOOP}
	rm -f ${GEN} Makefile.bak a.out core errs lint.out tags

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

print: msource FRC
	${HERE}

source: rsource
	${LOOP}

rsource: Distfile msource
	distrib ${DDEFS}

msource: ${SOURCE}

spotless: clean FRC
	rcsclean ${SOURCE}

tags: msource FRC
	${HERE}

${SOURCE}:
	co -q $@

FRC:
