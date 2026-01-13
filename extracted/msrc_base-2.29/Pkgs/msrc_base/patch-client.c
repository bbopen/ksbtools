# $Id: patch-client.c,v 2.10 2008/11/24 18:43:16 ksb Exp $
--- src/client.c	2008-03-17 16:13:02.000000000 -0500
+++ src/client.c	2008-03-17 16:26:55.000000000 -0500
@@ -578,12 +578,6 @@
 	int len;
 	int didupdate = 0;
 
-	/*
-	 * Don't descend into directory
-	 */
-	if (IS_ON(opts, DO_NODESCEND))
-		return(0);
-
 	if ((d = opendir(target)) == NULL) {
 		error("%s: opendir failed: %s", target, SYSERR);
 		return(-1);
@@ -597,6 +591,12 @@
 	if (response() < 0)
 		return(-1);
 
+	/*
+	 * Don't descend into directory
+	 */
+	if (IS_ON(opts, DO_NODESCEND))
+		return closedir(d);
+	
 	if (IS_ON(opts, DO_REMOVE))
 		if (rmchk(opts) > 0)
 			++didupdate;
@@ -761,7 +761,7 @@
 	unsigned short rmode;
 	char *owner = NULL, *group = NULL;
 	int done, n;
-	u_char *cp;
+	char *cp;
 
 	debugmsg(DM_CALL, "update(%s, 0x%x, 0x%x)\n", rname, opts, statp);
 
