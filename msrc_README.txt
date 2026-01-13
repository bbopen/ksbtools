# $Id: README,v 2.34 2012/01/27 23:32:54 ksb Exp $

This is the 2011 version of the master source base code.   You'll need all
of this installed to build anything else I've ever released.  You'll also
need install_base to keep going.  Don't let that scare you, they are a lot
easier to build than they used to be.  Really.

It is possible to build this package with a single command-line.  It
is also possible to build RPMs from the package to deploy across your
entire domain.  I would build it on a test host first, then worry about
the RPMs or other packaging systems.


This build process comes in 2 parts.  You should have gotten two files from
the repo:
	msrc_base-2.28.tgz
	level2s
These are the matches, now you need to start the fire.

Unpack the tar file (if you've not already):
	$ cd /tmp/
	$ gunzip -c msrc_base-2.28.tgz | tar xf -
	$ cd msrc_base-2.28


0 Prerequisites
---------------

We need to see an m4 with the -I and -D options.  We want to have rdist
installed, and better yet have a patched one installed (that can wait).

0.1 Fix m4 to be your friend
----------------------------

For Solaris you'll have to get the GNU version of m4 and install it, if it
is not in /usr/sfw/bin/gm4 you might use the http://www.sunfreeware.com/
package which puts it in /opt/gnu/bin/m4.  You need this version of m4 to
be first in your search path to make all this work, as Sun's m4 doesn't take
"-d" options or "-I" for an include path, and the push-back is too small.
FreeBSD's works OK, so you can just use it.

0.2 Remove older versions of xapply, mkcmd
------------------------------------------

If you have the older version of xapply, Tee, or mkcmd installed from FreeBSD
ports you should de-install those before you start.

The older versions shouldn't confuse the build, but they might confuse you.
The new version don't break anything that I can think of, except that some
bugs in mkcmd had work-arounds that are now bugs.  You shouldn't notice any
of those (my "glob" program had one, and I never released it that way).

0.3 Building for HPUX 11.x requires more packages
-------------------------------------------------

Same as Solaris or FreeBSD but you need to install a lot of packaged
depot files first.  I used these last time, the version numbers may
have changed (from http://hpux.connect.org.uk/hppd/hpux/) [Jan 2012]:
	swinstall -s /tmp/zlib-1.2.5-ia64-11.23.depot zlib
		.../libiconv-1.14-ia64-11.23.depot libiconv
		.../gettext-0.18.1.1-ia64-11.23.depot gettext
		.../libgcc-4.2.3-ia64-11.23.depot libgcc
		.../gcc-4.2.3-ia64-11.23.depot gcc
		.../perl-5.10.1-ia64-11.23.depot perl
	swinstall -s /tmp/libpcap-1.2.1-ia64-11.23.depot libpcap
If you have a non-ia64 host you'll have to diddle the patch names.

You might need gzip (gzip-1.4) and m4 (m4-1.4.16) too.  Later you need
rdist, so get it now if you don't have it (or build it with my patches).

Then build as any other ( HOSTTYPE=HPUX11 HOSTOS=112200 or 112300).

Some programs are really not going to like the non-ANSI compiler HP ships
with the default OS.  You'll have to use gcc all the time.  I might make
a link from /usr/local/bin/cc to gcc to make that easier.


0.4 Building on Solaris requires some path munging
--------------------------------------------------

We need a BSD compatible install (/usr/ucb/install), a C compiler named "cc"
/usr/sfw/bin/gcc, and a "make" that understands -C, /usr/sfw/bin/gmake.
Build a directory some place to add to your path (or use /usr/local/bin):
	R=/usr/local/bin		# or maybe /opt/$SITE/bin and in $PATH
	mkdir -p $R
	ln -s /usr/sfw/bin/gmake $R/make
	ln -s /usr/sfw/bin/gcc $R/cc
	ln -s /usr/ucb/install $R/install
	ln -s /usr/sfw/bin/gm4 $R/m4
	PATH=$R:/usr/ccs/bin:/usr/sfw/bin:/usr/local/sbin:$PATH
	export PATH

N.B. in install_base we must remove the link for install.  Remember this.

0.4 Building on Linux requires some RPMs
----------------------------------------

We need gcc, flex, bison, and pam-devel
	yum install gcc flex bison pam-devel groff

We'll use "pam-devel" in the install_base package, so install that now.
We use "groff" to build man pages at the end.


1 Build this chain
------------------

We need to build this chain to get to the next one (install_base).  One of
the sections below should work for you pick one based on the OS you are most
like.  To pick one you first need to determine what I would call your host's
OS and which version number I'd pick for it.

Pick a "HOSTTYPE" from one of these if you see a match for your OS:
	SUN5	Any Solaris after 2.5.1
	FREEBSD	Any FreeBSD after 4.6, most OpenBSD versions
	FREEBSD Any DARWIN (MacOS) host, set HOSTOS to 41100
	NETBSD	Any NetBSD after 3.0
	IBMR2	Any AIX
	LINUX	Any RedHat after AS2.1, and most modern distros like Ubuntu
	HPUX9	HP-UX 9.03 or better
	HPUX10	HP-UX 10.x
	HPUX11	HP-UX 11.x
	*	there are some others, see the "machine.h" in bin/mkcmd
	MSDOS	I'm pretty sure that will NOT work anymore we depend on
		unix domain sockets now way too much.

Make a HOSTOS by turning the version or your OS or (kernel for Linux) into
a three digit "base 100" number.  For example, a FreeBSD 7.2 a need three
digits, so I make it 7.2.0 and convert that to 07.02.00, that makes "70200".
(I remove any leading zero).  Another example HP-UX 10.20 -> 102000.  Note
that some Solaris documentation says "5.8", others "2.8".  I use the lower
version (as in 20800).

The combination of "HOSTTYPE" and "HOSTOS" picks 95% of the configuration
for my code.  Other little details are usually worked out in the make
recipe files.  Even if you don't love these two attributes, they are what
you need to build my tools.  And you only need them once.  You can make up
a local attribute, or use all autoconfig spells with your own configuration
spells.  It doesn't make any difference to msrc how you configure your
code, I just happen to use those two macros all over *my* code.

If you can't find an OS that matches you can look in bin/mkcmd/machine.h
and try any of those.  Most SYSV-style hosts work as HPUX9, or so.

1.0 Build with the boot target
------------------------------

If you've never built these you best bet is to try (as root):
	# make HOSTTYPE="$HOSTTYPE" HOSTOS="$HOSTOS" INTO=/tmp/foo$$ boot

For example I just built a RedHat AS5.3 host with
	# make HOSTTYPE=LINUX HOSTOS=50400 INTO=/tmp/foo$$ boot
and it worked (and made about 310 lines of output).

Under Solaris 10, (with the PATH fixes from step 0.4):
	# make HOSTTYPE=SUN5 HOSTOS=21000 INTO=/tmp/foo$$ boot

Under FreeBSD 8.2:
	# make HOSTTYPE=FREEBSD HOSTOS=80200 INTO=/tmp/foo$$ boot

I've no HPUX11 host to test the build on.  But it should work if you have
the right tools installed.  I tested it in 2009 and it worked.

That should build and install all the tools in this chain if you got the type
and version set correctly.  If it worked the directory you gave as INTO
should have all the source configured and built for any host that looks just
like this one.  You could copy that over to any other host to install there,
but I wouldn't do that because it mostly defeats the whole point.

If you don't need the configured copy:
	# rm -r /tmp/foo$$

Skip to 2.0 if that worked, or to beat it into working.  If that didn't work
for you and you want to try the RPM build you can.


1.1 Build RPMs for msrc_base under RedHat
-----------------------------------------

If you have RPM support on your local host you can use the ITO.spec
file to build RPMs for msrc_base (and all the other chains after).
This is _way_ easier to get started.  If you already unpacked the archive
this came in (msrc_base-2.28.tgz) and that file is in .. then cd back to ..,
else if you've not unpacked the archive you should run (add "--wildcards"
to the tar command after the dash if your tar is strange):
	$ chmod a+x level2s
	$ sudo
	# ./level2s rpm msrc_base-2.28.tgz

The last command has to be done as root to write in /usr/src/redhat.
If you know how to make RPMs work as yourself you can do that.  Then
you still have to install the code as root.  That's fine too.

If you got that to work skip to the next section (2).


1.2 Build by hand to debug where we went wrong
----------------------------------------------

Follow the logic in the Makefile "boot" target.  See where it breaks and
find a way around the blockage.

If the build got as far as installing "/usr/local/lib/hxmd/auto.cf"
then you should be able to pick up where it left off.  If mmsrc itself
won't build we have a bigger issue (since it uses the autoconfig spell).

Make mmsrc source build from local/sbin/mmsrc:
	$ ./mmsrc -V
	mmsrc: $Id: mmsrc.m,v 1.50 2009/09/15 22:42:27 ksb Exp...
	mmsrc:  ...
	mmsrc: default utility: make
	mmsrc: temporary directory template: /tmp/ms02XXXXXX
	$ su -m
	# make install
	# exit
	$ cd ../../../

Then use it to build the other directories:
	$ su -m
	# D=`mktemp -d /tmp/bm28XXXXXX`
	# export D
	# for dir in lib/hxmd bin/mkcmd lib/mkcmd bin/explode lib/explode \
		bin/ptbw bin/xclate bin/xapply sbin/hxmd sbin/msrc
	  do
		( mkdir -p $D/local/$dir &&
		  cd msrc_base-2.28/$dir &&
		  mmsrc -C /tmp/my.cf -yINTO=$D/local/$dir -- make boot
		)
	  done 2>&1 | ${PAGER:-less}
	... lots of output and installed files later you should get a prompt
	...
	# exit
	$ mkcmd -V
	mkcmd: $Id: main.c,v 8.17 ...
	...
	$ msrc -V
	msrc: $Id: msrc.m,v 1.72 ...
	...
	$

When you get that to work skip to the next section (2).  It you can't
figure it out ask me for help, mail to anyname at ksb dot npcguild.org.


2 Install a local configuration
-------------------------------

The last step is to install a local configuration which allows you to push
software and other configuration files out to your other hosts.  That is
explained in the HTML files in the msrc*/local/lib/hxmd directory.  The
goal is to replace "auto.cf" with a list of your local hosts so we can
push up-to-date configuration files to them all with this automation.

The files you really need to touch are:
	/usr/local/lib/hxmd/auto.cf
		A description of the local host for mmsrc, like my.cf but
		maybe use the hosts FQDN rather than "localhost".  (Note
		lib/hxmd installed one for you, which should do.)

	/usr/local/lib/hxmd/$YOUR_DOMAIN.cf
		A description of the hosts you want to command and control
		with hxmd+msrc.  I have a few: "dmz.cf" for the fire-walled
		hosts and "npc.cf" for my common network.  But don't build
		too many of them, you can split them later if you like.

	/usr/local/sbin/$YOUR_DOMAIN (copy of dmz.sh)
		Install this file as $YOUR_DOMAIN in /usr/local/sbin, then
		you can push to single machine from your domain with a more
		compact command-line.  Get the source from
		msrc_base-2.28/local/sbin/msrc/dmz.sh and read about it in
		the HTML files there (msrc.html and qstart.html).


3 Other things you might want to do
-----------------------------------

You might unpack this to (or move it under) "/usr/msrc", which is where it
really wants to live.  But if you can't that's OK, the forced "INTO" macros
in the included recipe files (named "Makefile") keep it mostly sane.

You can install all the manual pages, as local convention allows.  The RPM
build installed them.  After you install "install_base" you can put do that
with the command prefix "mk -mInstall" on each manpage.

The ones in "local/bin" go in section 1, local/lib in section 5, and the
ones in local/sbin in section 8.  But mk takes care of that for you.


3.1 Hook up the HTML documents.
-------------------------------

You can hook this into a web server.  If you build a virtual host for this
directory you can map this directory as the DocumentRoot under, the cgi-bin
directory (which only has a manpage.cgi in it) should be mapped to
"/cgi-bin", but really doesn't belong under /usr/msrc, so you can move the
CGI (or the whole directory) to some other place and map it there.  With
that in-place you can view the many HTML files and manual pages under
/usr/msrc with a browser, which is pretty handy.

See "http://msrc.npcguild.org/" for an example site, which might be more
up to the minute as well (and slower).  The README file there is the
latest version of this file.  How self-referential is that?

3.2 Patch rdist
---------------

[You can skip this, if you did it in from quick-start page.]

The last part is a patch to rdist(1) that makes the "nodescend" option
actually work, and lets rdist pass down a larger environment.  See that in
patch-*.c and patch-gram.y in the tar file.  On FreeBSD you can put those
files in /usr/ports/net/rdist6/files (by the same names) and ports will build
an rdist6 in /usr/local/bin that works {from the /usr/ports/net/rdist6/
directory}.  You can use that later in the RDIST_PATH macro.  Without these
patches rdist is useless, to me.  You could rename or link rdist6 to rdist,
since rdist 4 is not actually useful in any case.

I may have to take up support for rdist, is seems nobody loves it.  More
likely I'll move to the rZync I'm working on.

3.3 Build peer hosts
--------------------

Once this machine is enabled push to lots of other servers with:
	$ make MYSITE="-C $YOUR_DOMAIN.cf" push


3.4 Read the quick start page
-----------------------------
Read qstart.html (http:.../msrc/local/sbin/msrc/qstart.html) for
more details about this, if you have not already.


4.0  Advance to install_base
----------------------------

You must install install_base to get father along here.  Nothing will work
as documented without my install program and the other tools in there.  And
you really want to use op(1) rather than sudo(1), really.  Also mk(1) is
way nifty for simple CM tasks at every level.  I _always_ use it for
crontab tasks.

Download install_base at the latest release and boot that next.  It is way
easier now that you have the basic tools installed.  If you can't make this
go you can email me at my gmail account, I'm Kevin.Braunsdorf there.

4.1 Build your own copy of msrc locally
---------------------------------------

If you'd like to get all the "ksb tools" unpacked I've built a script that
unpacks all the packages and products into a local "msrc" directory.  This
makes it way easier to setup a local web-cache with all the HTML docs linked
to each other.

I cleaned up the script I use to build my website.  First make an empty
directory in /tmp (or /var/tmp), then download all the *.tgz and *.tbz files
from <http://carrera.databits.net/~ksb/dl/> into "dl/" then download the
script from <http://carrera.databits.net/~ksb/make-msrc.sh> into the current
directory.

Read the script.  Then run it as a mortal login.  It unpacks all the dl/*
files into the correct place under a new "msrc/" subdir.  You can migrate
the new msrc hierarchy to a webserver, install the manpage.cgi in a cgi-bin
and read all the docs locally (you may have to install all the manpages on
that server as well).



5.0 Ignore the Pkgs directory
-----------------------------

This directory is the seed that rebuilds this package from an updated master
source copy of itself.  It is referenced by later packages.  It is pretty
nifty how it includes itself in itself --  I've never seen anything quite
like it.  And the script that puts the parts back into the orginal structure
is another gee-whiz moment.


--
ksb, Jan 2012
