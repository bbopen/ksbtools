/*@Header@*/
/*@Version $Revision: 2.7 $@*/
/* $Id: wise.h,v 2.7 2010/02/26 21:30:39 ksb Exp $
 * common functions all state-side modules share -- ksb
 */

#if !defined(WISE_DOUBLE)

#if !defined(WISE_MOD_PATH)
#define WISE_MOD_PATH	"/opt/sso/lib/wise_apply"
#endif

/* do we include the multi thread support, via listen/camp		(ksb)
 */
#if !defined(WISE_MULTI)
#define WISE_MULTI	(defined(SUN5))
#endif

/* the default session timeout, 60 seconds times number of minutes	(ksb)
 * session length for sso is 8 hours (480 minutest)                     (sab)
 */
#if !defined(WISE_TIMEOUT)
#define	WISE_TIMEOUT	(60*480)
#endif

/* runtime loader dynamlic options
 */
#if !defined(WISE_RTLD_OPTS)
#if defined(RTLD_NODELETE)
#if defined(RTLD_PARENT)
#define WISE_RTLD_OPTS	(RTLD_NOW|RTLD_GLOBAL|RTLD_PARENT|RTLD_NODELETE)
#else
#define WISE_RTLD_OPTS	(RTLD_NOW|RTLD_GLOBAL|RTLD_NODELETE)
#endif
#else
#define WISE_RTLD_OPTS	(RTLD_NOW|RTLD_GLOBAL)
#endif
#endif

/* environement conventions, don't change these.
 */
#define WISE_ENV_FD	"WISE_FD"	/* name of connection to state our */
#define WISE_ENV_CODE	"WISE_REPLY"	/* reply code from our state, if any */
#define WISE_ENV_MSG	"WISE_MSG"	/* text of reply sent		*/
#define WISE_ENV_MOD	"WISE_MOD"	/* name of the module we wanted	*/

/* login tunes
 */
#if !defined(WISE_AUTH_COOKIE)
#define WISE_AUTH_COOKIE "fdx_login"
#endif
#if !defined(WISE_AUTH_DOMAIN)
#define WISE_AUTH_DOMAIN ".fedex.com"
#endif
#if !defined(WISE_STATDB)
#define WISE_STATDB	"/var/run/stater"
#endif

/* status database file tokens 0...N-1
 */
#define WISE_RESET_SLOT		(-3)
#define WISE_OUR_SLOT		(-2)
#define WISE_HEADER_SLOT	(-1)
#define WISE_DB_LINE		79		/* data on the line, plus \n */

/* reference data in a state process, read-only please
 */
extern long int wise_pad;		/* random mask for 1 time pads	*/
extern unsigned int wise_timeout;	/* present idle timeout in sec.	*/

/* current module table
 */
typedef struct MTnode {
	char *pcname;
	void *pvmodule;
} MOD_ENTRY;
extern MOD_ENTRY *pMTAll;	/* vector of all loaded modules		*/
extern int iMods;		/* number of loaded modules		*/

/* support call-backs in a stater process only
 */
typedef int (*wise_entry_t)(int, int, char **);
extern void wise_setpad(long int);
extern unsigned int wise_settimeout(unsigned int);
extern void wise_run(int fdState, int iConn, char *pcCred);

/* support call-backs, and fully explode enabled for all WISE projects
 */
extern char *wise_name(char *pcKey);
extern char *wise_host(char *pcKey, char *pcHost);
extern char *wfmt(char *pcDisplay, char *pcName, int fdBound, char *pcTag);
extern void *wise_module(char *pcName, void *pDLModule);
extern int wise_load(char *pcModule, char *pcPath);
extern wise_entry_t wise_entry(char *pcModule, char *pcEntry);
extern int wise_status(int iSlot, char *pcTxt);
extern int wise_port(int *piPort);
extern int wise_put(int fd, int iNum, const char *pcText);
extern int wise_get(int fd, int *piNum, char **ppcText);
extern int wise_cget(int fd, int *piNum, char **ppcText);

/* user module calls we support, most are optional
 */
/* XXX need a generic way to set attributes, maybe "testify" or "witness"
 *	+ a flag to add or replace
 *	+ flags for session-only | prefereces | both
 *	+ we probably need to account for non-LAITN1 values, someday.
 * Today they are set in the underlying store (AVL, Oracle, MySQL, etc.)
 */
extern char **attribute(char *pcWhich);
extern int init(int argc, char **argv, void *pCS);

/* common entry points
 */
extern int info(int fd, int argc, char **argv);
extern int chat(int fd, int argc, char **argv);
extern int auth(int fd, int argc, char **argv);
extern int cleanup(int fd, int argc, char **argv);
extern int spinup(int fd, int argc, char **argv);
extern int spindown(int fd, int argc, char **argv);

#if WISE_MULTI

#define _REENTRANT		/* from the manual for Solaris	*/
#include <pthread.h>

extern pthread_attr_t camp_attr;
extern int camp(int fd, int argc, char **argv);
#endif
#define WISE_DOUBLE	void
#endif /* wise double-includ protection */
