/* ----------------------------------------------------------------------- *\
|
|	C-callable function to get the file size under either
|	MS-DOS or OS/2.
|
|			    Brian W Johnson
|			       26-May-90  Support for WIN32
|			       30-Sep-07  Support for 64 bit file size
|
|	Calling sequence:
|
|	return = hgetsize (
|                    int       fd,     The open unbuffered CRT file handle
|                    PUINT64  pSize)   Pointer to the returned size, or NULL
|
|	int  return;                   0 for success, or nonzero for failure
|
\* ----------------------------------------------------------------------- */

#include  <windows.h>
#include  <io.h>
#include  "fwild.h"

/* ----------------------------------------------------------------------- *\
|  hgetsize ()  -  Get the file size (via open file handle)
\* ----------------------------------------------------------------------- */
    int                     /* 0 for success, or nonzero for failure */
hgetsize (
    int            fd,      /* The open unbuffered CRT file handle */
    PUINT64        pSize)   /* Pointer to the returned size, or NULL */

    {
    HANDLE         hFile;   /* WIN32 file handle */
    LARGE_INTEGER  size;    /* The returned file size */


    if (((hFile = (HANDLE)(_get_osfhandle(fd))) == INVALID_HANDLE_VALUE)
    ||      (((size.LowPart = GetFileSize(hFile, &size.HighPart)) == INVALID_FILE_SIZE)
        &&  (GetLastError() != NO_ERROR)))
        {
        if (pSize)
            *pSize = 0;
        return (-1);
        }

    if (pSize)
        *pSize = size.QuadPart;
    return (0);
    }

/* ----------------------------------------------------------------------- */
