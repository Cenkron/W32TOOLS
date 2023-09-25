/* ----------------------------------------------------------------------- *\
|
|							     FNPARSE
|
|			    Copyright (c) 2022, all rights reserved
|							Brian W Johnson
|							   29-Sep-22
|							   10-Sep-23 Compatibility with new fnreduce
|
|			int					// Returns 0 for success, else -1 if bad pathspec
|		fnParse (				// Point the non-directory tail of path s
|			char  *s,			// Pointer to the pathname string
|			int   *pCatIndex,	// Ptr to callers concatenation index
|			int   *pTermIndex)	// Ptr to callers termination index
|
|	This file is used to parse path strings for the purpose of separating out
|	wild path elements and/or filenames.
|
\* ----------------------------------------------------------------------- */

#include  <windows.h>
#include  <stdio.h>
#include  <stdlib.h>
#include  <malloc.h>
#include  <string.h>

#include  "fwild.h"

/* ----------------------------------------------------------------------- */

//#define DEBUG	// Define this to include diagnostics

#define PATHCH	('\\')

/* ----------------------------------------------------------------------- */
	int					// Returns 0 for success, else -1 if a bad pathspec
fnParse (				// Point the non-directory tail of path s
	char  *s,			// Pointer to the pathname string
	int   *pCatIndex,	// Ptr to callers concatenation index (if not NULL)
	int   *pTermIndex)	// Ptr to callers termination index (if not NULL)

	{
	char  *pPath = NULL;// Pointer to a temporary path buffer
	char  *p;			// Pointer past the prefix, if any
	char  *pStart;		// Pointer past the prefix, if any

#ifdef DEBUG
printf("fnParse() Entry \"%s\"\n", s);
fflush(stdout);
#endif
	if ((pPath = strdupMax(s)) != NULL)
		{
		strsetp(pPath, PATHCH);
		if (fnreduce(pPath) < 0)
			{
			if (pPath)
				{
				free(pPath);
				pPath = NULL;
				}
			return (-1);
			}

		pStart = PointPastPrefix(pPath, TRUE);

		if (strcmp(pStart, ".") == 0)					// "." is a special case
			{
			if (pTermIndex)
				*pTermIndex	= (int)(pStart - pPath);
			if (pCatIndex)
				*pCatIndex  = (int)(pStart - pPath);
			}
		else if (strcmp(pStart, "\\.") == 0)					// "\." is a special case
			{
			if (pTermIndex)
				*pTermIndex	= (int)(pStart+1 - pPath);
			if (pCatIndex)
				*pCatIndex  = (int)(pStart+1 - pPath);
			}
			
		else
			{

			// The following algorithm is:
			// Starting from the right, temporarily replace each path separator with NUL;
			// check if the truncated path is a valid directory; if not, continue to the left
			// until it is a valid directory.  Return the index of the path element following the
			// rightmost valid test.  (Do not do the replacement of the root separator, if any.)
			// CatIndex is of the path element following the rightmost valid directory test.
			// TermIndex is of the final separator of the rightmost valid directory test.

			// A root separator counts as a valid directory, as does an unrooted drive or UNC

#ifdef DEBUG
printf("Root \"%s\"\n", pPath);
printf("strlen %d\n", (int)(strlen(pPath)));
#endif

			p = &pPath[strlen(pPath)]; // This might be the same as pStart (if unrooted, and not wild)

			for (;;)
				{
#ifdef DEBUG
printf("Char %c\n", ((*p == '\0') ? '-' : *p));
#endif
				if (p == pStart)								// This is a loop termination condition
					{
					if ((*p == '/') || (*p == '\\'))
						{
						if (pTermIndex)
							*pTermIndex	= (int)((p + 1) - pPath);
						if (pCatIndex)
							*pCatIndex  = (int)((p + 1) - pPath);	// All path elements excluded, rooted
						break;
						}
					else // Root separator not present
						{
						if (pTermIndex)
							*pTermIndex	= (int)((p - pPath));
						if (pCatIndex)
							*pCatIndex  = (int)((p - pPath));		// All path elements excluded, unrooted or UNC
						break;
						}
					}

				else // (p > pStart)							// Look for element boundaries and test
					{
					if ((*p == '\0') || (*p == '\\') || (*p == '/'))
						{
						const char cs = *p;
						*p = '\0';
#ifdef DEBUG
printf("Test \"%s\"\n", pPath);
#endif
						if (fnchkdir(pPath))
							{
							if (pTermIndex)
								*pTermIndex	= (int)((p - pPath));
							if (pCatIndex)
								*pCatIndex  = (cs == '\0') ? (int)((p - pPath)) : (int)(((p +1) - pPath));
#ifdef DEBUG
printf("Accepted \"%s\"\n", pPath);
#endif
							break;
							}
						else
						*p = cs;
						}
					}
				--p;			// Move back one char
				}
			}

		if (pPath)
			{
			free(pPath);
			pPath = NULL;
			}
		}

#ifdef DEBUG
printf("fnParse() Return \"%s\"\n", s);
printf("CatIndex  %d\n", ((pCatIndex != NULL)  ? (*pCatIndex)  :"NULL"));
printf("TermIndex %d\n", ((pTermIndex != NULL) ? (*pTermIndex) :"NULL"));
fflush(stdout);
#endif

	if (pPath)
		free(pPath);
	return (0);
	}

/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */
