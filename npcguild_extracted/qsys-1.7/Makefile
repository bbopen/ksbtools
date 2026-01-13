#	$Id: Make.meta,v 1.1 1997/01/05 22:26:14 root Exp $
#
#	Makefile for qsys distriburtion
#

SUBDIR= lib/qcommon bin/qadd bin/qrm bin/qstat \
	etc/qmgr etc/qinit etc/qkill etc/qlock etc/qstart etc/qend
OTHER=	README INSTALL
SOURCE=	Makefile ${OTHER}

INTO=/usr/src/Pkgs/qsys-1.7
HOSTS= -S
MDEFS=	
DDEFS=	-dINTO=${INTO} ${MDEFS} ${HOSTS}

LOOP=	for i in ${SUBDIR}; do\
		echo $$i:;\
		( cd $$i;\
		[ -f Makefile ] || co -q Makefile;\
		make ${MFLAGS} DEBUG="${DEBUG}" DESTDIR="${DESTDIR}" INTO="${INTO}/$$i" HOSTS="${HOSTS}" P="$P" $@; \
		)\
	done
DEBUG=	-O

quit:
	echo "You are not in the right place to make all, for sure" 1>&2
	false

all: source FRC
	${LOOP}

calls: FRC
	echo "Too much output and CPU usage."

clean: FRC
	rm -f Makefile.bak a.out core errs lint.out tags
	${LOOP}

deinstall: FRC
	echo "Unsafe to deinstall a whole binary directory."
	false

depend: FRC
	${LOOP}

dirs: FRC
	${LOOP}

distrib: FRC
	${LOOP}

install: FRC
	${LOOP}

lint: FRC
	echo "Too much CPU usage."

mkcat: FRC
	${LOOP}

print: FRC
	echo "Too much output."

source: rsource
	${LOOP}

rsource: msource
	distrib -o nodescend ${DDEFS}
	${LOOP}

msource: ${SOURCE}
	${LOOP}

spotless: FRC
	${LOOP}

tags: FRC
	${LOOP}

${SOURCE}:
	co -q $@

FRC:
