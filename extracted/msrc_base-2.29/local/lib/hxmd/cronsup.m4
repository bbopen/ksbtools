dnl $Id: cronsup.m4,v 1.6 2009/11/09 17:11:24 ksb Exp $
dnl
dnl Functions to make it easier to expresss crontab lines.		(ksb)
dnl These assume that a 15-bit interger seed is bound to each target host.  That
dnl seed may be provided as cronsup_offset.
dnl Note that in a large population some offset collisions are usually fine.
dnl
dnl	$Test(*): efmd -C%s -I -- -j cronsup.m4 'cronsup_offset HOST' |oue -cl -a @ -e '@p:@[1 2]' -k '@[1 1]' -R "@c @k @(e,2-\\\$)" |sort -r -n -k1,1 -k2,2 |${PAGER:-less}
dnl
dnl maxhash => prime
dnl	This limits the value of default offsets, and to a prime less than
dnl	32768-8*255 (30728), like any of 30697, 30703, 30707, 30713, or 30727.
dnl	Tune to minimize collission for your list of hosts (via mk -mTest).
dnl	Some m4 implementations produce negative numbers for larger modulos.
ifdef(`cronsup_maxhash',`pushdef(cronsup_didhash,`no')',
	`pushdef(cronsup_didhash,`yes')pushdef(`cronsup_maxhash',`30707')')dnl
dnl
dnl ordinal(seed,hex_data) => hash
dnl	Create a number from the hex data based on a current hash, the 13 could
dnl	be tuned if that does not work for you.  The 8 is used to compute the
dnl	prime above (making it bigger means make the prime smaller).
pushdef(`cronsup_ordinal',
	`ifelse(`',$2,$1,
		`cronsup_ordinal(expr((13*($1+1))%cronsup_maxhash+8*`0x'substr($2,0,2)),substr($2,2))')')dnl
dnl
dnl seed(string[,n]) => offset
dnl	Generate a seed for the given string, the optional n allows you
dnl	to scale the weight of the digits (default first is 1, next 2,...).
dnl	The seed is not an offset hash, just part of its comupation.  Only
dnl	seed digits from the input string.  Slightly negative n is OK mostly.
pushdef(`cronsup_seed',
	`ifelse(`',$1,`',
		`ifelse(-1,index(`0123456789',substr($1,0,1)),`',`eval(substr($1,0,1)*ifelse(`',$2,1,$2))')`'cronsup_seed(substr($1,1),ifelse(`',$2,`1',`incr($2)'))')')dnl
dnl
dnl mkhash(string[,f1,f2,n])
dnl	Generate a offset hash from the given string and 2 numbers, the default
dnl	numbers work well for me (89 and 377).  Note that case of the string
dnl	doesn't change the generated hash -- I would NOT change that.
pushdef(`cronsup_mkhash',
	`cronsup_ordinal(expr(ifelse(`',$2,`89',$2)cronsup_seed($1,$4)+ifelse(`',$3,`377',$3)*len($1)),translit($1,`-_.abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ',`001abcdef12345678912345678937abcdef12345678912345678937'))')dnl
dnl
dnl cronsup_minutes(base,interval[,dow])
dnl	Given an interval of minutes, output a single cron time-spec
pushdef(`cronsup_minutes',
	`translit(cronsup_range(eval(0+ifelse(`',$1,cronsup_offset,$1)),eval(60+ifelse(`',$1,cronsup_offset,$1)),ifelse(`',$2,5,1,`eval(0 == $2)',2,$2),60,`@'),`@',`,') * * * 'ifelse(`',$3,`*',$3))dnl
dnl
dnl cronsup_range(cur,end,add,modulus,list)
dnl	Build the next offset time, return the list (order is not important)
pushdef(`cronsup_range',
	`eval($1 % $4)ifelse(0,eval(($1+$3)<$2),`',
		$5`cronsup_range(eval($1+$3),$2,$3,$4,`@')')')dnl
dnl
dnl cronsup_daily(base,start[,minutes[,dow]]) => min hr * * dow
dnl	Given a base offset, an hour (like 17) and a window in minutes (90),
dnl	output a cron timespec to run a daily task starting in the time-box.
dnl	The default day-of-week is `*', default start 22:00, minute 60
dnl		e.g. us(cronsup_offset,2,180) might output  `18 3 * * *'
dnl	This is also cronsup_weekly, just add the day-of-week to the end.
pushdef(`cronsup_daily',
	`cronsup__daily(ifelse(`',$1,cronsup_offset,$1),
		ifelse(`',$2,22,`expr($2%24)'),
		ifelse(`',$3,60,$3),
		ifelse(`',$4,`*',$4))')dnl
dnl
dnl cronsup__daily => <internal only>
dnl	(base,start,minutes,dow) same, but we don't have to check params
pushdef(`cronsup__daily',
	`pushdef(`cronsup_temp',`expr($1 % $3)')dnl
expr(cronsup_temp % 60) expr(($2+cronsup_temp/60)%24)` * * '$4`'popdef(`cronsup_temp')')dnl
dnl
dnl The default offset is built from the length of the FQDN, plus a seed from
dnl any literal digits in the name, and the ordinal of the FQDN.  Note that
dnl upper and lower case of the same name should get the same offset.
pushdef(`cronsup_offset',
	`cronsup_mkhash(ifelse(-1,index(HOST,`.'),`HOST',`substr(HOST,0,index(HOST,`.'))'))')dnl
dnl
dnl cronsup_pop
dnl	Remove all the macros we defined above
pushdef(`cronsup_pop',
	`ifelse(defn(`cronsup_didhash'),`yes',`popdef(`cronsup_maxhash')')popdef(`cronsup_didhash')popdef(`cronsup_offset')popdef(`cronsup_seed')popdef(`cronsup_range')popdef(`cronsup_minutes')popdef(`cronsup_ordinal')popdef(`cronsup_mkhash')popdef(`cronsup_daily')popdef(`cronsup__daily')popdef(`cronsup_pop')')dnl
dnl
dnl Examples
dnl cronsup_minutes(,4)		mk -smPri /some/peg/sampler/config
dnl cronsup_daily(,1,4,0)	mk -smSend /usr/local/libexec/netlint
dnl cronsup_daily(eval(cronsup_offset+2),1,4,0)	mk -smRecv /usr/local/...
`'dnl
