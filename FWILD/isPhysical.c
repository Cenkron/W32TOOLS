/* ----------------------------------------------------------------------- *\
|
|							   isPhysical
|
|			    Copyright (c) 2023, all rights reserved
|					Brian W Johnson
|					    1-Oct-23 (New)
|
|		int					Returns TRUE if the string "s" is the
|	isPhysical (			  name of a valid path root
|		char  *pathspec)	Pointer to the pathspec
|
\* ----------------------------------------------------------------------- */

#include  <windows.h>
#include  <stdio.h>
#include  <sys\stat.h>
#include  <tchar.h>

#include  "fWild.h"

/* ------------------------------------------------------------------------ */
//	#define TEST	// Define this to include the test main
//	#define DEBUG	// Define this to include the diagnostics

#ifdef  TEST
#define DEBUG
#endif

#define	NULCH	('\0')
#define	PATHCH	('\\')

static char temp [MAX_PATH];

/* ------------------------------------------------------------------------ */
	int							// Return TRUE if the path is physically valid
isPhysical (					// Verify the existence of a path
	const char  *pPathspec)		// Pointer to the pathspec

	{
	int      result;					// The returned result
	int      flags = 0;					// The returned result
	char    *pBuff = temp;				// Absoluted copy of the input string

	if (pPathspec == NULL)				// Null is an invalid pathspec
		return (FALSE);

#ifdef DEBUG
printf("isPhysical Entry  \"%s\"\n", pPathspec);
#endif

	if (_fnabspth(pBuff, pPathspec) != 0)	// Make absolute and reduce
		return (FALSE);						// Invalid path
	
	strsetp(pBuff, PATHCH);					// Standardize the path character

#ifdef DEBUG
printf("Buffer:           \"%s\"\n", pBuff);
#endif

	char *p;								// Isolate the path root

	if (p = QueryDrivePrefix(pBuff))		// Only two possibilities
		strcpy(p, "\\\0");
	else if (p = QueryUNCPrefix(pBuff))
		*p = ('\0');

#ifdef DEBUG
printf("Checking:         \"%s\"\n", pBuff);
#endif

	flags = fnFileType(pBuff);				// Get the flags for the path
	result = (flags > 0);					// Report the result

#if 0	// Saved just in future case
	else if (flags & _S_IFREG)
		result = (TRUE);					// Found a file is physical
	
	else if (flags & _S_IFDIR)				// Readable/writable directory is physical
		result = (flags & (_S_IREAD | _S_IWRITE));
#endif		

#ifdef DEBUG
printf("Flags:            \"%04X\"\n", flags);
printf("isPhysical Exit   \"%s\"\n", ((result) ? "TRUE" : "FALSE"));
#endif
	return (result);
	}

/* ----------------------------------------------------------------------- */
#ifdef TEST
main ()					/* Test main program */

	{
	char  s [1024];
	int result;

	for (;;)
		{
		printf("\nPattern: ");
		gets(s);
		printf("\n");
		result = isPhysical(s);
		printf("\nResult: %s\t  \"%s\"\n\n",
			((result) ? "TRUE" : "FALSE"), s);
		}
	}

#endif // TEST
/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */
