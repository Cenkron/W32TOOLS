/* ----------------------------------------------------------------------- *\
|
|					   FNCHKUNC
|
|			    Copyright (c) 1985, 1990, all rights reserved
|					Brian W Johnson
|					   25-Sep-97 New for UNC
|
|		int					Returns TRUE if the string "s" is the
|	fnchkunc (				  name of a potential UNC volume name
|	    char  *s)			Pointer to the string
|
|	This file is deprecated, and has been removeed from the build.
|
\* ----------------------------------------------------------------------- */

#include  <windows.h>
#include  <stdio.h>
#include  <ctype.h>
#include  <string.h>
#include  <malloc.h>

#define  FWILD_INTERNAL

#include  "fWild.h"

/* ----------------------------------------------------------------------- */
    int						/* Return TRUE if the directory exists */
fnchkunc (					/* Verify the existence of a directory */
	const char  *s)			/* Pointer to the filename string */

	{
	int      result;			/* The returned result */
	char    *pbuff1;			/* Absoluted copy of the input string */
	char    *pbuff2 = NULL;		/* Modified copy of the input string */


	if (strchr(s, '?')			// Wild is disallowed
	||  strchr(s, '*'))
		return (FALSE);

	if ((pbuff1 = fnabspth(s)) == NULL)		// Perform reduction
		result = FALSE;

	else if (strncmp(pbuff1, "\\\\", 2) != 0)
		result = FALSE;

	else if (fnchkdir(pbuff1))
		result = FALSE;

	else if ((pbuff2 = malloc(MAX_PATH)) == NULL)
		result = FALSE;

	else
		{
		char  Dummy[1024];
		pathCopy(pbuff2, pbuff1, MAX_COPY);
		strcat(pbuff2, "\\");		// Append the dummy root directory
		result = (GetVolumeInformation(
			pbuff2,
			Dummy,
			sizeof(Dummy),
			(LPDWORD)(Dummy),
			(LPDWORD)(Dummy),
			(LPDWORD)(Dummy),
			Dummy,
			sizeof(Dummy)) != 0);
		}

	if (pbuff2)
		free(pbuff2);

	if (pbuff1)
		free(pbuff1);

	return (result);
	}

/* ----------------------------------------------------------------------- */
#ifdef  TEST
extern int fnchkunc (char *s);

	void
main (
	int    argc,
	char  *argv [])

	{
	char  *p;

	if (argc > 1)
		p = *++argv;
	else
		p = "";
	if (fnchkunc(p))
		printf("TRUE\n");
	else
		printf("FALSE\n");
	}
#endif
/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */
