#	$Id: Makefile,v 8.2 2000/06/21 19:41:09 ksb Exp $
#
#	Meta-Makefile for conserver.cf

GEN=	
SEND=	INSTALL README conserver.m4 conserver.cf.man dummy.cf \
	header.m4 footer.m4
SOURCE=	Makefile Distfile ${SEND}
INTO=	/usr/src/local/lib/conserver.cf
HOSTS=	-S
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

check: conserver.cf FRC
	sed -n -e 's/#.*//' -e 's/.*@\([^:]*\):.*/\1/p' < conserver.cf | sort -u

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
