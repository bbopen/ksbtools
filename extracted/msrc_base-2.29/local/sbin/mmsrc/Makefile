# $Id: Makefile,v 1.23 2009/11/18 20:01:43 ksb Exp $
# This is the match that lights the new (2007) version of the ksb master
# source structure.  You need this program to build the first instance of
# the tool-chain.  From that you can remote-build as many as you like, or
# continue to build them with mmsrc.

# msrc support
INTO=/usr/src/local/sbin/mmsrc
LIBS=@LIBS@

# These files are rebuilt with "restart":
GEN=mmsrc.c mmsrc.h mkdtemp.c configure config.h config.h.in setenv.c setenv.h
JUNK=config.h config.log config.status autom4te.cache
SOURCE= Makefile Makefile.host README TODO mmsrc.html boot.html \
	mmsrc.man  makeme.man mmreg.ksh mbuild makeme \
	mmsrc.m machine.h machine.c \
        config.plain configure.ac

all: mmsrc

# Build mmsrc locally (here) without the benefit of msrc/hxmd or a build host.
Makefile.local.in:
	ln -s Makefile Makefile.local.in

config.h:
	cp config.plain $@

mmsrc: mmsrc.o machine.o
	${CC} ${CFLAGS} -o $@ mmsrc.o machine.o ${LIBS}

setenv.c setenv.h:
	explode -s $@

mkdtemp.c:
	explode -u dtemp mk.c

# When you want to build a new mmsrc.[ch] after upgrading any common modules
# or changing autoconf.ac use this target to close-the-loop.  --ksb
mmsrc.h: mmsrc.c

mmsrc.c: mmsrc.m ../msrc/make.m ../hxmd/evector.m \
	../hxmd/hostdb.m ../hxmd/slot.m ../hxmd/machine.h configure.ac
	rm -f *.o mmsrc.h mmsrc.c mkdtemp.c setenv.[ch] configure *.in
	make mkdtemp.c setenv.c setenv.h
	mkcmd -I../msrc -I../hxmd -G -n mmsrc util_fgetln.m mmsrc.m
	chmod u+w mmsrc.[ch]

config.h.in: configure.ac
	autoreconf configure.ac && rm -rf autom4te.cache
	rm -f ${JUNK}

configure: config.h Makefile.local.in

# The configure spell below creates a platfrom specific Makefile and config.h
boot: Makefile.local.in
	rm -f config.h
	./configure
	make -f Makefile.local mmsrc

clean: FRC
	rm -f mmsrc Makefile.bak *.o a.out core errs lint.out tags
	rm -f config.h Makefile.local Makefile.local.in

restart: mmsrc.c mmsrc.h config.h.in configure
	make config.h

shar: FRC
	find . -type f -print | grep -v /RCS | sed -e 's,\./,,' |\
	nushar -p K -i -

source: ${SOURCE} ${GEN}

${SOURCE}:
	co -q $@

FRC:

__msrc: ${SOURCE} ${GEN}

# DO NOT DELETE THIS LINE - make depend DEPENDS ON IT

mmsrc.o: mmsrc.c mmsrc.h machine.h config.h

machine.o: machine.c mkdtemp.c machine.h config.h

# *** Do not add anything here - It will go away. ***
