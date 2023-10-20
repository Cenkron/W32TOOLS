/* ----------------------------------------------------------------------- *\
|
|						fnBuildPath
|						fnMakePath
|						fnGetPath
|
|				    Copyright (c) 2023,  all rights reserved
|						Brian W Johnson
|						   24-Sep-23 (consistency update)
|
|		int						Returns (0) for success, else (-1)
|	fnBuildPath (				Build the pathspec portion of the filespec
|		const char *pFileSpec)	Ptr to the filespec containing a pathspec
|
|
|	    int						Returns TRUE if successful verify/construct
|	fnMakePath (				Verify/Construct the path (if necessary)
|		const char *pathspec)	Pointer to the pathspec
|
|
|		int						Returns TRUE if buildable path found, else FALSE
|	fnGetPath (					Extracts the path portion of the filespec
|				   *pDst,		Pointer to caller's buffer to receive the path
|		const char *filespec)	Pointer to the filespec containing a path
|
|
|		int						Returns TRUE if buildable path found, else FALSE
|	_fnGetPath (				Extracts the path body portion of the filespec
|		const char *filespec)	Pointer to the filespec containing a path
|
|
|	filespec is a string containing both directories and a filename
|	pathspec is string containing only directories (no filename)
|	fnMakePath() verifies that the elements of the pathspec exists, or
|	fnMakePath() attempts to construct any missing portions of the pathspec
|	fnGetPath () extracts the pathspec portion of the filespec
|
|	fnMakePath() returns (0) for success, or (-1) for failure.
|	fnGetPath () returns a ptr to an allocated string containing the pathspec
|	fnGetPath () returns NULL if it fails
|
|	These functions replace the deprecated pathmake() function.
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <string.h>
#include  <stdlib.h>
#include  <direct.h>

#include  "fWild.h"

// ---------------------------------------------------------------------------
//	#define TEST	// Define this to include the test main
//	#define DEBUG	// Define this to include the diagnostics

#ifdef  TEST
#define DEBUG
#endif


#define NULCH	('\0')
#define PATHCH	('\\')

static	char	PathSpec [MAX_PATH];	// Working buffer
static	char   *pBody;					// Points past prefix and root

// ---------------------------------------------------------------------------
	int						// Returns TRUE if buildable path found
_fnGetPath (				// Determine if there is a buildable path within the filespec
	const char *pFileSpec)	// Pointer to the filespec

	{
	char   *pPath = PathSpec;	// Pointer to the working copy of the filespec
	char   *pSep;				// Pointer to an element separator
	char   *pTemp;				// Working pointer into the body
	char   *pLast = NULL;		// Pointer to the rightmost filename separator


	if (pFileSpec == NULL)					// NULL pointer not allowed
		return (FALSE);

	// Make a working copy of the passed pFileSpec string

	strcpy(pPath, pFileSpec);				// Copy the filespec
	strsetp(pPath, PATHCH);					// Standardize the path characters

#ifdef DEBUG
printf("\nfnGetPath 1: \"%s\"\n", pPath);
#endif

	pBody  = PointPastPrefix(pPath);		// Skip over the path prefix (and root separator)
	pTemp  = pBody;							// Working pointer into the body

	// The path contains some number of directories and a file
	// and ignoring the root separator, if any,
	// NUL the last separator to truncate the file from the pathspec string
	
	while ((pSep = strchr(pTemp, PATHCH)) != NULL)
		{
		pLast = pSep;						// Points the last separator found
		pTemp = (pSep+1);					// Points the tail

#ifdef DEBUG
printf("\nfnGetPath 2: \"%s\"\n", pTemp);
#endif
		}

	// At this point pLast points the last separator, if any

	if (pLast)								// if a last separator,
		*pLast = NULCH;						// Truncate the path at the last separator

#ifdef DEBUG
printf("\nfnGetPath 3: \"%s\"\n", PathSpec);
#endif

	return (pLast != NULL);					// Request build of the path
	}

// ---------------------------------------------------------------------------
	int						// Returns TRUE if buildable path found
fnGetPath (					// Determine if there is a buildable path within the filespec
		  char *pDst,		// Ptr to caller's buffer
	const char *pFileSpec)	// Pointer to the filespec

	{
	if ((pFileSpec == NULL)			// NULL pointer not allowed
	||  (pDst      == NULL))		// NULL pointer not allowed
		return (FALSE);

	if (_fnGetPath(pFileSpec))		// Get the pathspec
		strcpy(pDst, PathSpec);

	return (TRUE);	
	}

// ---------------------------------------------------------------------------
	int							// The returned result, 0 for success, else (-1)
fnMakePath (void)

	{
	int		result = 0;			// Success if verified or built all path directories
	char   *p      = pBody;		// Saved pointer to the temporary string body


#ifdef DEBUG
printf("\nfnMakePath 1: \"%s\"\n", PathSpec);
#endif

		// Use the temporary copy of the pPathSpec string
		// For each path element beyond a possible root separator),
		// verify that the directory exists, or try to create it

		do  {
			p = strchr(p, PATHCH);			// Find the next path separator
			if (p)
				*p = '\0';					// Truncate the path
#ifdef DEBUG
printf("\nfnMakePath 2: \"%s\"\n", PathSpec);
#endif

// Technically, at this point we should skip ".." elements here, but Windows handles them OK

			if (fnchkdir(PathSpec))			// Accept the existing directory, or
				{
				result = 0;
#ifdef DEBUG
printf("\nfnMakePath 3: fnchkdir (%d)\n", result);
#endif
				}
			else
				{
				result = _mkdir(PathSpec);	//   construct the missing directory
#ifdef DEBUG
printf("\nfnMakePath 4: _mkdir (%d)\n", result);
#endif
				}
			if (p)
				*p++ = PATHCH;				// Replace the path separator
			} while ((result == 0) && (p != NULL)); // do-while

#ifdef DEBUG
printf("\nfnMakePath 5: (%d)\n", result);
#endif

	return  (result);
	}

// ---------------------------------------------------------------------------
	int						// The returned result, 0 for success, else (-1)
fnBuildPath (
	const char *pFileSpec)	// Ptr to a filespec containing a pathspec

	{
	int		result = 0;		// Result of the path build, assume success

	// Extract the pathspec from the filespec, then
	// construct the path according to the pathspec

	if (_fnGetPath(pFileSpec))
		result = fnMakePath();

	return (result);
	}

// ---------------------------------------------------------------------------
#ifdef TEST
main ()					/* Test main program */

	{
	char  s [1024];
	int     result = 0;
	 
	for (;;)
		{
		printf("\nPattern: ");
		gets(s);
		printf("\n");
		result = _fnGetPath(s);
		printf("PathSpec: \"%s\"\n", PathSpec);
#if 1
		result = fnMakePath();
#endif
		printf("\nResult: %d\n", result);
		}
	}

#endif // TEST
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
