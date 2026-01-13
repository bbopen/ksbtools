# a shell script to source to set the paths the q system needs to know
# some Makefile's source us to get -D values.

F="$-"
set +e
# this login name runs the queue system for root, so we do not
# expose root to bugs in the queue code
Q_GROUP=qmgr
Q_MASTER=daemon
Q_MASTER_HOME=`csh -c "echo ~${Q_MASTER}" 2>&1 || ksh -c "echo ~${Q_MASTER}"`

# where should we install things?
if [ _/ != _${Q_MASTER_HOME} -a -d ${Q_MASTER_HOME}/etc -a -d ${Q_MASTER_HOME}/bin ]
then
	Q_DEST=${Q_MASTER_HOME}
elif [ -d /usr/local/etc -a -d /usr/local/bin ]
then
	Q_DEST=/usr/local
elif [ -d /usr/etc -a -d /usr/bin ]
then
	Q_DEST=/usr
else 
	echo 1>&2 "q system can't find destination structure (abort)"
	exit 1
fi
Q_ETC=${Q_DEST}/etc
Q_BIN=${Q_DEST}/bin

if grep -s "^kmem:" </etc/group >/dev/null
then
	Q_KMEM=kmem
elif ypcat group 2>&1 | grep -s "^kmem:" >/dev/null
then
	Q_KMEM=kmem
else
	Q_KMEM=sys
fi

if [ -d ${Q_MASTER_HOME}/log ]
then
	Q_LOGDIR=${Q_MASTER_HOME}/log
elif [ -d /var/log ]
then
	Q_LOGDIR=/var/log
else
	Q_LOGDIR=/usr/adm
fi

if [ -f ${Q_LOGDIR}/qmgr.log ]
then
	Q_LOGFILE=${Q_LOGDIR}/qmgr.log
else
	Q_LOGFILE=${Q_LOGDIR}/qmgr
fi

if [ -d ${Q_MASTER_HOME}/spool ]
then
	Q_SPOOL=${Q_MASTER_HOME}/spool
elif [ -d /var/spool ]
then
	Q_SPOOL=/var/spool
else
	Q_SPOOL=/usr/spool
fi

if [ -d ${Q_SPOOL}/long ]
then
	Q_DIR=${Q_SPOOL}/long
else
	Q_DIR=${Q_SPOOL}/qsys
fi
#set -$F
:
