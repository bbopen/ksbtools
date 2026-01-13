/*@Version $Revision: 6.0 $@*/
/*@Header@*/
/* this is where we have to put all the common dot-h includes
 */
#include <stdio.h>
#include <ctype.h>

/* here's where we change static vars to global */
/*@Insert 'extern int i;'@*/

/*@Remove@*/
static int i;

/*@Version $Revision: 6.0 $@*/
/*@Explode -bar@*/
/* function comments for bar
 */
int
bar(argc, argv)
int argc;
char **argv;
{
	return 0;
}

/*@Version $Revision: 6.0 $ w/o the bug from 1992@*/
/*@Explode -foo@*/
/* function comments for foo_needs
 */
void
foo_needs()
{
	return;
}

/* comments for foo
 */
char *
foo(pcFmt)
char *pcFmt;
{
	foo_needs();
	return "OK";
}
