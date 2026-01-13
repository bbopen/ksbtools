# $Id: README,v 1.9 2000/07/30 22:20:14 ksb Exp $

This is the meta source fake-out code.  You need this to get started.


Start with a fake to the environment:
-------------------------------------

We fake having a meta source platform installed to build a ksb-meta-source
product on a machine that doesn't have ksb's tools installed yet.  This is
the part you need to get all the nifty toys started.

Type
	$ ./makeme

to see if we can auto-configure your hosttype out of the box.

If we can you can install "makeme" someplace (like in _your_ bin) where
we can see it long enough to get "mkcmd" and the other boot strap tools
installed (like install).

Else we need to boot-strap the meta source system by setting the environment
variable "HOSTTYPE" to one of these:
	FREEBSD	- FreeBSD 2.1 or better

	NETBSD	- if your FreeBSD is older or really NetBSD

	VAX780	- a Vax running 4.3 BSD or so -- a well known reference

	SUN3	- SunOS 4.0 (on old Sun hardware running)
	SUN4	- SunOS 4.1 (on sun3 or sun4* hardware)
	SUN5	- Solaris 2.3 to 2.5.1 (on sun4* hardware)

	HPUX7	- HP-UX release A 7.*
	HPUX8	- HP-UX release A 7.*
	HPUX9	- HP-UX release A 9.*
	HPUX10	- HP-UX release A 10.*

	IRIX5	- SGI Irix 5.?
	IRIX6	- SGI Irix 6.2 or so
	IBMR2	- AIX 3.? or 4.?

    CONSENSYS	- Consensys UNIX on Intel x86

	SCO	- SCO UNIX or some Xenix

	V863	- strange DEC console system

	EPIX	- you know if you've got it

	ULTRIX	- DEC Ultrix 2.0 maybe some later revs (I don't have any)

	NEXT1	- Really old (pre-release almost) NeXT cubes {not supported}
	NEXT2	- NeXT Step almost any version other than _really_ old (NEXT1)

So you should
	$ HOSTTYPE=FREEBSD
	$ export HOSTTYPE
	$ ./makeme

to see something like:
	Make.host: Our host type is HPUX9 about number 905
	Make.host: We are foo.panic.com with a short name of "foo"
	Make.host: We have sources (yes).

You can set HOSTOS, HOST, SHORTHOST, and HASSRC to change these boot
strap value.  You might not want to bother, if the msrc package builds
life gets _so_ much easier you won't believe it.

You can try other hosttype values (BSDI for example) -- some programs know
them and some don't.  If you send me patches for other platforms I will
include them in later releases.  Please send to "ksb@sa.fedex.com".


Debug notes:
------------

+ Some versions of bash (the GNU shell) export a variable HOSTTYPE and
  confuse the "makeme" shell script.  If you suffer from this I believe you
  can work around it.  (You can always hard code "makeme" to get started.)

+ The message "expr: syntax error" usually means that the environment
  variable "HOST" has a space in it.  Unset it or set it to a hostname.

+ HOSTTYPE is not a CPU type (like uname -m or uname -a on a Sun outputs):
  it is the OS name + a release level number (HPUX + 9 for example).
  Some of them don't have numbers.  I was wrong to do that.

At this point "makeme" must output something you think is sane.  Mine
said:
	Make.host: Our host type is FREEBSD about number 302
	Make.host: We are w01.sac.fedex.com with a short name of "w01"
	Make.host: We have sources (yes).

Next you can either:
	1) build from local tar archives of the master source,
	2) build from a network cache of the master source,

The network "rsrc" is new to most msrc users, but it pretty easy
to use (if you can see a rsync, ftp, or http service that has the
msrc available on it).  See PULL in this directory for more details.
Read the steps to decide which to do.


To build from the archived packages:
-----------------------------------

Pick an FTP site, or CDROM, or what ever you have to get the packages
from.  I used to keep stuff at
	ftp://ftp.physics.purdue.edu/pub/pundits/

+ Pick a place with some disk space that you can work, like "/usr/msrc", or
  "~/msrc".
	$ df -k /usr
	...
	$ mkdir /usr/msrc
	$ cd /usr/msrc

+ Don't use "/usr/src", use some other place because later we are going to
  map stuff to /usr/src and we can't use the same name space.  Really.

+ Build a subdirectory (really) call it "local" or name it after your local
  political power node ("custom", "elks", "demo", "sa", "walnuts"...).
  Then cd down into the place you made.  Remember the top level.
	$ mkdir local Pkgs
	$ TOP=`pwd`

You will need GNU gunzip to get going because the Pkgs are all gzip'd.
Get it from ftp://prep.ai.mit.edu/ if you don't have it (I use 1.2.4).


Start building packages:
------------------------

We are going to make stuff in big hunks called packages.  A package
in the "master source" world is related application that are distributed
in a release.

+ cd into you Pkg directory:
	$ cd $TOP/Pkgs

+ Get the "mkcmd-*" package from the your data source

+ Unpack mkcmd.  Drop down into mkcmd's bin:
	$ zcat $HOME/mkcmd-8.10.tgz | tar xf -
	$ cd mkcmd*/bin/mkcmd

