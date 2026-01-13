#	$Id: Makefile,v 3.12 2000/06/21 19:42:10 ksb Exp $
#
#	Meta-Makefile for calls

GEN=	main.c main.h
SEND=	INSTALL Make.host README TODO \
	calls.man pblock.sh pfunc.sh \
	calls.c calls.h calls.m machine.h scan.c scan.h
SOURCE=	Makefile Distfile ${SEND}
INTO=	/usr/src/local/bin/calls

DEPTS      = 
PLATFORMS  = -S
HOSTS      = ${DEPTS} ${PLATFORMS}

MDEFS=	
DDEFS=	-dINTO=${INTO} ${MDEFS} ${HOSTS}


LOOP=	-for i in `distrib -H ${HOSTS}` ; do \
		echo $$i: ;\
		rsh $$i -n sh -c '". /usr/local/lib/distrib/local.defs && export PATH && cd ${INTO} && ${MAKE} DESTDIR=${DESTDIR} DEBUG=${DEBUG} $@"' ;\
	done
HERE=	distrib -E -f Make.host -m `hostname` | ${MAKE} -f - $@

quit: FRC
	echo 1>&2 "You are in the wrong place to just do a make"

all: rsource FRC
	${LOOP}

# %% any local targets we need to distrib files go here %%

main.h: main.c

main.c: calls.m
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
