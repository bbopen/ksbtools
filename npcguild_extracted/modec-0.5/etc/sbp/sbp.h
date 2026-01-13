/* $Id: sbp.h,v 1.7 2000/05/08 21:12:52 ksb Exp $
 */

typedef struct SMnode {
	char *pcmeth;
	char *pchelp;
#if HAVE_ANSI_EXTERN
	int (*pficopy)(SBP_ENTRY *);		/* copy partition	*/
	int (*pfiumount)(SBP_ENTRY *);		/* unmount partition	*/
	void (*pfiparams)(char *);		/* decode :params	*/
#else
	int (*pficopy)();			/* copy partition	*/
	int (*pfiumount)();			/* unmount partition	*/
	void (*pfiparams)();			/* decode :params	*/
#endif
} SBP_METHOD;

#if !defined(MAX_BPI)
#define MAX_BPI	(1024*100)
#endif

#if HAVE_ANSI_EXTERN
extern void DoSync(int, char **, SBP_ENTRY *);
extern void MethHelp(FILE *);
extern void SetHow(char *);
#else
extern void DoSync();
extern void MethHelp();
extern void SetHow();
#endif
