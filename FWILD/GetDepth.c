/* -----------------------------------------------------------------------
|
|				    GETDEPTH
|
|		    Copyright (c) 2023, all rights reserved
|				Brian W Johnson
|				   10-Sep-23
|
|	    int					Return the directory depth of the cwd, or -1 if error
|	result = getdepth (n);	Get the cwd depth for drive n
|	    int    n;		The drive number (0 => current, 1 => A, etc)
|
\* ----------------------------------------------------------------------- */

#include  <windows.h>
#include  <stdlib.h>
#include  <string.h>
#include  <direct.h>

#include  "fwild.h"

#define	PATHCH	('\\')
#define	NULCH	('\0')

// -----------------------------------------------------------------------
	int				// The returned depth, or -1 for failure
getdepth (			// Convert a filename to absolute format
	int    drive)	// The drive number (0 => default, 1 => A), etc)

	{
	int depth = 0;			// Depth of the cwd
	char cwd [MAX_PATH];	// Ptr to the CWD string
	char *pTemp;			// Ptr past the drive spec
	char* p;				// Working pointer

	if (_getdcwd(drive, cwd, MAX_PATH) == NULL)
		return (ERROR);

	strsetp(cwd, PATHCH);	// Unitize the path characters

//printf("CWD: \"%s\"\n", cwd);

	if ((pTemp = QueryDrivePrefix(cwd, TRUE)) != NULL)	// Single mode
		p = pTemp;			// We have a drive spec; point the root
	else
		p = cwd;			// No drive spec

//printf("Suffix: \"%s\"\n", p);

	if (strlen(p) <= 1)
		depth = 0;

	else
		{
		do	{				// Count the path separators
			if (*p == PATHCH)
				++depth;
			} while (*(++p) != NULCH);
		}

//printf("CWD: %d; \"%s\"\n", depth, pTemp);

	return (depth);
	}

// -----------------------------------------------------------------------
