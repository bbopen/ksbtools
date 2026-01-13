/*
 * define tuning parameters and message strings				(ksb)
 */

#ifndef PTYD_PORT
#define PTYD_PORT	"/dev/ptyd"
#endif

#ifndef LOGFILE
#define LOGFILE		"/usr/adm/ptyd.log"
#endif

#define SAFE_ACK	00600		/* mode the acknowlegde the pty	*/
#define SAFE_DONE	04000		/* mode the user sets to free	*/
#define SAFE_MASK	07777		/* mode bits to check		*/

/*
 * commands we read
 */
#define PTYD_HELLO	"hello"		/* make sure the daemon is there*/
#define PTYD_OPEN	"get"		/* get a pty/tty pair for uid	*/
#define PTYD_MKUTMP	"mkutmp"	/* build utmp record for pty	*/
#define PTYD_RMUTMP	"rmutmp"	/* remove utmp entry for pty	*/
#define PTYD_LOGIN	"login"		/* build wtmp record for pty	*/
#define PTYD_LOGOUT	"logout"	/* build close wtmp for pty	*/
#define PTYD_CLOSE	"release"	/* return the pty to default	*/
#define PTYD_CLEAN	"clean"		/* set all unused to default	*/
#define PTYD_PID	"pid"		/* show me your pid		*/

/*
 * commands we write
 */
#define PTYD_IDENT	"running"	/* resond to a hello		*/
#define PTYD_TAKE	"take"		/* ptyd tells you to take it	*/
#define PTYD_OK		"ok"		/* ptyd believes you		*/
#define PTYD_NO		"liar"		/* ptyd says you lie		*/
#define PTYD_UNKNOWN	"unknown"	/* ptyd doesn't grok you	*/
#define PTYD_USER	"user"		/* unknown user error		*/
#define PTYD_SYSERR	"ouch"		/* a syscall returned -1	*/
#define PTYD_IMPL	"?eak"		/* should do something here	*/

#if !defined(PTYOWNER)
#define PTYOWNER	"root"
#endif
#if !defined(TTYOWNER)
#define TTYOWNER	PTYOWNER
#endif

#if !defined(PTYGROUP)
#define PTYGROUP	"tty"
#endif
#if !defined(TTYGROUP)
#define TTYGROUP	PTYGROUP
#endif

#if !defined(PTYMODE)
#define PTYMODE		0600
#endif
#if !defined(TTYMODE)
#define TTYMODE		PTYMODE
#endif

#define TTYNAMELEN	12

extern int Server(), Clean(), Status(), Killme();
