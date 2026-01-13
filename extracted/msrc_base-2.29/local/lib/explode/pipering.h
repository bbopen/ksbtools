/*@Header@*/
/*@Version $Revision: 1.2 $@*/
/* pipe ring definitions						(ksb)
 * $Id: pipering.h,v 1.2 2008/03/16 15:59:01 ksb Exp $
 *
 * A pipe ring is a group of processes that form a normal pipeline, with
 * the output of the last (right most) process directed back to the
 * input of the first (left most).  This is pretty bad (but not impossible)
 * to do in a shell script.  It is also VERY useful for sequential parallel
 * processing.
 */

/* What is the longest stall we'll accept (not the total time limit)
 */
#if !defined(PIPERING_STALL)
#define PIPERING_STALL	20
#endif

/* Start a work gang with the function Worker, include Count members,	(ksb)
 * pass them all pvData, record function returns in piRet.  Start
 * the pipe ring with the message Start (default "go\n").
 */
extern int PipeRing(int (*pfiWorker)(), int iCount, void *pvData, int *piRet, char *pcStart);

/* This is an example worker, push data from stdin to stdout with help	(ksb)
 * from the others in our gang.  We all read ahead (after upstream) then
 * write as the upstream tells us she is finished.  So with 2 in the gang
 * we "double buffer", with 6 we rock the world.
 */
extern int PipePump(int iWorker, int fdIn, int fdOut, void *pvData);
