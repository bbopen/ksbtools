#	$Id: Makefile,v 3.11 2000/06/21 19:42:14 ksb Exp $
#
#	Meta-Makefile for unrm

GEN=	main.c main.h
SEND=	Make.host changedir.c changedir.h \
	findfiles.c findfiles.h glob.c glob.h init.c init.h \
	lists.c lists.h listtombs.c listtombs.h ls.c ls.h \
	purge.c purge.h restore.c restore.h show.c show.h \
	unrm.man unrm.m
SOURCE=	Makefile Distfile ${SEND}
INTO=	/usr/src/local/bin/unrm

# known VAX780,VAX8800,S81,SUN3,SUN4,NEXT2,IBMR2,LINUX
HOSTS= -S
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

main.h main.c:
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
