#!/bin/sh
# $Id: config-info.sh,v 1.7 2000/08/01 12:52:58 ksb Dist $

. ./qpaths.sh

set _ `date`
DATE="$4 $3 $7"
set _ `echo _ '$Revision: 1.7 $ $State: Dist $' | sed -e 's/\$/%/g'`
VERSION="$4 $7"

cat -<<!
Q System Version $VERSION Configuration ${DATE}:
manager login $Q_MASTER.$Q_GROUP, kmem group $Q_KMEM
        home directory: $Q_MASTER_HOME
   top level directory: $Q_DEST
 user binary directory: $Q_BIN
admin binary directory: $Q_ETC
       spool directory: $Q_SPOOL  (default queue $Q_DIR)
         log directory: $Q_LOGDIR  (file $Q_LOGFILE)
!

exit 0
