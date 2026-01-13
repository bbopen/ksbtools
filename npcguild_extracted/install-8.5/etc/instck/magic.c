/*
 * Copyright 1990 Purdue Research Foundation, West Lafayette, Indiana
 * 47907.  All rights reserved.
 *
 * Written by Kevin S Braunsdorf, ksb@cc.purdue.edu, purdue!ksb
 *
 * This software is not subject to any license of the American Telephone
 * and Telegraph Company or the Regents of the University of California.
 *
 * Permission is granted to anyone to use this software for any purpose on
 * any computer system, and to alter it and redistribute it freely, subject
 * to the following restrictions:
 *
 * 1. Neither the authors nor Purdue University are responsible for any
 *    consequences of the use of this software.
 *
 * 2. The origin of this software must not be misrepresented, either by
 *    explicit claim or by omission.  Credit to the authors and Purdue
 *    University must appear in documentation and sources.
 *
 * 3. Altered versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 * 4. This notice may not be removed or altered.
 */

/*
 * these routines check the installation of binary (mostly) files	(ksb)
 * we look for glob expressions to match in given directories, and
 * at what modes the matched files have.  If they look OK we say
 * nothing, else we bitch about the errors.
 */
#if !defined(lint)
static char *rcsid = "$Id: magic.c,v 8.3 1997/02/15 21:12:47 ksb Exp $";
#endif	/* !lint */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>

#include "machine.h"
#include "configure.h"
#include "install.h"
#include "main.h"
#include "syscalls.h"
#include "special.h"
#include "instck.h"
#include "magic.h"
#include "gen.h"
#include "path.h"

#if HAVE_SGSPARAM
#include <sgsparam.h>
#endif

#include <ar.h>
#if HAVE_RANLIB
#if HAVE_RANLIB_H
#include <ranlib.h>
#endif
#endif

#if USE_FILEHDR
#include <filehdr.h>
#else
#if USE_ELF
#include <libelf.h>
#else
#if USE_SYS_EXECHDR_H
#include <sys/exechdr.h>
#else
#include <a.out.h>
#endif
#endif
#endif

extern off_t lseek();


/* return either "setuid" or "setgid" if the program is loaded with	(ksb)
 * `#!...' and is currently an unsafe mode
 */
#define	BAD_LEN	2
char *
BadLoad(fdFile, mMode)
int fdFile;
int mMode;
{
	auto char acLook[8];

	if (-1 == lseek(fdFile, (off_t)0, 0)) {
		fprintf(stderr, "%s: lseek: %d: %s\n", progname, fdFile, strerror(errno));
		return (char *)0;
	}
	if (BAD_LEN != read(fdFile, &acLook[0], BAD_LEN)) {
		return (char *)0;
	}
	if ('#' != acLook[0] || '!' != acLook[1]) {
		return (char *)0;
	}
	if (0 != (S_ISUID&mMode)) {
		return "setuid";
	}
	if (0 != (S_ISGID*mMode)) {
		return "setgid";
	}
	return (char *)0;
}
#undef BAD_LEN

/* is the file in question a binary and stripped?			(ksb)
 * -1 not a binary
 * 0 binary, not stripped
 * 1 binary, stripped
 */
int
IsStripped(fdFile, pstBin)
int fdFile;
struct stat *pstBin;
{
#if USE_ELF
	register Elf *elf;
	register Elf32_Ehdr *ehdr;
	auto int iRet;
#else
	auto EXHDR exhd;
#endif

	if (S_IFREG != (pstBin->st_mode & S_IFMT)) {
		/* libs must be plain files */
		return -1;
	}
	if (-1 == lseek(fdFile, (off_t)0, 0)) {
		fprintf(stderr, "%s: lseek: %d: %s\n", progname, fdFile, strerror(errno));
		return -1;
	}
#if USE_ELF
	elf = elf_begin(fdFile, ELF_C_READ, (Elf *)0);
	switch (elf_kind(elf)) {
	case ELF_K_ELF:
		if ((Elf32_Ehdr *)0 == (ehdr = elf32_getehdr(elf))) {
			iRet = 0;
			break;
		}
		iRet = ET_EXEC == ehdr->e_type ? (ehdr->e_shnum == 0 ? 1 : 0) : -1;
		break;
	case ELF_K_COFF:
		iRet = 0;
		break;
	case ELF_K_NONE:	/* nill elf pointer		*/
	case ELF_K_AR:		/* file.a			*/
	default:
		iRet = -1;
		break;
	}
	elf_end(elf);
	return iRet;
	/*NOTREACHED*/
#else
	if (sizeof(exhd) != read(fdFile, (char *) &exhd, sizeof(exhd))) {
		return -1;
	}
	switch (MNUMBER(exhd)) {
#if defined(A_MAGIC1)
	case A_MAGIC1:			/* more HP cruft?		*/
#endif
#if defined(A_MAGIC2)
	case A_MAGIC2:			/* more HP cruft?		*/
#endif
#if defined(FMAGIC)
	case FMAGIC:			/* HP format?			*/
#endif
#if defined(OMAGIC)
	case OMAGIC:			/* old impure format		*/
#endif
#if defined(NMAGIC)
	case NMAGIC:			/* read-only text		*/
#endif
#if defined(QMAGIC)
	case QMAGIC:			/* compact format (DEPRICATED)	*/
#endif
#if defined(ZMAGIC)
	case ZMAGIC:			/* demand load format		*/
#endif
#if defined(ETAMAGIC)
	case ETAMAGIC:			/* eta systems load format	*/
#endif
		if (0 == exhd.MSYMS)
			return 1;
		return 0;
	default:
		break;
	}
	return -1;
#endif /* ELF or a.out style binary files */
}

/* is the named file a ar random library				(ksb)
 * Read the ar magic number, is it a library?
 * read the first ar member, is it __.SYMDEF
 * is __.SYMDEF OLDer that the file (only ranlib does this)
 */
int
IsRanlibbed(fdFile, pstLib)
int fdFile;
struct stat *pstLib;
{
	extern long atol();
#if !defined(ARMAG) && defined(AIAMAG)
#define ARMAG	AIAMAG
#endif

#if !defined(SARMAG)
#if defined(SAIAMAG)
#define SARMAG	SAIAMAG
#else
	static char ar_gross[] = ARMAG;
#define SARMAG (sizeof(ar_gross)-1)
#endif
#endif

#if HAVE_SYMDEF
	static char acTOC[] = "__.SYMDEF";
#endif
#if USE_ELF
	register Elf *elf;
	register Elf32_Ehdr *ehdr;
	auto int iRet;
#else
	auto char acBuf[SARMAG+10];
#endif
#if HAVE_SYMDEF
	auto struct ar_hdr arhd;
#endif

	if (S_IFREG != (pstLib->st_mode & S_IFMT)) {
		/* libs must be plain files */
		return -1;
	}
	if (-1 == lseek(fdFile, (off_t)0, 0)) {
		fprintf(stderr, "%s: lseek: %d: %s\n", progname, fdFile, strerror(errno));
		return -1;
	}
#if USE_ELF
	elf = elf_begin(fdFile, ELF_C_READ, (Elf *)0);
	switch (elf_kind(elf)) {
	case ELF_K_AR:		/* file.a			*/
		iRet = 1;
		break;
	case ELF_K_ELF:
	case ELF_K_COFF:
	case ELF_K_NONE:	/* nill elf pointer		*/
	default:
		iRet = -1;
		break;
	}
	elf_end(elf);
	return iRet;
#else
	if (SARMAG != read(fdFile, acBuf, SARMAG)) {
		return -1;
	}
	if (0 != strncmp(ARMAG, acBuf, SARMAG)) {
		return -1;
	}
#if HAVE_SYMDEF
	if (sizeof(arhd) != read(fdFile, (char *)& arhd, sizeof(arhd))) {
		return 0;
	}
	if (0 != strncmp(acTOC, arhd.ar_name, sizeof(acTOC)-1)) {
		return 0;
	}
	return pstLib->st_mtime <= atol(arhd.ar_date);
#else
	return 1;
#endif
	/*NOTREACHED*/
#endif
}
