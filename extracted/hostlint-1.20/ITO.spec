# $Id: ITO.spec,v 1.6 2012/01/31 20:20:53 ksb Exp $
# $KeyFile: ${echo:-echo} hostlint.ksh.host

%define _tmpdir			/tmp
%define _local_prefix		/usr/local
%define _local_bindir		%{_local_prefix}/libexec
%define _local_mandir		%{_local_prefix}/man/man8

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

Name:		hostlint
Version:	1.20
Release:	1%{?dist}
BuildRoot:	%{_tmpdir}/%{name}-%{version}-%{release}
Vendor:		NPCGuild.org
Summary:	NPC tool for auditing the configuration of a host
Group:		Applications/System
License:	BSD
URL:		http://patch.sac.fedex.com/RPMS
Source:		http://patch.sac.fedex.com/tarball/%{name}-%{version}.tgz
BuildRequires:	gcc m4 msrc_base install_base 
# Autoreq:0 prevents misleading depencies from being generated. Normally, this
# is perfectly sane; however, the system is picking up mk directives and
# mkcmd directives as well as system depencies.
Autoreq:	1
Requires:	/bin/ksh install_base rsync

%description
This is a wrapper that rsync's a program from a local rsync repository and runs
it.  That program claims to screen the local host  for  common configuration 
errors.  In fact that program could do anything.  Do not run this program
without a clear understanding of this trust  relationship.

The downloaded program is executed with the privileges of the login that ran
hostlint.

The output is usually sent to a reporting agent to notify the  Administrator of
errors or inconsistant information found on the host.


%prep
%setup -q
%build
%install
umask 022
mkdir -p %{buildroot}/%{_local_bindir}
mkdir -p %{buildroot}/%{_local_mandir}
F=`mktemp -d /tmp/bmHMXXXXXX`
mmsrc -y INTO=$F -Cauto.cf -- make DESTDIR=%{buildroot} install
mk -mInstall -D DESTDIR=%{buildroot} *.man
rm -rf $F
find %{buildroot} -depth -type d -name OLD -exec rm -rf \{\} \;

%clean
# Do not confilct with the next user of the fixed-path build directory
rm -rf %{buildroot}

%files
%defattr(-,root,root)
%{_local_bindir}/%{name}
%doc %{_local_mandir}/

%changelog
* Tue Jan 31 2012
* -p's usage messages were wrong, fixed.  And a missing backslash in the manual
* Mon Dec 12 2011 KS Braunsdorf
- added -p for a forwarded port to rsync, prob. should have made it generic
* Thu Nov 20 2008 Ed Anderson
- Built the RPM spec file and added it to the msrc cache
