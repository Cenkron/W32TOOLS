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
|	fnBuildPath (				Build the patspec portion of the filespec
|		const char *pFileSpec)	Ptr to the filespec containing a pathspec
|
|
|	    int						Returns TRUE if successful verify/construct
|	fnMakePath (				Verify/Construct the path (if necessary)
|		const char *pathspec)	Pointer to the pathspec
|
|
|		char *					Returns ptr to the allocated path string
|	fnGetPath (					Extracts the path portion of the filespace
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

#include  "fwild.h"

// ---------------------------------------------------------------------------

//#define TEST	// Define this to include the test main

//#define DEBUG	// Define this to include the diagnostics

#ifdef  TEST
#define DEBUG
#endif


#define NULCH	('\0')
#define PATHCH	('\\')

// ---------------------------------------------------------------------------
	int						// The returned result, 0 for success, else (-1)
fnBuildPath (
	const char *pFileSpec)	// Ptr to a filespec containing a pathspec

	{
	int		result = 0;		// Result of the path build
	char   *pPathSpec;		// Pointer to the pathspec to be built

	// Extract the pathspec from the filespec, then
	// construct the path according to the pathspec

	if ((pPathSpec = fnGetPath(pFileSpec)) == NULL)
		result = (-1);
	else
		{
		result = fnMakePath(pPathSpec);
		free(pPathSpec);
		}

	return (result);
	}

// ---------------------------------------------------------------------------
	char *					// returns ptr to the allocated path string
fnGetPath (					// The caller must free the string
	const char *pFileSpec)	// Pointer to the pathspec

	{
	char   *pTemp;			// Pointer to the allocated pathname string
	char   *p;				// Pointer into the temporary pathname string
	char   *s;				// Pointer into the temporary pathname string
	char   *pLast = NULL;	// Pointer to the trailing filename seperator


#ifdef DEBUG
printf("\nfnGetPath 1: \"%s\"\n", ((pFileSpec != NULL) ? pFileSpec : "NULL"));
#endif

	// Make an allocated copy of the passed pFileSpec string

	if ((pTemp = strdupMax(pFileSpec)) != NULL)
		{
		strsetp(pTemp, PATHCH);				// Standardize the path characters

		s = PointPastPrefix(pTemp, TRUE);	// Skip over the path prefix
		if (*s == PATHCH)					// If rooted,
			++s;							//   bypass it

		// The path contains some number of directories and a file
		// and ignoring the root separator, if any,
		// NUL the last separator to truncate the file from the pathspec string
	
		while ((p = strchr(s, PATHCH)) != NULL)
			{
			pLast = p;	// Points the last separator found
			s = (p+1);	// Points the element following the last separator found
#ifdef DEBUG
printf("\nfnGetPath 2: \"%s\"\n", ((s != NULL) ? s : "NULL"));
#endif
			}

		// At this point pLast points the deepest found separator, if any

		if (pLast)
			{
			*pLast = NULCH;			// Path found; truncate the filename
#ifdef DEBUG
printf("\nfnGetPath 3: \"%s\"\n", ((pTemp != NULL) ? pTemp : "NULL"));
#endif
			}
		else
			{
			free(pTemp);			// Path not found; return failure
			pTemp = NULL;
			}
		}

#ifdef DEBUG
printf("\nfnGetPath 4: \"%s\"\n", ((pTemp != NULL) ? pTemp : "NULL"));
#endif

	return (pTemp);			// If successful, return the allocated path string
	}

// ---------------------------------------------------------------------------
	int						// The returned result, 0 for success, else (-1)
fnMakePath (
	const char *pPathSpec)	// Pointer to the pathspec to be built

	{
	int		result = 0;		// Success if verified or built all path directories
	char   *p;				// Pointer into the temporary string
	char   *pTemp;			// Pointer to the temporary pPathSpec string


#ifdef DEBUG
printf("\nfnMakePath 1: \"%s\"\n", ((pPathSpec != NULL) ? pPathSpec : "NULL"));
#endif
	// Make a temporary copy of the pPathSpec string

	if ((pTemp = strdupMax(pPathSpec)) == NULL)
		return (result = -1);

	strsetp(pTemp, PATHCH);				// Standardize the path characters
	p = PointPastPrefix(pTemp, TRUE);	// Skip over any prefix, single mode
	if (*p == PATHCH)					// If a root separator,
		++p;							//   skip over it

	// For each path element (except preceding a possible root separator),
	// verify that the directory exists, or try to create it

#ifdef DEBUG
printf("\nfnMakePath 2: \"%s\"\n", ((p != NULL) ? p : "NULL"));
#endif

	if (*p != NULCH)					// If no actual pathspec, just report success
		{
		do  {
			p = strchr(p, PATHCH);		// Find the next path separator
			if (p)
				*p = '\0';			// Truncate the path
#ifdef DEBUG
printf("\nfnMakePath 3: \"%s\"\n", ((pTemp != NULL) ? pTemp : "NULL"));
#endif

// Technically, at this point we should skip ".." elements here, but Windows handles them OK

			if (fnchkdir(pTemp))	// Accept the existing directory, or
				{
				result = 0;
#ifdef DEBUG
printf("\nfnMakePath 4: fnchkdir (%d)\n", result);
#endif
				}
			else
				{
				result = _mkdir(pTemp);		//   construct the missing directory
#ifdef DEBUG
printf("\nfnMakePath 5: _mkdir (%d)\n", result);
#endif
				}
			if (p)
				*p++ = PATHCH;				// Replace the path separator
			} while ((result == 0) && (p != NULL)); // do-while
		}

#ifdef DEBUG
printf("\nfnMakePath 6: (%d)\n", result);
#endif
	if (pTemp)
		free(pTemp);
	return  (result);
	}

// ---------------------------------------------------------------------------
#ifdef TEST
main ()					/* Test main program */

	{
	char  s [1024];
	char  *pPath;
	int     result = 0;
	 
	for (;;)
		{
		printf("\nPattern: ");
		gets(s);
		printf("\n");
		pPath = fnGetPath(s);
		printf("pPath: \"%s\"\n", ((pPath != NULL) ? pPath : "NULL"));
#if 1
		if (pPath)
			{
			result = fnMakePath(pPath);
			free(pPath);
			}
#endif
		printf("\nResult: %d\n", result);
		}
	}

#endif // TEST
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
