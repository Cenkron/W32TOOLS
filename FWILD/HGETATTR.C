/* ----------------------------------------------------------------------- *\
|
|	C-callable function to get the file attributes under either
|	MS-DOS or OS/2.
|
|			    Brian W Johnson
|			       26-May-90
|
|	Calling sequence:
|
|	return = hgetattr (int  fh)	The open file handle
|
|	    int  return;		The returned file attribute word
|					(or -1 for failure)
|
\* ----------------------------------------------------------------------- */

#include  <sys\types.h>
#include  <sys\stat.h>

#include  "fwild.h"

/* ----------------------------------------------------------------------- *\
|  hgetattr ()  -  Get the file attribute word (via open file handle)
\* ----------------------------------------------------------------------- */
	int
hgetattr (
	int  fh)				/* The open file handle */

	{
	int          result;	/* The returned file mode */
	struct stat  s;			/* The stat structure */


	if (fstat(fh, &s) == 0)
		result = s.st_mode;
	else
		result = -1;

	return (result);
	}

/* ----------------------------------------------------------------------- */
