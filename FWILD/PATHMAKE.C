/* ----------------------------------------------------------------------- *\
|
|				    PATHMAKE
|
|		    Copyright (c) 1985, 1990 all rights reserved
|				Brian W Johnson
|				   26-May-90
|				   17-Dec-94
|
|	    int
|	result = pathmake(path);	Build the path (if necessary)
|	    char  *path;		Pathname
|
|	    int    result;		0 if successful
|
|	path is the pathname of a directory.
|	pathmake() attempts to create the path if it does not exist.
|
|	pathmake() returns (0) for success, or (-1) for failure.
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <string.h>
#include  <stdlib.h>
#include  <io.h>
#include  <direct.h>

#include  "fwild.h"

#define  PATHLIMIT	1024

/* ----------------------------------------------------------------------- */
    int				/* Build path and open() a file */
pathmake (char *s)		/* Pointer to the directory pathname */

    {
    int  result = 0;		/* Returned result */
    char  *p;			/* Pointer into the temporary string */
    char  temp [PATHLIMIT + 1];	/* Temporary path/filename string */


    p = &temp[0];		/* Attempt to build the path */
    do  {
	strncpy(&temp[0], s, PATHLIMIT);
	if (p = strpbrk((p + 1), "/\\"))
	    {
	    *p = '\0';
	    if (( ! fnchkdir(&temp[0]))
	    && ((result = mkdir(&temp[0])) != 0))
		break;
	    }
	} while (p);

    return  (result);
    }

/* ----------------------------------------------------------------------- */
