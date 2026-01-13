/* $Id: setenv.h,v 2.1 1997/03/11 21:41:16 ksb Exp $
 * set en environment variable (see environ(7))
 */
#if defined(__STDC__)
extern int setenv(char *, char *, int);
#else
extern int setenv();
#endif
