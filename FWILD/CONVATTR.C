/* ----------------------------------------------------------------------- *\
|
|	C-callable functions to convert file attributes under WIN32
|
|			    Brian W Johnson
|			       16-Aug-97
|
\* ----------------------------------------------------------------------- */

#include  <windows.h>

#define  FWILD_INTERNAL

#include  "fwild.h"

/* ----------------------------------------------------------------------- *\
|  AttrFromWin32 () - Convert from WIN32 file attributes
\* ----------------------------------------------------------------------- */
	int						/* Returns the FWILD file attributes */
AttrFromWin32 (
	DWORD  Win32Attr)		/* The WIN32 file attribute bitmap */	

	{
	int    NewAttr = 0;		/* The returned FWILD file attribute bitmap */


	if (Win32Attr & FILE_ATTRIBUTE_READONLY)
		NewAttr |= ATT_RONLY;
	if (Win32Attr & FILE_ATTRIBUTE_HIDDEN)
		NewAttr |= ATT_HIDDEN;
	if (Win32Attr & FILE_ATTRIBUTE_SYSTEM)
		NewAttr |= ATT_SYSTEM;
	if (Win32Attr & FILE_ATTRIBUTE_DIRECTORY)
		NewAttr |= ATT_SUBD;
	if (Win32Attr & FILE_ATTRIBUTE_ARCHIVE)
		NewAttr |= ATT_ARCH;
	return (NewAttr);
	}

/* ----------------------------------------------------------------------- *\
|  AttrToWin32 () - Convert to WIN32 file attributes
\* ----------------------------------------------------------------------- */
	DWORD					/* Returns the WIN32 attributes */
AttrToWin32 (
	int    FwildAttr)		/* The FWILD file attribute bitmap */

	{
	DWORD  NewAttr = 0;		/* The returned WIN32 file attribute bitmap */


	if (FwildAttr & ATT_RONLY)
		NewAttr |= FILE_ATTRIBUTE_READONLY;
	if (FwildAttr & ATT_HIDDEN)
		NewAttr |= FILE_ATTRIBUTE_HIDDEN;
	if (FwildAttr & ATT_SYSTEM)
		NewAttr |= FILE_ATTRIBUTE_SYSTEM;
	if (FwildAttr & ATT_SUBD)
		NewAttr |= FILE_ATTRIBUTE_DIRECTORY;
	if (FwildAttr & ATT_ARCH)
		NewAttr |= FILE_ATTRIBUTE_ARCHIVE;

	if (NewAttr == 0)
		NewAttr = FILE_ATTRIBUTE_NORMAL;

	return (NewAttr);
	}

/* ----------------------------------------------------------------------- */
