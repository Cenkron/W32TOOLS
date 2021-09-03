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
	int    finished = 0;	/* Finished flag */
	int    fd;				/* File number or error code */
	int    SkipCount;		/* Ignore counter for the do loop */
	char  *p;				/* Pointer into the temporary string */
	char   temp [1024];		/* Temporary path/filename string */


				/* Allow for possible UNC path */
	SkipCount = (strncmp("\\\\", s, 2) == 0) ? (2) : (0);

	for (;;)
		{
		if (((fd = creat(s, perm)) >= 0) || finished++)
			break;		// File successfully opened

		p = &temp[0];		// Attempt to build the path
		do  {
			strcpy(&temp[0], s);
			if ((p = strpbrk((p + 1), "/\\"))
			&& (--SkipCount < 0))	/* Skip over the UNC part of the path */
				{
//printf("\npcreat: (%d) \"%s\"  \"%d\"\n", SkipCount, &temp[0], (p - &temp[0]));
//printf("\npcreat: (%d) \"%s\"  \"%d\"\n", SkipCount, p, (p - &temp[0]));
				*p = '\0';
				if ((!fnchkdir(&temp[0]))
				&& ((fd = mkdir(&temp[0])) != 0))
					{
//printf("\nmkdir: (%d) \"%s\"\n", fd, &temp[0]);
					break;	// Path building complete
					}
				}
			} while (p);

		}
		return  (fd);
	}

/* ----------------------------------------------------------------------- */
