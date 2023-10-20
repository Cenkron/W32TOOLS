/* ----------------------------------------------------------------------- */

#include  <windows.h>
#include  <stdio.h>
#include  <ctype.h>
#include  <string.h>

//#define  FWILD_INTERNAL

#include  "fwild.h"

/* ----------------------------------------------------------------------- */
#ifdef TEST
main ()					// Test main program

	{
	char  s [1024];
	int result = 0;

	for (;;)
		{
		printf("\nPattern: ");
		gets(s);
		printf("\n");
//		result = xxx(s);
		printf("\nResult:  %d      \"%s\"\n\n", result, s);
		}
	}

#endif // TEST
/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */
