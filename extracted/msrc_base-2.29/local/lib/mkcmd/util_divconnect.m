#!mkcmd
# $Id: util_divconnect.m,v 1.5 2008/09/01 17:35:25 ksb Exp $
# Connect to a diversion stack, setup by divstack al-la xclate or ptbw	(ksb)
from '<string.h>'
from '<sys/stat.h>'
from '<sys/types.h>'
from '<sys/socket.h>'

%i
void *(*divNextLevel)(void *pvThisLevel) = (void *(*)(void *))0;
int (*divConnHook)(int fd, void *pvLevel) = (int (*)(int, void *))0;
%%

require "util_errno.m"
require "util_divconnect.mi" "util_divconnect.mc"

key "divConnect" 1 init {
	"divConnect" "const char *pcSocket" "int (*pfiIndir)(int fd, char *pcCookie, void *pvData, char *pcType)" "void *pvLevel"
}
