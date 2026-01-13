#!mkcmd
# $Id: util_editor.m,v 8.10 1998/09/16 13:40:15 ksb Exp $
# spawn the User's editor for them on the given filename		(ksb)
from '<sys/types.h>'
from '<sys/wait.h>'
from '<sys/time.h>'
from '<sys/resource.h>'

require "util_fsearch.m"
require "util_editor.mh" "util_editor.mi" "util_editor.mc"

# update action to edit a filename (use %N for a FILE's name)
key "util_editor" 1 init {
	"util_editor" "%n"
}
key "util_editor_path" 1 init {
	"$VISUAL"
	"$EDITOR"
	"vi"
	"edit"
}
