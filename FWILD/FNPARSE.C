/* ----------------------------------------------------------------------- *\
|
|							     FNPARSE
|
|			    Copyright (c) 2022, all rights reserved
|							Brian W Johnson
|							   29-Sep-22
|							   10-Sep-23 (Major rewrite)
|
|			int					// Returns 0
|		fnParse (				// Point the non-directory tail of path s
|			char  *s,			// Pointer to the pathname string
|			int   *pCopyIndex,	// Ptr to callers concatenation index
|			int   *pTermIndex)	// Ptr to callers termination index
|
|	This file is used to parse path strings for the purpose of separating out
|	wild path elements and/or filenames.
|
\* ----------------------------------------------------------------------- */

#include  <windows.h>
#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>

#include  "fWild.h"

/* ----------------------------------------------------------------------- */
//	#define TEST	// Define this to include the test main and diagnostics
//	#define DEBUG	// Define this to include the diagnostics

#ifdef TEST
#define DEBUG
#endif


#define PERROR	(-1)		// Returned if a NULL ptr to the path string

#define PATHCH	('\\')
#define NULCH	('\0')

static	char	temp [MAX_PATH];

// ---------------------------------------------------------------------------
	int					// Returns 0 for success, else -1 if a bad pathspec
fnParse (				// Point the non-directory tail of path s
	const char  *s,		// Pointer to the pathname string
	int   *pCopyIndex,	// Ptr to callers concatenation index (if not NULL)
	int   *pTermIndex)	// Ptr to callers termination index (if not NULL)

	{
	char  *pPath = temp;// Pointer to a temporary path buffer


	if (s == NULL)					// Null string ptr is illegal
		return (PERROR);

#ifdef DEBUG
printf("fnParse() Entry \"%s\"\n", s);
fflush(stdout);
#endif

	pathCopy(pPath, s, MAX_COPY);
	strsetp(pPath, PATHCH);

	// The following algorithm is:
	// Starting from the right, in the copy, replace each possible point in the path
	// with NUL; check if the truncated path is a valid directory; if so note it and quit.
	// If not, continue to the left until it is a valid directory.
	// Return the two indices:
	// copyIndex indicates the path offset to copy the wild tail of the path.
	// termIndex indicates the place to terminate the path following the valid path.
	// Copying should preced termination, as they may overlap.
	
	// A root separator counts as a valid directory, as does an unrooted drive or UNC

#ifdef DEBUG
printf("Root \"%s\"\n", pPath);
printf("strlen %d\n", (int)(strlen(pPath)));
#endif

	char *pEnd  = pPath;	// Assume the path is entirely invalid
	char *pBody = PointPastPrefix(pPath);	// Also skips a root separator
	char *p		= &pPath[strlen(pPath)];	// Point the string NULCH
	char  ch    = *p;
	int   termIndex;
	int   copyIndex;

#ifdef DEBUG
printf("Tail \"%s\"\n", p);
#endif

	if ((strcmp(pBody, ".") == 0)		// Any of these terminate the parse
	||  (strcmp(pBody, "?") == 0)
	||  (strcmp(pBody, "*") == 0)
	||  (strcmp(pBody, "**") == 0))
		{
		termIndex = (int)(pBody - pPath);
		copyIndex = termIndex;
		}
	else
		{	
		while (p >= pBody)
			{
			ch = *p;

#ifdef DEBUG
printf("Test  \"%s\"\n", pPath);
#endif
			if ((ch == NULCH)		// All the points we should check
			||  (ch == PATHCH)
			||  (p  == pBody))
				{
				*p = '\0';			// Terminate the path at this terminator


				if (fnchkdir(pPath))
					{
					pEnd = p;	// This path is valid; remember it
#ifdef DEBUG
printf("Valid \"%s\"\n", pPath);
#endif
					break;		// No need to continue
					}
#ifdef DEBUG
printf("InValid \"%s\"\n", pPath);
#endif
				}
			--p;
			}

		termIndex = (int)(pEnd - pPath);
		copyIndex = termIndex + ((ch == PATHCH) ? 1 : 0);
		}

	if (pTermIndex)
		*pTermIndex	= termIndex;
	if (pCopyIndex)
		*pCopyIndex = copyIndex;

#ifdef DEBUG
printf("CopyIndex %d\n", copyIndex);
printf("TermIndex %d\n", termIndex);
#endif

	return (0);
	}

// ---------------------------------------------------------------------------
#ifdef TEST

static	char in [100];

//----------------------------------------------------------------------------
//	Display a graphic concatenation pointer in a diagnostic string
//----------------------------------------------------------------------------
	static char *
concatDisp (
	int offset)

	{
static	char	offsetb [100];	/* The returned caret string */

	int   count = min(70, (offset + 1));
	char *p = offsetb;

	while ((count-- > 0) && (count < sizeof(in)))
		*p++ = ' ';
	*p++ = '^';
	*p++ = '\0';

	return (offsetb);
	}

// ---------------------------------------------------------------------------
main ()						/* Test program */

	{
	int termindex;
	int copyindex;

	for (;;)
		{
		printf("Path: ");
		gets(in);
		if (tolower(in[0]) == 'q')
			break;

		fnParse(in, &copyindex, &termindex);
		printf("Path        \"%s\"\n", in);
		printf("Term [%2d]   %s\n",   termindex, concatDisp(termindex));
		printf("Copy [%2d]   %s\n",   copyindex, concatDisp(copyindex));
		}
    }
#endif
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
