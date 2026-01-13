#!mkcmd
# Get a hash value with:
#	auto char acHash[HASH_LENGTH+1];
#
#	pcGot = NextHash(fdLockFile, acHash);
# (char *) failed, others are a value unique hash

require "hash.m"

# set your own routine name, like "HashInit"

fd ["O_RDWR"] 'L' {
	named "fdHashLock" "pcHashLock"
	late init '"/var/run/hash.seq"'
	before "%rLU = 1;"
	param "counter"
	help "name of hash file to consult for UUID data"
}
