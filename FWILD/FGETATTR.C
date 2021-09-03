/* ----------------------------------------------------------------------- *\
|
|	C-callable functions to get the file attributes under WIN32
|
|			    Brian W Johnson
|			       26-May-90
|			       16-Aug-97
|
|	Calling sequences:
|
|	return = fgetattr (char *fnp)	Pointer to the path/filename
|
|	    int  return;		The returned file attribute word
|					(or -1 for failure)
|
\* ----------------------------------------------------------------------- */

#include  <windows.h>

#define  FWILD_INTERNAL

#include  "fwild.h"

/* ----------------------------------------------------------------------- *\
|  fgetattr ()  -  Get the file attribute word (via file pathname)
\* ----------------------------------------------------------------------- */
	int
fgetattr (
	char *fnp)			/* Pointer to the path/filename */

	{
	int    result;			/* The returned result */
	DWORD  Attr;			/* The WIN32 attributes */

	Attr = GetFileAttributes(fnp);
	if (Attr == 0xFFFFFFFF)
		result = -1;
	else
		result = AttrFromWin32(Attr);

	return (result);
	}

/* ----------------------------------------------------------------------- */
