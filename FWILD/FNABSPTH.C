/* ----------------------------------------------------------------------- *\
|
|					    FNABSPTH
|
|			Copyright (c) 1985, 1990, all rights reserved
|					Brian W Johnson
|						26-May-90
|						17-Dec-94
|						17-Aug-97
|						25-Sep-97 UNC
|						10-Sep-23 consistent with new fnreduce
|
|		int						Returns 0 for success, else (-1) if failed
|	_fnabspth (					Convert a pathspec to absolute format
|		char		*pDst,		Pointer to caller's destination buffer [MAX_PATH]
|		const char  *pPath)		Pointer to the source filename string
|
|		char *					Returns an allocated string
|	fnabspth (					Convert a filename to drive:/path/file
|		const char	*pPath)		Pointer to the source filename string
|
|	The fnabspth() returns NULL if fnreduce() reports an error.
|	The valid return string is allocated, and should be disposed with free()
|	The valid return string is guaranteed to contain at least "X:/" or UNC equivalent.
|	The valid return string path characters are standardized.
|
\* ----------------------------------------------------------------------- */

#include  <windows.h>
#include  <stdio.h>
#include  <ctype.h>
#include  <stdlib.h>
#include  <string.h>

#include  "fWild.h"

/* ----------------------------------------------------------------------- */
//	#define TEST	// Define this to include the test main
//	#define DEBUG	// Define this to include the diagnostics

#ifdef  TEST
#define DEBUG
#endif

#define  NULCH			('\0')
#define	 isPath(ch)		(((ch) == '/') || ((ch) == '\\'))
#define	 DriveIndex(ch)	((toupper(ch) - 'A') + 1)
#define	 DriveLetter(n)	('A' + (n-1))

static	char   temp [MAX_PATH];		/* Temporary string buffer */

/* ----------------------------------------------------------------------- */
	static int					// Returns 0 for success, else (-1) if failed
__fnabspth (					// Convert a pathspec to absolute format
		  char  *pDst,			// Pointer to the destination buffer [MAX_PATH]
	const char  *pPath)			// Pointer to the source filename string

	{
	char		*pTail;			// Pointer to the tail of the pathspec
	char        *pCWD;			// Pointer to the CWD of the pathspec drive
	int			drive = 0;		// Drive number, defaulted to default drive

	if (QueryUNCPrefix(pPath))				// If the filespec is UNC,
		{
#ifdef DEBUG
printf("__fnsabs UNC\n");
#endif
		strcpy(pDst, pPath);				//   it is absolute by definition
		return (0);
		}

	if (pTail = QueryDrivePrefix(pPath))
		{
#ifdef DEBUG
printf("__fnsabs Drv\n");
#endif
		if (isRooted())						// If drive prefixed and rooted,
			{
			pathCopy(pDst, pPath, MAX_COPY); //  the path is already absolute (with drive)
			return (0);
			}

		drive = DriveIndex(*pPath);			// Remember the drive number
		}

	else if (pTail = QueryRootPrefix(pPath))
		{	
#ifdef DEBUG
printf("__fnsabs Rooted\n");
#endif
//printf("\"%s\"\n", pPath);
		MakePrefixNumber(0, pDst);			// Make a valid rooted drive prefix
//printf("\"%s\"\n", pDst);
		pathCat(pDst, pPath, MAX_COPY);		// Add the path body to the path
//printf("\"%s\"\n", pDst);
		return (0);
		}

	else
		pTail = (char *)(pPath);			// The path IS the tail

	// At this point, we have only the tail of the unrooted path, and the updated drive number

	if ((pCWD = __getDir(drive)) == NULL)	// Get the CWD
		return (-1);						// Error, apparently not a valid drive

	pathCopy(pDst, pCWD, MAX_COPY);			// Copy the CWD
	pathCat(pDst, pTail, MAX_COPY);			// Concatenate the path body

	return (0);
	}

/* ----------------------------------------------------------------------- */
	int							// Returns 0 for success, else (-1) if failed
_fnabspth (						// Convert a pathspec to absolute format
		  char  *pDst,			// Pointer to caller's destination buffer [MAX_PATH]
	const char  *pPath)			// Pointer to the source filename string

	{
	int	result;					// The returned result

	if ((pDst == NULL)						// Check for NULL pointers
	||  (pPath == NULL))
		return (-1);

#ifdef DEBUG
printf("_fnsabs Entry     \"%s\"\n", pPath);
#endif

	if (pDst == pPath)						// Single buffer
		{
		pathCopy(temp, pPath, MAX_COPY);	// Copy the src buffer to temp
		result = __fnabspth(pDst, temp);	// Build the raw pathspec

#ifdef DEBUG
//printf("_fnsabs A  [%d] \"%s\"\n", result, pDst);
#endif

		}
	else									// Separate buffers
		{
		result = __fnabspth(pDst, pPath);	// Build the raw pathspec

#ifdef DEBUG
//printf("_fnsabs B  [%d] \"%s\"\n", result, pDst);
#endif
		}

	if (result == 0)						// Reduce the resultant pathspec
		fnreduce(pDst);

#ifdef DEBUG
printf("_fnsabs Exit [%d] \"%s\"\n", result, pDst);
#endif

	return (result);
	}

/* ----------------------------------------------------------------------- */
	char *						// Ptr to the returned allocated pathspec
fnabspth (						// Convert a pathspec to absolute format
	const char  *pPath)			// Pointer to the source filename string

	{
	char  *pAbsPath = fmalloc(MAX_PATH);

	if ((pPath == NULL)				// Check the path
	||  (pAbsPath == NULL))			// (only to make the compiler happy)
		return (NULL);

#ifdef DEBUG
printf("fnabspth Entry \"%s\"\n", pPath);
#endif

	if (_fnabspth(pAbsPath, pPath) != 0) // Build the pathspec
		{
		free(pAbsPath);				// Bad filespec
		return (NULL);
		}
		
#ifdef DEBUG
printf("fnabspth Exit  \"%s\"\n", pAbsPath);
#endif

	return (pAbsPath);
	}

/* ----------------------------------------------------------------------- */
#ifdef TEST
main ()					/* Test main program */

	{
	char  s [1024];
	int   result;

	for (;;)
		{
		printf("\nPattern: ");
		gets(s);
		printf("\n");
		result = _fnabspth(s, s);
		printf("\nResult:  %d      \"%s\"\n\n", result, s);
		}
	}
#endif
/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */
