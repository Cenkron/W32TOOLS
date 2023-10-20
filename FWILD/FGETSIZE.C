/* ----------------------------------------------------------------------- *\
|
|	C-callable function to get the file size under either
|	MS-DOS or OS/2.
|
|			    Brian W Johnson
|					26-May-90  Support for WIN32
|					30-Sep-07  Support for 64 bit file size
|					21-Oct-23  New version of Fwild
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
#include  <fileapi.h>

#include  "fWild.h"

// --------------------------------------------------------------------------
//	#define DEBUG		// Define to include debug output

#ifdef DEBUG
#include  <stdio.h>
#endif

#define ATT_BASE	(ATT_DIR | ATT_HIDDEN | ATT_SYSTEM)

/* ----------------------------------------------------------------------- *\
|  fgetsize ()  -  Get the file size (via filename)
\* ----------------------------------------------------------------------- */
	int					/* 0 for success, or nonzero for failure */
fgetsize (
	const char  *pName, // Pointer to the path/filename
	PUINT64		 pSize) // Pointer to the returned size, or NULL

	{
	PFI		pFi;
	UINT64	size = 0;		// The returned size
	int		result;			// The returned result
	
	if ((pName == NULL) || ((pFi = FileInfoOpen(FW_ALL, pName)) == NULL))
		{
#ifdef DEBUG
printf("fgetsize failed \"%s\"\n", pName);
#endif
		size   = 0;
		result = (-1);		// Failed		
		}
	else
		{
		size = FileInfoSize(pFi);
		FileInfoClose(pFi);
#ifdef DEBUG
printf("fgetsize suceeded [%lld] \"%s\"\n", size, pName);		// if (pSize)
#endif
		result = (0);		// Succeeded
		}

	if (pSize)
		*pSize = size;
	return (result);
	}

/* ----------------------------------------------------------------------- */
