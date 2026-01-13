# $Id: ITO.spec,v 3.10 2010/08/16 14:45:20 ksb Exp $
# $KeyFile: ${echo:-echo} Makefile.meta
# $Level3: ${echo:-echo} entomb_base

%define _local_prefix		/usr/local
%define _local_bindir		%{_local_prefix}/bin
%define _local_sbindir		%{_local_prefix}/sbin
%define _local_libdir		%{_local_prefix}/lib
%define _local_mandir		%{_local_prefix}/man
# If you want entombing to work over NFS charon must have a common uid/gid.
# I picked 602 because it is not special, or commonly used. --ksb
%define charonuid		602
%define charongid		602

# for signing of packages
#%_signature gpg
#%_gpg_name FIXME@example.org

# Do not generate debugging packages by default - older versions of rpmbuild
# (like RehHat 6.2) choke on the line with the percent in the macro name:
# We need a sed spell on RH6.x to "s/\(^.define \)%d/\1d/", Sigh. -- ksb
%define debug_package %{nil}

Name:		entomb_base
Version:	3.11
Release:	1%{osdist}
BuildRoot:	/tmp/%{name}-%{version}-%{release}
Vendor:		NPCGuild.org
Summary:	NPC Guild technology base for file entombing (unrm)
Group:		Utilities
License:	BSD
URL:		http://msrc.npcguild.org/
Source:		http://patch.sac.fedex.com/tarball/%{name}-%{version}.tgz
BuildRequires:	gcc m4 /bin/ksh make msrc_base /usr/local/sbin/msrc /usr/local/bin/mk
Autoreq:	0
Requires:	gcc bzip2 /bin/ksh m4 rdist /usr/bin/ssh make
Provides:	entomb unrm preend libtomb.a untmp rlfile

%description
This is the core technology that provides file entombing.  Mortal
users can undo a rm, mv, and cp operations that ate their files --
if you install this package and rebuild those programs with the
"libtomb.a" library.  A new daemon "preend" clean the file tombs
as the files age (older files are removed to make room for newer ones).

%prep
%setup

%setup -q
%build
mkdir -p $RPM_BUILD_ROOT/usr/local/bin
mkdir -p $RPM_BUILD_ROOT/usr/local/lib
mkdir -p $RPM_BUILD_ROOT/usr/local/man/man1 $RPM_BUILD_ROOT/usr/local/man/man5
mkdir -p $RPM_BUILD_ROOT/usr/local/man/man8

# Ignore the other payloads, just get the code for the RPM build a subspace
# for the platform mmsrc make to work under, parallel to our source dirs:
cd local
F=`mktemp -d /tmp/bmHMXXXXXX`
mkdir -p $F/bin $F/sbin $F/lib

for t in lib/libtomb lib/entomb bin/unrm sbin/preend
do
	(cd $t && mmsrc -y INTO=$F/$t -Cauto.cf -- make DESTDIR=$RPM_BUILD_ROOT boot )
done

# Now install the manaul pages
find . -name \*.man -print | xapply -f 'mk -smInstall -D DESTDIR=$RPM_BUILD_ROOT %1' -
# cleanup any install noise and the platform source cache
find $RPM_BUILD_ROOT -depth -type d -name OLD -exec rm -rf {} \;
rm -rf $F

%clean
# Do not confilct with the next user of the fixed-path build directory
rm -rf %{buildroot}

%pre
grep "^charon:" /etc/group >/dev/null || groupadd %{?charongid:-g %{charongid}} charon
grep "^charon:" /etc/passwd >/dev/null || useradd %{?charonuid:-u %{charonuid}} -g charon -d /var/Tomb charon

%files
%defattr(-,root,root)
%{_local_libdir}/libtomb.a
%{_local_libdir}/entomb
%{_local_bindir}/unrm
%{_local_sbindir}/preend
%doc %{_local_mandir}/man1/*
%doc %{_local_mandir}/man3/*
%doc %{_local_mandir}/man8/*

%changelog
* Sat Aug 14 2010 KS Braunsdorf 3.9
- formed a 2009 style package for this se
