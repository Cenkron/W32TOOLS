/* ----------------------------------------------------------------------- *\
|
|				    FNMATCH
|			 File Name Comparison Function
|			   (includes a test program)
|
|		    Copyright (c) 1985, 1990, all rights reserved
|				Brian W Johnson
|				   20-May-90
|				   17-Dec-94
|				   25-Apr-97
|				   22-Aug-97
|
|	    int			Return TRUE if match successful
|	fnmatch (p, q, mode);	Test two filenames for filename match
|	    char  *p;		Pattern name (may include wild cards)
|	    char  *q;		Test name (must be a pure file name)
|	    int    mode;	If TRUE, missing p ext matches any q ext
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <ctype.h>

#include  "fwild.h"

// #define   CONSOLEMODETEST

#define  MAX_PATH_X  (1024)

static	char   *basecopy  (char *, char *);
static	int	namematch (char *, char *);

/* ----------------------------------------------------------------------- */
#ifdef    CONSOLEMODETEST
    void
main ()				/* Test program for match */

    {
    char  s1 [1024];
    char  s2 [1024];

    while (TRUE)
	{
	printf("Wild  string: ");
	gets(s1);
	printf("Check string: ");
	gets(s2);
	if (fnmatch(s1, s2, 1))
	    printf("Matched\n\n");
	else
	    printf("Did not match\n\n");
	}
    }

#endif
/* ----------------------------------------------------------------------- */
    int				/* Return TRUE if the names match */
fnmatch (			/* Test two filenames for filename match */
    char  *p,			/* Pattern name (may include wild cards) */
    char  *q,			/* Test name (must be a pure file name) */
    int    mode)		/* If TRUE, missing ext matches any ext */

    {
    int    patternlen;		/* Length of the pattern extension */
    int    testlen;		/* Length of the tested  extension */
    char   r [MAX_PATH_X];	/* Copy of the pattern file name prefix */
    char   s [MAX_PATH_X];	/* Copy of the tested  file name prefix */


    if ((p == NULL)  ||  (q == NULL))
	return (FALSE);		/* A NULL ptr can't match anything */

    p = basecopy(r, p);		/* Copy the pattern file name prefix */
    q = basecopy(s, q);		/* Copy the tested file name prefix */

    if ( ! namematch(r, s))	/* Compare the file basename parts */
	return (FALSE);		/* Failed */

    patternlen = strlen(p);	/* Determine the extension lengths */
    testlen    = strlen(q);

    if (patternlen == 0)
	{
	if (testlen <= 1)
	    return (TRUE);	/* No extension on either name */
	else
	    return (mode);	/* No pattern extension, use the mode */
	}

    else if (testlen == 0)
	{
	if (patternlen <= 1)
	    return (TRUE);	/* No extension on either name */
	else
	    return (namematch(p+1, q));	/* Compare the extensions */
	}

    return (namematch(p+1, q+1));	/* Compare the extensions */
    }

/* ----------------------------------------------------------------------- */
    static int			/* Returns offset to the extension */
getext (			/* Find the last '.' */
    char  *p)			/* Ptr to the origin string */

    {
    int    length;		/* Length of the origin string */
    char  *end;			/* Ptr to the end of the src string */


    if ((length = strlen(p)) == 0)
	return (0);			/* There is no extension */

    end = p + (length - 1);		/* Point the last character */

    while (( end >= p)			/* While in the string */
    &&     (*end != '/')		/* and   not '/' */
    &&     (*end != '\\')		/* and   not '\' */
    &&     (*end != ':'))		/* and   not ':' */
	{
	if (*end == '.')		/* Check for '.' */
	    return (end - p);		/* Return the extension offset */
	--end;				/* Back up to the predecessor */
	}

    return (length);			/* There is no extension */
    }

/* ----------------------------------------------------------------------- */
    static char *		/* Returns ptr to the extension */
basecopy (dst, src)		/* String copy src to dst except the extension */
    char  *dst;			/* Destination string pointer */
    char  *src;			/* Source string pointer */

    {
    int  extoffset;		/* Offset to the extension (if any) */


    if ((extoffset = getext(src)) > 0)	/* Get the extension offset */
	strncpy(dst, src, extoffset);	/* Copy the basename */
    *(dst + extoffset) = '\0';		/* Terminate it */
    return (src + extoffset);		/* Return ptr to the extension part */
    }

/* ----------------------------------------------------------------------- */
    static int			/* Returns TRUE if the names match */
namematch (			/* Test two strings for filename match */
    char  *p,			/* Pattern string (may include wild cards */
    char  *q)			/* Test string (must not incl wild cards) */

    {
    while (*p != 0)
	{
	char  pc = tolower(*p);		/* Compare is case insensitive */
	char  qc = tolower(*q);

	if ((pc == qc) || ((pc == '?') && (qc != 0)))	/* Direct match cases */
	    {
	    ++p;
	    ++q;
	    }
	else if (pc == '*')		/* '*' match cases */
	    {
	    ++p;			/* Skip over the '*' */
	    do  {			/* Accept each character */
		if (namematch(p, q))	/* and try to match that way */
		    return (TRUE);
		}  while (*(q++) != 0);	/* while they are not the end */
	    return (FALSE);		/* '*' Mismatched */
	    }
	else
	    return (FALSE);		/* Totally mismatched */
	}
    return (*q == 0);			/* Verify test string also terminated */
    }

/* ----------------------------------------------------------------------- */
