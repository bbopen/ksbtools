/*@Header@*/
/*@Version $Revision: 6.2 $@*/
/* $Id: identd.h,v 6.2 2010/02/26 21:30:39 ksb Exp $
 *
 * Here's an unofficial patch that's in use locally that adds a bit more
 * identification to posters. It's great for limiting the number of
 * probablamtic posts from users who try and hide behind a forged note.
 *
 * Luke Mewburn			Programmer, Technical Services Group,
 * Email: <lm@rmit.edu.au>		Department of Computer Science, RMIT,
 * Phone: +61 3 660 3210		Melbourne, Victoria, Australia.

 * This adds an extra header `NNTP-posting-user:' which is determined by
 * the ident protocol. It contains the name of the user returned by
 * identd. (I.e, check out the headers on this post)
 *
 * A couple of comments.
 * - Some people are dead against using identd. Well, I run the only
 *   machines which are allowed to post to our server, and if anyone
 *   manages to hack those, I'm sure they'd do more than spoof identd
 *   so they can forge news. So, where's the `spoofing' problem.
 * - Rich $alz didn't want to incorporate this particular patch into the
 *   official tree. That's his prerogative, and I respect that. (He'd
 *   rather just be syslogging the info.) Our site decided to allow the
 *   world to see that Joe Bloggs is being a pain, rather than just
 *   making a note to the local sysop. That's our prerogative :)
 * - The header entry is as difficult to override as NNTP-Posting-Host.
 *   (I.e, a normal connection shouldn't be able to do it.)
 * - If the ident fails, it doesn't add the header
 * - Only one package has failed with connecting to a modified nnrpd
 *   system - an old (pre winsock.dll) version of WinQVT net on the PC -
 *   it couldn't handle the ident request...
 * - The request occurs at the same time that nntp-posting-host does.
 *
 * We've had it running for two months now, and besides the WinQVT
 * problem, all seems well.
 *
 * Anyway, here's the source. I couldn't think of a good place to post
 * it, so alt.sources and news.software.nntp has it.
 */
#if HAVE_PROTO
extern int ident_client(struct sockaddr_in, struct sockaddr_in, char *);
#else
extern int ident_client();
#endif
