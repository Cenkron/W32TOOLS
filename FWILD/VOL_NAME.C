/* ----------------------------------------------------------------------- *\
|
|				   VOL_NAME
|			   (includes a test program)
|
|		    Copyright (c) 1985, 1990, all rights reserved
|				Brian W Johnson
|				    2-Jun-90
|				   28-Jan-93
|				   16-Aug-97
|
|	    char *		Return the volume name of the disk
|	p = vol_name (s);	Drive listed in this string (or *.*)
|	    char  *s;		Pointer to the drive/pathname
|
|	Returns NULL if no volume name
|
|	    char *		Return the volume name of the disk
|	p = _volname (n);	
|	    int  n;		Drive number (0 => current, 1 => A)
|
|	Returns NULL if no volume name
|
\* ------------------------------------------------------------------------ */

#include  <windows.h>
#include  <stdio.h>
#include  <ctype.h>
#include  <string.h>

#include  "fwild.h"

// #define  VOLNAMETEST
	
/* ------------------------------------------------------------------------ *\
|  Return the volume name of the disk, based on drive number
\* ------------------------------------------------------------------------ */
	char *
_volname (
	int n)		/* The drive number (0 for current drive) */

	{
static char  volname  [MAX_PATH];	/* The recovered volume name */
static char  rootname [] = "X:\\";	/* The constructed root directory name */

	DWORD    dummy;
	char    *s;				/* The returned ptr to the volume name */

	if ((n < 0)  ||  (n > 26))
		return (NULL);

	rootname[0] = 'A' + n - 1;
	if (GetVolumeInformation(
			(n == 0) ? NULL : rootname,
			volname,
			MAX_PATH,
			&dummy,
			&dummy,
			&dummy,
			NULL,
			0))
		s = volname;
	else
		s = NULL;

	return (s);
	}

/* ------------------------------------------------------------------------ *\
|  Return the volume name of the disk, based on pathname
\* ------------------------------------------------------------------------ */
	char *
vol_name (
	char *s)		/* Drive listed in this string (or *.*) */

	{
	int  n;			/* The drive number (0 for current drive) */


	if (isalpha(s[0])  &&  (s[1] == ':'))
		n = toupper(s[0]) - ('A' - 1);
	else
		n = 0;

	return (_volname(n));
	}

/* ------------------------------------------------------------------------ */
#ifdef  VOLNAMETEST
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
		p = "\\*.*";
	printf("Scan string = %s\n", p);

	if (isdigit(*p))
		s = _volname(atoi(p));
	else
		s = vol_name(p);
	printf("Pointer = %04x\n", s);
	if (s)
		printf("Volume name = %s\n", s);
	}

#endif
/* ------------------------------------------------------------------------ */
