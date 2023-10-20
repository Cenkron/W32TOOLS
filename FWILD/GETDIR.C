/* ----------------------------------------------------------------------- *\
|
|							    GETDIR
|
|				    Copyright (c) 1990, all rights reserved
|							  Brian W Johnson
|								26-May-90
|								17-Dec-94
|								16-Aug-97
|								28-Sep-23 (Major revision)
|
|	    int					Returns the length of the returned path, or -1 if error
|	result = _getdir (		Get the directory path for drive n
|	    char  *s,			Pointer to the caller's pathname buffer
|	    int    n,			The drive number (0 => current)
|
|	    int					Returns the length of the returned path, or -1 if error
|	result = getdir (		Get the directory path for the reference path
|	    char  *s,			Pointer to the caller's pathname buffer
|	    const char pPath)	Pointer to the caller's reference path
|
|	A reference path must be an absolute drive type path
|	A relative pathspec or a UNC pathspec cannot have a CWD
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

#define	NULCH 	('\0')

static char prefix [] = "X:\\";

static char temp [MAX_PATH];

/* ----------------------------------------------------------------------- */
	char *				// Returns a ptr to the found CWD, or NULL if error
__getDir (				// Get the CWD path for a drive
	int    drive)		// The drive number (1 => A), 0 => default

	{
	if (! _isValidDrive(drive))
		{
#ifdef DEBUG
printf("__getdir - isValidDrive drive error\n");
#endif
		*temp = NULCH;
		return (NULL);			// Error, invalid drive
		}

	else if (_getdcwd(drive, temp, MAX_PATH) != NULL)
		{
#ifdef DEBUG
printf("__getdir CWD path: (%d)   \"%s\"\n", drive, temp);
#endif
		return (temp);			// Success
		}

//	else // returned NULL
//		{
#ifdef DEBUG
printf("__getdir error\n");
#endif
		return (NULL);			// Error, didn't get CWD for some reason
//		}
	}

/* ----------------------------------------------------------------------- */
	char *				// Returns a ptr to the found CWD, or NULL if error
_getDir (				// Get the CWD path for a drive
	const char  *pPath)	// Ptr to the caller's reference path

	{
	char *result = NULL;
	
	if (pPath == NULL)
		{
		result = (__getDir(0));	// CWD for the default drive
		}

	else if (QueryUNCPrefix(pPath))
		{
		result = NULL;					// UNC path cannot have a CWD
		}

	else if (QueryDrivePrefix(pPath))
		{
		int drive = DriveIndex(*pPath);
		result = __getDir(drive);	// CWD for the referenced path
		}
	else
		{
		result = (__getDir(0));	// CWD for the default drive
		}
		
#ifdef DEBUG
printf("_getdir  CWD path: \"%s\"\n", ((result) ? result : "<NULL>"));
#endif
	return (result);
	}

/* ----------------------------------------------------------------------- */
	char *				// Returns the pointer to an allocated CWD
getDir (				// Get the pointer to the found path, if any
	const char  *pPath)	// Ptr to the caller's reference path

	{
	char  *allocPath;
	
	if (_getDir(pPath))
		{
		allocPath = pathDup(temp);
		return (allocPath);
		}
		
	return (NULL);
	}

/* ----------------------------------------------------------------------- */
	int
MakePrefixNumber (		// Make patspec prefix from the drive number
	int    n,			// The drive number
	char  *pDst)		// The callers detination buffer

	{
	char  *pCWD = __getDir(n);
	
	if ((pDst == NULL)  ||  (pCWD == NULL))
		return (-1);

	pathCopy(pDst, pCWD, sizeof(prefix));	// Copy only the prefix part
	return (0);
	}

/* ----------------------------------------------------------------------- */
	int
MakePrefixLetter (		// Make patspec prefix from the drive lettter
	char   letter,		// The drive letter
	char  *pDst)		// The callers detination buffer

	{
	if (pDst == NULL)
		return (-1);

	prefix[0] = toupper(letter);
	pathCopy(pDst, prefix, sizeof(prefix));
	return (0);
	}

/* ----------------------------------------------------------------------- */
