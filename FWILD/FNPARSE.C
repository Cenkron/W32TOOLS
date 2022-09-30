/* ----------------------------------------------------------------------- *\
|
|							     FNPARSE
|
|			    Copyright (c) 2022, all rights reserved
|							Brian W Johnson
|							   29-Sep-22
|
|	This file is used to parse path strings for the purpose of sepsrating out
|	wild path elements and/or filenames.
|
\* ----------------------------------------------------------------------- */

#include  <windows.h>
#include  <stdio.h>
#include  <stdlib.h>
#include  <malloc.h>
#include  <string.h>

//#define  FWILD_INTERNAL

#include  "fwild.h"

/* ----------------------------------------------------------------------- */
	void
fnParse (				/* Point the non-directory tail of path s */
	char  *s,			/* Pointer to the pathname string */
	int   *pCatIndex,	// Ptr to callers concatenation index
	int   *pTermIndex)	// Ptr to callers termnation index		

	{
	char  *pPath;		/* Pointer to a temporary path buffer */
	char  *p;			/* Pointer past the prefix, if any */
	char  *pStart;		/* Pointer past the prefix, if any */

	if ((pPath = strdup(s)) != NULL)
		{
		fnreduce(pPath);
		pStart = PointPastPrefix(pPath, TRUE);

		// The following algorithm is:
		// Starting from the right, temporarily replace each path separator with NUL;
		// check if the truncated path is a valid directory; if not, continue to the left
		// until it is a valid directory.  Return the index of the path element following the
		// rightmost valid test.  (Do not do the replacement of the root separator, if any.)
		// CatIndex is of the path element following the rightmost valid directory test.
		// TermIndex is of the final separator of the rightmost valid directory test.

		// A root separator counts as a valid directory, as does an unrooted drive or UNC

// printf("Root \"%s\"\n", pPath);
// printf("strlen %d\n", strlen(pPath));

		p = &pPath[strlen(pPath)]; // This might be the same as pStart (if unrooted, and not wild)
		for (;;)
			{
// printf("Char %c\n", ((*p == '\0') ? '-' : *p));
			if (p == pStart)							// This is a loop termination condition
				{
				if ((*p == '/') || (*p == '\\'))
					{
					*pTermIndex	= ((p + 1) - pPath);
					*pCatIndex  = ((p + 1) - pPath);	// All path elements excluded, rooted
					break;
					}
				else // Root separator not present
					{
					*pTermIndex	= (p - pPath);
					*pCatIndex  = (p - pPath);			// All path elements excluded, unrooted or UNC
					break;
					}
				}

			else // (p > pStart)				// Look for element boundaries and test
				{
				if ((*p == '\0') || (*p == '\\') || (*p == '/'))
					{
					const char cs = *p;
					*p = '\0';
// printf("Test \"%s\"\n", pPath);
					if (fnchkdir(pPath))
						{
						*pTermIndex	= (p - pPath);
						*pCatIndex  = (cs == '\0') ? (p - pPath) : ((p +1) - pPath);
// printf("Accepted \"%s\"\n", pPath);
						break;
						}
					else
					*p = cs;
					}
				}
			--p;			// Move back one char
			}

		free(pPath);
		}

// printf("CatIndex  %d\n", *pCatIndex);
// printf("TermIndex %d\n", *pTermIndex);
// fflush(stdout);
	}

/* ----------------------------------------------------------------------- */
