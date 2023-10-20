/* ----------------------------------------------------------------------- *\
|
|				    PRENAME
|
|		    Copyright (c) 1985, 1990, all rights reserved
|				Brian W Johnson
|				   26-May-90
|				   17-Dec-94
|				   23-Sep-23 (consistency update)
|
|		int							Success (0) or error code (-1)
|	prename (						Build the path and rename the file
|	    const char  *pFileSpec1,	Old path/filename
|	    const char  *pFileSpec2)	New path/filename
|
|	prename() is equivalent to rename() except it will build the path also.
|
|	path1 is the old path/filename of a file to be renamed (moved).
|	path2 is the new path/filename of the file.
|	prename() attempts to rename the file.  If that fails, then the
|	new path is checked and created as necessary.  One last attempt is
|	made to rename the file.
|
|	prename() returns (0) for success, or (-1) for failure.
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <string.h>
#include  <stdlib.h>
#include  <io.h>
#include  <direct.h>

#include  "fWild.h"

// ---------------------------------------------------------------------------
//	#define DEBUG	// Define this for debug output

/* ----------------------------------------------------------------------- */
	int
prename (						// Build path and rename() a file
	const char  *pFileSpec1,	// Pointer to the old path/filename
	const char  *pFileSpec2)	// Pointer to the new path/filename

	{
	int  finished = FALSE;		// Finished flag (support retry after build)
	int  result;				// File number or error code


	for (;;)
		{
		if (((result = rename(pFileSpec1, pFileSpec2)) == 0) || finished++)
			break;

		// Rename failed; Attempt to build pFileSpec2

		if ((result = fnBuildPath(pFileSpec2) != 0))
			break;
		}

	return  (result);
	}

/* ----------------------------------------------------------------------- */
