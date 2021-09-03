/* ----------------------------------------------------------------------- *\
|
|				    FNREDUCE
|
|		    Copyright (c) 1985, 1990, all rights reserved
|				Brian W Johnson
|				   26-May-90
|				   25-Sep-97 UNC
|				   10-Aug-00 .xxxx filenames
|
|	    void
|	fnreduce (s);		Eliminate pathname redundancy
|	    char  *s;		Pointer to the pathname
|
|	The return string is reduced to minimal form, converted to all
|	upper case, and the path characters are standardized.
|
|	Rules:
|	Initial drive identification is always preserved.
|	Initial root slash is always preserved.
|	Initial parent directory notation ".." is always preserved.
|	".." following real path components is always reduced.
|	"." components are always reduced.
|	"//" is always reduced.
|	Final trailing "/" is always removed, unless it was initial.
|	Upper case conversion is always done.
|	Path characters are always standardized.
| 
|	NOTE:	Input string "." will produce an empty result string !
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <ctype.h>
#include  <string.h>

#include  "fwild.h"

#define	  ispath(ch)	((ch) == path[0])

static	char	path [] = "/";

static	char   *nextpath (char *);
static	void	backcopy (char *, char *);
static	int	condense (char *, int);

/* ----------------------------------------------------------------------- */
	void
fnreduce (				/* Eliminate pathname redundancy */
    char  *s)			/* Pointer to the pathname string */

	{
	int    n;
	char  *p;			/* Temporary string pointer */
	char  *rp;			/* Pointer to the path root */
	char  *pp;			/* Pointer to the path */


	path[0] = strpath(s);		/* Determine the path character */
	strsetp(s, path[0]);		/* Set the path character */

	if (isalpha(*s)				/* Point the path root */
	&&  (*(s + 1) == ':'))
		rp = s + 2;
	else
		rp = s;

	if ( ! ispath(*rp))			/* Point the path proper */
		pp = rp;				/* It's a relative path */
	else if ( ! ispath(*(rp + 1)))	/* Check for the UNC form */
		pp = rp + 1;			/* No, skip over the leading slash */
	else
		pp = rp + 2;			/* Yes, skip over the UNC prefix */

	condense(pp, 0);			/* Condense the pathname */

	if (n = strlen(pp))			/* Delete any trailing pathchar */
		{
		p = pp + n - 1;
		if (ispath(*p))
			*p = '\0';
		}

//	strupr(s);					/* Convert to all upper case */
	}

/* ----------------------------------------------------------------------- */
    static int				/* Return the condensation count */
condense (					/* Condense '.' and '..' from the path name */
    char  *s,				/* Pointer to the condensable string */
    int    m)				/* Pathname prefix component counter */

    {
    char  *p;				/* Pointer to next path component */
    char  *q;				/* Pointer to next path component + 1 */
    int    n;				/* Return ".." condensation counter */
    int    k;				/* Return ".." condensation counter */

#if 1					// New version, handles ".xxxx" filenames

	n = 0;
	while (*s)
		{
		p = s + 1;
		q = s + 2;
		while (ispath(*s))
			backcopy(s, p);			// Condense multiple pathchars
		if (*s != '.')
			break;					// Skip over non-dot elements

		if (ispath(*p)  ||  (*p == '\0'))
			backcopy(s, nextpath(s));	// Condense "." directories

		else if ((*p != '.')
			 ||  ( ! ispath(*q)  &&  (*q != '\0')))
			break;					// Skip over ".xxxx" filenames

		else
			{
			p = nextpath(++p);		// Process ".." directories
			if (m > 0)
				{
				backcopy(s, p);		// Delete intermediate ones,
				--m;
				++n;				//   but count them
				}
			else
				s = p;				// Leave initial ones in place
			}
		}

#else			// Old version, doesn't handle ".xxxx" filenames
	
	n = 0;
	while (*s)
		{
		p = s + 1;
		while (ispath(*s))
			backcopy(s, p);		/* Condense multiple pathchars */
		if (*s != '.')
			break;
		if (*p == '.')			/* Point past ".." directories */
			{
			p = nextpath(++p);
			if (m > 0)
				{
				backcopy(s, p);		/* Delete intermediate ones, */
				--m;
				++n;			/*   but count them */
				}
			else
				s = p;			/* Leave initial ones in place */
			}
		else
			backcopy(s, nextpath(p));	/* Condense "." directories */
		}

#endif

	if (*s == '\0')
		return (n);

	/*
	** At this point, the path string s points a non-NULL path component
	** which is not a "." or ".." directory.
	*/

	p = nextpath(s);			/* Point the next component */
	if (*p)
		{
		if (k = condense(p, (m + 1)))	/* Recursively call condense */
			{
			backcopy(s, p);		/* Strip one directory */
			n += k - 1;
			}
		}

	return (n);
	}

/* ----------------------------------------------------------------------- */
	static void
backcopy (				/* Copy string from p2 to p1 */
	char  *p1,			/* Destination pointer */
	char  *p2)			/* Source pointer */

	{
	while (*(p1++) = *(p2++))
		;
	}

/* ----------------------------------------------------------------------- */
	static char *
nextpath (				/* Point the next component */
	char  *s)			/* Pointer to the current component */

	{
	while (*s && ( ! ispath(*s)))
		++s;
	if (*s)
		++s;
	return (s);
	}

/* ----------------------------------------------------------------------- */
#ifdef TEST
main (					/* Test main program */
    int    argc,
    char  *argv [])

	{
	char  *p;

	if (argc > 1)
		{
		p = *++argv;
		printf("%s\n", p);
		fnreduce(p);
		printf("%s\n", p);
		}
	else
		printf("No pathname !\n");
	}
#endif
/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */
