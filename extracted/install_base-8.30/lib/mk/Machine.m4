dnl $Id: Machine.m4,v 5.1 2004/12/07 14:08:15 ksb Exp $
dnl  We map the distrib HOSTTYPEs to values for`
dnl	cc	the C compiler
dnl	pc	the Pascal compiler
dnl	fc	the FORTRAN compiler
dnl	...
dnl
dnl and we set USE_NEWCOLCRT if we have -U for colcrt
dnl
dnl and we set CAT_S_OPTION to cat option for suppressing multiple blank lines
dnl
dnl and col instead of colcrt (only on srm, keep and RS6000s)
dnl
define(DEF_cc,ifelse(
HOSTTYPE,HPUX9,`gcc',
`cc'))dnl
define(DEF_fc,ifelse(
HOSTTYPE,`S81',`fortran',
HOSTTYPE,`SUN4',`/usr/lang/f77',
`f77'))dnl
define(DEF_pc,ifelse(
HOSTTYPE,`SUN4',`/usr/lang/pc',
`pc'))dnl
define(USE_NEWCOLCRT,ifelse(
HOSTTYPE,`S81',` -U',
HOSTTYPE,`VAX780',` -U',
HOSTTYPE,`VAX8800',` -U',
HOSTTYPE,`SUN3',` -U',
HOSTTYPE,`ETA10',` -U',
`'))dnl
define(CAT_S_OPTION,ifelse(
HOSTTYPE,`IBMR2',`-S',
`-s'))dnl
define(NROFF,ifelse(
HOSTTYPE,`V386',`awf',
HOSTTYPE,`FREEBSD',`groff',
HOSTTYPE,`EPIX',`/usr/bsd43/bin/nroff',
`nroff'))dnl
define(NROFF_TAB_OPT,ifelse(
HOSTTYPE,`V386',`',
NROFF,`groff',` -Tascii',
` -h'))dnl
define(TBL,ifelse(
HOSTTYPE,`V386',`cat',
HOSTTYPE,`EPIX',`/usr/bsd43/bin/tbl',
`tbl'))dnl
define(COLCRT,ifelse(
HOSTTYPE,`V386',`col',
HOSTTYPE,`EPIX',`col',
HOSTTYPE,`IBMR2',`col',
HOSTTYPE,`SUN5',`col',
`colcrt'))dnl
