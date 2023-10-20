/* ----------------------------------------------------------------------- *\
|
|								pathCopy()
|								pathCat()
|								pathCatN()
|								pathDup()
|								fpathDup()
|
|			    Copyright (c) 2023, all rights reserved
|							Brian W Johnson
|							   12-Oct-23 (new)
|
|		char *				// Returned pointer to the destination pathname
|	pathCopy(				// (must be freed by the caller)
|			char *pDst,		// Pointer to the destination pathname
|	  const	char *pSrc,		// Pointer to the source pathname
|			int	  max)		// Size of the destination buffer (including termination)
|
|		char *				// Returned pointer to the destination pathname
|	pathCat(				// (must be freed by the caller)
|			char *pDst,		// Pointer to the destination pathname
|	  const	char *pSrc,		// Pointer to the source pathname
|			int	  max)		// Size of the destination buffer (including termination)
|
|		char *				// Returned pointer to the allocated destination pathname
|	pathDup(				// (must be freed by the caller)
|	  const	char *pSrc)		// Pointer to the source pathname
|
|		char *				// Returned pointer to the allocated destination pathname
|	fpathDup(				// (must be freed by the caller)
|	  const	char *pSrc)		// Pointer to the source pathname
|
|	fpathDup() terminates the application if allocation fails
|	pathDup() [both versions] allocates MAX_PATH and at most, copies MAX_COPY
|	All four functions ensure the destination string is properly terminated.
|	The returned pointer must be freed by the caller.
|
\* ----------------------------------------------------------------------- */

#include  <windows.h>
#include  <stdio.h>
#include  <stdlib.h>
#include  <malloc.h>
#include  <string.h>

#include  "fWild.h"

/* ----------------------------------------------------------------------- */
//	#define DEBUG		// Define to include debug output

#define	PATHCH	('\\')

#define isPath(ch)  ((ch == '\\') || (ch == '/'))  // Match either possible path character

/* ----------------------------------------------------------------------- */
	char *					// Returned pointer to the destination pathname
pathCopy(					// (must be freed by the caller)
		char *pDst,			// Pointer to the destination pathname string
  const char *pSrc,			// Pointer to the source string
		int   DstSize)		// Size of pDst buffer (including termination)

	{

#ifdef DEBUG
printf("PathCopy src: \"%s\"\n", pSrc);
printf("PathCopy dst: \"%s\"\n", pDst);
#endif

	--DstSize;				// Leave room for the terminator
	do	{					// Do the copy (might be partial,(iff too long))
		*(pDst++) = *pSrc;
		} while ((*(pSrc++) != 0) && (--DstSize > 0));
	*pDst = '\0';			// Ensure termination

	return (pDst);
	}

/* ----------------------------------------------------------------------- */
	char *					// Returned pointer to the destination pathname
pathCat(					// Concatenate pSrc to pDst
		char *pDst,			// Pointer to the destination pathname string
  const char *pSrc,			// Pointer to the source string (with NO leading PATHCH)
		int   DstSize)		// Size of pDst buffer (including termination)

	{
#ifdef DEBUG
printf("PathCat src: \"%s\"\n", pSrc);
printf("PathCat dst: \"%s\"\n", pDst);
#endif

	char *pPast = PointPastPrefix(pDst);	// Path body pointer
	char *pCat  = pPast + strlen(pPast);	// Copy destination pointer

	if ((pCat > pPast)			// If the destination has a nonempty path body,
	&& (! isPath(*(pCat-1))))	// and there is no existing trailing PATHCH,
		{
		*pCat++ = PATHCH;		//   append a PATHCH
		*pCat   = '\0';			// Terminate the string here, just in case
		}
	
	while (isPath(*pSrc))		// If the source path has any leading PATHCH,
		++pSrc;					//   skip over it (them)

	pathCopy(pCat, pSrc, (DstSize - (int)(strlen(pCat))));

	return (pDst);
	}

/* ----------------------------------------------------------------------- */
	char *					// Returned pointer to the duplicated pathname
pathDup(					// (must be freed by the caller)
  const char *pSrc)			// Pointer to the source pathname string, if provided

	{
	char *p = calloc(MAX_PATH, 1);

	if (p)
		pathCopy(p, pSrc, MAX_COPY);	// (must be freed by the caller)
	return (p);
	}

/* ----------------------------------------------------------------------- */
	char *					// Returned pointer to the duplicated pathname
fpathDup(					// (must be freed by the caller)
  const char *pSrc)			// Pointer to the source pathname string, if provided

	{
	char *p = fmalloc(MAX_PATH);	// Fatal if failed

	if (p)
		pathCopy(p, pSrc, MAX_COPY);	// (must be freed by the caller)
	return (p);
	}

/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */
