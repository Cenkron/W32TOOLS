/* ----------------------------------------------------------------------- *\
|
|  fwFirst.c
|
|  This function uses the fWild system to return a pointer to the first
|  filename found matching the supplied fWild pattern pathname.
|
|  The fWild system is gracefully terminated with all memory freed.
|
| - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
|
|	16-Jan-93	Copyright (c) 1993, Brian W. Johnson,
|			All rights reserved
|	17-Dec-94	Buffer increased
|
|	This file is deprecated, and is removed from the build.
|	It is replaced by the funcctions in Finder.c
|
\* ----------------------------------------------------------------------- */

#include  <windows.h>
#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>

#define FWILD_INTERNAL

#include  <fWild.h>

/* ----------------------------------------------------------------------- *\
|  Private variables
\* ----------------------------------------------------------------------- */

/* ----------------------------------------------------------------------- *\
|  fwFirst () - Find the first filename matching the supplied fWild string
\* ----------------------------------------------------------------------- */
	PFWH				// Return pointer to fWild instance
fwFirstOpen (
	int  fileTypeMask,	// Requested file type mask (FW_FILE, FW_DIR etc.)
	const char  *pName)	// Pointer to the (usually non-wild) search filename

	{
	PFWH	hp;  		// Pointer to the wild file data block
	char   *p;			// Pointer to the found file


	if ((hp = fwOpen()) == NULL)
		return (NULL);	// Report failure

	if (fwInit(hp, pName, fileTypeMask) != FWERR_NONE)	// Process the pattern
		return (NULL);	// Report failure

	if ((p = fWild(hp)) != NULL)
		return (hp);	// Report success

	hp = fwClose(hp);	// Failed, close and free the fWild instance
	return (NULL);		// Report failure
	}

/* ----------------------------------------------------------------------- *\
|  fwFirstName () - Return the name of the found file
\* ----------------------------------------------------------------------- */
	char *				// Return ptr to the found file, if any
fwFirstName (
	PFWH  hp)			// Pointer to the fWild instance

	{
	if (fwActive(hp))
		return (hp->file_name);

	return (NULL);	
	}

/* ----------------------------------------------------------------------- *\
|  fwFirstClose () - Find the first filename matching the supplied fWild string
\* ----------------------------------------------------------------------- */
	void				// Close the FindFirst instance
fwFirstClose (
	PFWH  hp)			// Pointer to the fWild instance

	{
	fwClose(hp);		// Close and free the fWild instance
	}

/* ----------------------------------------------------------------------- */
