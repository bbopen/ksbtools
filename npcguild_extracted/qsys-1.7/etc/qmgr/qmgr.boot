#!/bin/sh
# $Id: qmgr.boot,v 1.1 1997/11/24 16:25:40 ksb Exp $

stty erase "^H" kill "^U"
trap "" 2
exec /usr/local/etc/qstart
