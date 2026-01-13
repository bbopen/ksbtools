#	$Id: Makefile,v 2.4 2000/06/21 19:40:39 ksb Exp $
#
#	Meta-Makefile for  autologin

GEN=	main.c main.h
SOURCE= Make.host autologin.man autologin.c autologin.m autolocal.ksh machine.h
INTO=	/usr/src/local/etc/autologin

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

# %% any local targets we need to distrib files go here %%
main.h: main.c

main.c: autologin.m
	mkcmd std_help.m std_version.m autologin.m
	-(cmp -s prog.c main.c || (cp prog.c main.c && echo main.c updated))
	-(cmp -s prog.h main.h || (cp prog.h main.h && echo main.h updated))

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
