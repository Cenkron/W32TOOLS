/* ----------------------------------------------------------------------- *\
|
|				   FNCHKFIL
|
|		    Copyright (c) 1985, 1990, all rights reserved
|				Brian W Johnson
|				   26-May-90
|				   20-Oct-93
|				   17-Dec-94
|				   17-Aug-97
|
|	    int			Returns TRUE if the string "s" is the
|	bool = fnchkfil (s);	name of an existing disk file
|	    char  *s;		Pointer to the string
|
|	The file will be reported existing even if it is hidden or system.
|	The path/filename must not be wild.
|	The test avoids being fooled by path joining utilities.
|
|	NOTE: Returns TRUE for a Novell directory
|
\* ----------------------------------------------------------------------- */

#include  <windows.h>
#include  <stdio.h>
#include  <ctype.h>
#include  <string.h>

#define  FWILD_INTERNAL

#include  "fwild.h"

#define  MAX_PATH_X  (1024)

/* ----------------------------------------------------------------------- */
#ifdef  TEST
main (argc, argv)
    int    argc;
    char  *argv [];

    {
    char  *p;

    if (argc > 1)
	p = *++argv;
    else
	p = "";
    if (fnchkfil(p))
	printf("TRUE\n");
    else
	printf("FALSE\n");
    }
#endif
/* ------------------------------------------------------------------------ */
    int				/* Return TRUE if the file exists */
fnchkfil (s)			/* Verify the existence of a disk file */
    char  *s;			/* Pointer to the filename string */

    {
    int      result;		/* The returned result */
    DTA_BLK  dta;		/* The DTA_BLK for _findf() */
    char     temp [MAX_PATH_X];	/* Copy of the input string */


    if ((strlen(s) >= MAX_PATH_X)
    ||  (strpbrk(s, "?*")))		/* Wild is disallowed */
	return (FALSE);

    strcpy(temp, s);			/* Perform reduction */
    fnreduce(&temp[0]);
					/* Search for the file */

    result = (_findf(&dta, &temp[0], (ATT_HIDDEN | ATT_SYSTEM)) == 0);
    if (result)
	_findc(&dta);
    return (result);
    }

/* ----------------------------------------------------------------------- */
