# $Id: ITO.spec,v 2.54 2012/09/10 14:00:24 ksb Exp $
# $KeyFile: ${echo:-echo} oue.m

%define _local_prefix		/usr/local
%define _local_bindir		%{_local_prefix}/bin
%define _local_mandir		%{_local_prefix}/man/man1

# for signing of packages
#%_signature gpg
#%_gpg_name FIXME@example.org

# %dist should be defined on the target but in case it isn't we can guess one
%define distguess %(cat /etc/{redhat,fedora,suse,ubuntu}-release | sed -e 's/Fedora Core/fc@/; s/Fedora/f@/; s/Red Hat[^0-9]*\(Enterprise|Server\)*[^0-9]*/rhel@/; s/Red Hat[^0-9]*/rh/; s/.*Suse[^0-9]*/se@/; s/Ubuntu[^0-9]*/ubuntu@/; s/@[^0-9]*//;s/\([0-9][0-9]*\)[^0-9][^0-9]*\([0-9][0-9]*\)/\1_\2/; s/[^0-9]*$//')
%{!?dist:%define dist .%{distguess}}

# Do not generate debugging packages by default - older versions of rpmbuild
# (like RehHat 6.2) choke on the line with the percent in the macro name:
# We need a sed spell on RH6.x to "s/\(^.define \)%d/\1d/", Sigh. -- ksb
%define debug_package %{nil}

Name:		oue
Version:	2.31
Release:	2%{?dist}
BuildRoot:	/tmp/%{name}-%{version}-%{release}
Vendor:		NPCGuild.org
Summary:	NPC tool that filters a stream for unique elements
Group:		Utilities
License:	BSD
URL:		http://patch.sac.fedex.com/RPMS
Source:		http://patch.sac.fedex.com/tarball/%{name}-%{version}.tgz
BuildRequires:	gcc m4 /bin/bash /bin/ksh msrc_base gdbm-devel mk mktemp
# Autoreq:0 prevents misleading depencies from being generated. Normally, this
# is perfectly sane; however, the system is picking up mk directives and
# mkcmd directives as well as system depencies.
Autoreq:	0
Requires:	gcc bzip2 /bin/ksh msrc_base gdbm-devel gdbm

%description
This filter is used by variaous ITO filters: accounting, backups, master
source and move to production may all reference it.

%prep
%setup
%build

%install
mkdir -p %{buildroot}/usr/local/man/man1
mkdir -p %{buildroot}/usr/local/bin
F=`mktemp -d /tmp/bmHMXXXXXX`
PATH=$PATH:/usr/local/bin:/usr/local/sbin
mmsrc -y INTO=$F -Cauto.cf -- make DESTDIR=%{buildroot} install
mk -mInstall -D DESTDIR=%{buildroot} %{name}.man
rm -rf $F
find %{buildroot} -depth -type d -name OLD -exec rm -rf {} \;

%clean
rm -rf %{buildroot}
%files
%defattr(-,root,root)
%{_local_bindir}/
%doc %{_local_mandir}/

%changelog
* Mon Sep 10 2012 KS Braunsdorf
- manual page update
- allow RUN_LIB override for msrcmux pull spells
* Thu May  3 2012 KS Braunsdorf
- Added %U, %L to fold case, so mixed case keys may be better matched
- Put in an error for mismatched bracket/curly pairs, like %[1:2}
* Tue Oct 11 2011 KS Braunsdorf
- Added -S which I almost never need, but once was enough.
* Tue Dec 07 2010 Thomas Swan
- fixed missing directories for install
* Wed Mar 24 2010 KS Braunsdorf
- just some bugs in the -H output, nothing functional at all
* Mon Oct 05 2009 KS Braunsdorf
- added -c, -d, -l, -s and -x/-B options, updated the regression test.
- updated the manual page and the html document
- some small output bugs fixed from the original 2008 release
* Sat Nov 08 2008 KS Braunsdorf
- Built the RPM spec file and added it to the msrc cache
