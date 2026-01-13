/* $Id: util_zombie.mc,v 8.22 2000/09/05 21:01:09 ksb Exp $
 * burry dead zombies (put salt on them...)				(ksb)
 */
static SIGRET_T
%%K<SaltZombies>1v(iSig)
int iSig; /*ARGSUSED*/
{
	auto int wTrash;

	while (0 < wait3((void *)&wTrash, WNOHANG, (void *)0)) {
		/* nada */;
	}
#if !HAVE_STICKY_SIGNALS
%	(void)signal(SIGCHLD, %K<SaltZombies>1v)%;
#endif
}
