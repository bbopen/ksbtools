/*@Header@*/
/*@Version $Revision: 6.6 $@*/
/* $Id: binpack.h,v 6.6 2010/02/26 21:30:39 ksb Exp $
 * Pack an array of "things" into bins by ordering them in an order
 * that represents the packing order.
 *
 * packing { 2, 17, 7, 3, 5, 11, 13 } into 20 might return
 * { 17, 3, 13, 5, 2 11, 7 } which means put 17, 3 in the first bin,
 * 13, 5, 2 in the next, the 11, 7 in the last.  Things that are too
 * big to fit in a bin are on the end, and a count of them may be
 * requested.
 */

#if defined(TEST)
typedef int VE;
typedef int VEWEIGHT;
#endif

/* Lets one pass less that the whole VE, or the address of the VE	(ksb)
 * to the weight computation function
 */
#if !defined(VE_MASS)
#define VE_MASS(Mx)	(Mx)
#endif

#if !defined(VE_MASS_DECL)
#define VE_MASS_DECL	VE
#endif

extern int VEassemble();
extern size_t VEbinpack();
extern size_t VE_log2(VEWEIGHT), VE_unfib(VEWEIGHT);
extern size_t (*VE_scale)(VEWEIGHT);
