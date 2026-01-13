/* $Id: breakargs.h,v 3.0 1992/04/22 13:24:09 ksb Stab $
 * breakargs - break a string into a string vector for execv. 
 *
 * Returns NULL if can't malloc space for vector, else vector. 
 * Note, when done with the * vector, just "free" the vector. 
 * Written by Stephen Uitti, PUCC, Nov '85
 * for the new version of "popen" - "nshpopen", that doesn't use a shell.
 * (used here for the as filters, a newer option). 
 *
 * Permission is hereby given for its free reproduction and modification for
 * non-commercial purposes, provided that this notice and all embedded
 * notices be retained. Commercial organizations may give away copies as
 * part of their systems provided that they do so without charge, and that
 * they acknowledge the source of the software. 
 */

extern char **breakargs();		/* do the conversion to an argv */
