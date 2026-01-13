#!mkcmd
# Get a hash value with:
#	auto char acHash[HASH_LENGTH+1];
#
#	pcGot = NewHash(acHash);
# (char *) failed, others are a value unique hash
from "<unistd.h>"
from "<stdlib.h>"

require "util_errno.m"
require "hash.mi" "hash-pipe.mc"

%i
static int fdHashPipe;
extern char *NewHash(char *);
%%

# by setting this to 4096 we can prevent wasting so many hash
# values for testing
integer 'p' {
	static named "iHashPad"
	init '(PIPE_BUF/2)'
	param "width"
	help "pad each hash value to this many bytes, in a pipe"
}
char* 'H' {
	named "pcHashSeq"
	param "seq"
	help "hashd sequence file"
	after "fdHashPipe = PipeHashd(pcHashSeq);"
}


key "hashd_path" 2 init {
	"/usr/local/libexec/hashd"
}

key "hash_pipe_size" 2 init {
	"iHashPad"
}
