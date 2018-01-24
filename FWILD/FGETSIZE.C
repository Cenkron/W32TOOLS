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
|	return = fgetsize (
|                    char    *pName,   Pointer to the path/filename
|                    PUINT64  pSize)   Pointer to the returned size, or NULL
|
|	int  return;                   0 for success, or nonzero for failure
|
\* ----------------------------------------------------------------------- */

#include  <windows.h>
#include  "fwild.h"

/* ----------------------------------------------------------------------- *\
|  fgetsize ()  -  Get the file size (via filename)
\* ----------------------------------------------------------------------- */
    int                     /* 0 for success, or nonzero for failure */
fgetsize (
    char            *pName, /* Pointer to the path/filename */
    PUINT64          pSize) /* Pointer to the returned size, or NULL */

    {
    HANDLE           hFind; /* WIN32 file handle */
    WIN32_FIND_DATA  wfd;   /* The Windows file information */


    if ((hFind = FindFirstFile(pName, &wfd)) == INVALID_HANDLE_VALUE)
        {
        if (pSize)
            *pSize = 0;
        return (-1);
        }

    if (pSize)
        *pSize = (((UINT64)(wfd.nFileSizeHigh)) << 32)
               |  ((UINT64)(wfd.nFileSizeLow));

    FindClose(hFind);
    return (0);
    }

/* ----------------------------------------------------------------------- */
