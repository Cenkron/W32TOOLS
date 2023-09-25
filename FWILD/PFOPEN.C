/* ----------------------------------------------------------------------- *\
|
|					     PFOPEN
|
|			    Copyright (c) 2010 all rights reserved
|					Brian W Johnson
|					   08-Oct-2010
|					   23-Sep-23 (consistency update)
|
|	    FILE *					Returns the FILE pointer, or NULL if fail
|	fp = pfopen (				Build the path and open the file
|       const char *pathname,	Ptr to Path/filename
|       const char *fmode)		Open mode flags
|
|	pfopen() is equivalent to fopen() except it will build the path also.
|
|	pathname is the path/filename of a file to be opened for access.
|	pfopen() attempts to open the file.  If that fails, then the path
|	is checked and created as necessary.  One last attempt is made
|	to open the file.
|
|	pfopen() returns the open FILE pointer, or NULL for failure.
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <string.h>
#include  <stdlib.h>
#include  <direct.h>
#include  <ctype.h>

#include  "fwild.h"

// ---------------------------------------------------------------------------
	FILE *
pfopen (					// Build path and fopen() a file
	const char *pFileSpec,	// Pointer to the path/filename
	const char *fmode)		// Open mode flags

	{
	int    finished = FALSE;// Finished flag
	int    writeReq = FALSE;// TRUE if write requested
	FILE  *fp;				// FILE pointer or NULL


	// Determine whether this is a write request

	for (const char *pMode = (fmode); (*pMode != '\0'); ++pMode)
		{
		char ch = tolower(*pMode);

		if ((writeReq = ((ch == 'w')  ||  (ch == 'a')  ||  (ch == '+'))) != FALSE)
			break;
		}

	for (;;)
		{
		if (((fp = fopen(pFileSpec, fmode)) != NULL)  ||  finished++)
			break;			// File successfully opened

		if ( ! writeReq)
			break;			// Only wanted to read, and file was not found

		// Open failed; Attempt to build the path

		if (fnBuildPath(pFileSpec) != 0)
			{
//			fp = NULL;
			break;
			}
//printf("\npfopen: (%d) \"%s\"  \"%d\"\n", count, pTemp, (p - pTemp));
		}

	return  (fp);
	}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
