# $Id: ITO.spec,v 1.4 2009/01/28 14:22:24 ksb Exp $
# $KeyFile: ${echo:-echo} untmp.m

%define _tmpdir			/tmp
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

Name:		untmp
Version:	1.9
Release:	1
BuildRoot:	%{_tmpdir}/%{name}-%{version}-%{release}
Vendor:		NPCGuild.org
Summary:	NPC tool for removing files in /tmp
Group:		Utilities
License:	BSD
URL:		http://patch.sac.fedex.com/RPMS
Source:		http://patch.sac.fedex.com/tarball/%{name}-%{version}.tgz
BuildRequires:	gcc m4 msrc_base install_base
# Autoreq:0 prevents misleading depencies from being generated. Normally, this
# is perfectly sane; however, the system is picking up mk directives and
# mkcmd directives as well as system depencies.
Autoreq:	0
Requires:	/bin/ksh install_base

%description
untmp removes all the files from /tmp owned by the invoker.  It is useful for
those developing programs which  use  temporary  files  or  for stopping a C
compilation cold.

If  a  user  name  is  given, then files in /tmp owned by that user are removed
rather than the files owned by the invoker.

%prep
%setup
%build
umask 022
mkdir -p $RPM_BUILD_ROOT/%{_local_bindir}
mkdir -p $RPM_BUILD_ROOT/%{_local_mandir}
F=`mktemp -d /tmp/bmHMXXXXXX`
mmsrc -y INTO=$F -Cauto.cf -- make DESTDIR=$RPM_BUILD_ROOT install
mk -mInstall -D DESTDIR=$RPM_BUILD_ROOT *.man
rm -rf $F
find $RPM_BUILD_ROOT -depth -type d -name OLD -exec rm -rf \{\} \;

%clean
# Do not confilct with the next user of the fixed-path build directory
rm -rf %{buildroot}

%files
%defattr(-,root,root)
%{_local_bindir}/%{name}
%doc %{_local_mandir}/

%changelog
* Wed Nov 19 2008 Ed Anderson
- Built the RPM spec file and added it to the msrc cache
