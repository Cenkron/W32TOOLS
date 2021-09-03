/* ----------------------------------------------------------------------- *\
|
|				   GETDRIVE
|
|		    Copyright (c) 1990, all rights reserved
|				Brian W Johnson
|				   26-May-90
|				   16-Aug-97
|
|	    int			Return the drive number (1 => A)
|	n = getdrive (void);	Get the drive number
|
\* ----------------------------------------------------------------------- */

#include  <direct.h>

#include  "fwild.h"

/* ----------------------------------------------------------------------- */
	int
getdrive (void)		/* Return the number of the current drive */

	{
	return (_getdrive());
	}

/* ----------------------------------------------------------------------- */
