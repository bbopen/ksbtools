/* $Id: config.plain,v 1.2 2008/10/17 18:07:43 ksb Exp $
 *
 * Tell the msrc machine.h that we are not autoconfigured (yet)
 * the configure script overwrites this file when run and never
 * defines this specail macro.  (I know that's an odd invarient.)
 */
#if !defined(USE_MSRC_DEFAULT)
#define USE_MSRC_DEFAULT	1
#endif
