/* ----------------------------------------------------------------------- *\
|
|					     POPEN
|
|			    Copyright (c) 1985, 1990, all rights reserved
|					Brian W Johnson
|					   26-May-90
|					   17-Dec-94
|					   08-Oct-10 (corrections to looping)
|					   23-Sep-23 (consistency update)
|
|	    int						File number or error code
|	fd = popen (				Build the path and open the file
|		const char *FileSpec,	Path/filename
|	    int    mode,			File open mode
|	    int    perm)			Create permission (O_CREAT mode)
|
|	popen() is equivalent to open() except it will build the path also.
|
|	path is the path/filename of a file to be opened for writing.
|	popen() attempts to open the file.  If that fails, then the path
|	is checked and created as necessary.  One last attempt is made
|	to open the file.
|
|	popen() returns the open file descriptor, or (-1) for failure.
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <string.h>
#include  <stdlib.h>
#include  <fcntl.h>
#include  <direct.h>
#include  <io.h>

#include  "fwild.h"

// ---------------------------------------------------------------------------

//#define DEBUG	// Define this for debug output

// ---------------------------------------------------------------------------
	int						// Return file descriptor if successful, else (-1)
popen (						// Build path and open() a file
	const char  *pFileSpec,	// Pointer to the path/filename
	int			 mode,		// Open mode
	int			 perm)		// Create permission

	{
	int  finished = FALSE;	// Finished flag (support retry after build)
	int  writeReq;			// TRUE if write requested
	int  fd;				// File number or error code (-1)


	// Determine whether this is a write request

	writeReq = ((mode & (O_CREAT | O_WRONLY | O_RDWR)) != 0);

	// Make a temporary copy of the pathname string

	for (;;)
		{
		if (((fd = _open(pFileSpec, mode, perm)) >= 0) || finished++)
			break;			// File successfully opened

		if ( ! writeReq)
			break;			// Only wanted to read, and file was not found

		// Create failed; Attempt to build the path

		if ((fd = fnBuildPath(pFileSpec) != 0))
			break;

#ifdef DEBUG
printf("\npopen: \"%s\"\n", pTemp));
#endif

		}

	return  (fd);
	}

// ---------------------------------------------------------------------------
