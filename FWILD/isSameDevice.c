/* ----------------------------------------------------------------------- *\
|
|						    ISSAMEDEVICE
|
|				Copyright (c) 2023, all rights reserved
|						Brian W Johnson
|						   24-Sep-23 (new)
|
|			static int		// Returns TRUE if on the same device
|		SameDevice (
|			char *pS1,		// Pathspec 1
|			char *pS2)		// Pathspec 2
|
|	Determine whether the two pathspecs are on the same device
|	Each path may be relative or absolute
|
\* ----------------------------------------------------------------------- */

#include  <windows.h>
#include  <stdio.h>
#include  <ctype.h>
#include  <stdlib.h>

#include  "fWild.h"

// ---------------------------------------------------------------------------
//	#define DEBUG	// Define this to include the diagnostics

// ---------------------------------------------------------------------------
	int					// Returns TRUE if both on the same device
isSameDevice (
	const char *pS1,	// Pathspec 1
	const char *pS2)	// Pathspec 2

	{
#ifdef DEBUG
printf("P1: \"%s\"\n", pS1);
printf("P2: \"%s\"\n", pS2);
#endif
	char *pAS1 = NULL;	// Absolute version of pS1
	char *pPast1;		// Ptr past prefix1 (prefix required)
	char *pAS2 = NULL;	// Absolute version of pS2
	char *pPast2;		// Ptr past prefix2 (prefix required)

	int   result = FALSE;	// Assume not same device


	// None of these should fail since they are absolute paths
	
	if (((pAS1    = fnabspth(pS1)) != NULL)
	&&  ((pPast1  = PointPastPrefix(pAS1)))
	&&  ((pAS2    = fnabspth(pS2)) != NULL)
	&&  ((pPast2  = PointPastPrefix(pAS2))))
		{
		*pPast1 = '\0';		// Terminate after both prefixes
		*pPast2 = '\0';
		strupr(pAS1);		// Ensure matching case
		strupr(pAS2);
#ifdef DEBUG
printf("PA1: \"%s\"\n", pAS1);
printf("PA2: \"%s\"\n", pAS2);
#endif
		result = (strcmp(pAS1, pAS2) == 0);
		}
	if (pAS1)
		free(pAS1);
	if (pAS2)
		free(pAS2);

	return (result);
	}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
