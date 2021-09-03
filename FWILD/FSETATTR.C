/* ----------------------------------------------------------------------- *\
|
|	C-callable function to set the file attributes under WIN32
|
|			    Brian W Johnson
|			       26-May-90
|			       16-Aug-97
|
|	Calling sequence:
|
|	return = fsetattr (
|	    char  *fnp,		Pointer to the path/filename
|	    int    attr)	The new file attribute word
|
|	    int    result;	0 for success, -1 for failure	
|
\* ----------------------------------------------------------------------- */

#include  <windows.h>

#define  FWILD_INTERNAL

#include  "fwild.h"

/* ----------------------------------------------------------------------- *\
|  fsetattr ()  -  Set the file attributes (via filename)
\* ----------------------------------------------------------------------- */
	int
fsetattr (
	char  *fnp,					/* Pointer to the path/filename */
	int    FwildAttr)			/* The new file attributes */

	{
	int    result;			/* The success (0) fail (-1) result */
	DWORD  Attr;			/* The WIN32 attributes */

	Attr   = AttrToWin32(FwildAttr & (ATT_ARCH | ATT_HIDDEN | ATT_SYSTEM | ATT_RONLY));
	result = (SetFileAttributes(fnp, Attr)) ? 0 : -1;

	return (result);
	}

/* ----------------------------------------------------------------------- */
