# $Id: util_username.m,v 8.9 2000/07/29 19:04:14 ksb Exp $
# Find out who is running this program (real uid)
# [we know we get string{,s}.h from mkcmd or the user (-G)]
from '<pwd.h>'
from '<stdlib.h>'

require "util_errno.m"
require "util_username.mh" "util_username.mi" "util_username.mc"

init 2 '(void)%K<util_finduser>1v(%K<util_username>1v);'

# the function to find a username
key "util_finduser" 2 init {
	"u_FindUser" "%n"
}

# the global buffer we set at init time. (second is the size of that array)
key "util_username" 2 init {
	"username" "16"
}

# We could set a key value here to export/hide the username buffer but
# it is too much work for such a trivial issue -- ksb
