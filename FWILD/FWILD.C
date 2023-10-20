/* ------------------------------------------------------------------------ *\
|
|					FWOPEN - FWACTIVE - FWINIT - FWILD - FWCLOSE
|						   Wild Card File Name Server
|
|				Copyright (c) 1985 - 2023, all rights reserved
|								Brian W Johnson
|									8-Jun-90
|								   14-May-91
|								   21-Feb-93
|								   17-Aug-97  Win32
|								   24-Feb-98  Memory leak fixed
|									1-Aug-06  Accept ".*" file and directory names
|									1-Oct-23  Major rewrite
|
|		PFWH				Return a pointer to an allocated fWild instance header
|	fwOpen (void)			Create a new fWild instance
|
|		int					TRUE iff valid
|	fwActive (				Validate the fWild instance
|		PFWH	hp)			Pointer to the FW_HDR header
|
|		int					Returns 0 if successful, else error code
|	hp = fwInit (			Initialize the wild filename system
|		char   *s;			Drive/path/filename string
|		int		CallerAttr;	Caller's search attributes (FW_* from fWild.h) to use
|
|		char *				Return a drive/path/filename string
|	fnp = fWild (			Return the next filename
|		PFWH	hp;			Pointer to the fWild instance
|
|		void
|	fwExclEnable (			Enable/disable file exclusion
|		PFWH	hp,			Pointer to the fWild instance
|		PEX		xp,			Pointer to the Excl instance
|		int		enable)		TRUE to enable exclusion
|
|		PFWH				Always returns NULL (to NULL the caller's hp)
|	fwClose (				Close and free the fWild system instance
|		PFWH	hp)			Pointer to the instance header
|
\* ------------------------------------------------------------------------ */

#include  <windows.h>
#include  <VersionHelpers.h>
#include  <errhandlingapi.h>
#include  <stdio.h>
#include  <ctype.h>
#include  <string.h>
#include  <stdlib.h>

#define	 FWILD_INTERNAL

#include  "fWild.h"

//----------------------------------------------------------------------------

//	#define  TRUNCATE			// Define this to truncate execution of the caller
//	#define  VERBOSEOUT			// Define this in the makefile for verbose output
	#define  BUILD_DIAGS		// Define this to include the diagnostice methods
								// This is done so printfs can use the decoders
// -------------------------

#ifdef VERBOSEOUT

#define BUILD_DIAGS
#define SHOWHDR(arg)	hdr_disp(hp, arg);
#define SHOWLEV(arg)	lev_disp(pLev, arg); seg_disp(pLev->pSeg);
#define SHOWSEG			seg_disp(pSeg);
#define SHOWDTA			dta_disp(&pLev->dta);
#define SHOWPC			pc_disp(hp);
#define PROGRESS(arg)	printf((arg)); 

#else

#define SHOWHDR(arg)
#define SHOWLEV(arg)
#define SHOWSEG
#define SHOWDTA
#define SHOWPC
#define PROGRESS(arg)

#endif

//----------------------------------------------------------------------------
//	Private definitions
//----------------------------------------------------------------------------

#define	FW_WILD	(FW_DIR)	// Standard form for the Finder complex

// Internal error code symbols

typedef enum
	{
	NO_MEM = 1,		// Insufficient memory
	INV_STATE,		// Invalid state
	INV_TYPE,		// Invalid file type
	INV_RWLD,		// Invalid RWILD syntax
	INV_PATH,		// Invalid path syntax
	INV_FOPEN,		// FinderOpen() failed
	PAT_OVF,		// Pattern level overflow
	SRCH_OVF		// Search level overflow
	} ERR_CODE;

//----------------------------------------------------------------------------
//	Forward Declarations
//----------------------------------------------------------------------------

static void		HdrInit(PFW_HDR hp, int SearchAttr, int mode);
static void		StateInit(PFW_HDR hp, PFW_LEV pLev, int segIndex, int depthUp);
static char    *stateMachine (FW_HDR *hp);

//----------------------------------------------------------------------------
//	Private variables
//----------------------------------------------------------------------------

// Because this fWild library is intended to support multiple
// fWild instances within a single instance of the application,
// there must not be any private (application static) variables
// having long term scope.
// Static things used during a function call, and not expected
// to survive between calls, are OK, because this library
// is not designed to be used by multiple threads.

//----------------------------------------------------------------------------
//	Diagnostics
//----------------------------------------------------------------------------
//	Translate file search attributes to a diagnostic string
//----------------------------------------------------------------------------
#ifdef	VERBOSEOUT

#define showSrchAttr showFileAttr

//----------------------------------------------------------------------------
//	Display a graphic concatenation pointer in a diagnostic string
//----------------------------------------------------------------------------
	static char *
concatDisp (
	int offset)

	{
static	char	offsetb [60];	/* The returned caret string */

	int   count = min(50, (offset + 1));
	char *p = offsetb;

	while (count-- > 0)
		*p++ = ' ';
	*p++ = '^';
	sprintf(p, " (%d)", offset);

	return (offsetb);
	}

//----------------------------------------------------------------------------
//	Translate SM state to a diagnostic string
//----------------------------------------------------------------------------
	static char *
dispState (
	int state)

	{
	char *p;

	switch(state)
		{
		default:				p = "<???>";			break;
		case 	ST_ROOT_NT_0:	p = "Root NONT 0";		break;
		case 	ST_ROOT_NT_1:	p = "Root NONT 1";		break;
		case 	ST_ROOT_TL_0:	p = "Root TERM 0";		break;
		case 	ST_NORM_NT_0:	p = "Normal NONT 0";	break;
		case 	ST_NORM_NT_1:	p = "Normal NONT 1";	break;
		case 	ST_NORM_TL_0:	p = "Normal TERM 0";	break;
		case 	ST_NORM_TL_1:	p = "Normal TERM 1";	break;
		case 	ST_NORM_TL_2:	p = "Normal TERM 2";	break;
		case 	ST_WILD_NT_0:	p = "Wild NONT 0";		break;
		case 	ST_WILD_NT_1:	p = "Wild NONT 1";		break;
		case 	ST_WILD_NT_2:	p = "Wild NONT 2";		break;
		case 	ST_WILD_TL_0:	p = "Wild TERM 0";		break;
		case 	ST_WILD_TL_1:	p = "Wild TERM 1";		break;
		case 	ST_WILD_TL_2:	p = "Wild TERM 2";		break;
		case 	ST_RWLD_NT_0:	p = "RWild NONT 0";		break;
		case 	ST_RWLD_NT_1:	p = "RWild NONT 1";		break;
		case 	ST_RWLD_NT_2:	p = "RWild NONT 2";		break;
		case 	ST_RWLD_NT_3:	p = "RWild NONT 3";		break;
		case 	ST_RWLD_TL_0:	p = "RWild TERM 0";		break;
		case 	ST_RWLD_TL_1:	p = "RWild TERM 1";		break;
		case 	ST_RWLD_TL_2:	p = "RWild TERM 2";		break;
		case 	ST_RWLD_TL_3:	p = "RWild TERM 3";		break;
		}

	return (p);
	}

//----------------------------------------------------------------------------
//	Diagnostic display the FW_HDR
//----------------------------------------------------------------------------
	static void
hdr_disp(
	PFW_HDR	hp,
	char   *pMsg)

	{
	printf("\nSearch Header - (%s)\n", pMsg);
	printf("running.....%s\n",		(hp->running ? "TRUE" : "FALSE"));
	printf("SearchAttr..%s\n",		showSrchAttr(hp->SearchAttr));
	printf("CallerAttr..%s\n",		showSrchAttr(hp->CallerAttr));
	printf("rootDepth...%d\n",		hp->rootDepth);
	printf("curLevel....%d\n",		hp->curLevel);
	printf("segCount....%d\n",		hp->segmentCount);

	printf("foundFlag...%s\n",		(hp->foundFlag ? "Success" : "Failure"));
	if (hp->foundFlag)
		{
	printf("name........%s\n",		hp->file_name);
	printf("fdt.........%s\n",		fwtime(hp));
	printf("Attr........%s\n",		showFileAttr(hp->file_type));
	printf("size........%lld\n",	hp->file_size);
		}
	printf("xI.drive....%d\n",		hp->xItem.drive);
	printf("xI.type.....%s\n",		showFileAttr(hp->xItem.type));
	printf("xI.depth....%d\n",		hp->xItem.depth);
	printf("xI.name.....%s\n",		hp->xItem.name);
	printf("\n");
	fflush(stdout);
	}

//----------------------------------------------------------------------------
//	Diagnostic display the FW_LEV
//----------------------------------------------------------------------------
	static void
lev_disp (
	PFW_LEV	pLev,
	char   *pMsg)

	{
	printf("\nSearch Level - (%s)\n", pMsg);
	printf("level.......%d\n",		pLev->index);
	printf("state.......[%s]\n",	dispState(pLev->state));
	printf("depth.......%d\n",		pLev->depth);
	printf("pSeg........Seg %d\n",	pLev->pSeg->index);
	printf("segIndex....%d\n",		pLev->segIndex);
	printf("pattern.....\"%s\"\n",	pLev->pattern);
	printf("patConcat...%s\n",		concatDisp((int)(pLev->patConcat - pLev->pattern)));
	printf("found.......\"%s\"\n",	pLev->found);
	printf("fndConcat...%s\n",		concatDisp((int)(pLev->fndConcat - pLev->found)));
	printf("match.......\"%s\"\n",	pLev->pSeg->buffer);
	printf("\n");
	fflush(stdout);
	}

//----------------------------------------------------------------------------
//	Diagnostic display the FW_DTA
//----------------------------------------------------------------------------
	static void
dta_disp (
	PFW_DTA	pDta)

	{
	char handle [20];
	
	if (((INT64)(pDta->sh)) == (-1))
		strcpy(handle, "INVALID");
	else
		sprintf(handle, "%0llX", (UINT64)(pDta->sh));
	
	printf("\nDta Block\n");
	printf("valid.......%s\n",		(pDta->valid ? "Valid" : "Empty"));
	printf("Search Attr.%s\n",		showSrchAttr(pDta->SearchAttr));
	printf("file Attr...%s\n",		showFileAttr(pDta->dta_type));
	printf("file size...%lld\n",	pDta->dta_size);
	printf("file fdt....%lld\n",	pDta->dta_fdt);
	printf("dta_name....\"%s\"\n",	pDta->dta_name);
	printf("handle......%s\n",		handle);
	printf("\n");
	fflush(stdout);
	}

//----------------------------------------------------------------------------
//	Diagnostic display the FW_SEG
//----------------------------------------------------------------------------
	static void
seg_disp (
	PFW_SEG  pSeg)				// Pointer to the segment

	{
	printf("Segment %d\n", pSeg->index);

	SegType  t = pSeg->type;

//	printf("Index.......%d\n",		(pSeg->index));
	printf("Terminal....%s\n",		(pSeg->terminal ? "Terminal" : "Non-terminal"));
	printf("Type........%s\n",	   ((t == TYPE_ROOT)	? "Root" :
									(t == TYPE_NORMAL)	? "Normal" :
									(t == TYPE_WILD)	? "Wild"   :
									(t == TYPE_RWLD)	? "Recursive wild"
														: "<???>" ));
	printf("delta.......%d\n",	 	(pSeg->delta));
//	printf("Begin:......\"%s\"\n",	(pSeg->pBegin));
	printf("Buffer:.....\"%s\"\n",	(pSeg->buffer));
	printf("\n");
	}

//----------------------------------------------------------------------------
//	Diagnostic display all compiled FW_SEG segments
//----------------------------------------------------------------------------
	static void
pc_disp (
	PFW_HDR  hp)				// Pointer to the search level

	{
	printf("\nCompiled Pattern\n\n");

	for (int i = 0; (i < hp->segmentCount); ++i)
		seg_disp(&hp->segment[i]);
	printf("Compiled Pattern End\n");
	}

#endif // BUILD_DIAGS
//----------------------------------------------------------------------------
//	Private Methods
//----------------------------------------------------------------------------
//	Explain fatal runtime errors
//----------------------------------------------------------------------------
	static void
fat_err (						// Report fatal internal error and die
	ERR_CODE ec,				// Error code
	char    *path)				// Relevent path string

	{
	char  *p;

	switch (ec)
		{
		case NO_MEM:
			p = "Insufficient memory";	break;

		case INV_STATE:
			p = "Invalid state";		break;

		case INV_RWLD:
			p = "Invalid RWILD";		break;

		case INV_PATH:
			p = "Invalid path";			break;

		case PAT_OVF:
			p = "Pattern overflow";		break;

		case SRCH_OVF:
			p = "Search overflow";		break;

		case INV_FOPEN:
			p = "FinderOpen failure";	break;

		default:
			p = "Invalid error code";
		}
	fprintf(stderr, "Fwild-F: %s\n", p);

	if (path)
		fprintf(stderr, "Path: \"%s\"\n", path);

	exit(1);
	}

