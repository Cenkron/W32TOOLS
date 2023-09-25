/* ----------------------------------------------------------------------- *\
|
|					    QueryDrivePrefix
|						QueryUNCPrefix
|
|			    Copyright (c) 2022, all rights reserved
|						Brian W Johnson
|						   20-Sep-22
|
|
|		static char *		// Returns ptr past the found prefix, or NULL if no prefix found
|	QueryDrivePrefix (		// Skips over the drive spec, if found
|	    char  *s)			// Pointer to the pathname string
|
|		static char *		// Returns ptr past the found prefix, or NULL if no prefix found
|	QueryUNCPrefix (		// Skips over the UNC spec, if found
|	    char  *s)			// Pointer to the pathname string
|
|	In each case, the function checks for a path spec prefix (drive or UNC)
|	and returns a pointer to the first character following the found prefix.
|
|	If the prefix spec is not found, a NULL pointer is returned.
|
|	In the case of a drive spec, the returned pointer points the root separator,
|	if present, else it points the first character of the first path element,
|	(depending on whether the path is absolute or relative).
|
|	In the case of a UNC spec, the returned pointer points the UNC spec, to the first
|	element of the path.  A UNC spec is always treated as rooted, as it
|	cannot be a relative path.
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <ctype.h>
#include  <string.h>

#include  "fwild.h"

#define	  PATHCH		('\\')
#define	  NULCH			('\0')
#define	  COLCH			(':')

// ------------------------------------------------------------------------------------------------

int rooted = FALSE;		// "rooted" property of the most recent call to PointPastPrefix()

// ------------------------------------------------------------------------------------------------
// Skip over the UNC prefix, if any
// ------------------------------------------------------------------------------------------------
	char *				// Returns ptr past the found prefix, or NULL if no prefix found
QueryUNCPrefix (		// Skips over the UNC spec, if found
    char  *s)			// Pointer to the pathname string

	{
	char *pScan = s;	// Scanning pointer

	if (*(pScan++) == PATHCH)
		{
		if (*(pScan) != PATHCH)
			return (NULL);					// Not a UNC path, just rooted

		else // (*(pScan+1) == PATHCH)); signifying a UNC path
			{
			int countdown = 2;				// Need two more for a UNC path

			pScan += 2;						// Skip over the first two, already found
			while (*pScan)
				{
				if (*pScan == PATHCH)
					{
					if (--countdown == 0)
						return (++pScan);	// UNC path found; point just past the UNC
					}
				else if ( ! isValidUNC(*pScan))
					break;					// Invalid UNC char found
				++pScan;					// Continue scanning
				}
			}
		}

	return (NULL);							// Did not find a UNC prefix
	}

// ------------------------------------------------------------------------------------------------
// Skip over the drive prefix, if any (accepts multi-drive prefixes)
// ------------------------------------------------------------------------------------------------
	char *				// Returns ptr past the found prefix, or NULL if no prefix found
QueryDrivePrefix (		// Skips over the drive spec, if found
    char  *s,			// Pointer to the pathname string
	int    single)		// TRUE to limit acceptance to a single drive letter

	{
	char *pScan = s;	// Scanning pointer

	for (;;)
		{
		if (isalpha(*pScan))
			{
			if (single  &&  (pScan != s))
				return (NULL);
			++pScan;						// Found a drive letter, skip over it
			}
		else if (*pScan == COLCH)
			{
			++pScan;						// Found the ':'; point just past the drive spec
			return (pScan);					// Return the pointer to the path root
			}
		else // Not part of a drive spec
			break;
		}

	return (NULL);							// Did not find a drive prefix
	}

// ------------------------------------------------------------------------------------------------
// Skip over the UNC or DRIVE prefix, if any
// ------------------------------------------------------------------------------------------------
	char *				// Returns ptr past the found prefix, or NULL if no prefix found
PointPastPrefix (		// Skips over the UNC or drive spec, if found
    char  *s,			// Pointer to the pathname string
	int    single)		// TRUE to limit acceptance to a single drive letter

	{
	char *pResult;		// Returned pointer

	if ((pResult = QueryUNCPrefix(s)) != NULL)	// Skips over the UNC spec, if found
		{
		rooted = (TRUE); // by definition
		return (pResult);
		}

	if ((pResult = QueryDrivePrefix(s, single)) != NULL)	// Skips over the DRIVE spec, if found
		{
		rooted = (*pResult == PATHCH);			
		return (pResult);
		}

//	else
//		{
		rooted = (*s == PATHCH);
		return (s);							// Did not find any prefix, so nothing skipped
//		}
	}

// ------------------------------------------------------------------------------------------------
// Skip over the UNC or DRIVE prefix, if any
// ------------------------------------------------------------------------------------------------
	int					// Returns TRUE iff the most recent PointPastPrefix() call found it rooted
isRooted (void)			// Check ths root status of the most recent PointPastPrefix() call

	{
	return (rooted);
	}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
