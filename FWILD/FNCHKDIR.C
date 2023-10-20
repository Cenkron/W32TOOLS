/* ----------------------------------------------------------------------- *\
|
|						   FNCHKDIR
|
|			    Copyright (c) 1985, 2023, all rights reserved
|					Brian W Johnson
|					   26-May-90
|					   17-Dec-94
|					   17-Aug-97 NT
|					   26-Sep-97 UNC
|					   30-Sep-23 (Major update)
|
|		int				Returns TRUE if the string "s" is the
|	fnchkdir (			  name of an existing directory file
|		char  *s)		Pointer to the string
|
\* ----------------------------------------------------------------------- */

#include  <windows.h>
#include  <stdio.h>
#include  <string.h>

#define  FWILD_INTERNAL

#include  "fWild.h"

/* ------------------------------------------------------------------------ */
//	#define TEST	// Define this to include the test main
//	#define DEBUG	// Define this to include the diagnostics

#ifdef  TEST
#define DEBUG
#endif

#define	PATHCH	('\\')

static char temp [MAX_PATH];

/* ------------------------------------------------------------------------ */
	int						/* Return TRUE iff the directory exists */
fnchkdir (					/* Verify the existence of a directory */
	const char  *pPath)		/* Pointer to the filename string */

	{
	int      result = FALSE;	// The returned result
	char    *pbuff  = temp;		// Absoluted copy of the input string

	if (pPath == NULL)					// Null string ptr is illegal
		return (FALSE);

#ifdef DEBUG
printf("fnchkdir Entry   \"%s\"\n", pPath);
#endif

	if (isWild(pPath))					// Wild is not allowed
		return (FALSE);

	if (_fnabspth(pbuff, pPath) != 0)	// Make absolute and reduce
		return (FALSE);					// Invalid path

	strsetp(pbuff, PATHCH);				// Standardize the path character

#ifdef DEBUG
printf("Checking:        \"%s\"\n", pbuff);
#endif

	result = isDirectory(pbuff);			// Test it

#ifdef DEBUG
printf("fnchkdir Exit    %d\n", result);
printf("fnchkdir Exit    %s\n", ((result) ? "TRUE" : "FALSE"));
#endif
	return (result);
	}

/* ----------------------------------------------------------------------- */
#ifdef TEST
main ()					/* Test main program */

	{
	char  in [1024];
	int   result;

	for (;;)
		{
		printf("\nPattern: ");
		gets(in);
		if (tolower(in[0]) == 'q')	// Terminate testing
			break;

		printf("\n");
		result = fnchkdir(in);
		printf("\nResult: %d\n",  result);
		printf("\nResult: %s   \"%s\"\n\n",
			((result) ? "TRUE" : "FALSE"), in);
		}
	}

#endif // TEST
/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */
