/* this interface is connected to a select driver via a write-only	(ksb)
 * pipe or socket.  The interface blocks reading stdin for commands
 * then issues some commands to the pipe (those it doesn't recongnize).
 * the pipe reader must send a SIGCONT to this interface to wake it
 * up for the next command.
 */

#include <stdio.h>
#include <sys/types.h>

/* we use the mkcmd cmd_from_string() interface for parsing commands	(ksb)
 */

