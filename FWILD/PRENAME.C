/* ----------------------------------------------------------------------- *\
|
|				    PRENAME
|
|		    Copyright (c) 1985, 1990, all rights reserved
|				Brian W Johnson
|				   26-May-90
|				   17-Dec-94
|
|	    int
|	result = prename (path1, path2); Build the path and rename the file
|	    char  *path1;		Old path/filename
|	    char  *path2;		New path/filename
|
|	    int    result;		Returned result code
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

#include  "fwild.h"

/* ----------------------------------------------------------------------- */
	int
prename (				/* Build path and rename() a file */
	char  *path1,		/* Pointer to the old path/filename */
	char  *path2)		/* Pointer to the new path/filename */

	{
	int  finished = 0;		/* Finished flag */
	int  result;			/* File number or error code */
	char  *p;				/* Pointer into the temporary string */
	char  temp [1024];		/* Temporary path/filename string */


	for (;;)
		{
		if (((result = rename(path1, path2)) == 0) || finished++)
			break;

		p = &temp[0];		/* Attempt to build the path */
		do  {
			strcpy(&temp[0], path2);
			if (p = strpbrk((p + 1), "/\\"))
				{
				*p = '\0';
				if ( ! fnchkdir(&temp[0]))
					{
					if ((result = mkdir(&temp[0])) != 0)
						break;
					}
				}
			} while (p);

		}
	return  (result);
	}

/* ----------------------------------------------------------------------- */
