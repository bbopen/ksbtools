#	$Id: Makefile,v 8.4 2000/06/21 19:41:54 ksb Exp $
#
#	Meta-Makefile for mkcmd

GEN=	
SEND=	Make.host README TODO Changes mkcmd.m scan.m mkcmd.mh \
	atoc.c atoc.h check.c check.h emit.c emit.h \
	key.c key.h list.c list.h machine.h \
	main.c main.h mkcmd.man mkcmd.c mkcmd.h \
	mkman.c mkusage.h mkusage.c \
	option.c option.h parser.c parser.h routine.c routine.h \
	type.c type.h scan.h scan.c stracc.c stracc.h
SOURCE=	Makefile Distfile ${SEND}
INTO=	/usr/src/local/bin/mkcmd

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

boot: FRC
	${LOOP}

all: rsource FRC
	${LOOP}

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
