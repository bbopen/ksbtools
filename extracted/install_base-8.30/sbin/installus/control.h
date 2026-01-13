/* $Id: control.h,v 2.3 2001/09/26 18:14:43 ksb Exp $
 */

extern void ShowOwned();
extern void ShowOwner();
extern char *GetInstallOpts();
#if HAVE_PROTOTYPES
extern int ParseOwnersFor(char *, struct stat *, char *);
#else
extern int ParseOwnersFor();
#endif
extern char acOwners[];
