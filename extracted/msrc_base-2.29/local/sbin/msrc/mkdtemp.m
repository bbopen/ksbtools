#!mkcmd
# $Id: mkdtemp.m,v 1.72 2008/07/09 19:55:47 ksb Exp $
# This solves a forward definition error on platfroms that do not have
# mkdtemp (HPUX11, for example)
require "mkdtemp.c"

%i
extern char *mkdtemp(char *);
%%
