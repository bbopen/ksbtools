#	$Id: Makefile,v 3.8 2000/06/21 19:40:41 ksb Exp $
#
#	Meta-Makefile for labwatch

GEN=	main.c main.h
SEND=	INSTALL README TODO \
	client.c client.h daemon.c daemon.h host.c host.h \
	labwatch.m machine.h ops.c ops.h map.c map.h labwatch.man
SOURCE=	Makefile Distfile Make.host ${SEND}
INTO=	/usr/src/local/etc/labwatch

DEPTS      = 
PLATFORMS  = -St SUN3,SUN4,SUN5,NEXT2,FREEBSD
HOSTS      = ${DEPTS} ${PLATFORMS}

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

# %% any local targets we need to distrib files go here %%
main.c:
	${HERE}

main.h:
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
