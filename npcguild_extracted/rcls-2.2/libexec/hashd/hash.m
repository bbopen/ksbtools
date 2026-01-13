#!mkcmd
# $Id: hash.m,v 2.3 2000/06/14 01:26:34 ksb Exp $
# See the README for details.
from '<sys/types.h>'
from '<sys/param.h>'
from '<unistd.h>'
from '<fcntl.h>'

require "hash.mi" "hash.mc"

function 'b' {
	once param "start:bits"
	track "fGaveB"
	help "force bits at this position in the output"
	integer named "iStart" {
		param "start"
		help "start of forced bits"
	}
	char* named "pcBits" {
		param "bits"
		help "list of bits to force"
	}
	routine {
		list "iStart" ":" "pcBits"
		user "/* none */"
	}
	update "%mI(%a);"
	user "BitSweep(%rbMl);"
}

%i
static char acBase64[] =
	"0123456789abcdefghijklmnopqrstuvwxyz+@ABCDEFGHIJKLMNOPQRSTUVWXYZ";

typedef struct HMnode {		/* permutation table */
	int ipos, ibit;
} HASH_MAP;
static char *NextHash(int, char *);
%%

%c
/* this map removed the need for -b.  We can build the hash seq file
 * with the full 60 bits in it and put the host part in the MSB range. -- ksb
 */
HASH_MAP aHMMap[BITS_PER_DIGIT*HASH_LENGTH+2] = {
	{9, 0x01},	/* S01 J1  */
	{9, 0x02},	/* S02 J2  */
	{9, 0x04},	/* S03 J3  */
	{9, 0x08},	/* S04 J4  */
	{8, 0x01},	/* S05 I1  */
	{8, 0x02},	/* S06 I2  */
	{8, 0x04},	/* S07 I3  */
	{8, 0x08},	/* S08 I4  */
	{7, 0x01},	/* S09 H1  */
	{7, 0x02},	/* S10 H2  */
	{7, 0x04},	/* S11 H3  */
	{7, 0x08},	/* S12 H4  */
	{6, 0x01},	/* S13 G1  */
	{6, 0x02},	/* S14 G2  */
	{6, 0x04},	/* S15 G3  */
	{6, 0x08},	/* S16 G4  */
	{5, 0x01},	/* S17 F1  */
	{5, 0x02},	/* S18 F2  */
	{5, 0x04},	/* S19 F3  */
	{5, 0x08},	/* S20 F4  */
	{4, 0x01},	/* S21 E1  */
	{4, 0x02},	/* S22 E2  */
	{4, 0x04},	/* S23 E3  */
	{4, 0x08},	/* S24 E4  */
	{3, 0x01},	/* S25 D1  */
	{3, 0x02},	/* S26 D2  */
	{3, 0x04},	/* S27 D3  */
	{3, 0x08},	/* S28 D4  */
	{2, 0x01},	/* S29 C1  */
	{2, 0x02},	/* S30 C2  */
	{2, 0x04},	/* S31 C3  */
	{2, 0x08},	/* S32 C4  */
	{1, 0x01},	/* S33 B1  */
	{1, 0x02},	/* S34 B2  */
	{1, 0x04},	/* S35 B3  */
	{1, 0x08},	/* S36 B4  */
	{0, 0x01},	/* S37 A1  */
	{0, 0x02},	/* S38 A2  */
	{0, 0x04},	/* S39 A3  */
	{0, 0x08},	/* S40 A4  */
	{9, 0x20},	/* S41 J6  */
	{8, 0x20},	/* S42 I6  */
	{7, 0x20},	/* S43 H6  */
	{6, 0x20},	/* S44 G6  */
	{5, 0x20},	/* S45 F6  */
	{4, 0x20},	/* S46 E6  */
	{3, 0x20},	/* S47 D6  */
	{2, 0x20},	/* S48 C6  */
	{1, 0x20},	/* S49 B6  */
	{0, 0x20},	/* S50 A6  */
	{9, 0x10},	/* S51 J5  */
	{8, 0x10},	/* S52 I5  */
	{7, 0x10},	/* S53 H5  */
	{6, 0x10},	/* S54 G5  */
	{5, 0x10},	/* S55 F5  */
	{4, 0x10},	/* S56 E5  */
	{3, 0x10},	/* S57 D5  */
	{2, 0x10},	/* S58 C5  */
	{1, 0x10},	/* S59 B5  */
	{0, 0x10},	/* S60 A5  */
	{0, 0},		/* incase of a leading zero */	/*  */
	{-1, 0}		/* sentinal value */
};
%%
