/* ----------------------------------------------------------------------- *\
|
|							    isValidDrive
|
|				    Copyright (c) 2023, all rights reserved
|							  Brian W Johnson
|								30-Sep-23 (New)
|
|
|	    int						Returns TRUE iff valid
|	result = _isValidDrive (	Get the directory path for drive n
|	    int    n)				The drive number (0 => current)
|
|	    int						Returns TRUE iff valid
|	result = isValidDrive (		Get the directory path for drive in pathspec
|	    const char pPath)		Pointer to the pathspec
|
|	A relative UNC pathspec cannot be a valid drive
|
\* ----------------------------------------------------------------------- */

#include  <windows.h>
#include  <stdio.h>
#include  <direct.h>
#include  <stdlib.h>

#include  "fWild.h"

/* ----------------------------------------------------------------------- */
//	#define DEBUG	// Define this to include the diagnostics

#define	DriveIndex(ch)	((toupper(ch) - 'A') + 1)

static char rootPath [] = { "X:\\" };

/* ----------------------------------------------------------------------- */
	int					// Returns TRUE if the drive is valid
_isValidDrive (			// Test for valid drive number
	int    drive)		// The drive number (1 => A), 0 => default

	{
	int  driveType;
#ifdef DEBUG
printf("_isValidDrive drive: \"%d\"\n", drive);
#endif
	if (drive == 0)
		driveType = GetDriveTypeA(NULL);
	else 
		{
		rootPath[0] = (('A' + drive) - 1);
		driveType = GetDriveTypeA(rootPath);
		}
#ifdef DEBUG
printf("_isValidDrive type: \"%d\"\n", driveType);
#endif
	return (driveType >= 2);
	}

/* ----------------------------------------------------------------------- */
	static int			// Returns TRUE if the drive is valid
isValidDrive (			// Test for valid drive number
	const char  *pPath)	// Ptr to the pathspec

	{

#ifdef DEBUG
printf("isValidDrive drive: \"%s\"\n", drive);
#endif

	if (pPath == NULL)
		{
		return (FALSE);					// NULL path is not a valid drive
		}

	else if (QueryUNCPrefix(pPath))
		{
		return (FALSE);					// UNC path cannot be a valid drive
		}

	else if (QueryDrivePrefix(pPath))
		{
		int drive = DriveIndex(*pPath);
		return (_isValidDrive(drive));	// Check the referenced path
		}

//	else
//		{
		return (TRUE);					// Default drive is a valid drive
//		}
	}

/* ----------------------------------------------------------------------- */
