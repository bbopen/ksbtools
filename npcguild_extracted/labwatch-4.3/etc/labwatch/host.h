/* $Id: host.h,v 3.3 1997/06/29 17:14:25 root Exp $
 */
#define MAXUDPSIZE	1400

/* for sizes below to work ANY_STATE must be last
 */
typedef enum { QUICK, ALIVE, DEAD, ANY_STATE } MState;

typedef struct {
	char acName[15];	/* who am i?				*/
	int iHeapPos;		/* where am I on the heap?		*/
	struct in_addr addr;	/* what is my IP address?		*/
	u_short iPort;		/* what is my favorite port?		*/
	u_long avenrun[AVEN_LEN]; /* 1, 5, 15 minute load average	*/
	int iUsers;		/* count of listed users		*/
	int iUniqUsers;		/* count of unique users		*/
	time_t tConsoleLogin;	/* when did the console user log in?	*/
	char acConsoleUser[9];	/* console user string			*/
	char acUsers[100];	/* list of connected users string	*/
	char acFSlist[80];	/* list of mounted FS's string		*/
	int iLoadFac;		/* load factor				*/
	int iUserFac;		/* user factor				*/
	time_t tLastReport;	/* last time this machine responsed	*/
	time_t tBoottime;	/* when this machine was booted		*/
	time_t tLastGivenOut;	/* last time we gave this machine away	*/
	time_t tMarked;		/* last time we reported down time	*/
	MState eState;		/* have we been reported dead?		*/
	int fShowMe:1;		/* show info on next packet?		*/
	char *pcAckMesg;	/* what to send them on their next ack	*/
} HOST;


extern int iHeapSize;
extern HOST **appHsubnets[];
extern int fdLoadSock, iHeaped;
extern void ProcessSyslog(struct in_addr addr, char *pcLog);
extern char *ProcessLoad(struct in_addr addr, u_short iPort, char *pcLoadBuf);
extern HOST *HeapDequeue(int iN), *HeapSuggest(HOST *, int);
extern HOST *LookupHost(struct in_addr addr, int fCreate);
extern HOST *HeapPeek(void);
extern void AckeachHostState(char *pcWith, MState eState);
extern void ForeachHostState(void (*func)(HOST *), MState eState);
extern void ForeachHostStr(void (*func)(HOST *), char *pcList);
