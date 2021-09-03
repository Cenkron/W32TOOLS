/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/

#ifndef _DTYPES_H
#include <dtypes.h>
#endif

typedef
	struct
		{
		A_Z_FLAGS	flags;
		long int	lVal  [26];
		char *		szVal [26];
		int (*		pProc [26]) (int c, char *optarg);
		} GETOPT_DATA;

#define	GETOPT_A	0
#define	GETOPT_B	1
#define	GETOPT_C	2
#define	GETOPT_D	3
#define	GETOPT_E	4
#define	GETOPT_F	5
#define	GETOPT_G	6
#define	GETOPT_H	7
#define	GETOPT_I	8
#define	GETOPT_J	9
#define	GETOPT_K	10
#define	GETOPT_L	11
#define	GETOPT_M	12
#define	GETOPT_N	13
#define	GETOPT_O	14
#define	GETOPT_P	15
#define	GETOPT_Q	16
#define	GETOPT_R	17
#define	GETOPT_S	18
#define	GETOPT_T	19
#define	GETOPT_U	20
#define	GETOPT_V	21
#define	GETOPT_W	22
#define	GETOPT_X	23
#define	GETOPT_Y	24
#define	GETOPT_Z	25

/*----------------------------------------------------------------------*/

extern int           GetOptions (char *optname, int argc, char *argv[], char *optstring);
extern char /*key=*/ get_key(BOOL flag, BOOL ResponseSet);

extern	GETOPT_DATA	optdata;

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
