#	$Id: Makefile,v 8.6 2008/10/28 00:04:59 ksb Exp $
#
#	Meta-Makefile for install
INTO=	/usr/src/local/bin/install.d

GEN=
SOURCE=	Makefile Makefile.host README INSTALL TODO \
	configure.h dir.c dir.h file.c file.h \
	install.man install.h machine.h main.c main.h \
	myself.cf special.c special.h special.i syscalls.c syscalls.h

clean: FRC
	rm -f ${GEN}

source: ${SOURCE} ${GEN}

${SOURCE}:
	co -q $@

FRC:

# msrc patch to get up-to-date and checked-out sources
__msrc: source
