#	$Id: Makefile,v 1.9 2000/06/21 19:40:29 ksb Exp $
#
#	Meta-Makefile for ptyd

GEN=	main.c main.h
SEND=	Make.host README TODO S82ptyd \
	daemon.c daemon.h logutmp.c machine.h \
	ptyd.man ptyd.c ptyd.m
SOURCE=	Makefile Distfile ${SEND}
INTO=	/usr/src/local/etc/ptyd

DEPTS      = 
PLATFORMS  = -St VAX780,VAX8800,S81,SUN3,SUN4
HOSTS      = ${DEPTS} ${PLATFORMS}

MDEFS=	
DDEFS=	-dINTO=${INTO} ${MDEFS} ${HOSTS}


LOOP=	-for i in `distrib -H ${HOSTS}` ; do \
		echo $$i: ;\
		rsh $$i -n sh -c '". /usr/local/lib/distrib/local.defs && cd ${INTO} && ${MAKE} DESTDIR=${DESTDIR} DEBUG=${DEBUG} $@"' ;\
	done
HERE=	distrib -E -f Make.host -m `hostname` | ${MAKE} -f - $@

quit: FRC
	echo 1>&2 "You are in the wrong place to just do a make"

all: rsource FRC
	${LOOP}

main.h: main.c

main.c: ptyd.m
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
