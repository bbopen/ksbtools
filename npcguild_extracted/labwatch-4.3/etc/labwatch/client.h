/* $Id: client.h,v 3.1 1994/01/21 15:23:16 bj Beta $
 */
#define LISTENBACKLOG	5

typedef struct _CLIENT {
	int fdSock;		/* socket of the connection */
	struct in_addr addr;	/* address of client host */
	time_t tLastActive;	/* last time we were able to work with it */
	char acBuf[4096];
	int iLen;
	void (*fill_func)(struct _CLIENT *);
	union {
		struct {
			short ihost;
			short isubnet;
			short iflag;
		} raw;
		struct {
			MAP *pMP;
			int inext;
		} map;
	} u;
	struct _CLIENT *next, **prev;
} CLIENT;	

extern int fdClientSock;

#if KEEP_STATISTICS
extern int iTicketsGranted, iTicketsDenied, iTicketsSuggested;
#endif /* KEEP_STATISTICS */

extern void DeleteClient(CLIENT *pCthis);
extern void ProcessClientWrite(CLIENT *pCthis);
extern void ProcessClientRead(CLIENT *pCthis);
extern void ClientInit();
extern void NewClient(CLIENT **prev, int fdSock, struct in_addr addr);
