/*@Header@*/
/* $Id: mk.h,v 1.4 2010/02/02 20:14:59 ksb Exp $
 */
/*@Explode temp@*/
extern char *mktemp(char *);
/*@Explode stemp@*/
extern int mkstemp(char *);
/*@Explode stemps@*/
extern int mkstemps(char *, int);
/*@Explode dtemp@*/
extern char *mkdtemp(char *);
/*@Explode dtemps@*/
/* Not in the stock system, but if you needed mkstemps I bet you need
 * this one as well -- ksb
 */
extern char *mkdtemps(char *, int);
/*@Explode utemp@*/
/* Not in any system by mine, make a unix domain socket for a wrapper
 */
extern int mkutemp(char *), mkutemps(char *, int);
/*@Explode utemps@*/
/* build a unix domain socket with a suffix
 */
extern char *mkutemps(char *, int);
