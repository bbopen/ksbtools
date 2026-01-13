# $Id: patch-setargs.c,v 2.11 2009/09/30 17:16:03 ksb Exp $
*** src/setargs.c	Wed Apr 16 15:18:39 2008
--- src/setargs.c	Wed Apr 16 15:18:39 2008
***************
*** 62,66 ****
  char 			      **Argv = NULL;
  char 			       *LastArgv = NULL;
- char 			       *UserEnviron[MAXUSERENVIRON+1];
  
  /*
--- 62,65 ----
***************
*** 74,81 ****
  	register int 		i;
  	extern char 	      **environ;
  
    	/* Remember the User Environment */
  
! 	for (i = 0; i < MAXUSERENVIRON && envp[i] != NULL; i++)
  		UserEnviron[i] = strdup(envp[i]);
  	UserEnviron[i] = NULL;
--- 73,85 ----
  	register int 		i;
  	extern char 	      **environ;
+ 	static char	      **UserEnviron = (char **)0;
  
    	/* Remember the User Environment */
  
! 	for (i = 0; (char *)0 != envp[i] ; ++i)
! 		/* count them */;
! 	if ((char **)0 == (UserEnviron = (char **)xmalloc(sizeof(char *)*((i|7)+9))))
! 		error("out of memory");
! 	for (i = 0; (char *)0 != envp[i] ; ++i)
  		UserEnviron[i] = strdup(envp[i]);
  	UserEnviron[i] = NULL;
***************
*** 90,93 ****
--- 94,99 ----
  }
  
+ #ifndef HAVE_SETPROCTITLE
+ 
  /*
   * Set process title
***************
*** 166,169 ****
--- 172,177 ----
  }
  #endif	/* !ARG_TYPE */
+ 
+ #endif /* !HAVE_SETPROCTITLE */
  
  #endif 	/* SETARGS */
