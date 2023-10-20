/* ----------------------------------------------------------------------- *\
|
|						    GETDEPTH
|
|				    Copyright (c) 2023, all rights reserved
|						Brian W Johnson
|						   10-Sep-23
|
|	    int					Return the directory depth of the cwd, or -1 if error
|	result = _getdepth (	Get the cwd depth for drive n
|	    int    n)			The drive number (0 => current, 1 => A, etc)
|
|	    int					Return the directory depth of the cwd, or -1 if error
|	result = GetDepth (		Get the cwd depth for the specified path
|		const char *pPath)	Pointer to the caller's reference path
|
|	A reference path must be an absolute drive type path
|	A relative pathspec or a UNC pathspec cannot have a CWD
|
\* ----------------------------------------------------------------------- */

#include  <windows.h>
#include  <stdlib.h>
#include  <string.h>
#include  <direct.h>

#include  "fWild.h"

// -----------------------------------------------------------------------
//	#define TEST	// Define this to include the test main
//	#define DEBUG	// Define this to include the diagnostics

#ifdef DEBUG
#include  <stdio.h>
#endif

#define	DriveIndex(ch)	((toupper(ch) - 'A') + 1)

#define	PATHCH	('\\')
#define	NULCH	('\0')

static	char	buffer [MAX_PATH];	// CWD string buffer
static	int		drive;				// Effective drive number

// -----------------------------------------------------------------------
	int				// The returned depth, or (-1) for failure
_getdepth (			// Get the depth of the specified CWD
	int    drive)	// The drive number (0 => default, 1 => A), etc)

	{
	int depth = 0;			// Depth of the cwd
	char *pBody;			// Ptr past the drive spec and root separator
	char* p;				// Working pointer

	if (_getdcwd(drive, buffer, MAX_PATH) == NULL)
		return (-1);		// Invalid drive number

	strsetp(buffer, PATHCH);	// Unitize the path characters

//printf("CWD: \"%s\"\n", buffer);

	if ((pBody = QueryDrivePrefix(buffer)) == NULL)
		return (0);		// No drive spec, ???

	p = pBody;			// We have a drive spec; point the body
	drive = DriveIndex(buffer[0]);

//printf("Body: \"%s\"\n", p);

	int  inPath = FALSE;
	char ch;
	while ((ch = *(p++)) != NULCH)
		{
		if (ch == PATHCH)
			inPath = FALSE;
		else if (inPath == FALSE)
			{		
			++depth;		// Count the non-empty path elements
			inPath = TRUE;
			}
		}

#ifdef DEBUG
printf("_getdepth: [%d];      \"%s\"\n", depth, pBody);
#endif

	return (depth);
	}

// -----------------------------------------------------------------------
	int					// Returns initial depth of pathspec, or (-1) if error
GetDepth (				// Get the initial depth of the pathspec
    const char  *pPath,	// Pointer to the pathspec string
	int			*pDrive)	// Point to returned drive number (if provided)

	{
	char  *pBody;		// Returned result from prefix queries
	int    CWDdepth;	// The CWD depth

#ifdef DEBUG
printf("GetDepth Entry:      \"%s\"\n", pPath);
#endif

	if (pPath == NULL)
		return (_getdepth(0));		// No path given; use the default CWD depth

	pathCopy(buffer, pPath, MAX_COPY);
	strsetp(buffer, PATHCH);		// Normalize the standard path character in the string

	if ((pBody = QueryUNCPrefix(buffer)) != NULL)
		{
		CWDdepth = 0;				// rooted by definition
		drive    = 0;				// No drive number
		}

	else if ((pBody = QueryDrivePrefix(buffer)) != NULL)
		{
		drive = DriveIndex(buffer[0]);	// Using specified drive
		if (isRooted())				// If the path is drive spec, rooted
			CWDdepth = 0;			// pathspec is rooted, so the CWD depth is zero
		else
			CWDdepth = _getdepth(drive);	// Drive spec, unrooted
		}

	else if ((pBody = QueryRootPrefix(buffer)) != NULL)
		{
		drive = _getdrive();		// Using the default drive
		CWDdepth = 0;				// pathspec is rooted, so the CWD depth is zero
		}

	else // No UNC, no drive, unrooted
		{
		drive    = _getdrive();		// Using the default drive
		CWDdepth = _getdepth(0);	// Request for the default drive CWD depth
		}

#ifdef DEBUG
printf("GetDepth Return: [%d] \"%s\"\n", CWDdepth, pPath);
fflush(stdout);
#endif
	if (pDrive)				// If requested,
		*pDrive = drive;	//   report the drive number
	return (CWDdepth);		// Report the effective depth of the tested path
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
		result = fnCondense(s);
		printf("\nResult:  %d      \"%s\"\n\n", result, s);
		}
	}

#endif // TEST
// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
