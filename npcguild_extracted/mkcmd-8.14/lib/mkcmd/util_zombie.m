#!mkcmd
# $Id: util_zombie.m,v 8.21 1998/11/29 03:23:56 ksb Exp $
# require me to get rid of pesky zombie processes			(ksb)
from '<sys/types.h>'
from '<sys/wait.h>'
from '<sys/signal.h>'

require "util_sigret.m"
require "util_zombie.mc"

init 1 "(void)signal(SIGCHLD, %K<SaltZombies>1v);"

# change this key to change the name of the zombie discard routine
key "SaltZombies" 1 init {
	"SaltZombies"
}
