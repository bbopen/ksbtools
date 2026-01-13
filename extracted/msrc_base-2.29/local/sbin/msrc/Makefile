# $Id: Makefile,v 1.19 2010/07/14 19:40:00 ksb Exp $
# Master Makefile for msrc version 2008

INTO=/usr/src/local/sbin/msrc
# msrc sets MODE, SEND, IGNORE, HXINCLUDE, SUBDIR, and MAP.

GEN=	local.jpg remote.jpg
SOURCE=	Makefile Msrc.hxmd Makefile.host local.fig remote.fig qstart.html \
	README TODO dmz.sh dmz.man \
	envfrom.m machine.h make.m msrc.m mkdtemp.m \
	mreg.ksh msrc.html msrc.man

local.jpg: local.fig
	fig2dev -L jpeg -q 98 $? >$@

remote.jpg: remote.fig
	fig2dev -L jpeg -q 98 $? >$@

clean: FRC
	rm -f ${GEN}

source: ${SOURCE} ${GEN}

${SOURCE}:
	co -q $@

FRC:

# msrc patch to get up-to-date and checked-out sources
__msrc: source
