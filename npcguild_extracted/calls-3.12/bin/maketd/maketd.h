/*
 * $Id: maketd.h,v 4.12 1997/10/01 21:10:39 ksb Exp $
 *
 * here is the real brains of maketd
 *
 * a DependInfo node contains all the info we need to generate depends
 * for a target.
 */
#define BUFSIZE	256
#define MAXCOL	78

typedef struct DInode {
	int
		localo,		/* 0 == local o, 1 == non-local o	*/
		binarydep;	/* 0 == not binary, 1 == binary		*/
	char
		*filename,	/* The filename to process (optarg)	*/
		*inlib,		/* The library in which we live		*/
		*cppflags,	/* The CPP flags for this file (-CDIU)	*/
		*destdir,	/* The destination directory (-o)	*/
		*suffix,	/* The suffix for the dest file (-s)	*/
		*explicit,	/* the explicit rule to use, if any	*/
		*basename,	/* The basename for the dest file (-t)	*/
		*preprocessor;	/* The preprocessor to use (-c, -4, -f)	*/
	struct DInode
		*next;		/* The next structure in the linked list*/
} DependInfo;

extern DependInfo
	*pDIList;
extern char
	*pchBackup;	/* the backup file name				*/
