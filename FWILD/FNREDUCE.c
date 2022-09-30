/* ----------------------------------------------------------------------- *\
|
|				    FNREDUCE
|
|		    Copyright (c) 1985, 1990, all rights reserved
|				Brian W Johnson
|				   26-May-90
|				   25-Sep-97 UNC
|				   10-Aug-00 .xxxx filenames
|				   19-Sep-22 corrected UNC skipover
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

#define	  PATHCH		('\\')
#define	  NULCH			('\0')
#define	  COLCH			(':')

#define	  ispath(ch)	((ch) == PATHCH)

static	int		rooted = FALSE;

// ------------------------------------------------------------------------------------------------
// Private methods
// ------------------------------------------------------------------------------------------------
// Backcopy string from p2 to p1
// ------------------------------------------------------------------------------------------------
	static void
backcopy (				/* Copy string from p2 to p1 */
	char  *p1,			/* Destination pointer */
	char  *p2)			/* Source pointer */

	{
	while (*(p1++) = *(p2++))
		;
	}

// ------------------------------------------------------------------------------------------------
// Skip to the next path element
// ------------------------------------------------------------------------------------------------
	static char *
nextpath (				/* Point the next component */
	char  *s)			/* Pointer to the current component */

	{
	while (*s && ( ! ispath(*s)))
		++s;
	if (*s)
		++s;
	return (s);			// Returns ptr to the next path element
	}

// ------------------------------------------------------------------------------------------------
// Condense the path pointed by s (uses a recursive algorithm)
// ------------------------------------------------------------------------------------------------
    static int				/* Return the condensation count */
condense (					/* Condense '.' and '..' from the path name */
    char  *s,				/* Pointer to the condensable string */
    int    m)				/* Pathname prefix component counter */

    {
    char  *p;				/* Pointer to next path component */
    char  *q;				/* Pointer to next path component + 1 */
    int    n;				/* Return ".." condensation counter */
    int    k;				/* Return ".." condensation counter */

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

// ------------------------------------------------------------------------------------------------
// Path is rooted; delete any illegitimate leading "." or ".." elements
// ------------------------------------------------------------------------------------------------
	static void			// Returns ptr to actual path
StripLeadingDots (		// Deletes any leading "." or ".." directory specs
    char  *s)			// Pointer to the pathname string

	{
	for (;;)
		{
		char *pScan = s;	// Scanning pointer

		if ((*pScan == '.')
		&& ((*(pScan+1) == PATHCH)  ||  (*(pScan+1) == NULCH)))
			{
			strcpy(s, pScan+2);
			continue;
			}

		if ((*(pScan  ) == '.')
		&&  (*(pScan+1) == '.')
		&& ((*(pScan+2) == PATHCH)  ||  (*(pScan+1) == NULCH)))
			{
			strcpy(s, pScan+3);
			continue;
			}
		break;
		}
	}

// ------------------------------------------------------------------------------------------------
// fnreduce - Remove any redundancies in get the shortest possible path
// ------------------------------------------------------------------------------------------------
	void
fnreduce (				// Eliminate pathname redundancy
    char  *s)			// Pointer to the pathname string

	{
	char  *p = s;		// Pointer to the path following the prefix and root separator
	char  *pTemp;		// Returned result from prefix queries

	if (s == NULL)
		return;

	strsetp(s, PATHCH);				// Set the standard path character in the string

	if ((pTemp = QueryDrivePrefix(s, TRUE)) != NULL)	// Single mode
		{
		p = pTemp;
		if (ispath(*p))				// If we now find a root separator
			{
			rooted = TRUE;			// Yes, we are rooted (absolute path)
			++p;					// Skip over the root separator
			}
		}

	else if ((pTemp = QueryUNCPrefix(s)) != NULL)
		{
		p = pTemp;
		rooted = TRUE;				// We are by default rooted (absolute path)
		}

	else if (ispath(*p))			// No prefix found; if we have a root separator
		{
		rooted = TRUE;				// Yes, we are rooted (absolute path)
		++p;						// Skip over the root separator
		}
	else
		rooted = FALSE;				// We are not rooted (relative path)

	if (rooted)						// In the case of an absolute path,
		StripLeadingDots(p);		// eliminate any preceding "." or ".." prefixes

	condense(p, 0);					// Condense the path elements
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
