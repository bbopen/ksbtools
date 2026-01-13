#	$Id: Makefile,v 3.9 2000/06/21 19:41:13 ksb Exp $
#
#	Meta-Makefile for libtomb

GEN=	
SEND=	Make.host README TODO \
	debug.c debug.h \
	ecreat.c eopen.c erename.c etruncate.c eunlink.c \
	libtomb.man libtomb.c libtomb.h
SOURCE=	Makefile Distfile ${SEND}
INTO=	/usr/src/local/lib/libtomb

# known platforms  VAX780,VAX8800,S81,SUN3,SUN4,NEXT2,IBMR2,LINUX
HOSTS= -S
MDEFS=
DDEFS= -dINTO=${INTO} ${MDEFS} ${HOSTS}


LOOP=	-for i in `distrib -H ${HOSTS}` ; do \
		echo $$i: ;\
		rsh $$i -n sh -c '". /usr/local/lib/distrib/local.defs && cd ${INTO} && ${MAKE} DESTDIR=${DESTDIR} DEBUG=${DEBUG} $@"' ;\
	done
HERE=	distrib -E -f Make.host -m `hostname` | ${MAKE} -f - $@

quit: FRC
	echo 1>&2 "You are in the wrong place to just do a make"

all: rsource FRC
	${LOOP}

# any local targets we need to distrib files go here

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

# DO NOT DELETE THIS LINE - maketd DEPENDS ON IT

# *** Do not add anything here - It will go away. ***
