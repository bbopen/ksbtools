/*@Header@*/
/* $Id: cred.h,v 1.3 2008/09/01 21:27:01 ksb Exp $
 *
 * Interface for implementations of ptbw/escrow's send/recv creds	(ksb)
 */
#if defined(SCM_CREDS)
#if !defined(CRED_SCM_MSG)
#define CRED_SCM_MSG	SCM_CREDS		/* BSD, <pid, uid,...lots> */
#endif
#if !defined(CRED_TYPE)
#define CRED_TYPE	struct cmsgcred
#endif

#else
#if defined(SCM_CREDENTIALS)
#if !defined(CRED_SCM_MSG)
#define CRED_SCM_MSG	SCM_CREDENTIALS		/* Linux <pid, uid, gid> */
#endif
#if !defined(CRED_TYPE)
#define CRED_TYPE	struct ucred
#endif

/* In client code "#if !defined(CRED_SCM_MSG)" means no way to get creds, the
 * client code may #abort, or accept text creds based on its security level.
 * (We might be able to get the pid from a msgsnd or in text.)
 */
#if !defined(CRED_TYPE)
#define CRED_TYPE	int			/* just a pid */
#endif
#endif
#endif

/*@Explode dump@*/
extern void DumpCred(CRED_TYPE *);
/*@Explode send@*/
extern int SendCred(int s);
/*@Explode recv@*/
extern int RecvCred(int, CRED_TYPE *pCred);
