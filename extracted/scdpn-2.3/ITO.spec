# $Id: ITO.spec,v 2.7 2012/02/28 23:32:37 ksb Exp $
# $KeyFile: ${echo:-echo} scdpn.m

%define _tmpdir			/tmp
%define _local_prefix		/usr/local
%define _local_bindir		%{_local_prefix}/sbin
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

Name:		scdpn
Version:	2.3
Release:	1%{?dist}
BuildRoot:	%{_tmpdir}/%{name}-%{version}-%{release}
Vendor:		NPCGuild.org
Summary:	NPC tool that displays Cisco discovery information in PEG format
Group:		Utilities
License:	BSD
URL:		http://patch.sac.fedex.com/RPMS
Source:		http://patch.sac.fedex.com/tarball/%{name}-%{version}.tgz
BuildRequires:	gcc m4 msrc_base install_base
BuildRequires:	libpcap >= 0.7.0
# Autoreq:0 prevents misleading depencies from being generated. Normally, this
# is perfectly sane; however, the system is picking up mk directives and
# mkcmd directives as well as system depencies.
Autoreq:	1
Requires:	libpcap >= 0.7.0

%description
This create an entry to tell PEG which hosts are connected to which switches.
That enable backups to load balance network switches, network time protocol
to pick "near" hosts for a local time-base, and other nifty tricks.

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
* Tue Feb 28 2012 KS Braunsdorf
- tried to fix some issues with Linux binding interfaces
* Wed Nov 19 2008 KS Braunsdorf
- Built the RPM spec file and added it to the msrc cache