//----------------------------------------------------------------------------
//	Validate a FW_HDR pointer
//----------------------------------------------------------------------------
	static int
validHeader (hp)				/* Release a header and all its elements */
	FW_HDR	 *hp;				/* Pointer to the header */

	{
	return ((hp) && (hp->magic == hp));
	}

//----------------------------------------------------------------------------
//	Allocate a new FW_HDR pointer
//----------------------------------------------------------------------------
	static FW_HDR *			// Return a pointer to the new fWild instance
newHeader (void)

	{
	PFW_HDR  hp;

	if ((hp = (PFW_HDR)(calloc(sizeof(FW_HDR), 1))) == NULL)
		fat_err(NO_MEM, NULL);
	else
		hp->magic = hp;

	return (hp);
	}

//----------------------------------------------------------------------------
//	Free a FW_HDR pointer
//----------------------------------------------------------------------------
	static void
freeHeader (hp)				// Release the instance header and all its elements
	FW_HDR	 *hp;			// Pointer to the header

	{
	hp->magic = NULL;		// Make the object invalid
	free(hp);				// Return the memory
	}

//----------------------------------------------------------------------------
//	Append a path element to a base path (specified length)
//	(does not require a separator because a base path segment might not have one)
//----------------------------------------------------------------------------
	static void
append (
	char  *pDst,		// Destination buffer
	char  *pSrc,		// Source buffer
	int    n)			// Number of bytes to copy

	{
	while (n-- > 0)
		*(pDst++) = *(pSrc++);
	*pDst = '\0';
	}

//----------------------------------------------------------------------------
//	Make a copy of the caller's pattern without parentheses
//----------------------------------------------------------------------------
	static int			// Return 0 for success, else (-1) for fail
patternCopy (
		  char *pPattern,
	const char *pOrgPattern)

	{
	char *pTail;
	
	
	if (pOrgPattern == NULL)		// Reject a NULL pointer
		return (-1);

	if (*pOrgPattern == '"')		// Remove a possible leading paarenthsis
		++pOrgPattern;

	pathCopy(pPattern, pOrgPattern, MAX_COPY);

	pTail = (pPattern + strlen(pPattern) - 1);
	if (*pTail == '"')				// Remove a possible trailing paarenthsis
		*pTail = '\0';
	return (0);
	}

//----------------------------------------------------------------------------
//	Set the initial SM state for a level within the Pattern Compiler
//----------------------------------------------------------------------------
	static void
InitialLevelState (			// Configure the Level starting state
	PFW_SEG	pSeg)

	{
	switch (pSeg->type)
		{
		default:
			{
			pSeg->levelState	 = ST_FINISHED;
			break;
			}
		case TYPE_ROOT:
			{
			if (pSeg->terminal)
				pSeg->levelState = ST_ROOT_TL_0;
			else
				pSeg->levelState = ST_ROOT_NT_0;
			break;
			}
		case TYPE_NORMAL:		// Normal non-wild segment
			{
			if (pSeg->terminal)
				pSeg->levelState = ST_NORM_TL_0;
			else
				pSeg->levelState = ST_NORM_NT_0;
			break;
			}
		case TYPE_WILD:			// Wild segment
			{
			if (pSeg->terminal)
				pSeg->levelState = ST_WILD_TL_0;
			else
				pSeg->levelState = ST_WILD_NT_0;
			break;
			}
		case TYPE_RWLD:		// Recursive wild segment
			{
			if (pSeg->terminal)
				pSeg->levelState = ST_RWLD_TL_0;
			else
				pSeg->levelState = ST_RWLD_NT_0;
			}
		}
	}

//----------------------------------------------------------------------------
//	Close a segment compilation within the Pattern Compiler
// ---------------------------------------------------------------------------
	static void
SegmentClose (
	PFW_SEG  pSeg,				// Ptr to the segment
	char	*pEnd)				// Ptr to the terminating character

	{
	int  length = (int)(pEnd - pSeg->pBegin);	// Compute the length
	
	InitialLevelState(pSeg);					// Configure the Level starting state

	switch (pSeg->type)
		{
		case TYPE_ROOT:		// ROOT segment can't be terminal (has at least a "*")
			{
				append(pSeg->buffer, pSeg->pBegin, length);		// Copy (with seg)
			pSeg->delta = 0;									// by definition
			break;
			}
		case TYPE_NORMAL:
			{
			if (pSeg->terminal)
				append(pSeg->buffer, pSeg->pBegin, length);		// Search (no seg)
			else // NOT terminal
				append(pSeg->buffer, pSeg->pBegin, length);		// Copy (with seg)
			pSeg->delta = ((strncmp(pSeg->buffer, "..", 2) == 0) ? (-1) : (1));
			break;
			}
		case TYPE_WILD:
			{
			if (pSeg->terminal)
				append(pSeg->buffer, pSeg->pBegin, length);		// Search (no seg)
			else // NOT terminal
				append(pSeg->buffer, pSeg->pBegin, length-1);	// Search (no seg)
			pSeg->delta = 1;									// by definition
			break;
			}
		case TYPE_RWLD:
			{
				append(pSeg->buffer, "*\0", 1);					// Search (no seg)
			pSeg->delta = 1;									// by definition
			break;
			}
		default:
			{
			pSeg->buffer[0] = NULCH;	// Should never happen
			break;
			}
		}
	}
	
// ---------------------------------------------------------------------------
//	Pattern Compiler
// ---------------------------------------------------------------------------
	static int					// Returns TRUE if successful, else FALSE if error
PatternCompiler (				// Compile the copy of the caller's pattern
	FW_HDR	*hp,				// Pointer to the DTA header
	char	*pPattern)			// Pointer to the search pattern

	{
	char		ch;								// Workong character
	int			rwildCount	= 0;				// Count of RWILD segments
	int			segIndex	= 0;				// Start in the first segment
	int			segOpen		= FALSE;			// TRUE when the segment is in progress
	int			terminal;						// TRUE if segment is terminal
	int			segType;						// type found when scanning
	PFW_SEG		pSeg		= NULL;				// Point the current segment descriptor
	char	   *pScan;							// Ptr to the current pattern segment
	char	   *pWork;							// Working Ptr into the current pattern segment
	char	   *pEnd;							// Ptr to the end segment name character
	
	// While scanning, open and build the segment structures when each pattern segment scan is complete.
	// Leave the current segment open for NORMAL and WILD segments, closed for an RWILD segment.
	// Always leave an open segment complete other than the terminal flag.
	// When a new segment matches the open segment update it at the end of the segment.

	// Point past the prefix, if a prefix or root separator, init the first segment 

	pScan = pPattern;							// Init the pattern scan pointer
	pWork = PointPastPrefix(pScan);				// Also skips the root separator
	ch    = *pWork;								// Get the segment termination

	segType = TYPE_ROOT;						// Configure segment type
	terminal = (ch == NULCH);					// Configure terminal property
	segOpen = (pWork > pScan);					// Normal segment (can be continued)
	if (segOpen)								// If a prefix or root separator found, open the root segment
		{
		pSeg = &hp->segment[segIndex];			// Point the first segment descriptor
		pSeg->index	   = segIndex++;			// Install the segment index (0 is always good)
		pSeg->terminal = terminal;				// Determine if this is the terminal segment
		pSeg->type	   = segType;				// If a prefix found, it is always normal
		pSeg->pBegin   = pScan;					// Point the segment begin
		SegmentClose(pSeg, pWork);
		segOpen        = FALSE;
		pEnd		   = pWork;					// Mark the segment end
		}

	// Process all remaining segments

	pScan = pWork;									// Point the next segment
	for (;;)										// Outer loop: Loop over all path segments
		{
		ch = *pWork;								// Check if a path segment has ended
		if (ch == NULCH)							// Scanning is complete
			{
			pEnd = pWork;
			if (segOpen)							// Done; if a segment is open,
				{									//   close it (can't be RWILD)
				pSeg->terminal = TRUE;				// Show this is the terminal segment
				SegmentClose(pSeg, pEnd);			// Close the segment
				}
			break;									// NULCH: terminate the outer loop
			}
		else if (ch == PATHCH)						// Continue scanning the next path segment
			{
			pEnd    = ++pWork;						// Note the buffer end point, skip over the separator
			pScan   = pWork;						// Point the next segment
			}

		segType = TYPE_NORMAL;						// Assume the coming segment will be normal
		for (;;)									// Inner loop: Process another path segment
			{
			ch = *pWork;							// Get the current character
			// Process a RWILD pattern segment
			
			if ((pWork == pScan)						// If this is the beginning of the segment
			&&  ((ch == '*') && (*(pWork+1) == '*')))	// and the segment is "**" then process as RWILD
				{
				pWork = (pScan + 2);
				ch = *(pWork);						// Check for a root separator, or terminating NULCH

				if ((ch != PATHCH)
				&&  (ch != NULCH))
					return (0);						// Invalid '**' sequence, must be followed by PATHCH or NULCH

				terminal = (ch == NULCH);			// Configure terminal

				if (segOpen)						// If the preceding segment is open, then
					{								//   close it
					SegmentClose(pSeg, pEnd);
					segOpen = FALSE;
					}

				// Build the complete RWILD segment

				pSeg = &hp->segment[segIndex];		// RWILD always gets a new segment
				pSeg->index	   = segIndex++;		// Install the segment index
				if (segIndex > SEG_MAX)				// Check for segment overflow
					fat_err(PAT_OVF, NULL);			// Fatal; no segments remaining
				pSeg->terminal = terminal;			// Configure terminal property
				pSeg->type     = TYPE_RWLD;			// RWILD by definition
				pSeg->pBegin   = pScan;				// Point the first segment char
				SegmentClose(pSeg, pWork);			// Not pEnd (because it is wrong here)
				segOpen = FALSE;					// This segment doesn't need delayed closing

				++rwildCount;						// RWILD is valid; count it (only one allowed)
				break;								// Terminate the inner loop
				}

			// Process a non-RWILD pattern segment

			if ((ch != NULCH)
			&&  (ch != PATHCH))						// End of segment not yet reached
				{
				if ((ch == '*')  ||  (ch == '?'))	// If a wild character found,
					segType = TYPE_WILD;			//   then it needs to be a WILD segment
				++pWork;							// Move on to the next character
				}

			else // the path segment has ended; type is known
				{
				if (pWork == pScan)					// if this segment is empty,
					return (0);						//   apparently an illegal PATHCH

				terminal = (ch == NULCH);			// Configure terminal

				if (segOpen)						// If a previous segment is open,
					{
					SegmentClose(pSeg, pEnd);		//   close it
					segOpen = FALSE;
					}
				
				if (! segOpen)						// If no segment is open,
					{
					pSeg = &hp->segment[segIndex];	// Point the first segment descriptor
					pSeg->index	   = segIndex++;	// Install the segment index (0 is always good)
					if (segIndex > SEG_MAX)			// Check for segment overflow
						fat_err(PAT_OVF, NULL);		// Fatal; no segments remaining
					pSeg->terminal = terminal;		// Configure terminal property
					pSeg->type	   = segType;		// If a prefix found, it is always normal
					pSeg->pBegin   = pScan;			// Point the first segment char
					segOpen        = TRUE;
					}
				break;								// Terminate the inner loop
				}

			} // Inner loop, processing one path segment
		} // Outer loop, processing inter-segment affairs

	hp->segmentCount = segIndex;

SHOWPC

	if (rwildCount > 1)
		fat_err(INV_RWLD, pPattern);			// Fatal; too many RWILD segments

	return (segIndex >= 1);						// Need at least one segment
	}

//----------------------------------------------------------------------------
//	Public Methods
//----------------------------------------------------------------------------
//	fwOpen () - Create a new fWild instance
//----------------------------------------------------------------------------
	FW_HDR *						// Return a pointer to an allocated DTA header
fwOpen (void)						// Create a new fWild instance

	{
	return (newHeader());			// Return the instance pointer
	}

//----------------------------------------------------------------------------
//	fwInit () - Initialize the fWild instance for a wild search
//----------------------------------------------------------------------------
	int								// Returns 0 for success, else FWERR_* code
fwInit (							// Initialize the wild filename system
	FW_HDR		*hp,				// Pointer to an allocated DTA header
	const char  *pOrgPattern,		// Caller's search string (Prefix.Body)
	int			 CallerAttr)		// Caller's search mode

	{
	char   *pPattern = hp->pattern;	// Working copy of pattern string


	// The instance must have been opened, and
	// the passed pattern pointer must be non-NULL (empty is OK, however)

	if (! fwActive(hp))
		return (fwerrno = FWERR_INSTANCE);		// Bad instance pointer

	if (pOrgPattern == NULL)
		return (fwerrno = FWERR_NULL);			// NULL pattern pointer

	// The target device must be valid (includes default drive)

	if (! isPhysical(pOrgPattern))
		return (fwerrno = FWERR_PHYSICAL);		// Non-physical pattern path

	// Make a temporary working copy of the pattern
	// Remove surrounding parentheses, if present

	if (patternCopy(pPattern, pOrgPattern))
		return (fwerrno = FWERR_INVALID);		// Invalid pattern pointer

	fnreduce(pPattern);							// Simplify any redundance

	strsetp(pPattern, PATHCH);

	if ((fwValid(pPattern) != FWERR_NONE)		// Validate the pattern
	&&  (fwerrno != FWERR_TRAIL))
		return (fwerrno);						// Invalid pattern

	// Setup the exclusion instance initial path depth

	if ((hp->rootDepth = GetDepth(pPattern, &hp->xItem.drive)) < 0)
		return (fwerrno = FWERR_INVALID);		// Invalid pattern content
	int SearchAttr = (FW_WILD | CallerAttr);

#ifdef VERBOSEOUT
printf("Caller attributes: %02X, ( %s)\n", CallerAttr,	showSrchAttr(CallerAttr));
printf("Search attributes: %02X, ( %s)\n", SearchAttr,	showSrchAttr(SearchAttr));
printf("Search path:  \"%s\"\n", pOrgPattern);
#endif

	// Skip past the prefix and fix up the pattern string if it is incomplete.

	char *pBody = PointPastPrefix(pPattern);
	char *pTail = _fntail(pBody);			// Skip to the tail

#if 0	// Design for the following algorithm
	If caller wants:		FILE		DIR			FILE || DIR
	if no body				use "*"		use "*"		use "*"
	if body is "."			use "*"		use "*"		use "*"
	if body is ".."			append "*"	as is		append "*"
	if path wild			as is		as is		as is
	if path is a file		as is		as is		as is
	if path is a dir		append "*"	as is		as is
#endif

	if (strlen(pBody) == 0)					// If no search string body,
		strcpy(pBody, "*");					//   use "*"

	else if (strcmp(pBody, ".") == 0)		// If the body is ".",
		strcpy(pBody, "*");					//   use "*"

	else if ((strcmp(pBody, "..") == 0)		// If the body is "..",
		 &&  (CallerAttr & FW_FILE))		// and caller wants files
		pathCat(pBody, "*", MAX_COPY);		//   append "*"

	else if ((! isWild(pBody))				// If the body is not wild,
		 &&  (fnchkdir(pPattern))			// and the path is a dir,
		 &&  (! (CallerAttr & FW_DIR)))		// and the caller doesn't want dirs,
			pathCat(pBody, "*", MAX_COPY);	//   append "*" for files

#ifdef VERBOSEOUT
printf("Pattern path:  \"%s\"\n", pPattern);
#endif

	if (PatternCompiler(hp, pPattern))		// Compile the caller's search pattern
		{
		HdrInit(hp, SearchAttr, CallerAttr);// Initialize the header
		}

	PFW_LEV pLev = &hp->level[hp->curLevel];
	SHOWHDR("Initial")

#ifdef TRUNCATE
	return (-1);
#else
	return (0);
#endif
	}

//----------------------------------------------------------------------------
//	fwExclEnable () - Enable the file exclusion mechanism for the fWild object
//	(called from the caller to enable/initialize the path exclusion mechanism)
//----------------------------------------------------------------------------
	void
fwExclEnable (					// Enable/disable file exclusion
	PFWH	hp,					// Pointer to the fWild instance
	PEX		xp,					// Pointer to the Excl instance
	int     enable)				// TRUE to enable exclusion

	{
	if ( ! validHeader(hp))
		{
		fwerrno = FWERR_INSTANCE;
		fwInitError("Exclusion interface");
		}

	hp->xp = ((enable) ? xp : NULL);	// Install Excl instance pointer
	}

//----------------------------------------------------------------------------
//	fwActive () - Check the hp for current validity
//----------------------------------------------------------------------------
	int							// TRUE iff valid
fwActive (						// Check the fWild system instance
	FW_HDR	 *hp)				// Pointer to the FW_HDR header

	{
	return (validHeader(hp));
	}

//----------------------------------------------------------------------------
//	fwFree () - Close and free the fWild system instance
//----------------------------------------------------------------------------
	static void
fwFree (						// Close and free the fWild system instance
	FW_HDR	*hp)				// Pointer to the DTA header

	{
	if (validHeader(hp))
		{
		hp->xp = NULL;			// Disconnect the fexclude mechanism
		freeHeader(hp);
		}
	}

//----------------------------------------------------------------------------
//	fwClose () - Close and free the fWild system instance
//----------------------------------------------------------------------------
	FW_HDR *					// Always returns NULL (to NULL the caller's hp)
fwClose (						// Close and free the fWild system instance
	FW_HDR	*hp)				// Pointer to the instance header

	{
	fwFree(hp);
	return (NULL);
	}

//----------------------------------------------------------------------------
//	fWild () - Request the first/next matching pathname
//----------------------------------------------------------------------------
	char *						// Return a drive/path/filename string
fWild (							// Find the next filename
	FW_HDR	 *hp)				// Pointer to the DTA header

	{
	char *pFound = NULL;
	
	if (! validHeader(hp))		// Require a valid header
		return (NULL);

	hp->running = TRUE;			// Enable the state machine

	pFound = stateMachine(hp);	// Find the first/next matching filename

	if (pFound)
		return (pFound);		// Return with a filespec

	return (NULL);				// Return finished
	}

//----------------------------------------------------------------------------
//	State Machine support functions
//----------------------------------------------------------------------------
//	Initialize the fWild system header; done once per instance
//	This must not damage the fexclude initialization; it is already done.
//----------------------------------------------------------------------------
	static void
HdrInit (						// Common init for all search states
	PFW_HDR hp,					// The search header
	int		SearchAttr,			// Mode used to search
	int		CallerAttr)			// Caller's requested search mode

	{
	if (hp)
		{
		hp->SearchAttr	= SearchAttr;	// Search attribute bitmap
		hp->CallerAttr	= CallerAttr;	// Caller's search bitmap
		hp->curLevel	= 0;			// Initial search level

		// Initialize the search level indices

		for (int i = 0; (i < LEV_MAX); ++i)
			{
			PFW_LEV p = &hp->level[i];
			p->index = i;
			}

		StateInit(hp, &hp->level[0], 0, 0);	// Init the initial search level
		}
//SHOWHDR("HdrInit")
	}
	
//----------------------------------------------------------------------------
//	Initialize a search level; done whenever it is entered from below
//----------------------------------------------------------------------------
	static void
StateInit (						// Common init for all search states
	PFW_HDR	hp,					// Pointer to the search header
	PFW_LEV	pLev,				// Pointer to the search level
	int		segIndex,			// Requested segment index
	int		depthUp)			// Depth should increase

	{
	PFW_SEG  pSeg	= &hp->segment[segIndex];

	pLev->pHdr		= (void *)(hp);
	pLev->state		= pSeg->levelState;
	pLev->pSeg		= pSeg;
	pLev->segIndex	= segIndex;

	memset(&pLev->dta, 0, sizeof(FW_DTA));
	pLev->dta.sh = INVALID_HANDLE_VALUE;	// Make the DTA invalid
	pLev->dta.SearchAttr = hp->SearchAttr;	// Install the search bitmap

	if (pLev->index == 0)				// There is no parent level
		{								// (depthup does not apply)
#ifdef VERBOSEOUT
printf("\n No parent; in level 0\n");
#endif
		pLev->depth		 = hp->rootDepth + pSeg->delta - 1;	// 0-based
		pLev->pattern[0] = '\0';
		pLev->found[0]   = '\0';
		pLev->patConcat  = pLev->pattern;
		pLev->fndConcat  = pLev->found;
		}
	else // (Index > 0)					// Inherit from the parent
		{
#ifdef VERBOSEOUT
printf("\n Inherit from parent - level %d\n", (pLev-1)->index);
#endif
		PFW_LEV pParent = (pLev-1);
		pLev->depth	= pParent->depth;
		if (depthUp)
			pLev->depth	+= pSeg->delta;
		pathCopy(pLev->pattern, pParent->found, FWP_MAX);
		pathCopy(pLev->found,   pParent->found, FWP_MAX);
		pLev->patConcat  = (pLev->pattern + strlen(pLev->pattern));
		pLev->fndConcat  = (pLev->found  + strlen(pLev->found));
		}

	switch (pSeg->type)			// Prepare the pattern or found path
		{
		default:
			fat_err(INV_TYPE, pLev->pattern);
			break;

		case TYPE_ROOT:
				strcat(pLev->fndConcat, pSeg->buffer);
			break;

		case TYPE_NORMAL:
			{
			if (pSeg->terminal)
				strcat(pLev->patConcat, "*");
			else			
				strcat(pLev->fndConcat, pSeg->buffer);
			break;
			}

		case TYPE_WILD:
		case TYPE_RWLD:
			{
				strcat(pLev->patConcat, "*");
			break;
			}
		}

	if ((pSeg->type == TYPE_RWLD) && ! pSeg->terminal)	// Handled internally
		strcpy(pLev->patConcat, pSeg->buffer);

//SHOWLEV("StateInit")
//SHOWSEG
	}
	
//----------------------------------------------------------------------------
//	Transition UP one level during processing
//----------------------------------------------------------------------------
	static int			// Returns FALSE to fail, fatal error to abort
AnalyzeError (
	const char *path,	// Search pathspec that failed to open
	int			enable)	// TRUE to enable printing access denied meassage
  
	{
	DWORD error = GetLastError();		// The windows error code

	switch (error)
		{
		case ERROR_ACCESS_DENIED:
			if (enable)
				fprintf(stderr, "\7Access denied:  \"%s\"\n", path);
			break;

		case ERROR_FILE_NOT_FOUND:
			fprintf(stderr, "File not found:  \"%s\"\n", path);
			break;

		case ERROR_PATH_NOT_FOUND:		// This happens normally
//			fprintf(stderr, "Path not found:  \"%s\"\n", path);
			break;

		default:
			fprintf(stderr, "\7ErrorCode (%d)  \"%s\"\n", error, path);
//			fat_err(INV_FOPEN, pLev->pattern);
			break;
		}

	return (FALSE);
	}

//----------------------------------------------------------------------------
//	Open theFinder
//----------------------------------------------------------------------------
	static int
SearchOpen (		// Returns TRUE for OK to continue, FALSE to fail
	PFW_LEV	pLev)

	{
	if (FinderOpen(&pLev->dta, pLev->pattern))
		return (TRUE);
	else
		return (AnalyzeError(pLev->pattern,
					(((PHP)(pLev->pHdr))->CallerAttr & FW_AD)));
	}

//----------------------------------------------------------------------------
//	Transition UP one level during processing
//----------------------------------------------------------------------------
	static void
Search_UP (
	PFW_HDR	hp,			// Instance pointer
	PFW_LEV	pLev,		// Current level pointer
	int		segIndex,	// Requested next segment (may or may not increase in RWLD)
	int		depthUp)	// TRUE if depth should increase

	{
PROGRESS("Up...\n")
	if (hp->curLevel >= LEV_MAX)
		fat_err(SRCH_OVF, NULL);		// Level overflow

	PFW_LEV	pLevUp = (pLev+1);			// Point the next up DTE
	StateInit(hp,						// Init the search level
		pLevUp,							// Requested level
		segIndex,						// Requested pattern segment
		depthUp);						// TRUE if depth should increase
	hp->curLevel = pLevUp->index;		// Update the current search level
//SHOWLEV("UP")
//SHOWHDR("UP")
	}

//----------------------------------------------------------------------------
//	Transition DOWN one level during processing
//----------------------------------------------------------------------------
	static int
Search_DN (
	PFW_HDR	hp,			// Instance pointer
	PFW_LEV	pLev)		// Current level pointer

	{
PROGRESS("Down...\n")
//printf("closed\n");
	if (hp->curLevel > 0)
		{
		--(hp->curLevel);				// Go down one search level
		pLev = &hp->level[hp->curLevel];
SHOWHDR("DN")
SHOWLEV("DN")
		return (FALSE);					// Not back to level 0 yet
		}
	else // we are at level zero, stop the state machine
		{
//printf("true\n");
PROGRESS("Done\n")
		hp->running = FALSE;			// At level 0, finished
		return (TRUE);
		}
//SHOWLEV("DN")
//SHOWHDR("DN")
	}

//----------------------------------------------------------------------------
//	Copy the exclusion check name, don't copy separator
//----------------------------------------------------------------------------
	static void
nameCopy (
	char  *pDst,
	char  *pSrc)

	{
	char  ch;
	while ((ch = *pSrc++) != NULCH)
		{
		if (ch != PATHCH)
			*pDst++ = ch;
		}
	*pDst = NULCH;	
	}

//----------------------------------------------------------------------------
//	Check if a file/dir name is excluded; fExcludeCheck will make it monocase
//----------------------------------------------------------------------------
	static int
isExcluded (
	PFW_HDR	hp,
	PFW_LEV	pLev,
	int		FileAttr)

	{
	hp->xItem.type  = pLev->dta.dta_type & (ATT_DIR | ATT_FILE);
	hp->xItem.depth = pLev->depth;
	nameCopy(hp->xItem.name, pLev->fndConcat);
	return (fExcludeCheck(hp->xp, &hp->xItem));
	}

//----------------------------------------------------------------------------
//	Set the file properties when fWild returns a filespec
//----------------------------------------------------------------------------
	static void
SetProperties (
	PFW_HDR	hp,
	PFW_LEV	pLev)

	{
	hp->foundFlag	= TRUE;		// Successful search
	hp->file_name	= pLev->found;
	hp->file_fdt	= pLev->dta.dta_fdt;
	hp->file_type	= pLev->dta.dta_type;
	hp->file_size	= pLev->dta.dta_size;
//SHOWLEV("Set properties")
//SHOWHDR("Set properties")
	}

//----------------------------------------------------------------------------
//	State Machine
//----------------------------------------------------------------------------
//	stateMachine () - Request the first/next matching pathname
//	called exclusively from fWild()
//----------------------------------------------------------------------------
	static char *				// Return a drive/path/filename string
stateMachine (					// Find the next filename
	FW_HDR	 *hp)				// Pointer to the DTA header

	{
	PFW_LEV  pLev;
	PFW_SEG  pSeg;

	if (hp == NULL)
		return (NULL);

	hp->foundFlag  = FALSE;		// No search success

	for (;;)
		{
//SHOWHDR("SM loop top")

		if (! hp->running)
			return (NULL);		// Finished, shut down the state machine

//SHOWHDR("Entering SM loop")
		pLev = &hp->level[hp->curLevel];
		pSeg = &hp->segment[pLev->segIndex];
//SHOWLEV("Entering SM loop")
//SHOWSEG
		switch (pLev->state)
			{
			default:
				{
				fat_err(INV_STATE, NULL);
				}

			case ST_FINISHED:
				{
				return (NULL);
				}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//	Root segment (non-terminal)
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

case ST_ROOT_NT_0:
	{
SHOWLEV("ST_ROOT_NT_0")
SHOWDTA

	// Buffer was copied in StateInit()

	pLev->state = ST_ROOT_NT_1;
	if (! isExcluded(hp, pLev, ATT_DIR))			// If not excluded,
		Search_UP(hp, pLev, (pLev->segIndex+1), 1);	//   advance to the next level
	break; // exit the state
	}

case ST_ROOT_NT_1:
	{
SHOWLEV("ST_ROOT_NT_1")
	if (Search_DN(hp, pLev))		// Search End
		return (NULL);
	break; // exit the state
	}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//	Root segment (terminal)
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

case ST_ROOT_TL_0:			// Root segment should never be terminal (at least a '*')
	{
SHOWLEV("ST_ROOT_TL_0")
	fat_err(INV_PATH, NULL);
	}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//	Normal segment (non-terminal)
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

case ST_NORM_NT_0:
	{
SHOWLEV("ST_NORM_NT_0")
SHOWDTA

	// Buffer was copied in StateInit()

	pLev->state = ST_NORM_NT_1;
	if (! isExcluded(hp, pLev, ATT_DIR))			// If not excluded,
		Search_UP(hp, pLev, (pLev->segIndex+1), 1);	//   advance to the next level
	break; // exit the state
	}

case ST_NORM_NT_1:
	{
SHOWLEV("ST_NORM_NT_1")
	if (Search_DN(hp, pLev))		// Search End
		return (NULL);
	break; // exit the state
	}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//	Normal segment (terminal)
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

case ST_NORM_TL_0:
	{
SHOWLEV("ST_NORM_TL_0")
SHOWDTA

	// Pattern was copied in StateInit()

	if (! SearchOpen(pLev))
		{		
		pLev->state = ST_NORM_TL_2;						// Access denied
		break; // exit the loop
		}
	pLev->state = ST_NORM_TL_1;							// Access denied
	}

case ST_NORM_TL_1:
	{
SHOWLEV("ST_NORM_TL_1")
	for (;;)
		{
		int  FileAttr = FinderFile(&pLev->dta, pLev->fndConcat);

		hp->foundFlag = FALSE;
		if (FileAttr == ATT_NONE)
			{
			pLev->state = ST_NORM_TL_2;
			break; // exit the loop
			}
		if (( ! fnMatch(pSeg->buffer, pLev->fndConcat))	// Must match,
		||  (isExcluded(hp, pLev, FileAttr)))			//   and must be not excluded
			continue;									// Otherwise, skip over it

		if (((FileAttr & ATT_FILE)  &&  hp->CallerAttr & FW_FILE)
		||  ((FileAttr & ATT_DIR)   &&  hp->CallerAttr & FW_DIR))
			{
			hp->foundFlag = TRUE;
			break; // exit the loop
			}
		}
			
	if (hp->foundFlag)
		{
		SetProperties(hp, pLev);		// Report this file/directory
		return (pLev->found); // exit the state machine
		}
	}

case ST_NORM_TL_2:
	{
SHOWLEV("ST_NORM_TL_2")
	FinderClose(&pLev->dta);
	if (Search_DN(hp, pLev))		// Search End
		return (NULL);
	break; // exit the state
	}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//	Simple wild segment (non-terminal)
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

case ST_WILD_NT_0:
	{
SHOWLEV("ST_WILD_NT_0")
SHOWDTA

	// Pattern was copied in StateInit()

	if (! SearchOpen(pLev))
		{		
		pLev->state = ST_WILD_NT_2;						// Access denied
		break; // exit the loop
		}
	pLev->state = ST_WILD_NT_1;
	}

case ST_WILD_NT_1:
	{
SHOWLEV("ST_WILD_NT_1")
	for (;;)
		{
		int  FileAttr = FinderFile(&pLev->dta, pLev->fndConcat);

		if (FileAttr == ATT_NONE)
			{
			pLev->state = ST_WILD_NT_2;
			break; // exit the loop
			}

		if ((FileAttr != ATT_DIR)						// Must be a directory,
		||  (! fnMatch(pSeg->buffer, pLev->fndConcat))	//   and must match,
		||  (isExcluded(hp, pLev, FileAttr)))			//   and must be not excluded
			continue;									// Otherwise, skip over it

		strcat(pLev->found, "\\");						// Add the separator, and
		Search_UP(hp, pLev, (pLev->segIndex+1), 1);		//   advance to next level
		break; // exit the loop
		}
	break; // exit the state
	}

case ST_WILD_NT_2:
	{
SHOWLEV("ST_WILD_NT_2")
	FinderClose(&pLev->dta);
	if (Search_DN(hp, pLev))		// Search End
		return (NULL);
	break; // exit the state
	}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//	Simple wild segment (terminal)
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

case ST_WILD_TL_0:
	{
SHOWLEV("ST_WILD_TL_0")
SHOWDTA

	// Pattern was copied in StateInit()

	if (! SearchOpen(pLev))
		{		
		pLev->state = ST_WILD_TL_2;						// Access denied
		break; // exit the loop
		}
	pLev->state = ST_WILD_TL_1;
	}

case ST_WILD_TL_1:
	{
SHOWLEV("ST_WILD_TL_1")
	for (;;)
		{
		int  FileAttr = FinderFile(&pLev->dta, pLev->fndConcat);

		hp->foundFlag = FALSE;
		if (FileAttr == ATT_NONE)
			{
			pLev->state = ST_WILD_TL_2;
			break; // exit the loop
			}

		if (( ! fnMatch(pSeg->buffer, pLev->fndConcat))	// Must match,
		||  (isExcluded(hp, pLev, FileAttr)))			//   and must be not excluded
			continue;									// Otherwise, skip over it

		if (((FileAttr & ATT_FILE)  &&  hp->CallerAttr & FW_FILE)
		||  ((FileAttr & ATT_DIR)   &&  hp->CallerAttr & FW_DIR))
			{
			hp->foundFlag = TRUE;
			break; // exit the loop
			}
		}
					
	if (hp->foundFlag)
		{
		SetProperties(hp, pLev);		// Report this file/directory
		return (pLev->found); // exit the state machine
		}
	}

case ST_WILD_TL_2:
	{
SHOWLEV("ST_WILD_TL_2")
	FinderClose(&pLev->dta);
	if (Search_DN(hp, pLev))		// Search End
		return (NULL);
//printf("break\n");
	break; // exit the state
	}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//	Recursive wild segment (non-terminal)
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

case ST_RWLD_NT_0:
	{
SHOWLEV("ST_RWLD_NT_0")
SHOWDTA

	pLev->state = ST_RWLD_NT_1;
	if (! isExcluded(hp, pLev, ATT_DIR))			// If not excluded,
		Search_UP(hp, pLev, (pLev->segIndex+1), 0);	// Skip no directories
	break;
	}

case ST_RWLD_NT_1:
	{
	if (! SearchOpen(pLev))
		{		
		pLev->state = ST_RWLD_NT_3;						// Access denied
		break; // exit the loop
		}
	pLev->state = ST_RWLD_NT_2;
	}

case ST_RWLD_NT_2:
	{
	for (;;)
		{
		int  FileAttr = FinderFile(&pLev->dta, pLev->fndConcat);

		if (FileAttr == ATT_NONE)
			{
			pLev->state = ST_RWLD_NT_3;
			break; // exit the loop
			}

		if ((FileAttr != ATT_DIR)				// Must be a directory, and
		||  (isExcluded(hp, pLev, FileAttr)))	//   must be not excluded
			continue;							// Otherwise, skip over it

		strcat(pLev->found, "\\");				// Add the separator
		Search_UP(hp, pLev, pLev->segIndex, 1);	// Skip a directory
		break; // exit the loop
		}
	break; // exit the state
	}

case ST_RWLD_NT_3:
	{
SHOWLEV("ST_RWLD_NT_3")
	FinderClose(&pLev->dta);
	if (Search_DN(hp, pLev))		// Search End
		return (NULL);
	break; // exit the state
	}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//	Recursive wild segment (terminal)
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

case ST_RWLD_TL_0:
	{
SHOWLEV("ST_RWLD_TL_0")
SHOWDTA

	if (! SearchOpen(pLev))
		{		
		pLev->state = ST_RWLD_TL_3;						// Access denied
		break; // exit the loop
		}
	pLev->state = ST_RWLD_TL_1;
	}

case ST_RWLD_TL_1:
	{
SHOWLEV("ST_RWLD_TL_1")
	for (;;)
		{
		int  FileAttr = FinderFile(&pLev->dta, pLev->fndConcat);

		hp->foundFlag = FALSE;
		if (FileAttr == ATT_NONE)
			{
			pLev->state = ST_RWLD_TL_3;
			break; // exit the loop
			}

		if (isExcluded(hp, pLev, FileAttr))		// Must not be excluded
			continue;

		if ((FileAttr & ATT_FILE)  &&  hp->CallerAttr & FW_FILE)
			{
			hp->foundFlag = TRUE;
			break; // exit the loop
			}

		if (FileAttr & ATT_DIR)
			{
			if (hp->CallerAttr & FW_DIR)
				hp->foundFlag = TRUE;
			pLev->state = ST_RWLD_TL_2;
			break; // exit the loop
			}
		}
			
	if (hp->foundFlag)
		{
		SetProperties(hp, pLev);		// Report this file/directory
		return (pLev->found); // exit the state machine
		}
	break; // exit the state
	}

case ST_RWLD_TL_2:
	{
SHOWLEV("ST_RWLD_TL_2")
	strcat(pLev->found, "\\");		// Add the separator
	pLev->state = ST_RWLD_TL_1;
	Search_UP(hp, pLev, pLev->segIndex, 1);
	break; // exit the state
	}

case ST_RWLD_TL_3:
	{
SHOWLEV("ST_RWLD_TL_3")
	FinderClose(&pLev->dta);
	if (Search_DN(hp, pLev))		// Search End
		return (NULL);
	break; // exit the state
	}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

			} // End of switch
		} // End for (;;)
	} // End of FWild()

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
