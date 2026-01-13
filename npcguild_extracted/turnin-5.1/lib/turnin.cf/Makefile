#	$Id: Makefile,v 4.10 1996/07/14 18:49:25 kb207252 Exp $
#
#	Meta-Makefile for turnin.cf

GEN=	
SEND=	Make.host ckcf.sh turnin.man turnin.cf turnin.cf.man
SOURCE=	Makefile Distfile ${SEND}
INTO=	/usr/src/local/lib/turnin.cf
HOSTS=	-mmentor
MDEFS=	
DDEFS=	-dINTO=${INTO} ${MDEFS} ${HOSTS}


LOOP=	-for i in `distrib -H ${HOSTS}` ; do \
		echo $$i: ;\
		rsh $$i -n sh -c '". /usr/local/lib/distrib/local.defs && cd ${INTO} && make DESTDIR=${DESTDIR} DEBUG=${DEBUG} $@"' ;\
	done
HERE=	distrib -E -f Make.host -m `hostname` | make -f - $@

quit: FRC
	echo 1>&2 "You are in the wrong place to just do a make"

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

mkcat: FRCS
	${LOOP}

print: msource FRC
	${HERE}

self-test: FRC
	${LOOP}

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
