#!mkcmd
# read in an "unlimited" vector of integers				(ksb)

require "std_help.m" "std_version.m" "std_noargs.m"
require "util_ppm.m"

%i
#define DEBUG
static char rcsid[] =
	"$Id: example_ppm.m,v 8.31 2000/07/30 20:37:37 ksb Exp $";
%%

after {
}

%c
void
after_func()
{
	static PPM_BUF PPMOne;
	auto int iNew;
	register int iHave, *piList, iSum;

	printf("Input integer and I'll remember them:\n");
	util_ppm_init(& PPMOne, sizeof(int), 10);
	util_ppm_print(& PPMOne, stdout);
	for (iSum = iHave = 0; 1 == scanf(" %d", & iNew); iSum += iNew) {
		piList = (int*)util_ppm_size(& PPMOne, iHave+1);
		piList[iHave++] = iNew;
	}

	/* sort them or what ever here
	 */
	printf("I saw %d integers.  Their sum is %d.  They are stored at %p\n", iHave, iSum, util_ppm_size(& PPMOne, 0));
	util_ppm_print(& PPMOne, stdout);

	/* dispose of the allocated RAM
	 */
	printf("Cleanup\n");
	util_ppm_free(& PPMOne);
	util_ppm_print(& PPMOne, stdout);
}
%%
