/* ----------------------------------------------------------------------- *\
|
|	C-callable function to get the UNIX file time/date under either
|	MS-DOS or OS/2.
|	Does not automatically correct for the FAT/NTFS difference.
|
|			    Brian W Johnson
|			       26-May-90
|			        4-Apr-00 NTFS vs FAT timestamp resolution fix
|
|	Calling sequence:
|
|	return = hgetfdt (int  fh)	The open file handle
|
|	    long  return;		The returned File TimeDate
|
\* ----------------------------------------------------------------------- */

#include  <sys\types.h>
#include  <sys\stat.h>

#include  "fwild.h"

/* ----------------------------------------------------------------------- *\
|  hgetfdt ()  -  Get the file time/date (via open file handle)
\* ----------------------------------------------------------------------- */
    long
hgetfdt (int  fh)			/* The open file handle */

    {
    long         result;		/* The returned time */
    struct stat  s;			/* The stat structure */


    if (fstat(fh, &s) == 0)
	result = ((s.st_mtime + 1) & ~1);  /* FAT vs NTFS resolution */
    else
	result = -1L;

    return (result);
    }

/* ----------------------------------------------------------------------- */
