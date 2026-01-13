# $Id: xapply.spec,v 1.4 2004/11/02 22:09:58 petef Exp $
# $Compile: rpmbuild -bb %f
# to gen tarball, smsrc to a linux host, cd /usr/src/local/bin;
#	tar czf /usr/src/redhat/SOURCES/xapply-LINUX.tar.gz xapply

Summary: Extended apply and xargs replacement
Name: xapply
Version: 3.13
Release: 1
Group: SAC
Copyright: BSDL
Packager: Pete Fritchman<petef@sa.fedex.com>
Source: xapply-LINUX.tar.gz
%description
Extended apply is intended to be a complete replacement for the apply
and xargs commands.  In the normal mode xapply emulates the best features
of apply (substitutes limited percent escapes to form and execute shell
commands).  Under -f it emulates a xargs-like processor.

%prep
%setup -n xapply

%install
install -m 755 -o root xapply /usr/local/bin/xapply

%files
/usr/local/bin/xclate
/usr/local/bin/xapply
