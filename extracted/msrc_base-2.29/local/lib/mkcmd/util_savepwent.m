#!mkcmd
# $Id: util_savepwent.m,v 8.12 1998/11/29 18:51:59 ksb Exp $
from '<pwd.h>'

require "hosttype.m"
require "util_savepwent.mh" "util_savepwent.mi" "util_savepwent.mc"

# return a clone of the passwd entry, deep copy
key "util_savepwent" 2 init {
	"util_savepasswd" "%n"
}

# You can add things to this if there are other fields in a (struct passwd)
# add (char*) to strings and non-pointers to box. -- ksb
key "passwd_strings" 1 init once {
	"pw_name" "pw_passwd" "pw_gecos" "pw_dir" "pw_shell"
}

key "passwd_box" 1 init once {
	"pw_uid" "pw_gid"
}

# For any hosttype with more passwd members add "passwd_strings_"$type
# and "passwd_box_"$type and mkcmd will copy thos as well.  This is based
# on the foreach extension in 8.12 that lets the extra keys in a @foreach
# meta control be undefined (so unknown HostType's don't stop compilation).
# The worst thing that happens is that we forget to put a valid on here
# and it doesn't get copied.  Portable code won't see that. --ksb

# on HPUX add stuff with:
key "passwd_strings_HPUX" 1 init once {
	"pw_comment"
}
key "passwd_box_HPUX" 1 init once {
	"pw_audid" "pw_audflg"
}