+ Use "makeme" to force the silly (as far as you know now) msrc to build like
  any other product in the world.  Run `./mkcmd -V'
	$ makeme
		gcc -O .... scan.c
		....
	$ ./mkcmd -V
	mkcmd: $Id:....
	mkcmd: templates from...

  If the template directory is not someplace you can write see the Make.host
  file for a macro to diddle (or machine.h).  Most products have these to
  configuration files.  Remember this for later.

+ If the tuning looks OK (the templates may be installed there) you can
  install mkcmd in the search path someplace.  You need mkcmd to install
  EVERY OTHER THING I've written in the last 14 years.
	$ makeme BIN=/usr/local/bin boot

+ Then install mkcmd's lib. (Replace /usr/local/lib/mkcmd with whatever path
  you made mkcmd think the templates are in.)
	$ cd ../../lib/mkcmd
	$ makeme LIB=/usr/local/lib/mkcmd boot

+ You should have a working mkcmd at this point.  This is more than 1/2 the
  battle.  Some programs need "explode" and it's library of portable C
  routines.  Just repeat the steps we did above:
	$ cd ../explode
	$ makeme LIB=/usr/local/lib/explode boot
	$ cd ../../bin/explode
	$ makeme all
	$ ./explode -V
	explode: $Id...
	...
	$ makeme BIN=/usr/local/bin boot

+ Install explode's library to finish the mkcmd install:
	$ cd ../../lib/explode
	$ makeme LIB=/usr/local/lib/explode boot

+ Take a short break.  FTP down the next package (install-* would be the
  next one I'd do so we can stop using "cp ... ; chmod ..." to install stuff).

Build install-* package:
------------------------

+ Return to the Pkgs level
	$ cd $TOP/Pkgs

+ Unpack the install-* package, so we can build it.
	$ zcat install-*z | tar xf -

+ Use "makeme" to build install.  My install package can be hidden from
  normal users in a place only the meta source sees, but you really want to
  use "install" from my set because it makes minus one backout copies to save
  you from mistakes.
	$ cd install-*/
	$ vi INSTALL README
	$ cd bin/install.d
	$ vi configure.h
	$ makeme all
	$ ./installp -V
	installp: $Id...
	....

+ Now decide to install installp over the top of the system install (maybe)
  or put it in /usr/local/bin/install or hide it in /usr/local/lib/distrib/bin
  (the last case is explained in the msrc docs, but you need to do it now).
	? mkdir -p /usr/local/lib/distrib/bin

	$ ./installp -vC/dev/null ./installp ....
	$ makeme clean

+ Change your PATH (for this shell at least) to find that install program
  first.  Export that PATH if you didn't.

+ If you are going to use my install as the system install you should
  build "purge" and "instck" from this kit as well.
	$ cd ../../etc/purge
	$ makeme all
	$ ./purge -V
	purge: $Id...
	....
	$ makeme install
	$ makeme clean

	$ cd ../instck
	$ makeme
	$ ./instck -V
	instck: $Id...
	instck: ...
	$ makeme install
	$ makeme clean

+ Later (write this down) edit root's crontab to run "purge -d 48" every
  week after the level 0 dumps are finished (or there abouts).

+ If you want normal users to (maybe) install some system files you might
  build "installus".  Read about it, or save it until after this is done.



Build the msrc-* package:
-------------------------

+ Return to the package level
	$ cd $TOP/Pkgs

+ Unpack the msrc-* kit so we can build that.
	$ zcat msrc-* | tar xf -

+ Use "makeme" to build distrib.  It remembers the HOSTTYPE you have set
  when you build it so don't let it mis-match the real hosttype.  If you
  have a new hosttype to register mail it to me (ksb@fedex.com).
	$ cd msrc-*
	$ vi README INSTALL

	$ cd local/etc/distrib
	$ makeme all
	$ ./distrib.x -V -C/dev/null
	distrib.x: $Id...
	.... lines of stuff

	# makeme boot

+ Last "walking" step (we run after this).  In the INSTALL and README there
  is a lot of talk about building a distrib.cf (or more than one) to push
  this stuff out to other OS+CPU types (HOSTTYPEs) with some automated stuff.
  This is the _big_ payoff.  Build that table as described.
	$ cd ../../lib/distrib
	$ vi README
	...
	$ distrib -aH | more

+ Now we set up equivalent host access for who ever we are (~root/.rhosts
  on the remote machine or /etc/hosts.equiv).  Follow the steps in
  "Papers/new-platform" to bring mkcmd and such up on the remote machines.

+ Read the papers about converting your code to use the msrc stuff.
  You can push updated products to any host configured in seconds.
  For a few hours work on the front end you'll get back hours/day.


Thanks to all:
--------------

Thanks to Greg Becker <greg@katin.codeconcepts.com> for helping me debug
the order of installation to get this stuff started and ports to LINUX.

Thanks to the SAC (FedEx's SysAdmin and Consulting) group at FedEx who
supported all this work, and their manager Glenn Geiger.

Also to my brother, Allen Braunsdorf, who helped me make "new-platform"
clearer and faster.

--
ksb, 30 July 2000
