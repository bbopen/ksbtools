#	$Id: Makefile,v 6.0 2000/07/30 15:57:11 ksb Exp $
#
#	Meta-Makefile for explode lib

GEN=	
SEND=	Make.host README explode.man \
	avl.html avl.c avl.h \
	ckpass.html ckpass.c ckpass.h \
	identd.html identd.c identd.h \
	scandir.html scandir.c scandir.h \
	setenv.html setenv.c setenv.h \
	strcasecmp.html strcasecmp.c strcasecmp.h \
	srtunq.html srtunq.c srtunq.h
SOURCE=	Makefile Distfile ${SEND}
INTO=	/usr/src/local/lib/explode

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

boot: rsource FRC
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
