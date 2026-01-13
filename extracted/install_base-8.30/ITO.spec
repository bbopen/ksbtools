# $Id: ITO.spec,v 8.36 2010/08/13 19:59:30 ksb Exp $
# $Level3: ${echo:-echo} install_base
# $KeyFile: ${echo:-echo} Makefile.meta

%define _local_prefix		/usr/local
%define _local_bindir		%{_local_prefix}/bin
%define _local_libdir		%{_local_prefix}/lib
%define _local_sbindir		%{_local_prefix}/sbin
%define _local_mandir		%{_local_prefix}/man

# for signing of packages
#%_signature gpg
#%_gpg_name FIXME@example.org

# Do not generate debugging packages by default - older versions of rpmbuild
# (like RehHat 6.2) choke on the line with the percent in the macro name:
# We need a sed spell on RH6.x to "s/\(^.define \)%d/\1d/", Sigh. -- ksb
%define debug_package %{nil}

Name:		install_base
Version:	8.30
Release:	1%{osdist}
BuildRoot:	/tmp/%{name}-%{version}-%{release}
Vendor:		NPCGuild.org
Summary:	NPCGuild local version of mk, op, and installation tools
Group:		Utilities
License:	BSD
URL:		http://msrc.npcguild.org/
Source:		http://patch.sac.fedex.com/tarball/%{name}-%{version}.tgz
BuildRequires:	gcc m4 /bin/bash /bin/ksh msrc_base /usr/include/security/pam_appl.h
Autoreq:	0
Requires:	gcc /bin/ksh msrc_base
Provides:	mk mk_lib op op_lib purge installus instck vinst

%description
These tools build on top of install (and msrc in general) to provide a
clean backout of changes, cleanup after those backout files are no longer
needed, allow escalated installation for mortal logins.  These programs
are often used by admins to update their systems.   In fact almost of the
NPCGuild packages depend on this package.

%prep
%setup
%build
mkdir -p $RPM_BUILD_ROOT/usr/local/sbin $RPM_BUILD_ROOT/usr/local/bin
mkdir -p $RPM_BUILD_ROOT/usr/local/man $RPM_BUILD_ROOT/usr/local/lib
mkdir -p $RPM_BUILD_ROOT/usr/local/man/man8 $RPM_BUILD_ROOT/usr/local/man/man5
mkdir -p $RPM_BUILD_ROOT/usr/local/man/man1
PATH=$RPM_BUILD_ROOT/usr/local/bin:$PATH
INSTALL="-C$RPM_BUILD_ROOT/usr/local/lib/install.cf"
MK="-T$RPM_BUILD_ROOT/usr/local/lib/mk"
touch $RPM_BUILD_ROOT/usr/local/lib/install.cf
export PATH INSTALL MK
F=`mktemp -d /tmp/bmHMXXXXXX`
mkdir -p $F/bin $F/sbin $F/lib
mkdir -p $F/lib/mk
(
cd bin/install.d && mmsrc -Cauto.cf -y INTO=$F/bin/install.d -- \
	make RUN_LIB=/usr/local/lib DESTDIR=$RPM_BUILD_ROOT install
)
(
cd lib/install.cf && mmsrc -Cauto.cf -y INTO=$F/lib/install.cf -- \
	make DESTDIR=$RPM_BUILD_ROOT boot
)
(
cd sbin/purge && mmsrc -Cauto.cf -y INTO=$F/sbin/purge -- \
	make DESTDIR=$RPM_BUILD_ROOT install
)
(
cd sbin/instck && mmsrc -Cauto.cf -y INTO=$F/sbin/instck -- \
	make DESTDIR=$RPM_BUILD_ROOT OVERRIDE=override.m install
)
(
cd sbin/installus && mmsrc -Cauto.cf -y INTO=$F/sbin/installus -- \
	make DESTDIR=$RPM_BUILD_ROOT OVERRIDE=override.m install
)
(
cd bin/vinst && mmsrc -Cauto.cf -y INTO=$F/bin/vinst -- \
	make DESTDIR=$RPM_BUILD_ROOT OVERRIDE=override.m install
)
(
cd bin/mk && mmsrc -Cauto.cf -y INTO=$F/bin/mk -- \
	make DESTDIR=$RPM_BUILD_ROOT install
)
mkdir -p $RPM_BUILD_ROOT/usr/local/lib/op
(
cd bin/op && mmsrc -Cauto.cf -y INTO=$F/bin/op -- \
	make DESTDIR=$RPM_BUILD_ROOT install
)
install -c -m 400 access.cf $RPM_BUILD_ROOT/usr/local/lib/op/access.cf
(
cd lib/mk && mmsrc -Cauto.cf -y INTO=$F/lib/mk -- \
	make DESTDIR=$RPM_BUILD_ROOT install
)

mk -mInstall -D DESTDIR=$RPM_BUILD_ROOT $F/*/*/*.man
find $RPM_BUILD_ROOT -depth -type d -name OLD -exec rm -rf {} \;
rm -rf $F

%clean
%files
%defattr(-,root,root)
%{_local_bindir}/install
%{_local_bindir}/op
%{_local_bindir}/mk
%{_local_bindir}/vinst
%{_local_sbindir}/instck
%{_local_sbindir}/installus
%{_local_sbindir}/purge
%{_local_libdir}/install.cf
%{_local_libdir}/mk
%config %{_local_libdir}/op/access.cf
%doc %{_local_mandir}/man1/*
%doc %{_local_mandir}/man5/*
%doc %{_local_mandir}/man8/*

%changelog
* Fri Aug 13 2010 KS Braunsdorfg
- some op fixes for MAGIC_SHELL, and setfsuid, setfsgid for Linux+NFS
- added some spelling fixes in the documentation (both manual and html files)
* Tue May  4 2010 KS Braunsdorfg
- the instuctions had the wrong option for the INTO yoke (s/-Y/-y/)
- the manual instructions were missing elements in PATH for Solaris 10
- the release tracking file is now Makefile.meta
- some compile errors repaired for DARWIN (MacOS, aka FREEBSD 4.11)
* Fri Dec 18 2009 KS Braunsdorf
- moved programs from etc to sbin as per plan
- FreeBSD: minimal support for chflags in instck and install
* Fri Jun  5 2009 E Anderson
- op default rules are more general purpose
* Thu Jun  4 2009 KS Braunsdorf
- op default help is better now
* Mon Jan 27 2009 E Anderson C Campbell
- installus is now less picky about passwords
* Mon Jan 12 2009 E Anderson C Campbell
- major updates to op, mostly
* Sat Nov 15 2008 KS Braunsdorf
- finished everything but the op library
* Sun Nov 09 2008 KS Braunsdorf
- Built the RPM spec file and added it to the msrc cache
