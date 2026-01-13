/* $Id: ops.h,v 3.1 1994/01/21 15:23:52 bj Beta $
 */
extern int iExpected;
#if KEEP_STATISTICS
extern time_t tLastZeroStats;
#endif

extern void op_INFO_print_host(HOST *);
extern void ProcessStdin();
extern void ReadExpectList();
