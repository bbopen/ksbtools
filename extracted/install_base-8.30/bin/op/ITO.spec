# $Id: ITO.spec,v 2.20 2010/07/28 22:47:23 ksb Exp $
# $KeyFile: ${echo:-echo} op.m
# $Level2: ${echo:-echo} ${LEVEL2_NAME:-.}
# $Level3: ${echo:-echo} install_base

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

Name:		op
Version:	2.81
Release:	1
BuildRoot:	%{_tmpdir}/%{name}-%{version}-%{release}
Vendor:		NPCGuild.org
Summary:	NPC tool for Restricted Superuser Access
Group:		Utilities
License:	BSD
URL:		http://patch.sac.fedex.com/RPMS
Source:		http://patch.sac.fedex.com/tarball/%{name}-%{version}.tgz
BuildRequires:	gcc m4 /bin/bash /bin/ksh msrc /usr/bin/lex /usr/include/security/pam_appl.h
# Autoreq:0 prevents misleading depends from being generated. Normally, this
# is perfectly sane; however, the system is picking up mk directives and
# mkcmd directives as well as system depends.
Autoreq:	0
Requires:	gcc /bin/ksh msrc

%description
The op tool provides a flexible means for system administrators to grant
trusted users access to certain root operations without having to give
them full superuser privileges.  Different sets of users may access
different operations, and the security-related aspects of environment
of each operation can be carefully controlled.  This version supports a
better interface to PAM authentication and sessions.

%prep
%setup
%build
mkdir -p $RPM_BUILD_ROOT/%{_local_bindir} $RPM_BUILD_ROOT/%{_local_mandir}
F=`mktemp -d /tmp/bmHMXXXXXX`
mmsrc -y INTO=$F -Cauto.cf -- make DESTDIR=$RPM_BUILD_ROOT install
mk -mInstall -D DESTDIR=$RPM_BUILD_ROOT *.man

%clean
# Do not conflict with the next user of the fixed-path build directory
rm -rf %{buildroot}

%files
%defattr(-,root,root)
%{_local_bindir}/%{name}
%doc %{_local_mandir}/

%changelog
* Fri Jul 16 2010 KS Braunsdorf
- No feature changes at all.
- changed some error messages to be more specific, for example "full path"
-  really meant "absolute path", so that is what op says now.
- More specific error messages for cleanup and session w/o a login spec.
- Some minor code cleanup (const decls, better variable names).
* Thu Mar 25 2010 KS Braunsdorf
- the Sanity check code reports argument conflicts better, but not prefectly
- fixed -l output for commands that matched $1 (and/or $2) but passed $*
- number code recognized IP and CIDR REs
- op can use sudo to wrap itself, so op may installed without setuid bits
* Mon Jul 27 2009 KS Braunsdorf
- added pam, session, and cleanup PAM configuration options
- added fib configuration for BSD systems [doesn't apply to Linux platforms]
- added %g@u to select a group by login inclusion, parallel to %u@g
- added $e, $E, $~, $b, $b, $p, $P expanders (see man page)
* Wed Jun  3 2009 KS Braunsdorf
- any DEFAULT for an in-line script didn't copy the script text and broke with
- a shell message "Bad -c option".  Not usually a security issue.
* Tue Mar 10 2009 KS Braunsdorf
- major upgrade in op form and function, -f, -u, -g checking
- many percent expanders added
- multi-word mnemonics (e.g. both "apache stop" and "apache start")
* Sat Nov 08 2008 KS Braunsdorf and Ed Anderson
- Built the RPM spec file and added it to the msrc cache
