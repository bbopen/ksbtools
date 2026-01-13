/* -*-C-*-
********************************************************************************
*
* File:         opcapi.h
* RCS:          $Header: /usr/msrc/usr/local/etc/tpsh/RCS/opcapi.h,v 2.0 1996/11/07 17:35:40 kb207252 Exp $
* Description:  This file contains constants and function prototypes to use the
*               OpC message and monitor API.
* Language:     ANSI C
* Package:      HP OpenView OperationsCenter
* Status:       @(#)OpC A.01.10 (08/05/94)
*
* (c) Copyright 1993, Hewlett-Packard Company, all rights reserved.
*
********************************************************************************
*/

#ifndef OPCAPI_INCLUDED
#define OPCAPI_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

/* Severity values to be passed in the severity parameter. */
#define OPC_SEV_NORMAL       (0)
#define OPC_SEV_WARNING      (1)
#define OPC_SEV_CRITICAL     (2)
#define OPC_SEV_UNKNOWN      (3)

/* The local node to be used in the node parameter. */
#define OPC_LOCAL_NODE       ((char *) 0)

/* Error codes. */
/*--------------*/

#define OPC_ERR_OK                  (0)       /* No error. */

/* Parameter errors. */
#define OPC_ERR_APPL_REQUIRED       (-1)      /* Application parameter empty. */
#define OPC_ERR_OBJ_REQUIRED        (-2)      /* Object parameter empty. */
#define OPC_ERR_TEXT_REQUIRED       (-3)      /* Msg text parameter empty. */
#define OPC_ERR_INVAL_SEVERITY      (-4)      /* Unknown severity value. */
#define OPC_ERR_OBJNAME_REQUIRED    (-5)      /* Name of monitored object */
                                              /* empty. */
#define OPC_ERR_MISC_NOT_ALLOWED    (-6)      /* Message group 'Misc' is not */
                                              /* allowed. */
#define OPC_ERR_INVAL_NODE          (-7)      /* Invalid node parameter. */

/* Runtime errors. */
#define OPC_ERR_NO_MEMORY           (-8)      /* Out of memory. */
#define OPC_ERR_NO_AGENT            (-9)      /* The message interceptor resp.*/
                                              /* monitor agent is not running.*/
/* OpC errors. */
#define OPC_ERR_CANT_INIT           (-10)
#define OPC_ERR_NO_QUEUE_NAME       (-11)
#define OPC_ERR_CANT_OPEN_QUEUE     (-12)
#define OPC_ERR_CANT_WRITE_QUEUE    (-13)
#define OPC_ERR_NO_PIPE_NAME        (-14)
#define OPC_ERR_CANT_OPEN_PIPE      (-15)
#define OPC_ERR_CANT_GET_LOCAL_ADDR (-16)

/* Function prototypes. */
/*----------------------*/

#  if defined(__STDC__) || defined(__cplusplus)

extern int opcmsg(const int severity,
                  const char *application,
                  const char *object,
                  const char *msg_text,
                  const char *msg_group,
                  const char *node
                  );

extern int opcmon(const char *objname,
                  const double monval
                  );

#  else /* not __STDC__ || __cplusplus */

extern int opcmsg();
extern int opcmon();

#  endif /* __STDC__ || __cplusplus */

#ifdef __cplusplus
}
#endif

#endif  /* OPCAPI_INCLUDED */

/*
********************************************************************************
* end of opcapi.h
********************************************************************************
*/
