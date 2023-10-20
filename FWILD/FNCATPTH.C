/* ------------------------------------------------------------------------ *\
|
|				   FNCATPTH
|
|		    Copyright (c) 1985, 1990, all rights reserved
|				Brian W Johnson
|				   26-May-90
|				   10-Sep-23 Adapted to the revised fnreturn.c
|
|		int					Returns 0 for success, else (-1) if error
|	_fncatpth (				Return pcat concatenated onto base
|		char  *pDst,		Ptr to the caller's buffer for the returned pathspec
|		const char  *base,	Pointer to the base path string
|		const char  *pcat)	Pointer to the path to be concatenated
|
|		char *				Return an allocated pathname
|	p = fncat (				Return pcat concatenated onto base
|		const char  *base,	Pointer to the base path string
|		const char  *pcat)	Pointer to the path to be concatenated
|
|	Returns a pointer to path s1 with path s2 concatenated.
|	The return string is allocated, and should be disposed with free()
|
\* ------------------------------------------------------------------------ */

#include  <windows.h>
#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>

#include  "fWild.h"

// ---------------------------------------------------------------------------
//	#define TEST	// Define this to include the test main and diagnostics
//	#define DEBUG	// Define this to include the diagnostics

#ifdef TEST
#define DEBUG
#endif

#define  _ERROR	(-1)	 
#define  isPath(ch)	(((ch) == '/') || ((ch) == '\\'))

// ---------------------------------------------------------------------------
	int						// Returns 0 for success, else (-1) if reduce error
_fncatpth (					// Concatenate pConcat onto pBase
		  char  *pDst,		// Ptr to the caller's buffer for the returned pathspec
	const char  *pBase,		// Pointer to the base path string
	const char  *pConcat)	// Pointer to the path to be concatenated

	{
	if ((pBase   == NULL)				// Validate all pointers
	||  (pConcat == NULL)
	||  (pDst    == NULL))
		return (_ERROR);
	
#ifdef DEBUG
printf("fncatpth Entry Dest \"%s\"\n", pDst);
printf("fncatpth Entry Base \"%s\"\n", pBase);
printf("fncatpth Entry Cat  \"%s\"\n", pConcat);
fflush(stdout);
#endif

	pathCopy(pDst, pBase, MAX_COPY);	// Copy Path 1 to the destination buffer
	pathCat(pDst, pConcat, MAX_COPY); 	// Concatenate Path2 to the destination buffer

#ifdef DEBUG
printf("fncatpth - fnreduce: \"%s\"\n", pDst);
fflush(stdout);
#endif

	fnreduce(pDst);

#ifdef DEBUG
printf("fncatpth Return: \"%s\"\n", pDst);
fflush(stdout);
#endif
	return (0);
	}

// ---------------------------------------------------------------------------
	char *					// Return a newly allocated string, else NULL if a reduce error
fncatpth (					// Concatenate pConcat onto pBase
	const char  *pBase,		// Pointer to the base path string
	const char  *pConcat)	// Pointer to the path to be concatenated

	{
	char		*pDst;		// Pointer to the returned pathspec string

#ifdef DEBUG
printf("fncatpth Entry \"%s\"   \"%s\"\n", pBase, pConcat);
fflush(stdout);
#endif

	if ((pBase   == NULL)
	||  (pConcat == NULL))
		return (NULL);
		
	pDst = fmalloc(MAX_PATH);		// Allocate the concat path

	if ((_fncatpth(pDst, pBase, pConcat)) == _ERROR)	// Build the path
		{
		free(pDst);
		pDst = NULL;
		}

	return (pDst);
	}
	
// ---------------------------------------------------------------------------
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
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
