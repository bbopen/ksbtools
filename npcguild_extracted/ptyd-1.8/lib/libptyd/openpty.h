#define OPTY_NOP	0	/* nothing special			*/
#define OPTY_UTMP	1	/* make a utmp entry			*/
#define OPTY_WTMP	2	/* make a wtmp entry			*/
#define OPTY_SECURE	4	/* be sure it is a secure line		*/

#define OPTY_LOGIN	(OPTY_UTMP|OPTY_WTMP)

#if 0
extern int openpty(char *, char *, int, uid_t);
extern int openrpty(char *, char *, int, uid_t, char *);
extern int forkpty(char *, char *, int, uid_t, char **);
extern int closepty(char *, char *, int, int);
#else
extern int openpty();
extern int openrpty();
extern int forkpty();
extern int closepty();
#endif
