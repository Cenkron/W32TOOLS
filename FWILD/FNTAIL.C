/* ----------------------------------------------------------------------- *\
|
|						    FNTAIL
|
|				    Copyright (c) 1985, 1990, all rights reserved
|						Brian W Johnson
|						   26-May-90
|						   26-Sep-23 (added _fntail())
|
|	    char *				Return pointer to the tail of pPath
|	_fntail (				Point the tail of the path
|	    const char *pPath)	Pointer to the pathname (after the prefix)
|
|	    char *				Return pointer to the tail of pPath
|	fntail (				Point the tail of the path
|	    const char *pPath)	Pointer to the pathname
|
|	Returns a pointer to the last component of pathspec pPath
|	The path delimiter is not returned as part of the string.
|
|	Normally _fntail() is called with a post prefix filespec.
|	If there is no separate last component, _fntail() points the original string
|
|	Normally fntail() is called with a raw filespec.
|	If there is no separate last component,  fntail() points past the prefix, if any
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <string.h>

#include  "fWild.h"

/* ----------------------------------------------------------------------- */

#define NULCH	('\0')
#define PATHCH1	('\\')
#define PATHCH2	('/')

/* ----------------------------------------------------------------------- */
	char *				// Return pointer to the last component of path pPath
_fntail (
	const char  *pPath)	// Pointer to the path (past the prefix)

	{
	const char  *pTail = pPath;		// Pointer to the found tail, if any
		  char   ch;

	// Find the last path separator, and remember the following path element

	for (char *p = (char *)(pPath); ((ch = *p) != NULCH); ++p)
		{
		if ((ch == PATHCH1) || (ch == PATHCH2))
			pTail = (p+1);
		}
								
	return ((char *)(pTail));
	}

/* ----------------------------------------------------------------------- */
	char *				// Return pointer to the last component of the path
fntail (					
	const char  *pPath)	// Pointer to the path string

	{
	return ((char *)(_fntail(PointPastPrefix(pPath))));
	}

/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */
