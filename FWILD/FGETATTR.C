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
|							(or -1 for failure)
|
\* ----------------------------------------------------------------------- */

#include  <windows.h>

#define  FWILD_INTERNAL

#include  "fWild.h"

/* ----------------------------------------------------------------------- *\
|  fgetattr ()  -  Get the file attribute word (via file pathname)
\* ----------------------------------------------------------------------- */
	int
fgetattr (
	const char *fnp)			/* Pointer to the path/filename */

	{
	int    result;			/* The returned result */
	DWORD  Attr;			/* The WIN32 attributes */

	Attr = GetFileAttributesA(fnp);
	if (Attr == INVALID_FILE_ATTRIBUTES)
		result = -1;
	else
		result = AttrFromWin32(Attr);

	return (result);
	}

/* ----------------------------------------------------------------------- */
