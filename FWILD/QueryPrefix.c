/* ----------------------------------------------------------------------- *\
|
|					    QueryDrivePrefixMulti
|					    QueryDrivePrefix
|						QueryUNCPrefix
|						PointPastPrefix
|
|			    Copyright (c) 2022 = 2023, all rights reserved
|						Brian W Johnson
|						   20-Sep-22
|						   30-Sep-23 (Major update)
|
|
|		static char *		Returns ptr past the found prefix,
|	QueryDrivePrefixMulti (	  or NULL if no prefix found (for boolean usage)
|	    char  *s)			Pointer to the pathname string
|
|		static char *		Returns ptr past the found prefix,
|	QueryDrivePrefix (		  or NULL if no prefix found (for boolean usage)
|	    char  *s)			Pointer to the pathname string
|
|		static char *		Returns ptr past the found prefix,
|	QueryUNCPrefix (		  or NULL if no prefix found (for boolean usage)
|	    char  *s)			Pointer to the pathname string
|
|		static char *		Returns ptr past the found prefix,
|	PointPastPrefix (		  or the original pointer, if no prefix found
|	    char  *s)			Pointer to the pathname string
|
|	In each case, the function checks for a path spec prefix (drive or UNC)
|	and returns a pointer to the first character following the found prefix.
|
|	If the prefix spec is not found, a NULL pointer is returned.
|
|	In the case of a drive spec, the returned pointer points past the root separator,
|	if present.  The character pointed will not be a separator in a valid path.
|
|	In the case of a UNC spec, the returned pointer points past the UNC prefix,
|	to the first element of the path.  A UNC spec is always treated as rooted,
|	as it cannot be a relative path.
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <ctype.h>
#include  <string.h>

#include  "fWild.h"

#define	  NULCH			('\0')
#define	  COLCH			(':')

#define   isPath(ch)	(((ch) == '/') || ((ch) == '\\'))

// ------------------------------------------------------------------------------------------------

int rooted = FALSE;		// "rooted" property of the most recent call to PointPastPrefix()

// ------------------------------------------------------------------------------------------------
// Skip over the drive prefix, if any (accepts multi-drive prefixes)
// ------------------------------------------------------------------------------------------------
#if 0 // Probably will never be used
	char *					// Returns ptr past the found prefix, or NULL if no prefix found
QueryDrivePrefixMulti (		// Skips over the drive spec, if found
	const char  *s)			// Pointer to the pathname string

	{
	const char *pScan = s;	// Scanning pointer

	rooted = FALSE;					// unrooted until determined true
	for (;;)
		{
		if (isalpha(*pScan))
			++pScan;						// Found a drive letter, skip over it

		else if (*pScan == COLCH)
			return ((char *)(++pScan));		// ':'found, return the pointer to the path root
// root
		else // Not part of a drive spec
			break;
		}

	return (NULL);							// Did not find a drive prefix
	}
#endif
// ------------------------------------------------------------------------------------------------
// Skip over the UNC prefix, if any
// ------------------------------------------------------------------------------------------------
	char *				// Returns ptr past the found prefix, or NULL if no prefix found
QueryUNCPrefix (		// Skips over the UNC spec, if found
	const char  *s)		// Pointer to the pathname string

	{
	const char *pScan = s;	// Scanning pointer

	rooted = FALSE;					// unrooted until determined true
	if (! isPath(*s))
		return (NULL);				// Not a UNC path, something else

	if (! isPath(*(s+1)))
		return (NULL);				// Not a UNC path, just rooted

	pScan = s+2;					// Skip over the first two, already found
	int countdown = 2;				// Need two more path chars for a UNC path
	while (*pScan)
		{
		if (isPath(*pScan))
			{
			if (--countdown == 0)
				{
				rooted = TRUE;				// UNC is rooted by definition
				return ((char *)(++pScan));	// UNC path found; point just past the UNC
				}
			}
		else if ( ! isValidUNC(*pScan))
			return (NULL);			// Invalid UNC char found
		++pScan;					// Continue scanning
		}

	return (NULL);					// Did not find a UNC prefix
	}

// ------------------------------------------------------------------------------------------------
// Skip over the drive prefix, if any (accepts multi-drive prefixes)
// ------------------------------------------------------------------------------------------------
	char *					// Returns ptr past the found prefix, or NULL if no prefix found
QueryDrivePrefix (			// Skips over the drive spec, if found
	const char  *s)			// Pointer to the pathname string

	{
	rooted = FALSE;					// unrooted until determined true
	if ((isalpha(*s++))
	&&  (*s++ == ':'))
		{
		if (isPath(*s))			// If there is a root separator,
			{
			rooted = TRUE;
			++s;				//   skip over it
			}
		return ((char *)(s));	// Drive prefix found
		}
		
	return (NULL);				// Did not find a drive prefix
	}

// ------------------------------------------------------------------------------------------------
// Skip over the drive prefix, if any (accepts multi-drive prefixes)
// ------------------------------------------------------------------------------------------------
	char *					// Returns ptr past the found root separator, if found, or NULL if no root separator found
QueryRootPrefix (			// Skips over the drive spec, if found
	const char  *s)			// Pointer to the pathname string

	{
	if (rooted = isPath(*s))
		return ((char *)(s+1));	// Root separator found
		
	return (NULL);				// Did not find a drive prefix
	}

// ------------------------------------------------------------------------------------------------
// Skip over the UNC or DRIVE prefix, if any
// ------------------------------------------------------------------------------------------------
	char *					// Returns ptr past the found prefix, or unchanged if no prefix found
PointPastPrefix (			// Skips over the UNC or drive spec, if found
	const char  *pPath)			// Pointer to the pathname string

	{
	char *pResult;		// Returned pointer

	if ((pResult = QueryUNCPrefix(pPath)) != NULL)		// Skips over the UNC spec, if found
		{
		rooted = (TRUE); // by definition
		return (pResult);
		}

	if ((pResult = QueryDrivePrefix(pPath)) != NULL)	// Skips over the DRIVE spec, if found
		{
		rooted = isPath(*pResult);			
		return (pResult);
		}

	if ((pResult = QueryRootPrefix(pPath)) != NULL)		// Skips over the root separator, if found
		return (pResult);

	return ((char *)(pPath));							// No prefix found
	}

// ------------------------------------------------------------------------------------------------
// Skip over the UNC or DRIVE prefix, if any
// ------------------------------------------------------------------------------------------------
	int					// Returns TRUE iff the path has a prefix
isPrefixed (			// Check for a prefix
const char *pPath)		// Pointer to the pathname string

	{
	return ((QueryDrivePrefix(pPath) != NULL)
		||  (QueryUNCPrefix(pPath) != NULL));
	}

// ------------------------------------------------------------------------------------------------
// Skip over the UNC or DRIVE prefix, if any
// ------------------------------------------------------------------------------------------------
	int					// Returns TRUE iff the most recent PointPastPrefix() call found it rooted
isRooted (void)			// Check the root status of the most recent PointPastPrefix() call

	{
	return (rooted);
	}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
