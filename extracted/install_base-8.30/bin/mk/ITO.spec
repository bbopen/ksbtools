# $Id: ITO.spec,v 5.31 2009/11/18 16:11:41 ksb Exp $
# $KeyFile: ${echo:-echo} mk.c
# $Level3: ${echo:-echo} install_base

%define _local_prefix		/usr/local
%define _local_bindir		%{_local_prefix}/bin
%define _local_mandir		%{_local_prefix}/man/man1

# for signing of packages
#%_signature gpg
#%_gpg_name FIXME@example.org

# Do not generate debugging packages by default - older versions of rpmbuild
# (like RehHat 6.2) choke on the line with the percent in the macro name:
# We need a sed spell on RH6.x to "s/\(^.define \)%d/\1d/", Sigh. -- ksb
%define debug_package %{nil}

Name:		mk
Version:	5.23
Release:	1
BuildRoot:	/tmp/%{name}-%{release}-%{version}
Vendor:		NPCGuild.org
Summary:	NPC tool for locating marked shell commands in files
Group:		Utilities
License:	BSD
URL:		http://patch.sac.fedex.com/RPMS
Source:		http://patch.sac.fedex.com/tarball/%{name}-%{version}.tgz
BuildRequires:	gcc m4 /bin/bash /bin/ksh msrc mktemp
# Autoreq:0 prevents misleading depencies from being generated. Normally, this
# is perfectly sane; however, the system is picking up mk directives and
# mkcmd directives as well as system depencies.
Autoreq:	0
Requires:	gcc bzip2 msrc

%description
Many files provided in the ITO tools have embeded shell markup.  Without
this tool they are just comments.  With this tool they become useful
attributes of the file.  More importantly it is how we install all
manual pages.

%prep
%setup
%build
mkdir -p %{_local_bindir} %{_local_mandir}
F=`mktemp -d /tmp/bmHMXXXXXX`
mmsrc -y INTO=$F -Cauto.cf -- make DESTDIR=$RPM_BUILD_ROOT install
$RPM_BUILD_ROOT/%{_local_bindir}/mk -mInstall -D DESTDIR=$RPM_BUILD_ROOT *.man
rm -rf $F
%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root)
%{_local_bindir}/mk
%doc %{_local_mandir}/mk.1

%changelog
* Wed Nov 12 2008 ksb and Ed
- made builddir work better
* Sat Nov 08 2008 KS Braunsdorf
- fixed the build recipe a lot with Ed's help
- changed the tactic to install as we build, which we don't like at all
* Sat Nov 08 2008 KS Braunsdorf
- Built the RPM spec file and added it to the msrc cache
