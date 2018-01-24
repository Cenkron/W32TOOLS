/* ----------------------------------------------------------------------- *\
|
|				     PFOPEN
|
|		    Copyright (c) 2010 all rights reserved
|				Brian W Johnson
|				   08-Oct-2010
|
|	    FILE *
|	fp = pfopen (			Build the path and open the file
|       const char *pathname,		Path/filename
|       const char *mode)		Open mode flags
|
|	    FILE   *fp;			FILE pointer or NULL
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

/* ----------------------------------------------------------------------- */
    FILE *
pfopen (			/* Build path and fopen() a file */
    const char *pathname,	/* Pointer to the path/filename */
    const char *mode)		/* Open mode flags */

    {
    int    finished = 0;	/* Finished flag */
    FILE  *fp;			/* FILE pointer or NULL */
    int    SkipCount;		/* Ignore counter for the do loop */
    char  *p;			/* Pointer into the temporary string */
    int    state = FALSE;	/* Path building state */
    char   temp [1024];		/* Temporary path/filename string */


				/* Allow for possible UNC path */
    SkipCount = (strncmp("\\\\", pathname, 2) == 0) ? (2) : (0);

    for (;;)
	{
	char  *pm;
	if (((fp = fopen(pathname, mode)) != NULL)  ||  finished++)
	    break;		// File successfully opened

	for (pm = (char *)mode; (*pm != '\0'); ++pm)
	    {
	    char  c = tolower(*pm);

	    if ((state = ((c == 'w')  ||  (c == 'a')  ||  (c == '+'))) != FALSE)
		break;		// Writing requested, so try to build the path
	    }

	if ( ! state)
	    break;		// Only wanted to read, and file was not found

	p = &temp[0];		// Attempt to build the path
	do  {
	    strcpy(&temp[0], pathname);
	    if ((p = strpbrk((p + 1), "/\\"))
	    && (--SkipCount < 0))	/* Skip over the UNC part of the path */
		{
//printf("\npfopen: (%d) \"%s\"  \"%d\"\n", count, &temp[0], (p - &temp[0]));
		*p = '\0';
		if (( ! fnchkdir(&temp[0]))
		&& (mkdir(&temp[0]) != 0))
		    break;	// Path building complete
		}
	    } while (p);

	}
    return  (fp);
    }

/* ----------------------------------------------------------------------- */
