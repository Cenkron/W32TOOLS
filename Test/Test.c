/* ----------------------------------------------------------------------- */

#include  <windows.h>
#include  <stdio.h>
#include  <ctype.h>
#include  <string.h>

//#define  FWILD_INTERNAL

#include  "fwild.h"

/* ----------------------------------------------------------------------- */
#ifdef  TEST
main (
	int    argc,
	char  *argv [])

	{
	char  *p;

	if (argc > 1)
		p = *++argv;
	else
		p = "";
	if (fnchkfil(p))
		printf("TRUE\n");
	else
		printf("FALSE\n");
	}
#endif
/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */
