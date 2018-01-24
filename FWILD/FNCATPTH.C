/* ----------------------------------------------------------------------- *\
|
|				   FNCATPTH
|
|		    Copyright (c) 1985, 1990, all rights reserved
|				Brian W Johnson
|				   26-May-90
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

#define	  ispath(ch)	(((ch) == '/') || ((ch) == '\\'))

static	char	path [] = "/";

/* ----------------------------------------------------------------------- */
#ifdef TEST
main (argc, argv)		/* Test program */
    int    argc;
    char  *argv [];

    {
    char  *p;
    char  *q;
    char  *r;

    if (argc > 2)
	{
	p = *++argv;
	q = *++argv;
	printf("%s   CONCATENATE   %s\n", p, q);
	r = fncatpth(p, q);
	printf("%s\n", r);
	free(r);
	}
    else
	printf("No pathname !\n");
    }
#endif
/* ----------------------------------------------------------------------- */
    char *		/* Return a newly allocated string */
fncatpth (s1, s2)	/* Return s2 concatenated onto s1 */
    char  *s1;		/* Pointer to the base path string */
    char  *s2;		/* Pointer to the concatenated path string */

    {
    int    len1;	/* Length of s1 */
    int    len2;	/* Length of s2 */
    char  *p;		/* Pointer to the returned path string */
    char  *q;		/* Temorary pointer to the path string */


    while (ispath(*s2))			/* Eliminate any s2 path character */
	++s2;

    len1 = strlen(s1);
    len2 = strlen(s2);
    p = fmalloc(len1 + len2 + 2);

    if (len1 == 0)
	strcpy(p, s2);			/* Path 1 is an empty string */
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

    fnreduce(p);
    return (p);
    }

/* ----------------------------------------------------------------------- */
