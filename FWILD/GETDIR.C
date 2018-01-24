/* ----------------------------------------------------------------------- *\
|
|				    GETDIR
|
|		    Copyright (c) 1990, all rights reserved
|				Brian W Johnson
|				   26-May-90
|				   17-Dec-94
|				   16-Aug-97
|
|	    int			Return the length of the returned path
|	result = getdir (n, s);	Get the directory path for drive n
|	    int    n;		The drive number (0 => current)
|	    char  *s;		Pointer to the pathname buffer
|
\* ----------------------------------------------------------------------- */

#include  <stdlib.h>
#include  <string.h>
#include  <direct.h>

#include  "fwild.h"

#define  MAX_PATH_X  (1024)

/* ----------------------------------------------------------------------- */
    int
getdir (		/* Convert a filename to absolute format */
    int    drive,	/* The drive number (1 => A) */
    char  *s)		/* Pointer to the destination path string buffer */

    {
    _getdcwd(drive, s, (MAX_PATH_X + 1));

    return (strlen(s));
    }

/* ----------------------------------------------------------------------- */
