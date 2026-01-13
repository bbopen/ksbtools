dnl $Id: class.m4,v 1.17 2010/02/02 23:06:24 pv Exp $
dnl Compute the class for a host (from any site.cf, for example).    (ksb/jad)
dnl Include this in your script, use the CLASSOF(host) macro where needed,
dnl or use hxmd's -j feature.
dnl Note the "-I --" passed an include path to m4, which is what you need.
dnl Some versions of m4 (viz. Sun) do not like it, so install BSD or GNU m4.
dnl We used to divert and we shall not because -Y diversion get broken by that.
dnl
dnl Use as a host selection criteria with -Y:
dnl	WANT=$name mk -d local.cf $THIS_FILE
dnl  $Compile(*): hxmd -C%s -I -- -Y "include(%f)" -E "%{WANT}=CLASS" 'echo HOST'
dnl
dnl In the command string you might call to find the another host
dnl	REPO is a macro for some other host each references:
dnl  $Compile(*): hxmd -C%s -P1 -I -- "include(class.m4)echo HOST CLASSOF(REPO)"
dnl
dnl Or include a commmand line string for xapply's dicer (-F2) like this:
dnl  $Compile(*): hxmd -C%s -P1 -I -- -F2 'echo %%1 %%(1,$-1)' 'include(%f)CLASS'
dnl
dnl Now the implementation, which you will have to tune for local site policy:
dnl
dnl chomp(WORD,tail) -> word with tail.* removed
dnl
pushdef(`CLASS_chomp',
	`ifelse(-1,index($1,$2),$1,`substr($1,0,index($1,$2))')')dnl
dnl
dnl basename(SHORTHOST, `tail1',`tail2',...`tailN') -> basename
dnl
pushdef(`CLASS_basename',
	`ifelse(`',$2,$1,`CLASS_basename(CLASS_chomp($1,$2),shift(shift($@)))')')dnl
dnl
dnl base(hostname) -> a guess at the class based on local naming
dnl
pushdef(`CLASS_base',
	`CLASS_basename($1,`.fedex.com',`.sac',`.zmd',`.dmz',`.list',`.idev',`.ecdev',`-new',`test',`beta',`lab',`stress',`0',`1',`2',`3',`4',`5',`6',`7',`8',`9')')dnl
dnl
dnl CLASSOF(host) -> a guess at the class from the name and the table below
dnl
pushdef(`CLASSOF',
	`pushdef(`CLASS_b',CLASS_base($1))ifelse(0,index($1,`lnx-'),`linux',
0,index($1,`sol-'),`solaris',
CLASS_b,`behest',`request',
CLASS_b,`crewtoo',`crew',
CLASS_b,`grace',`dns',
CLASS_b,`kate',`dns',
CLASS_b,`land',`dns',
CLASS_b,`ideal',`perfect',
CLASS_b,`meg',`svr',
CLASS_b,`mem',`tsm',
CLASS_b,`rad',`vanallen',
CLASS_b,`van',`vanallen',
CLASS_b,`allen',`vanallen',
CLASS_b,`scorch',`vanallen',
CLASS_b,`blackbody',`vanallen',
CLASS_b,`knot',`untie',
CLASS_b,`pod-zmd',`wjax',
CLASS_b,`pod-dmz',`www',
CLASS_b)`'popdef(`CLASS_b')')dnl
dnl
dnl This shorthand should make for cleaner integration with hxmd --jad
dnl
pushdef(`CLASS',`CLASSOF(HOST)')dnl
dnl
dnl pop() -- The common undo spell
dnl
pushdef(`CLASS_pop',
	`popdef(`CLASS_pop')popdef(`CLASS')popdef(`CLASS_chomp')popdef(`CLASS_basename')popdef(`CLASS_base')popdef(`CLASSOF')')dnl
`'dnl
