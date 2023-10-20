/* ----------------------------------------------------------------------- *\
|
|							   FNMATCH2
|					 File Name Comparison Function
|					   (includes a test program)
|
|				    Copyright (c) 1985 - 2023, all rights reserved
|						Brian W Johnson
|						   15-Sep-23	New version of fnmatch
|
|
|	    int				Return TRUE if match successful
|	fnmatch2 (			Test two filenames for filename match
|	    char  *p,		Pattern name (may include wild cards)
|	    char  *q)		Test name (must be a pure file name)
|
|
|	This version treats '*' as a total wild card which includes any "."
|	within the match so that '*' works as *.* previously worked.
|	If a '.' is specified, it must still match a separate suffix.
|
|	Rules:
|
|	Wild '?' must match exactly one path character
|	Wild '*' can  match 0 or more path characters
|
|	This file is deprecated.  It is no longer included in the build.
|	It's functionality has been subsumed into the file pnMatch.c
|
\* ----------------------------------------------------------------------- */

#include  <windows.h>
#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <ctype.h>

#include  "fwild.h"

// --------------------------------------------------------------------------
//	#define   TEST
//	#define   DEBUG

#ifdef DEBUG
#define   DBG(arg) printf("(%d) %d   Pattern: \"%s\"   Name: \"%s\"\n", arg, nest, pp, pn)
#else
#define   DBG(arg)
#endif

#define NULCH			('\0')
#define isvalid(arg)	(isgraph(arg))

int nest = -1;
// --------------------------------------------------------------------------
	static int				// Return TRUE if the name fragments match
pnmatch(					// Test two filename fragments for filename match
	char* pp,				// Pattern name pointer (may include wild cards)
	char* pn)				// Test name pointer (must be a pure file name)

	{
	char chp;				// Current pattern character
	char chn;				// Current name character

#ifdef DEBUG
printf("Pattern and Name: \"%s\"   \"%s\"\n", pp, pn);
#endif

	for (;;)
		{
		++nest;	
		chp = tolower(*pp);					// Disregard character case
		chn = tolower(*pn);

DBG(1);
		if (chn == NULCH)
			{
			while ((chp = *pp) == '*')
				++pp;
			}

		if ((chn == NULCH)
		&&	(chp == NULCH))
			return (TRUE);
						
DBG(2);
		if ((chp == NULCH)				// If only one string terminated,
		||  (chn == NULCH))
			{
			--nest;	
			return (FALSE);				// failed match
			}

		if ((! isvalid(chp))			// if an invalid pattern character,
		||  (! isvalid(chn))			// or an invalid name character,
		||  (chn == '?')				// or an invalid name '?',
		||  (chn == '*'))				// or an invalid name '*',
			{
			--nest;	
			return (FALSE);				// failed match
			}

DBG(3);
		if (chp == '*')					// Try star matching case
			{
			// Point and remember the next non-STAR path character

			for (++pp; ((chp = *pp) == '*'); ++pp)
				;

			// Point the next name char (if any) that matches the non-STAR path character,

			for (;;)
				{
				for (; ((chn = *pn) != chp); ++pn)
					if (chn == NULCH)		// without passing the NULCH
						break;

				if (pnmatch(pp, pn))	// Try not subsuming more path character(s)
					return (TRUE);

				else if (chn == NULCH)
  					break;		
				else
					++pn;
				}
			}

DBG(4);
		if ((chp == chn)				// If the characters match,
		||  (chp == '?'))				// or its a QUESTION match,
			{							// (includes '.')
			++pp;						// advance to the next characters
			++pn;
			continue;					// Good so far
			}

		--nest;	
		return (FALSE);					// failed match
		}
	}

// --------------------------------------------------------------------------
	int						// Return TRUE if the names match
fnmatch2 (					// Test two filenames for filename match
	char  *pPat,			// Pattern name (may include wild cards)
	char  *pName)			// Test name (must be a pure file name)

	{
	if ((strcmp(pName, ".") == 0)	// "."  is unmatchable
	|| (strcmp(pName, "..") == 0))	// ".." is unmatchable
		return (FALSE);

	if ((strcmp(pPat, "*")  == 0)	// "*"  matches everything
	||  (strcmp(pPat, "**") == 0))	// "**" matches everything
		return (TRUE);

	nest = -1;
	char *pp = pPat;
	char *pn = pName;
	return (pnmatch(pPat, pName));
	}

// --------------------------------------------------------------------------
#ifdef TEST
    void
main ()					/* Test program for match */

	{
	char  s1 [1024];
	char  s2 [1024];

	while (TRUE)
		{
		printf("Pattern string: ");
		gets(s1);
		printf("   Name string: ");
		gets(s2);

		if (fnmatch2(s1, s2))
			printf("\"%s\" Matched \"%s\"\n", s1, s2);
		else
			printf("\"%s\"  Failed \"%s\"\n", s1, s2);
		}
	}

#endif
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
