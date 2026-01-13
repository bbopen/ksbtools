/* $Id: scan.h,v 8.2 1997/10/18 12:57:08 ksb Exp $
 */
typedef int TOKEN;

typedef struct TEnode {
	TOKEN tvalue;
	char acname[40];
	int iunq;
} TENTRY;

extern int iLine;
extern char *pchScanFile;
extern FILE *fpDivert, *fpHeader, *fpTop;

extern TOKEN GetTok();
extern void SetFile();
extern char *TextTok();
