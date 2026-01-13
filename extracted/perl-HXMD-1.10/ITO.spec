# $Id: ITO.spec,v 1.15 2010/12/09 19:36:37 ksb Exp $
# $KeyFile: ${echo:-echo} lib/HXMD.pm
# $Level2: ${echo:-echo} perl-HXMD

%define _tmpdir			/tmp
%define _local_prefix		/usr/local
%define _local_bindir		%{_local_prefix}/bin
%define _local_libdir		%{_local_prefix}/lib/sac
%define _local_mandir		%{_local_prefix}/man/man3

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

Name:		perl-HXMD
Version:	1.10
Release:	2%{?dist}
BuildRoot:	%{_tmpdir}/%{name}-%{version}-%{release}
Vendor:		NPCGuild.org
Summary:	NPC tool for using HXMD configuration files from perl
Group:		Utilities
License:	BSD
URL:		http://patch.sac.fedex.com/RPMS
Source:		http://patch.sac.fedex.com/tarball/%{name}-%{version}.tgz
BuildRequires:	gcc m4 msrc_base install_base
# Autoreq:0 prevents misleading depencies from being generated. Normally, this
# is perfectly sane; however, the system is picking up mk directives and
# mkcmd directives as well as system depencies.
Autoreq:	0
Requires:	/bin/ksh install_base perl

%description
HXMD provides some basic input and output operations for hxmd (and distrib)
configuration files.  It populates a hash keyed on the value of the KeyField
macro, each value for that key is a hash of the columns that were specified for
the key, as well as any line-based macros inscope for any definition of that
key.

The datastructure will be :
	$hashref{ValueOfKeyField} =  { COL1 => VAL1,
	                               COL2 => VAL2 }
%prep
%setup -q
%build
%install
umask 022
make clean
perl Makefile.PL \
INSTALLARCHLIB=%{buildroot}/%{_local_libdir}/ \
INSTALLSITEARCH=%{buildroot}/%{_local_libdir}/ \
INSTALLVENDORARCH=%{buildroot}/%{_local_libdir}/ \
INSTALLPRIVLIB=%{buildroot}/%{_local_libdir}/ \
INSTALLSITELIB=%{buildroot}/%{_local_libdir}/ \
INSTALLVENDORLIB=%{buildroot}/%{_local_libdir}/ \
INSTALLBIN=%{buildroot}/%{_local_bindir} \
INSTALLSITEBIN=%{buildroot}/%{_local_bindir} \
INSTALLVENDORBIN=%{buildroot}/%{_local_bindir} \
INSTALLSCRIPT=%{buildroot}/%{_local_bindir} \
INSTALLSITESCRIPT=%{buildroot}/%{_local_bindir} \
INSTALLVENDORSCRIPT=%{buildroot}/%{_local_bindir} \
INSTALLMAN1DIR=%{buildroot}/%{_local_prefix}/man/man1 \
INSTALLSITEMAN1DIR=%{buildroot}/%{_local_prefix}/man/man1 \
INSTALLVENDORMAN1DIR=%{buildroot}/%{_local_prefix}/man/man1 \
INSTALLMAN3DIR=%{buildroot}/%{_local_prefix}/man/man3 \
INSTALLSITEMAN3DIR=%{buildroot}/%{_local_prefix}/man/man3 \
INSTALLVENDORMAN3DIR=%{buildroot}/%{_local_prefix}/man/man3
make install

find %{buildroot} -depth -type d -name OLD -exec rm -rf \{\} \;
find %{buildroot} \( -name perllocal.pod -o -name .packlist -o -name Makefile \) -exec rm -v \{\} \;
find %{buildroot}/usr -type f | \
sed "s@^%{buildroot}@@g" | \
grep -v \/man[13]\/ | grep -v \/Makefile > %{name}.filelist

if [ "$(cat %{name}.filelist)X" = "X" ]; then
        echo "EMPTY FILELIST";
        exit 1;
fi

%clean
# Do not confilct with the next user of the fixed-path build directory
rm -rf %{buildroot}

%files -f %{name}.filelist
%defattr(-,root,root)
%doc %{_local_prefix}/man/*/*

%changelog
* Tue Dec 22 2009  KS Braunsdorf
- We published the wrong version of the HXMD.pm because of a label error
* Tue Dec 02 2008  Ed Anderson
- Built the RPM spec file and added it to the msrc cache
