/* ----------------------------------------------------------------------- *\
|
|				   FNCHKDIR
|
|		    Copyright (c) 1985, 1990, all rights reserved
|				Brian W Johnson
|				   26-May-90
|				   17-Dec-94
|				   17-Aug-97 NT
|				   26-Sep-97 UNC
|
|	    int			Returns TRUE if the string "s" is the
|	bool = fnchkdir (s);	name of an existing directory file
|	    char  *s;		Pointer to the string
|
\* ----------------------------------------------------------------------- */

#include  <windows.h>
#include  <stdio.h>
#include  <ctype.h>
#include  <string.h>
#include  <malloc.h>

#define  FWILD_INTERNAL

#include  "fwild.h"

/* ----------------------------------------------------------------------- */
#ifdef  TEST
    void
main (argc, argv)
    int    argc;
    char  *argv [];

    {
    char  *p;

    if (argc > 1)
	p = *++argv;
    else
	p = "";
    if (fnchkdir(p))
	printf("TRUE\n");
    else
	printf("FALSE\n");
    }
#endif
/* ------------------------------------------------------------------------ */
    int				/* Return TRUE if the directory exists */
fnchkdir (s)			/* Verify the existence of a directory */
    char  *s;			/* Pointer to the filename string */

    {
    return (_fnchkdir(s) || fnchkunc(s));
    }

/* ------------------------------------------------------------------------ */
    int				/* Return TRUE if the directory exists */
_fnchkdir (s)			/* Verify the existence of a directory */
    char  *s;			/* Pointer to the filename string */

    {
    int      result;		/* The returned result */
    char    *p;			/* Temporary string pointer */
    char    *pbuff;		/* Absoluted copy of the input string */
    DTA_BLK  dta;		/* A DTA for _findf() */


    if (strchr(s, '?')			/* Wild is disallowed */
    ||  strchr(s, '*'))
	return (FALSE);

    p = pbuff = fnabspth(s);		/* Perform reduction */

    if (isalpha(*p) && (*(p + 1) == ':'))
	p += 2;

    for (;;)
	{
	if ((*p == '/') || (*p == '\\'))
	    ++p;
	else if (strncmp(p, "..", 2) == 0)
	    p += 2;
	else if ((*p == '.')
	&&      ((*(p+1) == '/')  ||  (*(p+1) == '\\')  ||  (*(p+1) == '\0')))
	    ++p;
	else
	    break;
	}

// printf("Check:      \"%s\"\n", pbuff);
    if (*p == '\0')			/* "X:"  "X:/"  "."  ".."  "/"  "X:." */
	result = TRUE;			/* are all good directories */

    else if (result = _findf(&dta, pbuff, ATT_SUBD | ATT_HIDDEN) == 0)	/* Test it */
        {
	result = ((dta.dta_type & ATT_SUBD) != 0);
	_findc(&dta);
        }
   
    free(pbuff);
    return (result);
    }

/* ----------------------------------------------------------------------- */
