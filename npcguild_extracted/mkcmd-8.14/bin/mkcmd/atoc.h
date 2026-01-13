/* $Id: atoc.h,v 8.0 1997/01/29 02:41:57 ksb Exp $
 * convert a string of input chars into C code ascii string		(ksb)
 * input	m = "\n";
 * output 	"m = \"\\n\";\n",
 */
#define TTOC_NOP	0x0000	/* do nothing special			*/
#define TTOC_PERCENT	0x0001	/* this is a printf format string	*/
#define TTOC_NEWLINE	0x0002	/* always break strings at newlines	*/
#define TTOC_BLOCK	0x0004	/* never break the string for newlines	*/

#if defined(__STDC__)
extern void atoc(FILE *, char *, char *, char *, int, int);
#else
extern void atoc();
#endif
