/* ----------------------------------------------------------------------- *\
|
|				    PATHMAKE
|
|		    Copyright (c) 1985, 1990 all rights reserved
|				Brian W Johnson
|				   26-May-90
|				   17-Dec-94
|				   23-Sep-23 (consistency update)
|
|	    int
|	result = pathmake(path);	Build the path (if necessary)
|	    char  *path;			Pathname
|
|	    int    result;			0 if successful
|
|	path is the pathname of a directory.
|	pathmake() attempts to create the path if it does not exist.
|
|	pathmake() returns (0) for success, or (-1) for failure.
|
|	This function has been deprecated and is removed from the build.
|	It is replaced by the new function fnBuildPath().
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <string.h>
#include  <stdlib.h>
#include  <io.h>
#include  <direct.h>

#include  "fwild.h"

/* ----------------------------------------------------------------------- */

#define PATHCH	('\\')

/* ----------------------------------------------------------------------- */
	int						// returns TRUE if successful, else FALSE
pathmake (
	const char *pathname)	/* Pointer to the pathspec */

	{
	int	   fail = FALSE;	// TRUE if failed to verify or build
	char  *p;				/* Pointer into the temporary string */
	char  *pTemp;			/* Pointer to the temporary pathname string */


	// Make a temporary copy of the pathname string

	if ((pTemp = strdupMax(pathname)) == NULL)
		return (FALSE);

	strsetp(pTemp, PATHCH);				// Standardize the path characters
	p = PointPastPrefix(pTemp, TRUE);	// Skip over any prefix, single mode
	if (*p == PATHCH)					// Skip past any root separator
		++p;

	// For each path element,
	// verify that the directory exists, or try to create it

	if (*p != NULCH)					// If no path, don't bother 
		{
		do  {
			if ((p = strpbrk(p, "\\")) != NULL)	// Find the next path separator
				{
//printf("\npathmake: (%d) \"%s\"  \"%d\"\n", count, pTemp, (p - pTemp));
				if (p)
					*p = '\0';					// Truncate the path
				fail  = ((! fnchkdir(pTemp))	// Accept existing directory, or
					 &&  (mkdir(pTemp) != 0))	// Make missing directory
				if (p)
					*p = PATHCH;				// Replace the path separator
				}
			} while ((! fail) && (p != NULL); // do-while
		}

	if (pTemp)
		free(pTemp);
	return  (! fail);
	}

/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */

	do  {
		if ((p = strpbrk(p, "\\")) != NULL)	// Find the next path separator
			{
//printf("\npathmake: (%d) \"%s\"  \"%d\"\n", count, pTemp, (p - pTemp));
			if (p)
				*p = '\0';					// Truncate the path
			fail  = ((! fnchkdir(pTemp))	// Accept existing directory, or
				 &&  (mkdir(pTemp) != 0))	// Make missing directory
			if (p)
				*p = PATHCH;				// Replace the path separator
			}
		} while ((! fail) && (p != NULL); // do-while

	if (pTemp)
		free(pTemp);
	return  (result);
	}
