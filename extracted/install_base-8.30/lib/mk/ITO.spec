# $Id: ITO.spec,v 5.8 2009/10/06 23:50:14 ksb Exp $
# $KeyFile: ${echo:-echo} Makefile
# $Level2: ${echo:-echo} mk_lib

%define _local_prefix		/usr/local
%define _local_libdir		%{_local_prefix}/lib
%define _local_mandir		%{_local_prefix}/man/man5
%define _local_mklib		%{_local_libdir}/mk

# for signing of packages
#%_signature gpg
#%_gpg_name FIXME@example.org

# Do not generate debugging packages by default - older versions of rpmbuild
# (like RehHat 6.2) choke on the line with the percent in the macro name:
# We need a sed spell on RH6.x to "s/\(^.define \)%d/\1d/", Sigh. -- ksb
%define debug_package %{nil}

Name:		mk_lib
Version:	5.6
Release:	3
BuildRoot:      /tmp/%{name}-%{version}-%{release}
Vendor:		NPCGuild.org
Summary:	NPC tool for mk supporting libraries
Group:		Utilities
License:	BSD
URL:		http://patch.sac.fedex.com/RPMS
Source:		http://patch.sac.fedex.com/tarball/%{name}-%{version}.tgz
BuildRequires:	/bin/ksh msrc mk mktemp
# Autoreq:0 prevents misleading depencies from being generated. Normally, this
# is perfectly sane; however, the system is picking up mk directives and
# mkcmd directives as well as system depencies.
Autoreq:	0
Requires:	msrc mk

%description
These files help mk to be more useful.

%prep
%setup
%build
F=`mktemp -d /tmp/bmHMXXXXXX`
mmsrc -y INTO=$F -Cauto.cf -- make DESTDIR=$RPM_BUILD_ROOT install
mk -mInstall -D DESTDIR=$RPM_BUILD_ROOT mk.man
rm -rf $F
find $RPM_BUILD_ROOT -type d -name OLD -depth -exec rm -rf {} \;

%clean
rm -rf %{buildroot}
%files
%defattr(-,root,root)
%{_local_mklib}/
%doc %{_local_mandir}/

%changelog
* Mon Nov 12 2008 ksb and anderson
- Built the RPM spec file and added it to the msrc cache
