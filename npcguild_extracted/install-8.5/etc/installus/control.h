/* $Id: control.h,v 2.2 2000/08/02 16:20:28 ksb Exp $
 */

extern void ShowOwned();
extern void ShowOwner();
extern char *GetInstallOpts();
extern int ParseOwnersFor(char *, struct stat *, char *);
extern char acOwners[];
