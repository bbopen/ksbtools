# $Id: Makefile,v 1.2 1992/09/20 10:12:03 becker Exp $
#
#

PAPER1=	information
PAPER2=	INSTALL

SRC1=	abstract intro requirements design implementation maintenance \
	conclusions acks

SRC2=	installation

SOURCE= Makefile ${SRC1} ${SRC2}

all: ${PAPER2} ${PAPER1}

${PAPER1}:$P ${SRC1}
	nroff -ms ${SRC1} > ${PAPER1}

${PAPER2}:$P ${SRC2}
	nroff -ms ${SRC2} > ${PAPER2}

cover_p: cover
	ltroff -ms cover

laser: ${SRC1} ${SRC2}
	ltroff -ms ${SRC2}
	ltroff -ms ${SRC1}

lint: ${SRC1}
	/usr/local/wwb/wwb ${SRC1} | $${PAGER-more}

clean: FRC
	-rm -f *.out ${PAPER1} ${PAPER2} styl.tmp

spotless: clean FRC
	-rcsclean ${SOURCE}

source: ${SOURCE}

${SOURCE}:
	co $@

FRC:
