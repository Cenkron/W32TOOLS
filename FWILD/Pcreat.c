/* ----------------------------------------------------------------------- *\
|
|				    PCREAT
|
|		    Copyright (c) 1985, 1990, all rights reserved
|				Brian W Johnson
|				   26-May-90
|				   17-Dec-94
|				   08-Oct-10 (corrections to looping)
|
|	    int
|	fd = pcreat (path, perm);	Build the path and create the file
|	    char  *path;		Path/filename
|	    int    perm;		Creat permission
|
|	    int    fd;			File number or error code
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

/* ----------------------------------------------------------------------- */
	int
pcreat (				/* Build path and creat() a file */
	char  *s,			/* Pointer to the path/filename */
	int    perm)		/* Create permission */

	{
	int    finished = 0;	/* Try once more after creating the directory string */
	int    fd;				/* File number or error code */
	char  *p;				/* Pointer into the temporary string */
	char   temp [1024];		/* Temporary path/filename string */


	strcpy(&temp[0], s);
	p = PointPastPrefix(temp, TRUE);	// Skip over any prefix, single mode
	for (;;)
		{
		if (((fd = creat(s, perm)) >= 0) || finished++)
			break;		// File successfully created

		// Failed; skip past a possible root separator,
		// and attempt to build the path, one directory at a time

		do  {
			if ((p = strpbrk((p + 1), "/\\")) != NULL)
				{
//printf("\npcreat: (%d) \"%s\"  \"%d\"\n", count, &temp[0], (p - &temp[0]));
				*p = '\0';							// Truncate the path
				if (( ! fnchkdir(&temp[0]))			// Accept existing directory
				&& ((fd = mkdir(&temp[0])) != 0))	// Make missing directory
					break;	// Path building complete
				}
			strcpy(&temp[0], s);	// Recopy to reverse the truncation
			} while (p); // do-while
		} // for

	return  (fd);
	}

/* ----------------------------------------------------------------------- */
