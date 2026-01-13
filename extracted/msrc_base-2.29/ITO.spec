# $Id: ITO.spec,v 2.27 2010/08/14 18:43:18 ksb Exp $
# $KeyFile: ${echo:-echo} Makefile.meta
# $Level3: ${echo:-echo} msrc_base

%define _local_prefix		/usr/local
%define _local_bindir		%{_local_prefix}/bin
%define _local_sbindir		%{_local_prefix}/sbin
%define _local_libdir		%{_local_prefix}/lib
%define _local_mandir		%{_local_prefix}/man
# the logic for HOSTTYPE and HOSTOS has been moved to later in the script.

# for signing of packages
#%_signature gpg
#%_gpg_name FIXME@example.org

# Do not generate debugging packages by default - older versions of rpmbuild
# (like RehHat 6.2) choke on the line with the percent in the macro name:
# We need a sed spell on RH6.x to "s/\(^.define \)%d/\1d/", Sigh. -- ksb
%define debug_package %{nil}

Name:		msrc_base
Version:	2.29
Release:	1%{osdist}
BuildRoot:	/tmp/%{name}-%{version}-%{release}
Vendor:		NPCGuild.org
Summary:	NPC Guild technology base for all products
Group:		Utilities
License:	BSD
URL:		http://msrc.npcguild.org/
Source:		http://patch.sac.fedex.com/tarball/%{name}-%{version}.tgz
BuildRequires:	gcc m4 /bin/bash /bin/ksh flex bison make /usr/bin/install
Autoreq:	0
Requires:	gcc bzip2 /bin/ksh m4 rdist /usr/bin/ssh make
Provides:	mkcmd mkcmd_lib explode explode_lib ptbw xclate xapply hxmd msrc mmsrc

%description
This is the core technology which provided wrappers, master source,
parallel process management, collated output diversion, the basis
of host-based configuration management and makes all your dreams come
true.  It is a must-have for all sites with more than ten computers.
Learn to use it and you won't ever be sorry.  I promise.

%prep
%setup

%setup -q
%build
mkdir -p $RPM_BUILD_ROOT/usr/local/bin $RPM_BUILD_ROOT/usr/local/sbin
mkdir -p $RPM_BUILD_ROOT/usr/local/lib
mkdir -p $RPM_BUILD_ROOT/usr/local/man/man1 $RPM_BUILD_ROOT/usr/local/man/man5
mkdir -p $RPM_BUILD_ROOT/usr/local/man/man8
mkdir -p $RPM_BUILD_ROOT/usr/local/lib/mkcmd
mkdir -p $RPM_BUILD_ROOT/usr/local/lib/explode
mkdir -p $RPM_BUILD_ROOT/usr/local/lib/hxmd
HOSTTYPE=`uname -s | tr [:lower:] [:upper:]`
if [ -r /etc/redhat-release ] ; then
	HOSTOS=`sed -e 's/[^0-9]//g' -e 's/\([0-9]\)\([0-9]\)/\10\200/' /etc/redhat-release`
else
	HOSTOS="00"
fi
PATH=$RPM_BUILD_ROOT/usr/local/sbin:$RPM_BUILD_ROOT/usr/local/bin:$PATH
MKCMD="-I $RPM_BUILD_ROOT/usr/local/lib/mkcmd/type:$RPM_BUILD_ROOT/usr/local/lib/mkcmd"
EXPLODE="-I $RPM_BUILD_ROOT/usr/local/lib/explode"
export PATH MKCMD EXPLODE HOSTOS HOSTTYPE

# Ignore the other payloads, just get the code for the RPM build a subspace
# for the platform mmsrc make to work under, parallel to our source dirs:
cd local
F=`mktemp -d /tmp/bmHMXXXXXX`
mkdir -p $F/bin $F/sbin $F/lib
mkdir -p $F/lib/mkcmd $F/lib/explode

# The hard one, get mmsrc going, and copy it into our path
(
cd sbin/mmsrc
make Makefile.in
./configure
make mmsrc
/usr/bin/install -c -m 755 mmsrc $RPM_BUILD_ROOT/usr/local/sbin/mmsrc
make clean
rm -f Makefile.in
)

# Now mmsrc is in our path -- much better.  Use it to boot the others:
# Set HOSTOS in with "--define myhostos=VRRPP"

for t in lib/mkcmd lib/explode lib/hxmd bin/mkcmd bin/explode \
	bin/ptbw bin/xclate bin/xapply sbin/msrc sbin/hxmd sbin/msrc
do
	(cd $t && mmsrc -y INTO=$F/$t -DHOST=localhost -DHOSTTYPE=$HOSTTYPE -DHOSTOS=$HOSTOS -- make DESTDIR=$RPM_BUILD_ROOT boot )
done

# Now install the manaul pages
# If only we had mk... Sigh.
for t in bin/mkcmd/mkcmd.man bin/explode/explode.man bin/ptbw/ptbw.man \
	bin/xclate/xclate.man bin/xapply/xapply.man
do
	M=`echo $t | sed -e s/\.man/\.1/`
	install -c -m 644 $t $RPM_BUILD_ROOT/usr/local/man/man1/`basename $M`
done
for t in lib/explode/dicer.man lib/explode/explode.man lib/mkcmd/mkcmd.man \
	lib/hxmd/hxmd.man
do
	M=`echo $t | sed -e s/\.man/\.5/`
	install -c -m 644 $t $RPM_BUILD_ROOT/usr/local/man/man5/`basename $M`
done
for t in sbin/hxmd/hxmd.man sbin/msrc/msrc.man sbin/mmsrc/mmsrc.man
do
	M=`echo $t | sed -e s/\.man/\.8/`
	install -c -m 644 $t $RPM_BUILD_ROOT/usr/local/man/man8/`basename $M`
done

find $RPM_BUILD_ROOT -depth -type d -name OLD -exec rm -rf {} \;
rm -rf $F

%clean
# Do not confilct with the next user of the fixed-path build directory
rm -rf %{buildroot}

%pre
grep "^source:" /etc/group >/dev/null || groupadd %{?sourcegid:-g %{sourcegid}} source
grep "^source:" /etc/passwd >/dev/null || useradd %{?sourceuid:-u %{sourceuid}} -g source -d /usr/src source

%files
%defattr(-,root,root)
%{_local_sbindir}/mmsrc
%{_local_sbindir}/msrc
%{_local_sbindir}/hxmd
%{_local_bindir}/xapply
%{_local_bindir}/xclate
%{_local_bindir}/ptbw
%{_local_bindir}/explode
%{_local_bindir}/mkcmd
%{_local_libdir}/mkcmd
%{_local_libdir}/explode
%config %{_local_libdir}/hxmd
%doc %{_local_mandir}/man1/*
%doc %{_local_mandir}/man5/*
%doc %{_local_mandir}/man8/*

%changelog
* Sat Aug 14 2010 KS Braunsdorf 2.29
- mmsrc's default -m was wrong (__mmsrc != __msrc)
- options.html was missing -m prereq[:postreq]
* Fri Aug 13 2010 KS Braunsdorf 2.28
- mkcmd manual page fix to bind option to its argument
- more examples in ptbw manual page
- hxmd had missing files in recipes, updated README+TODO
-  added cache directory support, which is way cool (via dir/Cache.m4)
-  failure to init the hostdb now gets sent back to the main for cleanup
-  support the HXMD_PHASE macro for -j files to know which m4 included them
-  the -N action was mistakenly run when hostdb failed to init, fixed
-  lots of HTML and manpage updates
-  some reports of signal issues with the wait loops, not sure I fixed that
-  slot.m could add make's command echo output to caches (now spec -s always)
-  added the posse script (and example in the HTML docs)
- msrc added "dmz.man"
-  dmz.sh got the -S option to push be service (as an example in HTML docs)
-  added a second specification to -m to force a cleanup update after a push
-  allow the token "+++" to specify that a macro list accepts more files/dirs
-  allow the token "."  to specify that a macro list is as full as is can be
-  dirs can be added to MAP if they end in .host (as a cache dir for hxmd)
-  added regression tests for new features
-  upgrades to the HTML documents, but references to flock/oue dangle
- mmsrc got all the fixes from hxmd and msrc
-  the phases for mmsrc are close to hxmd, but not exactly the same viz. "cmd"
* Sun Dec 06 2009 KS Braunsdorf 2.17
- mkcmd got a new hook only I care about (%m to defer meta code), NULL checks
- explode more version information in some modules
- hxmd does cache directories, so you don't have to use m4 for everything
-  added -j every.m4 to allow a markup package to include globally
-  (et al) a command-line explict "./file" now defeates the -I search code
-  hostdb.m, the macro "%" explans in the header markup to -k's spec
-  hostdb.m fixed some trailing value cases under -Z
- mmsrc spelling error in the include of stdlib.h
-  msrc/mmsrc/efmd fixes to add the above to these
- ptbw/xclate a kill -STOP bug-fix
- xapply changed expander bindings for -N expansion to something useful
-  added Dicer version to -V, fixed ptbw slot count
-  some lint in printf of host specific types "%ld" matched to (long) casts
* Sat May 09 2009 KS Braunsdorf 2.15
- upgraded hostdb.m to allow for -o "MACRO:OTHER"
- update html/man pages
- bug fix in mmsrc for -V/-h leaving a temp directory
- moved hxmd.1 to hxmd.8 where I thought it was
- fixed a quoting botch in "gnumake.m4"
- set the default HOSTOS to 00
* Tue Nov 18 2008 KS Braunsdorf
- allow HOSTOS to be a command-line define -- we should be able
  to compute it from /etc/redhat-release at least.
* Thu Nov 13 2008 KS Braunsdorf, Ed Anderson, and Thomas Swan
- Redid the RPM spec file, changed the name, tactics, and style a lot
* Fri Oct 17 2008 KS Braunsdorf <kevin.braunsdorf@fake.gmail.com>
- allow autoconfig to try if the default msrc build fails for mmsrc
  fix this change log
* Tue Jun 03 2008 Thomas Swan <thomas.swan@gmail.com> - 1
- Initial package
