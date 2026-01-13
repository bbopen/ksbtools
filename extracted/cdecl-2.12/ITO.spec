# $Id: ITO.spec,v 1.7 2012/03/01 16:19:46 ksb Exp $
# $KeyFile: ${echo:-echo} cdecl.man

%define _tmpdir			/tmp
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

Name:		cdecl
Version:	2.12
Release:	1%{?dist}
BuildRoot:	%{_tmpdir}/%{name}-%{version}-%{release}
Vendor:		NPCGuild.org
Summary:	NPC tool for compose C declarations
Group:		Utilities
License:	BSD
URL:		http://patch.sac.fedex.com/RPMS
Source:		http://patch.sac.fedex.com/tarball/%{name}-%{version}.tgz
BuildRequires:	gcc m4 msrc_base install_base
# Autoreq:0 prevents misleading depencies from being generated. Normally, this
# is perfectly sane; however, the system is picking up mk directives and
# mkcmd directives as well as system depencies.
Autoreq:	1
Requires:	/bin/ksh install_base

%description
cdecl is a program for encoding and decoding C type-declarations.  It reads
standard input for statements in the language described below.  The results are
written on standard output.

cdecl's scope is intentionally small.  It doesn't help you figure out storage
classes or initializations.

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
* Thu Mar  1 2012 KS Braunsdorf
- updated the manual page for style and format
* Wed Feb 29 2012 KS Braunsdorf
- fixes to the manual page
* Sat Dec 26 2009 KS Braunsdorf
+ updated manual page with better mk markup, no code changes
* Wed Nov 19 2008 Ed Anderson
- Built the RPM spec file and added it to the msrc cache
