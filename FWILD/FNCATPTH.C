/* ----------------------------------------------------------------------- *\
|
|				   FNCATPTH
|
|		    Copyright (c) 1985, 1990, all rights reserved
|				Brian W Johnson
|				   26-May-90
|				   10-Sep-23 Adapted to the revised fnreturn.c
|
|	    char *		Return an allocated pathname
|	p = fncatpth (s1, s2);	Concatenate a path to a path
|	    char  *s1;		Pointer to a pathname head
|	    char  *s2;		Pointer to a pathname tail
|
|	Returns a pointer to path s1 with path s2 concatenated.
|	The return string is allocated, and should disposed with free()
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>

#include  "fwild.h"

/* ----------------------------------------------------------------------- */

//#define TEST	// Define this to include the test main and diagnostics

//#define DEBUG	// Define this to include the diagnostics

#ifdef TEST
#define DEBUG
#endif

#define	  ispath(ch)	(((ch) == '/') || ((ch) == '\\'))

static	char	path [] = "\\";

/* ----------------------------------------------------------------------- */
	char *					/* Return a newly allocated string */
fncatpth (					/* Return s2 concatenated onto s1 */
	char  *s1,				/* Pointer to the base path string */
	char  *s2)				/* Pointer to the concatenated path string */

	{
	int    len1;			/* Length of s1 */
	int    len2;			/* Length of s2 */
	char  *p;				/* Pointer to the returned path string */
	char  *q;				/* Temorary pointer into the p path string */


#ifdef DEBUG
printf("fncatpth Entry \"%s\"   \"%s\"\n", s1, s2);
fflush(stdout);
#endif
	while (ispath(*s2))		/* Eliminate any leading s2 path character */
		++s2;

	len1 = (int)(strlen(s1));
	len2 = (int)(strlen(s2));
	p = fmalloc(len1 + len2 + 2);

	if (len1 == 0)
		strcpy(p, s2);		/* Path 1 is an empty string */
	else
		{
		strcpy(p, s1);
		if (len2 != 0)
			{
			q = s1 + len1 - 1;
			if (( ! ispath(*q)) && (*q != ':'))
				{
				path[0] = strpath(p);
				strcat(p, &path[0]);
				}
			strcat(p, s2);
			}
		}
#ifdef DEBUG
printf("fncatpth - fnreduce: \"%s\"\n", p);
fflush(stdout);
#endif
	if (fnreduce(p) < 0)
		{
#ifdef DEBUG
printf("fncatpth Return: NULL\n");
fflush(stdout);
#endif
		free(p);
		return (NULL);
		}

#ifdef DEBUG
printf("fncatpth Return: \"%s\"\n", p);
fflush(stdout);
#endif
	return (p);
	}

/* ----------------------------------------------------------------------- */
#ifdef TEST
main (						/* Test program */
	int    argc,
	char  *argv [])

	{
	char  *p;
	char  *q;
	char  *r;

	if (argc > 2)
		{
		p = *++argv;
		q = *++argv;
printf("Orig   string:   \"%s\" CONCATENATE \"%s\"\n", p, q);
printf("Concat string:   \"%s\" CONCATENATE \"%s\"\n", p, q);
		r = fncatpth(p, q);
		if (r)
printf("After  fncatpth: \"%s\"\n", r);
		else
			{
printf("After  fncatpth: \"(NULL)\"\n");
			free(r);
			}
		}
	else
		printf("Needs two strings !\n");
    }
#endif
/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */
