#	$Id: Makefile,v 4.14 2000/06/21 19:41:35 ksb Exp $
#
#	Meta-Makefile for project

GEN=	main.c main.h
SEND=	Make.host README install.doc template \
	machine.h path.m path.mh \
	project.man project.c project.h project.m
SOURCE=	Makefile Distfile ${SEND}
INTO=	/usr/src/local/bin/project

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

main.c: project.m
	mkcmd std_help.m project.m
	-(cmp -s prog.c main.c || (mv prog.c main.c && echo main.c updated))
	-(cmp -s prog.h main.h || (mv prog.h main.h && echo main.h updated))
	rm -rf prog.[ch]

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
