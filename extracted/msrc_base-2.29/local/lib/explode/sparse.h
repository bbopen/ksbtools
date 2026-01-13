/*@Header@*/
/*@Version: $Revision: 1.7 $@*/

/* The type of a tag in the sparse space (scalar type, unsigned work best)
 */
typedef unsigned long sp_key;

/* the largest tag we expect to use
 */
#define SP_MAX_KEY	(~(sp_key)0)

/* How do I see that a element is a terminal (data) node?
 * 0, 1, 2, are, 3 is split into 2{0,1} to get {3,4}, 5 is split into 3...
 * There are at least 2 data nodes per vector, and at worse 3 of 46.
 */
#define SP_TERM(Mi)	((Mi) < 3)


/*#define SP_ALLOC_CACHE	2048
 *
 * Chunk sp_init function calls to calloc(3) into larger cached
 * slabs.  Since we never free the DAG we build for the space this
 * saves a lot of malloc headers and such.  I use it at 4096.
 */

/*#define SP_ALLOC_TRIM		64
 *
 * Declare how much space you are willing to discard to avoid calling
 * calloc(3).  Many of the calls we make to calloc were for 2 and 3
 * element vectors, the cache helped reduce that. By allowing the rare
 * request for 20 to 41 element vectors to fall directory to calloc we
 * can save the small "trim part" for the next request for a small bit.
 */

/*#define SP_RECURSE		1
 *
 * Use the recursive version of sp_walk, rather than the itterative one.
 * The recursive one is slower as far as I can tell.
 */
/*#define SP_VERBOSE		1
 *
 * Output a trace of the major plays in the game.  Helps understand how
 * this works.  Of course a picture helps a lot more.
 */

/* Resources we export:
 */
extern sp_key sp_fibs[];
extern size_t sp_unfib(sp_key wN);
extern void **sp_init(sp_key wRange);
extern void **sp_index(void **ppvFrom, sp_key wFind);
extern int sp_walk(void **ppvFrom, sp_key wLow, sp_key wHi, int (*pfiMap)(sp_key, void *));
extern int sp_exists(void **ppvRoot, sp_key wKey);
