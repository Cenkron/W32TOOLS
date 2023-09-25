/* ----------------------------------------------------------------------- *\
|
|					    PCREAT
|
|			    Copyright (c) 1985, 1990, all rights reserved
|					Brian W Johnson
|					   26-May-90
|					   17-Dec-94
|					   08-Oct-10 (corrections to looping)
|					   23-Sep-23 (consistency update)
|
|	    int					File number or error code
|	fd = pcreat (			Build the path and create the file
|	    const char *path,	Path/filename
|	    int    perm)		Creat permission
|
|	pcreat() is equivalent to creat() except it will build the path also.
|
|	path is the path/filename of a file to be created for writing.
|	pcreat() attempts to create the file.  If that fails, then the
|	path is checked and created as necessary.  One last attempt is
|	made to create the file.
|
|	pcreat() returns the open file descriptor, or (-1) for failure.
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <string.h>
#include  <stdlib.h>
#include  <io.h>
#include  <direct.h>

#include  "fwild.h"

// ---------------------------------------------------------------------------

//#define DEBUG	// Define this for debug output

// ---------------------------------------------------------------------------
	int
pcreat (					// Build path and creat() a file
	const char  *pFileSpec,	// Pointer to the path/filename
	int			 perm)		// Create permission

	{
	int  finished = FALSE;	// Finished flag (support retry after build)
	int  fd = -1;			// File number or error code (-1)


	for (;;)
		{
		if (((fd = _creat(pFileSpec, perm)) >= 0) || finished++)
			break;			// File successfully opened

#ifdef DEBUG
printf("\npcreat retry: \"%s\"\n", pFileSpec);
#endif

		// Create failed; Attempt to build the path

		if ((fd = fnBuildPath(pFileSpec) != 0))
			break;
		}

#ifdef DEBUG
printf("\npcreat: %d \"%s\"\n", fd, pFileSpec);
#endif

	return  (fd);
	}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
