/* ----------------------------------------------------------------------- *\
|
|				    FNABSPTH
|
|		    Copyright (c) 1985, 1990, all rights reserved
|				Brian W Johnson
|				   26-May-90
|				   17-Dec-94
|				   17-Aug-97
|				   25-Sep-97 UNC
|
|	    char *		Return an allocated string
|	p = fnabspth (s);	Convert a filename to drive:/path/file
|	    char  *s;		Pointer to the pathname
|
|	The return string is allocated, and should disposed with free()
|	The return string is guaranteed to contain at least "X:/".
|	The return string is converted to all upper case.
|	The return string path characters are standardized.
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <ctype.h>
#include  <stdlib.h>
#include  <string.h>

#include  "fwild.h"

#define	  ispath(ch)	(((ch) == '/') || ((ch) == '\\'))

static	char	path [] = "/";

/* ----------------------------------------------------------------------- */
	char *
fnabspth (					/* Convert a filename to absolute format */
	char  *s)				/* Pointer to the source filename string */

	{
	char  *p;				/* Temporary string pointer */
	char   drive;			/* Drive number (1 => A) */
	char   temp [1024];		/* Temporary string buffer */


	path[0] = strpath(s);		/* Determine the path character */

	if (isalpha(*s) && (*(s + 1) == ':'))
		{
		drive = toupper(*s) - ('A' - 1);
		s += 2;						/* Determine the specified drive number */
		}
	else
		drive = (char)(getdrive());	/* Determine the default drive number */

	if (ispath(*s))					/* If the incoming path begins with '\' */
		{
		if (ispath(*(s + 1)))
			strcpy(temp, s);		/* It is an absolute UNC path */
		else
			{
			temp[0] = drive + ('A' - 1);/* Place the drive letter */
			temp[1] = ':';			/* Place the ':' */
			strcpy(&temp[2], s);	/* Use the specified absolute path */
			}
		}

	else							/* The path is relative */
		{
		getdir(drive, &temp[0]);	/* Use the default directory path... */
		if (*s)
			{
			int  Length = (int)(strlen(temp));

			if ( ! ispath(temp[Length - 1]))
			strcat(temp, &path[0]);
			strcat(temp, s);		/* ...plus the relative path */
			}
		}

	fnreduce(&temp[0]);				/* Reduce the pathname */

	p = fmalloc((int)(strlen(&temp[0])) + 1);	/* Build the return string */
	strcpy(p, &temp[0]);

	return (p);
	}

/* ----------------------------------------------------------------------- */
#ifdef TEST
void main (
	argc, argv)		/* Test program */

	int    argc;
	char  *argv [];

    {
    char  *p;

    if (argc > 1)
		{
		p = *++argv;
		printf("%s\n", p);
		p = fnabspth(p);
		printf("%s\n", p);
		free(p);
		}
	else
		printf("No pathname !\n");
    }
#endif
/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */
