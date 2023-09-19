/* ----------------------------------------------------------------------- *\
|
|							 FWINIT - FWILD - FWFREE
|						   Wild Card File Name Server
|
|			Copyright (c) 1985, 1990, 1991, 1993, all rights reserved
|								Brian W Johnson
|									8-Jun-90
|								   14-May-91
|								   21-Feb-93
|								   17-Aug-97  Win32
|								   24-Feb-98  Memory leak fixed
|									1-Aug-06  Accept ".*" file and directory names
|
|			void *				Return a pointer to an fwild header
|		hp = fwinit (s, mode);	Initialize the wild filename system
|			char  *s;			Drive/path/filename string
|			int	   mode;		Search mode to use (FW_* from fwild.h)
|
|			char *				Return a drive/path/filename string
|		fnp = fwild (hp);		Return the next filename
|			void  *hp;			Pointer to the fwild header
|
|			void *				Always returns NULL
|		fwfree (hp);			Close and free the fwild system instance
|			void  *hp;			Pointer to the fwild header
|
|		fwfree() need only be called if aborting the fwild sequence early
|
\* ----------------------------------------------------------------------- */

#ifdef _WIN32
#include  <windows.h>
#include  <VersionHelpers.h>
#endif
#include  <stdio.h>
#include  <ctype.h>
#include  <string.h>
#include  <stdlib.h>

#define	 FWILD_INTERNAL

#include  "fwild.h"

// -----------------------------------------------------------------------

// #define  SHOWSRCH		// Define this to show search progress
// #define  VERBOSEOUT	 1	// Define this in the makefile for verbose output
// #define  MEMORYWATCH	 1	// Define this in the makefile to watch for memory leaks

#ifdef	MEMORYWATCH
#define	 mwprintf(a,b)	printf(a,b)
#else
#define	 mwprintf(a,b)
#endif

#ifdef	VERBOSEOUT
static void		m_disp (char *s1, char *s2);
static void		h_disp (PDTA_HDR p, char *s);
static void		e_disp (PDTA_ELE p, char *s, int flag);
#endif

//BWJ
//static void		pc_disp(PDTA_HDR p);

// States

// -----------------------------------------------------------------------
// Private definitions
// -----------------------------------------------------------------------

#define	 FRESH		0					/* Initial state of DTA_ELE */
#define	 NONW_F		2					/* Do findf for not wild */
#define	 NONW_N		3					/* Do findn for not wild */
#define	 NONW_T		4					/* Non-wild transition state */
#define	 WILD_F		5					/* Do findf for ordinary wild */
#define	 WILD_N		6					/* Do findn for ordinary wild */
#define	 RECW_F		7					/* Do findf for recursive wild */
#define	 RECW_N		8					/* Do findn for recursive wild */
#define	 RECW_T		9					/* Recursive wild transition state */

#define	 NOT_WILD	0					/* Not a wild expression */
#define	 ORD_WILD	1					/* Ordinary wild expression */
#define	 REC_WILD	2					/* Recursive wild expression */

#define	 FW_ALL		(FW_HIDDEN | FW_SYSTEM | FW_SUBD)
#define	 FW_FLS		(FW_HIDDEN | FW_SYSTEM | FW_FILE)

#define PATHCH	('\\')
#define NULCH	('\0')

typedef enum		// Internal error codes
	{
	NO_MEM = 1,		// Insufficient memory
	EP_ERR,			// Element ptr error
	INV_STATE,		// Invalid state
	INV_UNC,		// Invalid UNC syntax
	INV_DRV,		// Invalid drive syntax
	INV_RWILD,		// Invalid RWILD syntax
	INV_PATH		// Invalid path syntax
	} ERR_CODE;

#define ONENTRY (0)
#define ONEXIT  (1)

// -----------------------------------------------------------------------
// Private variables
// -----------------------------------------------------------------------

#ifdef MEMORYWATCH
static	unsigned int	AllocCount = 0;
#endif

static  char	 rwild_str [] = "\\**";
static  char	 owild_str [] = "\\*";

int			    xporlater = 0;					// TRUE if Windows XP or later (global)

/* ----------------------------------------------------------------------- */
// Default file exclusion management

// Force the inclusion of the corrected DTOXTIME library file
// Removed because no longer in use
//extern int	ForceDtoxtime;
//static int *pForceDtoxtime = &ForceDtoxtime;

// -----------------------------------------------------------------------
// Private methods
// -----------------------------------------------------------------------

/* ----------------------------------------------------------------------- */
	static void
fat_err (						// Report fatal internal error and die
	ERR_CODE ec,				// Error code
	char    *s)					// Relevent path string

	{
	char  *p;

	switch (ec)
		{
		case NO_MEM:
			p = "Insufficient memory";	break;

		case EP_ERR:
			p = "Element ptr error";	break;

		case INV_STATE:
			p = "Invalid state";		break;

		case INV_UNC:
			p = "Invalid UNC";			break;

		case INV_DRV:
			p = "Invalid drive";		break;

		case INV_RWILD:
			p = "Invalid RWILD";		break;

		case INV_PATH:
			p = "Invalid path";			break;

		default:
			p = "Invalid error code";
		}
	fprintf(stderr, "Fwild-F-%s\n\7", p);

	if (s)
		fprintf(stderr, "%s\n", s);

	exit(1);
	}

/* ----------------------------------------------------------------------- *\
|  Copy n bytes
\* ----------------------------------------------------------------------- */
	static void
copyn (p2, p1, n)				/* Copy n bytes from p1 to p2 */
	char  *p2;					/* and NULL terminate the string */
	char  *p1;
	int	   n;

	{
	while (n--)
		*(p2++) = *(p1++);
	*p2 = '\0';
	}

/* ----------------------------------------------------------------------- */
	static char *				/* Return a pointer to the new block */
new_object (size)				/* Allocate a block of memory */
	int	 size;					/* Size of memory to allocate */

	{
	char  *p;

	if ((p = calloc(size, 1)) == NULL)
		fat_err(NO_MEM, NULL);
#ifdef MEMORYWATCH
	else
		++AllocCount;
#endif
	return (p);
	}

/* ----------------------------------------------------------------------- */
	static char *				/* Always returns NULL */
dispose_object (				/* Free a block of object memory */
	char  *s)					/* Pointer to object to free */

	{
	if (s != NULL)
		{
		free(s);
#ifdef MEMORYWATCH
		--AllocCount;
#endif
		}
	return (NULL);
	}

/* ----------------------------------------------------------------------- */
	static DTA_HDR *			/* Return a pointer to the new DTA header */
new_header ()

	{
	DTA_HDR *hp = (DTA_HDR *)(new_object(sizeof(DTA_HDR)));
mwprintf("New header %d\n", AllocCount);
	return (hp);
	}

/* ----------------------------------------------------------------------- */
	static DTA_ELE *			/* Return a pointer to the new DTA element */
new_element ()					/* Allocate an initialized DTA element */

	{
	DTA_ELE	 *ep;

	ep = (DTA_ELE *)(new_object(sizeof(DTA_ELE)));
	ep->pLink	   = NULL;
	ep->wild	   = NOT_WILD;
	ep->state	   = FRESH;
	ep->pLast	   = NULL;
	ep->pNext	   = NULL;
	ep->search[0]  = '\0';
	ep->pattern[0] = '\0';
	ep->proto[0]   = '\0';
	ep->found[0]   = '\0';
mwprintf("New element %d\n", AllocCount);
	return (ep);
	}

/* ----------------------------------------------------------------------- */
	static DTA_ELE *			/* Returned pointer to the successor element */
unnest_element (				/* Unnest a DTA element */
	DTA_ELE	 *ep)				/* Pointer to the element */

	{
	DTA_ELE	 *epLink;			/* Pointer to the next element */

	if (ep != NULL)
		{
		epLink = ep->pLink;						/* Point the successor */
		dispose_object((char *)(ep));			/* Then, release the element */
mwprintf("Free element %d\n", AllocCount);
		}
	else
		epLink = NULL;							/* There was no element */

	return (epLink);
	}

/* ----------------------------------------------------------------------- */
	static void
release_header (hp)				/* Release a header and all its elements */
	DTA_HDR	 *hp;				/* Pointer to the header */

	{
	while (hp->pLink != NULL)					/* Release all DTA elements */
		hp->pLink = unnest_element(hp->pLink);

	dispose_object((char *)(hp));						/* Then, release the header */
mwprintf("Free header %d\n", AllocCount);
	}

/* ----------------------------------------------------------------------- */
	static void
CheckVersion (void)

	{
	xporlater = IsWindowsXPOrGreater();
    }

/* ----------------------------------------------------------------------- *\
|  build_fn () - Build the found pathname from the DTA
\* ----------------------------------------------------------------------- */
	static void
build_fn (ep)					/* Build the found filename in the DTA */
	DTA_ELE	 *ep;

	{
	int	  n;
	char  ch;

	if (ep->pLast)
		{
		n = (int)(ep->pLast - ep->proto) + 1;
		copyn(ep->found, ep->proto, n);
		if (strlen(ep->found) > 0)
			{
			ch = ep->found[strlen(ep->found) - 1];
			if ((ch != ':')	&&	(ch != '/')	&&	(ch != '\\'))
				strcat(ep->found, "\\");
			}
		strcat(ep->found, ep->dta.dta_name);
		}
	else
		{
		strcpy(ep->found, ep->dta.dta_name);
		}
	}

// ------------------------------------------------------------------------------------------------
// Pattern compiler
// ------------------------------------------------------------------------------------------------
	static void
PatternCompiler (				// Compile the caller's pattern
	DTA_HDR	 *hp)				// Pointer to the DTA header

	{
	char  *pRawPattern = hp->rawPattern;	// Ptr to the caller's raw search pattern
	int    insertSeparator = FALSE;			// Set TRUE to prefix a path char before a segment copy


	int			level	 = 0;					// Start in level 0
	PDTA_SEG	pSeg	 = &hp->segment[level];	// Pointer to path origin
	char	   *pSegBuff = pSeg->segBuffer;		// Pointer to the current segment

	char	   *pPatSrc;					// Ptr to rawPattern copy origin
	char	   *pScan;						// Scan Ptr into the origin string
	char	   *pSegDst;					// Ptr to segment destination buffer string
	char	   *pResult;					// Temporary result value

//?? Deal with quotes here ?

	pScan = hp->rawPattern;

	// Compile level 0 (if a UNC spec)

	if ((pResult = QueryUNCPrefix(pScan)) != NULL)
		{
//		(pScan == hp->rawPattern now)		// pScan is already initialized
		pSeg->type = TYPE_UNC;				// Set segment 0 to UNC type
		pScan      = pResult;				// (pScan now points past the prefix)

		pPatSrc	   = hp->rawPattern;
		pSegDst	   = pSegBuff;				// Point the segment string begin
		while (pPatSrc < pScan)				// Copy the UNC prefix string
			*(pSegDst++) = *(pPatSrc++);
		*pSegDst = NULCH;					// Terminate the segment string

		pSeg->pEnd   = pSegDst;						// Points to string trailing NUL
		pSeg->length = (int)(pSegDst - pSegBuff);	// Standard string length

		hp->rooted = TRUE;					// All UNC paths are rooted, by definition
		}

	// Compile level 0 (if a drive spec(s)

	else if ((pResult = QueryDrivePrefix(pScan, FALSE)) != NULL)	// Multiple mode
		{
//		(pScan == hp->rawPattern now)		// pScan is already initialized
		pSeg->type = TYPE_DRIVE;			// Set segment 0 to DRIVE type
		pScan      = pResult;				// (pScan now points past the prefix)

		pPatSrc	   = hp->rawPattern;
		pSegDst	   = pSegBuff;				// Point the segment string begin
		while (pPatSrc < pScan-1)			// Copy the DRIVE prefix string (but not the ':')
			*(pSegDst++) = *(pPatSrc++);
		*pSegDst = NULCH;					// Terminate the segment string

		pSeg->pEnd   = pSegDst;						// Points to string trailing NUL
		pSeg->length = (int)(pSegDst - pSegBuff);	// Standard string length

		hp->rooted = (*pScan == PATHCH);	// Check if rooted
		if (hp->rooted)
			++pScan;						// Skip over the root separator
		}

	// Compile level 0 (if no prefix (other than a possible root separator) present)

	else
		{
//		(pScan == hp->rawPattern now)		// pScan is already initialized
		pSeg->type   = TYPE_EMPTY;
		pSeg->pEnd   = NULL;
		pSeg->length = 0;
		pSegBuff[0]  = NULCH;

		hp->rooted   = (*pScan == PATHCH);	// Check if rooted
		if (hp->rooted)
			++pScan;						// Skip over the root separator
		}

	// End of level 0 compilation

	// Compile level 1 and above, if present

//	(pScan points the first character of the first segment following the UNC/DRIVE/Root specs
	for (;;)								// Compile all remaining levels
		{
		if (*pScan == NULCH)				// Pattern exhausted,
			{
			pSeg->terminal = TRUE;			// Mark (still prev) segment terminal if no further pattern now
			break;							// Compiling is complete
			}
		if (*pScan == PATHCH)				// Should be a path char (except the first time)
			++pScan;						// Skip over the path char

		pSeg	 = &hp->segment[++level];	// Advance to the next segment level
		pSegBuff = pSeg->segBuffer;
		pPatSrc  = pScan;

		// Process a possible recursive wild segment
		// pScan points to the first char after the separator (if any)

		if ((((*(pPatSrc+0)) == '*')   &&   ((*(pPatSrc+1)) == '*'))
		&&  (((*(pPatSrc+2)) == NULCH)  ||  ((*(pPatSrc+2)) == PATHCH)))
			{
			pScan += 2;						// Recursive wild segment found

			pSeg->type = TYPE_RWILD;
			pSegDst    = pSegBuff;

			while (pPatSrc < pScan)				// Copy the segment body string
				*(pSegDst++) = *(pPatSrc++);
			*pSegDst = NULCH;					// Terminate the segment string

			pSeg->pEnd   = pSegDst;						// Points to trailing NUL
			pSeg->length = (int)(pSegDst - pSegBuff);	// Standard string length

			// At runtime, we will copy segments with a preceding separator when...

			pSeg->separator = 
				   ((level > 1)
				|| ((level == 1)  &&  (hp->segment[0].type != TYPE_UNC)  &&  (hp->rooted)));
			continue;
			}

		// Process a nonrecursive segment
		// pScan points to the first char after the separator (if any)

		pSeg->type = TYPE_NONWILD;	// Starting assumption, until determined wrong
		pPatSrc    = pScan;

		for (char ch; (isValidPath(ch = *pScan)); ++pScan)
			{
			if (ch == PATHCH)
				break;

			else if (ch == '?')
				pSeg->type = TYPE_WILD;			// '?' makes it wild

			else if (ch == '*')
				{
				pSeg->type = TYPE_WILD;			// '*' makes it wild,

				if (*(pScan + 1) == '*')		// but, can't be recursive
					fat_err(INV_RWILD, hp->rawPattern);	// Report invalid RWILD syntax
				}
			}	// Scan complete

//printf("scanned %X %X\n\n", (UINT)pPatSrc, (UINT)pScan);

		if ((*pScan != NULCH)  &&  (*pScan != PATHCH))
			fat_err(INV_PATH, hp->rawPattern);	// Report invalid path syntax

		// If this segment and the preceding segment are both TYPE_NONWILD, and
		// the preceding segment will not be a search term for a preceding wild
		// segment, concatenate this segment to the preceding segment (with separator)
		// (and, don't consult level 0.)

		if ((level >= 2)
		&&  (hp->segment[level  ].type == TYPE_NONWILD)
		&&  (hp->segment[level-1].type == TYPE_NONWILD)
		&&  ((level <= 2)  ||  (hp->segment[level-2].type != TYPE_RWILD)))
			{
			pSeg	 = &hp->segment[--level];	// Return to the previous level
			pSegBuff = pSeg->segBuffer;

			pSegDst = pSeg->pEnd;				// Concatenate to previous segment level
			insertSeparator = TRUE;				// Require separator for concatenation
//printf("prev\n\n");
			}
		else
			{
			pSegDst = pSegBuff;					// Copy into the current level
			insertSeparator = FALSE;			// Require separator for concatenation
//printf("cur\n\n");
			}

		// Copy this segment string to the selected segment

		if (insertSeparator)
			*pSegDst++ = PATHCH;				// Write a required separator

		while (pPatSrc < pScan)					// Copy the segment body string
			*(pSegDst++) = *(pPatSrc++);
		*pSegDst = NULCH;						// Terminate the segment string
//printf("copied %X\n\n", (UINT)pSegDst);
		pSeg->pEnd   = pSegDst;						// Points to trailing NUL
		pSeg->length = (int)(pSegDst - pSegBuff);	// Standard string length

		// At runtime, we will copy segments with a preceding separator when...

		pSeg->separator = 
			   ((level > 1)
			|| ((level == 1)  &&  (hp->segment[0].type != TYPE_UNC)  &&  (hp->rooted)));
		continue;
		}

//BWJ pc_disp(hp);								// Display monitor data on the way out
	}

/* ----------------------------------------------------------------------- *\
|  fwinit () - Initialize the fwild system for a wild search
\* ----------------------------------------------------------------------- */
	DTA_HDR *					/* Return a pointer to a DTA header */
fwinit (						/* Initialize the wild filename system */
	char *pattern,				/* Drive/path/filename search string */
	int fmode)					/* Find search mode to use */

	{
	PDTA_HDR	hp;
	PDTA_ELE	ep;
	char         p [MAX_PATH];	// Working pattern string

//printf("pattern1: \'%s\'\n", pattern);
//fflush(stdout);

	if (fnreduce(pattern) < 0)		// Simplify any redundance
		return (NULL);				// Invalid pattern

//printf("pattern2: \'%s\'\n", pattern);
//fflush(stdout);

	int errCode = fwvalid(pattern);	// Validate the pattern
	if ((errCode != FWERR_NONE)  &&  (errCode != FWERR_TRAIL))
		return	(NULL);

// Make an internal working copy of the pattern

	strcpy_s(p, MAX_PATH, pattern);

	// Fix up the file name pattern string if it is incomplete.


	char *pBody = PointPastPrefix(p, TRUE);	// Skip past the prefix
	char *pTail = fntail(pBody);			// Skip to the tail

	if (fmode & FW_FILE)					// If looking for files...
		{
		if (strlen(pBody) == 0)				// If no search string body
			strcpy(pBody, "*");				// Use "*"

		else if (*pTail == NULCH)			// If ends in path character
			strcat(pTail, "*");				// Append "*"

		else if (strcmp(pTail, ".") == 0)	// If ends in "."
			strcpy(pTail, "*");				// Replace it with "*"

		else if (strcmp(pTail, "..") == 0)	// If ends in ".."
			strcat(pTail, "\\*");			// Append "\*"
		}
// BWJ PROBABLY DOESNT MAKE SENSE HERE
	else if (fmode & FW_SUBD)				// If looking only for directories...
		{
		if (strlen(pBody) == 0)				// If no search string body
			strcpy(pBody, ".");				// Use "."

		else if (*pTail == NULCH)			// If ends in path character
			strcat(pTail, ".");				// Append "."

//		else if (*pTail == NULCH)			// If ends in path character
//			strcat(pTail, "*");				// Append "*"
		}

//printf("pattern3: \'%s\'\n", p);
//fflush(stdout);

	// Allocate a DTA header, and allocate the first DTA element for
	// the DTA list.  Initialize the first element with the supplied
	// prototype file name pattern.

	hp = new_header();
	hp->xmode = 0;					// File exclusion mode, updated by fexclude
	hp->mode  = fmode;				// Find mode to use
	hp->pLink = ep = new_element();
	strcpy(hp->rawPattern, p);

	PatternCompiler(hp);			// Compile the caller's pattern

//BWJ to be removed eventually (after compiiled pattern is used)
	strcpy(ep->proto, p);

	fexcludeInit(&(hp->xmode));		// Init the file exclusion system, effective once only

#ifdef	VERBOSEOUT
h_disp(hp, "FWINIT");
#endif

	return (hp);
	}

/* ----------------------------------------------------------------------- *\
|  fwfree () - Close and free the fwild system instance
\* ----------------------------------------------------------------------- */
	DTA_HDR *					/* Always returns NULL */
fwfree (						/* Close and free the fwild system instance */
	DTA_HDR	 *hp)				/* Pointer to the DTA header */

	{
	fexcludeClean();			// Terminate the fexclude mechanism
	release_header(hp);
mwprintf("Allocs (done) %d\n", AllocCount);
	return (NULL);
	}

/* ----------------------------------------------------------------------- */

/* ----------------------------------------------------------------------- *\
|  Public Methods
\* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- *\
|  fwild () - Request the next (or first) matching pathname
\* ----------------------------------------------------------------------- */
	char *						/* Return a drive/path/filename string */
fwild (							/* Find the next filename */
	DTA_HDR	 *hp)				/* Pointer to the DTA header */

	{
	DTA_ELE	 *ep;
	DTA_ELE	 *xep;
	int		  findFail ;
	int		  chflag;
	int		  m_mode;
	char	 *p;
	char	 *ps;

	if (hp == NULL)
		return (NULL);

	if ((ep = hp->pLink) == NULL)
		fat_err(EP_ERR, NULL);

	for (;;)
		{

#ifdef	VERBOSEOUT
e_disp(ep, "Main switch", ONENTRY);
#endif

		switch (ep->state)
			{

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

			case FRESH:			/* Freshly initialized DTA element */
				p = ep->proto;
				chflag = FALSE;
				do	{
					if (*p == '?')
						{
						if (ep->wild != REC_WILD)
							ep->wild = ORD_WILD;
						}
					else if (*p == '*')
						{
						if ((chflag == FALSE)  &&  (*(p + 1) == '*'))
							{
							++p;
							ep->wild = REC_WILD;
							}
						else
							ep->wild = ORD_WILD;
						}
					else if ((*p == '/') || (*p == ':') || (*p == '\\'))
						{
						chflag = FALSE;
						if (ep->wild)
							break;
						ep->pLast = p;	// Point the last replaced proto separator
						}
					else
						chflag = TRUE;
					} while (*(++p));	// Point end of active proto string
				ep->pNext = p;			// Point end of the proto string

				if (ep->wild)
					{
					int  n;			// Copy length

					if (ep->wild == REC_WILD)	/* Make match pattern */
						ep->state = RECW_F;
					else
						{
						ep->state = WILD_F;
						if (ep->pLast)
							{
							n =(int)((p - (ep->pLast + 1)));
							copyn(ep->pattern, (ep->pLast + 1), n);
							}
						else
							{
							n = (int)(p - (ep->proto));
							copyn(ep->pattern, ep->proto, n);
							}
						}
					if (ep->pLast)				/* Make search pattern */
						{
						n = (int)((ep->pLast + 1) - (ep->proto));
						copyn(ep->search, ep->proto, n);
						strcat(ep->search, "*");
						}
					else
						strcpy(ep->search, "*");
					}
				else			/* Not wild, make only a search string */
					{
					int  n;		// Copy length

					ep->state = NONW_F;
					ps = ep->search;
					n = (int)(p - ep->proto);
					copyn(ps, ep->proto, n);
// printf("proto: %s\n", ep->proto	? ep->proto	 : "null" );
// printf("last:  %s\n", ep->pLast	? ep->pLast	 : "null" );
// printf("next:  %s\n", ep->pNext	? ep->pNext	 : "null" );
// printf("srch:  %s\n", ep->search ? ep->search : "null" );

					if (fnchkunc(ps))	 // UNC fixup
						{
#ifdef SHOWSRCH
printf("srch:  %s\n", ep->search ? ep->search : "null" );
#endif
						if (_findf(&ep->dta, ps, (hp->mode & FW_ALL) | FW_SUBD) != 0)
							{
							strcat(ps, "\\*");
							ep->state = NONW_F;
							ep->pLast  = ep->proto + strlen(ep->proto);
							}
						}
					}
				break;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

			case RECW_F:						/* Recursive wild case */
			case RECW_N:

#ifdef	VERBOSEOUT
e_disp(ep, "RECW", ONENTRY);
#endif
				if (ep->state == RECW_F)		/* Search for file */
					{
#ifdef SHOWSRCH
printf("srch:  %s\n", ep->search ? ep->search : "null" );
#endif
					findFail  = _findf(&ep->dta, ep->search, (hp->mode & FW_ALL) | FW_SUBD);
					ep->state = RECW_N;
					}
				else
					findFail  = _findn(&ep->dta);

				if (findFail )						/* If no file found... */
					{
					if ((hp->pLink = ep = unnest_element(ep)) == NULL)
						{
						fwfree(hp);				/* Terminate the object */
						return (NULL);			/* Return failure */
						}
					break;						/* Continue unnested */
					}


#ifdef	VERBOSEOUT
e_disp(ep, "RECW", ONEXIT);
#endif

				if (ep->dta.dta_type & ATT_SUBD)		/* Check if SUBD */
					{
					if (*ep->pNext)			/* Process non-terminal subd */
						{
						if (fnmatch2(ep->pNext + 1, ep->dta.dta_name))
							{
							if (hp->exActive && (hp->xmode & XD_BYNAME) && fexcludeCheck(ep->dta.dta_name))
								break;
							build_fn(ep);
							if (hp->exActive && (hp->xmode & XD_BYPATH) && fexcludeCheck(ep->found))
								break;

							ep->state = RECW_T;
							if (hp->mode & FW_SUBD)
								{
								ep->fdt = fgetfdt(ep->found);
								return (ep->found);
								}
							}
						}
					else // (not *ep->pNext)					/* Process terminal subd */
						{
						if (hp->exActive && (hp->xmode & XD_BYNAME) && fexcludeCheck(ep->dta.dta_name))
							break;
						build_fn(ep);		/* Terminal match case */
						if (hp->exActive && (hp->xmode & XD_BYPATH) && fexcludeCheck(ep->found))
							break;

						ep->state = RECW_T;
						if (hp->mode & FW_SUBD)
							{
							ep->fdt = fgetfdt(ep->found);
							return (ep->found);
							}
						}
					ep->state = RECW_T;
					break;
					}

				else if (hp->mode & FW_FLS)		/* Process file case */
					{
					if (*ep->pNext)				/* Process non-terminal file */
						{
						if (fnmatch2(ep->pNext + 1, ep->dta.dta_name))
							{
							if (hp->exActive && (hp->xmode & XF_BYNAME) && fexcludeCheck(ep->dta.dta_name))
								break;
							build_fn(ep);		/* Terminal match case */
							if (hp->exActive && (hp->xmode & XF_BYPATH) && fexcludeCheck(ep->found))
								break;

							ep->fdt = fgetfdt(ep->found);
							return (ep->found);
							}
						}
					else // (not *ep->pNext)		/* Process terminal file */
						{
						if (hp->exActive && (hp->xmode & XF_BYNAME) && fexcludeCheck(ep->dta.dta_name))
							break;
						build_fn(ep);		/* Terminal match case */
						if (hp->exActive && (hp->xmode & XF_BYPATH) && fexcludeCheck(ep->found))
							break;

						ep->fdt = fgetfdt(ep->found);
						return (ep->found);
						}
					}
				break;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

			case RECW_T:

				ep->state = RECW_N;
//				if (ep->dta.dta_name[0] != '.')
				if ((strcmp(ep->dta.dta_name,  ".") != 0)
				&&	(strcmp(ep->dta.dta_name, "..") != 0))
					{			/* Wild "." and ".." don't nest */
					build_fn(ep);
					xep = ep;			/* Nest the DTA element list */
					hp->pLink = ep = new_element();
					ep->pLink = xep;
					strcpy(ep->proto, xep->found);
					strcat(ep->proto, rwild_str);
					strcat(ep->proto, xep->pNext);
					}
				break;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


			case WILD_F:						/* Ordinary wild case */
			case WILD_N:

#ifdef	VERBOSEOUT
e_disp(ep, "WILD", ONENTRY);
#endif
				if (ep->state == WILD_F)		/* Search for file */
					{
#ifdef SHOWSRCH
printf("srch:  %s\n", ep->search ? ep->search : "null" );
#endif
					findFail  = _findf(&ep->dta, ep->search, (hp->mode & FW_ALL) | FW_SUBD);
					ep->state = WILD_N;
					}
				else
					findFail  = _findn(&ep->dta);

				if (findFail )						/* If no file found... */
					{
					if ((hp->pLink = ep = unnest_element(ep)) == NULL)
						{
						fwfree(hp);				/* Terminate the object */
						return (NULL);			/* Return failure */
						}
					break;						/* Continue unnested */
					}

#ifdef	VERBOSEOUT
e_disp(ep, "WILD", ONEXIT);
#endif
				if (*ep->pNext)			/* If terminal name, set match mode */
					m_mode = 0;

				if (!fnmatch2(ep->pattern, ep->dta.dta_name))
					break;

				if (ep->dta.dta_type & ATT_SUBD)		/* Check if SUBD */
					{
					if (*ep->pNext)					/* Yes, process non-terminal subd */
						{
						if ((strcmp(ep->dta.dta_name,  ".") == 0)	// No
						||	(strcmp(ep->dta.dta_name, "..") == 0))
							break;		/* Wild "." and ".." don't nest */

						if (hp->exActive && (hp->xmode & XD_BYNAME) && fexcludeCheck(ep->dta.dta_name))
							break;
						build_fn(ep);
						if (hp->exActive && (hp->xmode & XD_BYPATH) && fexcludeCheck(ep->found))
							break;
						xep = ep;				/* Nest the DTA element list */
						hp->pLink = ep = new_element();
						ep->pLink = xep;
						strcpy(ep->proto, xep->found);
						strcat(ep->proto, xep->pNext);
						}
					else // (not *ep->pNext)		/* Process terminal subd */
						{
						if (hp->exActive && (hp->xmode & XD_BYNAME) && fexcludeCheck(ep->dta.dta_name))
							break;
						build_fn(ep);
						if (hp->exActive && (hp->xmode & XD_BYPATH) && fexcludeCheck(ep->found))
							break;

						if (hp->mode & FW_SUBD)	/* If reading subd's... */
							{
							ep->fdt = fgetfdt(ep->found);
							return (ep->found);
							}
						}
					}

				else								/* Process file */
					{
					if ((*ep->pNext == '\0')  &&  (hp->mode & FW_FLS))
						{
						if (hp->exActive && (hp->xmode & XF_BYNAME) && fexcludeCheck(ep->dta.dta_name))
							break;
						build_fn(ep);
						if (hp->exActive && (hp->xmode & XF_BYPATH) && fexcludeCheck(ep->found))
							break;

						ep->fdt = fgetfdt(ep->found);
						return (ep->found);
						}
					}
				break;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

			case NONW_F:
			case NONW_N:

#ifdef	VERBOSEOUT
e_disp(ep, "NONW", ONENTRY);
#endif
				if (ep->state == NONW_F)
					{
// printf("\nNONW: findf on \"%s\" (2)\n", ep->search);
#ifdef SHOWSRCH
printf("srch:  %s\n", ep->search ? ep->search : "null" );
#endif
					findFail  = _findf(&ep->dta, ep->search, (hp->mode & FW_ALL) | FW_SUBD);
					}

				if (ep->state == NONW_N)
					{
// printf("\nNONW: findn on \"%s\" (3)\n", ep->search);
					findFail  = _findn(&ep->dta);
					}

				ep->state = NONW_N;

				if (findFail )						/* If no file found... */
					{
					if ((hp->pLink = ep = unnest_element(ep)) == NULL)
						{
						fwfree(hp);				/* Terminate the object */
						return (NULL);			/* Return failure */
						}
					break;						/* Continue unnested */
					}

#ifdef	VERBOSEOUT
e_disp(ep, "NONW", ONEXIT);
#endif

				if (ep->dta.dta_type & ATT_SUBD)
					{
					if (hp->exActive && (hp->xmode & XD_BYNAME) && fexcludeCheck(ep->dta.dta_name))
						break;
					build_fn(ep);
					if (hp->exActive && (hp->xmode & XD_BYPATH) && fexcludeCheck(ep->found))
						break;

					ep->state = NONW_T;
					if (hp->mode & FW_SUBD)
						{
						ep->fdt = fgetfdt(ep->found);
						return (ep->found);	/* Return the directory */
						}
					break;						// Return to top of loop
					}
				else if (hp->mode & FW_FLS)		// And is a file (because not a directory)
					{
					if (hp->exActive && (hp->xmode & XF_BYNAME) && fexcludeCheck(ep->dta.dta_name))
						break;
					build_fn(ep);
					if (hp->exActive && (hp->xmode & XF_BYPATH) && fexcludeCheck(ep->found))
						break;

					ep->fdt = fgetfdt(ep->found);
					return (ep->found);		/* Return the filename */
					}
				break;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

			case NONW_T:

// printf("\nNONW_T: entered, mode = %04X, DTA.type = %04X\n", hp->mode, ep->dta.dta_type);
				ep->state = NONW_N;
				if (hp->mode & (FW_FILE | FW_ALL))
					{
// printf("\nNONW_T: nesting\n");
					xep = ep;			/* Nest the DTA element list */
					hp->pLink = ep = new_element();
					ep->pLink = xep;
					strcpy(ep->proto, xep->found);
					strcat(ep->proto, owild_str);
					}
				break;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

			default:
				fat_err(INV_STATE, NULL);
			}					// End of the state switch table
		}						// Return to top of the state loop

//	fwfree(hp);					// Terminate the object (if it were a real exit)
	return	(NULL);				// Dummy to make the compiler happy, never taken
	}

/* ----------------------------------------------------------------------- *\
|  fwExclEnable () - Enable the file exclusion mechanism for the fwild object
\* ----------------------------------------------------------------------- */
	void
fwExclEnable (			// Enable/disable file exclusion
	DTA_HDR	 *hp,		// Pointer to the DTA header
	int       enable)	// TRUE to enable exclusion

	{
	if (hp)
		hp->exActive = enable;
	}

/* ----------------------------------------------------------------------- */

/* ----------------------------------------------------------------------- */
#ifdef	VERBOSEOUT
	static void
m_disp(s1, s2)
	char  *s1;
	char  *s2;

	{
	printf("\n");
	printf("Match strings:\n");

	if (s1)
		printf("String1..%s\n", s1);
	else
		printf("String1 is NULL\n");

	if (s2)
		printf("String2..%s\n", s2);
	else
		printf("String2 is NULL\n");

	printf("\n");
	fflush(stdout);
	}

#endif
/* ----------------------------------------------------------------------- */
#ifdef	VERBOSEOUT
	static void
h_disp(hp, s)
	DTA_HDR	 *hp;
	char	 *s;

	{
	printf("\n");
	printf("From: %s\n", s);
	printf("Pointer....%04x\n", (int)(INT64)(hp));
	printf("Link.......%04x\n", (int)(INT64)(hp->pLink));
	printf("Mode.......%04x\n", (int)(INT64)(hp->mode));
	printf("\n");
	fflush(stdout);
	}

#endif
/* ----------------------------------------------------------------------- */
#ifdef	VERBOSEOUT
	static void
e_disp(ep, s, flag)
	DTA_ELE	 *ep;				/* Pointer to the DTA element */
	char	 *s;				/* Originination string */
	int		  flag;				/* True for a search result */

	{
	printf("\n");

	if (flag == (ONEXIT))
		printf("Result from: %s\n", s);
	else //  == (ONENTRY)
		printf("Entry to: %s\n", s);

	printf("Pointer____%04x\n", (int)(INT64)(ep));

	printf("Link_______%04x\n", (int)(INT64)(ep->pLink));

	switch (ep->wild)
		{
		case NOT_WILD:	printf("Wild_______NOT_WILD\n");  break;
		case ORD_WILD:	printf("Wild_______ORD_WILD\n");  break;
		case REC_WILD:	printf("Wild_______REC_WILD\n");  break;
		default:		printf("Wild_______BAD\n");
		}

	switch (ep->state)
		{
		case FRESH:		printf("State______FRESH\n");	break;
		case NONW_F:	printf("State______NONW_F\n");	break;
		case NONW_N:	printf("State______NONW_N\n");	break;
		case NONW_T:	printf("State______NONW_T\n");	break;
		case WILD_F:	printf("State______WILD_F\n");	break;
		case WILD_N:	printf("State______WILD_N\n");	break;
		case RECW_F:	printf("State______RECW_F\n");	break;
		case RECW_N:	printf("State______RECW_N\n");	break;
		case RECW_T:	printf("State______RECW_T\n");	break;
		default:		printf("State______BAD\n");
		}

	if (ep->proto[0])
		printf("Prototype__%s\n",	ep->proto);

	if (ep->pLast)
		printf("Last_______%3lld\n", (ep->pLast - ep->proto));
	else
		printf("Last_______  0\n");

	if (ep->pNext)
		printf("Next_______%3lld\n", (ep->pNext - ep->proto));
	else
		printf("Next_______  0\n");

	if (ep->search)
		printf("Search_____%s\n",	ep->search);

	if (ep->pattern)
		printf("Pattern____%s\n",	ep->pattern);

	if (ep->found[0])
		printf("Found______%s\n",	ep->found);

	if (flag)
		{
		printf("DTA.name___%s\n",	ep->dta.dta_name);
		printf("DTA.type___%04X\n", ep->dta.dta_type);
		printf("DTA.size___%u\n",	(int)(ep->dta.dta_size));
		}

	printf("\n");
	fflush(stdout);
	}

#endif
/* ----------------------------------------------------------------------- */
#if 1	//VERBOSEOUT
	static void
pc_disp (
	PDTA_HDR  hp)				/* Pointer to the DTA element */

	{
	printf("\n");

	printf("Raw pattern: \"%s\"\n", hp->rawPattern);
	printf("Rooted:      %s\n\n", (hp->rooted ? "TRUE" : "FALSE"));

	for (int i = 0; (i < LEVEL_MAX); ++i)
		{
		PDTA_SEG pSeg = &hp->segment[i];
		SegType  t = pSeg->type;

		if ((i >= 2)  &&  (pSeg->pEnd == 0))
			break;

		printf("Level     %d\n", i);
		printf("Type      %s\n", (	(t == TYPE_UNKNOWN)	? "Unknown" :
									(t == TYPE_EMPTY)	? "Empty"   :
									(t == TYPE_DRIVE)	? "Drive"   :
									(t == TYPE_UNC)		? "UNC"     :
									(t == TYPE_NONWILD)	? "NONWILD" :
									(t == TYPE_WILD)	? "WILD"    :
									(t == TYPE_RWILD)	? "RWILD"   : "Bad"));
		printf("Terminal:  %s\n", (pSeg->terminal ? "TRUE" : "FALSE"));
		printf("Separator: %s\n", (pSeg->separator ? "TRUE" : "FALSE"));
		printf("Length:    %d\n", pSeg->length);
#ifdef _WIN64
		printf("pEnd:      %llX\n", (UINT64)(pSeg->pEnd));
		printf("Buffer:    %llX\n", (UINT64)(pSeg->segBuffer));
#else
		printf("pEnd:      %X\n", (UINT32)(pSeg->pEnd));
		printf("Buffer:    %X\n", (UINT32)(pSeg->segBuffer));
#endif
		printf("String:    \"%s\"\n", pSeg->segBuffer);
		printf("\n");
		}
	}

#endif
/* ----------------------------------------------------------------------- */
