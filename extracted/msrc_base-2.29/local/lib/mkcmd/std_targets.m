# this file contains comment lines that put
# mk(1) targets in the generated parser.				(ksb)
# $Id: std_targets.m,v 8.7 1997/10/27 11:55:22 ksb Exp $
#
comment "%ctargets for mk"
comment "%c%kCompile: $%{cc-cc%} -I/usr/local/include %%f"
comment "%c%kDepend: $%{maketd-maketd%} -a -d -I/usr/local/include %%f"
comment "%c%kLint: $%{lint-lint%} -hnx -I/usr/local/include %%f"
