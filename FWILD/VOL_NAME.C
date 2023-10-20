/* ----------------------------------------------------------------------- *\
|
|						volName
|					(includes a test program)
|
|				Copyright (c) 1985, 1990, all rights reserved
|					Brian W Johnson
|					    2-Jun-90
|					   28-Jan-93
|					   16-Aug-97
|					   26-Oct-23 (updated)
|
|		char *		Return the volume name of the drive
|	_volName (		Get the volume name
|		int n);		Drive number (or 0 for default drive)
|
|		char *		Return the volume name of the disk
|	volName (		Get the volume name
|		char  *s);	Pointer to the pathspec
|
\* ------------------------------------------------------------------------ */

#include  <windows.h>
#include  <stdio.h>
#include  <ctype.h>
#include  <string.h>

#include  "fWild.h"

/* ------------------------------------------------------------------------ */
//	#define  TEST
	
static char  volname  [64];			/* The recovered volume name */
static char  rootname [] = "X:\\";	/* The constructed root directory name */

/* ------------------------------------------------------------------------ *\
|  Return the volume name of the disk, based on drive number
\* ------------------------------------------------------------------------ */
	char *		// Returns found volume name, or NULL if not found
_volName (
	int n)		// The drive number (0 for current drive)

	{
	DWORD    dummy;

	if ((n < 0)  ||  (n > 26))
		return (NULL);

	rootname[0] = 'A' + n - 1;
	if (GetVolumeInformation(
			((n > 0) ? rootname : NULL),	// If n ==0, get default drive info
			volname,
			(int)(sizeof(volname)),
			&dummy,
			&dummy,
			&dummy,
			NULL,
			0) != 0)
		return (volname);

	return (NULL);
	}

/* ------------------------------------------------------------------------ *\
|  Return the volume name of the disk, based on pathname
\* ------------------------------------------------------------------------ */
	char *		// Returns found volume name, or NULL if not found
volName (
	const char  *pPath)	/* Path to be checked */

	{
	int  n = 0;			// The drive number (0 for current drive)

	if (QueryUNCPrefix(pPath))
		return ("<network>");

	if (QueryDrivePrefix(pPath))
		{
		if (isPhysical(pPath))
			n = toupper(pPath[0]) - ('A' - 1);	// Valid drive specified
		else
			return (NULL);
		}

	return (_volName(n));
	}

/* ------------------------------------------------------------------------ */
#ifdef  TEST
	void
main (
    int    argc,
    char  *argv [])

	{
	char  *p;
	char  *s;

	if (argc >= 2)
		p = argv[1];
	else
		p = "\\*";
	printf("Scan string = %s\n", p);

	if (isdigit(*p))
		s = _volName(atoi(p));
	else
		s = volName(p);
	printf("Pointer = %04x\n", s);
	if (s)
		printf("Volume name = %s\n", s);
	}

#endif
/* ------------------------------------------------------------------------ */
