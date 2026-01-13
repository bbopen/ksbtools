/* $Id: key.h,v 8.5 1998/09/19 21:15:30 ksb Exp $
 *
 * API keys.  These very simple string widgets are used to substitue
 * various strings between API modules.  I wish I had put these in in
 * version 7.0 -- but I didn't realize how useful they could be.
 *
 * Many of mkcmd's top level keywords should have been keys.
 */

#define KV_MASK		0x000f	/* key is...				*/
#define KV_NAMED	0x0000	/*   a named API key			*/
#define KV_CLIENT	0x0001	/*   a client list for a synth type	*/
#define KV_OPTION	0x0002	/*   a trigger's unnamed		*/
#define KV_ROUTINE	0x0003	/*   a routine's unnamed		*/
#define KV_OUTPUT	0x0004	/*   an output's unnamed		*/
#define KV_AUGMENT	0x0010	/* augmented values not added (yet)	*/
#define KV_CUR_INIT	0x0020	/* key is in the init state, locked	*/
#define KV_SYNTH	0x0040	/* key is made from synthetic data	*/
#define KV_V_ONCE	0x0100	/* we saw a "once" at init declaration	*/
#define KV_A_ONCE	0x0200	/* we saw a "once" on any augment XXX	*/
#define KV_DONE		0x1000	/* key is fully init'd			*/

typedef struct KVnode {
	ATTR wkattr;		/* key attributes			*/
	unsigned int uindex;	/* index into the EMIT_MAP array	*/
	char *pcowner;		/* the file that initializes us		*/
	char *pcversion;	/* API version compatible number	*/
	OPTION *pORin;		/* option that defined us		*/
	LIST LIvalues;		/* list of values			*/
	LIST LIinputs;		/* list of values before "init" seen	*/
	LIST LIaugments;	/* list of augment values, added late	*/
	/* used for error checking only */
	struct KVnode *pKVnext;	/* link to follow for check code	*/
	struct KVnode *pKVinit;	/* key that we init from		*/
	char *pcclient;		/* a file that used us			*/
	unsigned int uoline;	/* line number of owner			*/
	unsigned int ucline;	/* line number of first client		*/
} KEY;

#define KEY_NAMED(MpKV)		(KV_NAMED == ((MpKV)->wkattr & KV_MASK))
#define KEY_CLIENT(MpKV)	(KV_CLIENT == ((MpKV)->wkattr & KV_MASK))
#define KEY_OPTION(MpKV)	(KV_OPTION == ((MpKV)->wkattr & KV_MASK))
#define KEY_ROUTINE(MpKV)	(KV_ROUTINE == ((MpKV)->wkattr & KV_MASK))
#define KEY_OUTPUT(MpKV)	(KV_OUTPUT == ((MpKV)->wkattr & KV_MASK))
#define KEY_OPT_BOUND(MpKV)	((OPTION *)0 != ((MpKV)->pORin))
#define KEY_AUGMENT(MpKV)	(0 != ((MpKV)->wkattr & KV_AUGMENT))
#define KEY_CUR_INIT(MpKV)	(0 != ((MpKV)->wkattr & KV_CUR_INIT))
#define KEY_SYNTH(MpKV)		(0 != ((MpKV)->wkattr & KV_SYNTH))
#define KEY_V_ONCE(MpKV)	(0 != ((MpKV)->wkattr & KV_V_ONCE))
#define KEY_DONE(MpKV)		(0 != ((MpKV)->wkattr & KV_DONE))

#define KEY_NOSHORT	'~'	/* pass as arg2 when no short name given*/

extern KEY
	*pKVDeclare,		/* types that declare data		*/
	*pKVVerify;		/* types which use "verify" action	*/

extern KEY *KeyFind(/* char *, int */);
extern KEY *KeyAttach(/* iType, pOR, pcName, cName, ppKVInstall */);
extern KEY *KeyParse(/* char ** */);
extern int KeyEsc(/* char **, OPTION *, EMIT_OPTS *, char * */);
extern char *ValueEsc(/* ppcCursor, pcScan, pcWhere, iWhere, pOR, pEO */);
extern void KeyFix();
extern struct EHnode EHKey, EHValue;
