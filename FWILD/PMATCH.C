/* ----------------------------------------------------------------------- *\
|
|				    PMATCH
|			 Path Name Comparison Function
|			   (includes a test program)
|
|		    Copyright (c) 1985, 1990, all rights reserved
|				Brian W Johnson
|				   26-May-90
|				   17-Dec-94
|
|	    int			Return TRUE if match successful
|	pmatch (p, q, mode);	Test two pathnames for match
|	    char  *p;		Pattern name (may include wild cards)
|	    char  *q;		Test name (must be a non-wild pathname)
|	    int    mode;	If TRUE, missing ext matches any file ext
|
|	Drive specifications, if any, are stripped off before matching
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <ctype.h>
#include  <string.h>
#include  <stdlib.h>
#include  <io.h>

#include  "fwild.h"


static	int	m_mode;			/* Semi-global match mode */
static	int	_pmatch (char *, char *);

/* ----------------------------------------------------------------------- */
	int
pmatch (				/* Test two filenames for filename match */
	char  *p,			/* Pattern name (may include wild cards) */
	char  *q,			/* Test name (must be a pure file name) */
	int    mode)		/* If TRUE, missing ext matches any ext */

	{
	int    result;		/* Returned result */
	int    r_len;		/* Length of the r string */
	char  *r;			/* Copy of the pattern pathname */
	char  *s;			/* Copy of the tested pathname */


	m_mode = mode;			/* Save the match mode */

	/* Copy and standardize both strings, stripping off any drive specs */

	if (isalpha(*p) && (*(p + 1) == ':'))
		p += 2;
	r = fmalloc(strlen(p) + 4);
	strcpy(r, p);
	strsetp(r, (char)('/'));
	fnreduce(r);

    /* Handle a reduced "." directories by replacing it with "*.*" */

	r_len = strlen(r);
	if ((r_len == 0) || (*(r + (r_len - 1)) == '/'))
		strcat(r, "*.*");

	if (isalpha(*q) && (*(q + 1) == ':'))
		q += 2;
	s = fmalloc(strlen(q) + 1);
	strcpy(s, q);
	strsetp(s, (char)('/'));
	fnreduce(s);

	if (((*r == '/') && (*s == '/'))	/* Ensure root match */
	||  ((*r != '/') && (*s != '/')))
		result = _pmatch(r, s);			/* Perform the actual match */
	else
		result = FALSE;

	free(r);
	free(s);
	return  (result);
	}

/* ----------------------------------------------------------------------- */
	static int
_pmatch (				/* Test two strings for filename match */
    char  *p,			/* Pattern string (may include wild cards */
    char  *q)			/* Test string (must not incl wild cards) */

	{
	int    result = FALSE;	/* Returned match result */
	char  *ps;			/* p component separator pointer */
	char  *pc;			/* p component component pointer */
	char  *qs;			/* q component separator pointer */
	char  *qc;			/* q component component pointer */


#ifdef    TEST
printf("p = %s\n", p);
printf("q = %s\n", q);
#endif

	ps = strchr(p, '/');		/* Point the next p separator */
	pc = (ps) ? (ps + 1) : ("");	/* Point the next p component */
	qs = strchr(q, '/');		/* Point the next q separator */
	qc = (qs) ? (qs + 1) : ("");	/* Point the next q component */

	if (strncmp(p, "..", 2) == MATCH)		/* Handle the case ".." */
		{
		if (strncmp(q, "..", 2) == MATCH)
			result = _pmatch(pc, qc);
		}

	else if (strncmp(q, "..", 2) == MATCH)	/* ".." can only match ".." */
		result = FALSE;

	else if (strncmp(p, "**", 2) == MATCH)	/* Handle the case "**" */
		{
		if (ps == NULL)
			result = TRUE;			/* "**<EOS>" matches anything */
		else
			{
			do  {				/* Match any number of dirs */
				if (_pmatch(pc, q))
					{
					result = TRUE;
					break;
					}
				qs = strchr(q, '/');
				q = (qs) ? (qs + 1) : ("");
				}  while (qs);
			}
		}

	else if (ps || qs)			/* One string is not terminal yet */
		{
		if (ps)
			*ps = '\0';
		if (qs)
			*qs = '\0';
		if (fnmatch(p, q, FALSE))
			result = _pmatch(pc, qc);
		if (ps)
			*ps = '/';
		if (qs)
			*qs = '/';
		}

	else
		result = fnmatch(p, q, m_mode);	/* Both strings are terminal */

	return  (result);
	}

/* ----------------------------------------------------------------------- */
#ifdef    TEST

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
		if (pmatch(s1, s2, 1))
			printf("Matched\n\n");
		else
			printf("Did not match\n\n");
		}
	}

#endif
/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */
