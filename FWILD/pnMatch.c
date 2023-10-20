/* ----------------------------------------------------------------------- *\
|
|							   pnMatch
|					 Filespec and pathspec Comparison Functions
|					   (includes a test program)
|
|				    Copyright (c) 1985 - 2023, all rights reserved
|						Brian W Johnson
|						   15-Sep-23	New version of fnmatch
|
|
|		int				Return TRUE iff match successful
|	_nameMatch (		Test two filenames for filename match
|		char  *arg1,	Pattern name (may include wild cards)
|		char  *arg2)	Test name (must be a pure file name)
|
|		int				Return TRUE iff match successful
|	pnOverlap (			Test two pathspecs for pathspec overlap
|		char  *arg1,	Pattern name (may include wild cards)
|		char  *arg2)	Test name (must be a pure file name)
|
|
|		int				Return TRUE if match successful
|	pnMatch (			Test two pathspecs for pathspec match
|		char  *arg1,	Pattern name (may include wild cards)
|		char  *arg2)	Test name (must be a pure file name)
|
|
|		int				Return TRUE if match successful
|	fnMatch (			Test two filenames for filename match
|		char  *arg1,	Pattern name (may include wild cards)
|		char  *arg2)	Test name (must be a pure file name)
|
|
|	pnOverlap() expects a full pathspec with prefix.
|	pnMatch()   expects a full pathspec with prefix.
|	fnMatch()   expects only a single segment or file name.
|
|	For each function, arg1 may include wild cards, while arg2
|	must be pure (or physical), without wild cards.
|
|	This version treats '*' as a total wild card which includes any "."
|	within the match so that '*' works as *.* previously worked.  If a
|	'.' is specified , it is included in the match like any other character.
|
|	Rules:
|
|	Wild '?' must match exactly one path character
|	Wild '*' can  match 0 or more path segments
|	When used, "**" must standalone in a segment name
|
\* ----------------------------------------------------------------------- */

#include  <windows.h>
#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <ctype.h>

#include  "fWild.h"

// --------------------------------------------------------------------------
//	#define TEST
//	#define DEBUG

#ifdef TEST
#define DEBUG
#endif

#ifdef DEBUG
#define   DBGD(arg) printf("(%2d) DIR %d   Pattern: \"%s\"   Name: \"%s\"\n", arg, nest, pPat, pNam)
#define   DBGP(arg) printf("( %d) PTH %d   Pattern: \"%s\"   Name: \"%s\"\n", arg, nest, pPat, pNam);

static void dump (void);

#else
#define   DBGD(arg)
#define   DBGP(arg)
#endif

// --------------------------------------------------------------------------

#define NULCH			('\0')
#define	PATHCH			('\\')
#define isPath(arg)		((arg) == PATHCH)
#define isNul(arg)		((arg) == NULCH)


#define MAX_SEGMENT	(50)

#define PointPattern(index)	(&patSegTable[index])
#define PointName(index)	(&namSegTable[index])

typedef struct
	{
	int   index;	// 0-based index
	int   wild;		// Boolean TRUE if any kind of wild (pattern use only)
	int   rwild;	// Boolean TRUE if recursive wild (pattern use only)
	char *pBuffer;	// Ptr to the segment buffer
	}
SEGMENT, *pSEGMENT;

static	SEGMENT	patSegTable [MAX_SEGMENT] = {0, 0, 0};
static	SEGMENT	namSegTable [MAX_SEGMENT] = {0, 0, 0};

static int	numPatIndex = 0;	// Max pattern segment index
static int	numNamIndex = 0;	// Max name segment index

static char	PatternBuffer [MAX_PATH];
static char	NameBuffer	  [MAX_PATH];

// --------------------------------------------------------------------------
	int						// Return TRUE if the name fragments match
_nameMatch (				// Test two filename fragments for filename match
	const char *pPat,		// Pattern name pointer (may include wild cards)
	const char *pNam,		// Test name pointer (must be a pure file name)
	int			nest)		// Nesting level (for debug use)

	{
	char chPat;				// Current pattern character
	char chNam;				// Current name character

	// Match a physical pathspec segment to a pattern pathspec segment.
	// The pattern may contain wildcards.  The name must be physical.
	// The name is tested for compatibility with the pattern.
	// The algorithm is recursive at choice points.

	int result = FALSE;

	for (;;)				// Loop through one segment match
		{
#ifdef DEBUG
printf("_nameMatch: Pattern and Name: [%d]  \"%s\"   \"%s\"\n", nest, pPat, pNam);
#endif
		chPat = *pPat;
		chNam = *pNam;

		if ((chPat == '*')
		&&  (chNam == NULCH))		// If a trailing '*'
			chPat = *(++pPat);		// Just drop it (only viable option)
	
		if ((chPat == NULCH)		// Check if done success
		&&  (chNam == NULCH))
			{
DBGD(0);
			result = TRUE;			// both exhausted
			goto exit;
			}

		if ((chPat == NULCH)		// Check if done fail
		||  (chNam == NULCH))
			{
DBGD(1);
			result = FALSE;			// length mismatch
			goto exit;
			}

DBGD(2);
		if (chPat != '*')
			{
			if ((chPat == chNam)	// If the characters match,
			||  (chPat == '?'))		// or its a QUESTION match,
				{					// (includes '.')
				++pPat;				// advance to the next characters
				++pNam;
				continue;			// Good so far
				}
			else
				{
DBGD(3);
				result = FALSE;		// character mismatch
				goto exit;
				}
			}

		// (chPat == '*')	// Special processing for '*'
DBGD(4);
		const char *pPatNext = (pPat+1);	// Ptr to the next non-star character of the segment
		const char *pNamNext = (pNam+1);	// Ptr to the next character of the name segment

		// Point the next required name segment char (if any) or '?' or NUL

		for (;;)
			{
			chPat = *pPatNext;
			if (isNul(chPat))	// Stop at the end of the segment
				break;
			if (chPat != '*')	// (is either a '?', or the next required char)
				break;
			++pPatNext;
			}

		// At this point chPat (next) is one of: NUL, '?', or a normal char

		if (chPat != '?') // (chPat (next) == a normal segment character, or NUL)
			{
DBGD(5);	// (NUL is treated as the required character here)
			// For the normal char case, the algorithm is:
			// Try subsume all up to the required one, and complete the '*'
			// Try subsume all up to the required one, and leave the '*
			// Try subsume no characters and complete the '*'

			for (;;)
				{
				chNam = *pNamNext;
				if (isNul(chNam))		// Stop at the end of the segment
					break;
				if (chNam == chPat)		// Specific required character found
					break;				
				++pNamNext;
				}

DBGD(6);	// Try subsume all up to the required one, and complete the '*':
			if (_nameMatch(pPatNext, pNamNext, (nest+1)))
				{
DBGD(7);
				result = TRUE;			// alternate path succeeded
				goto exit;
				}

DBGD(8);	// Try subsume all up to the required one, and leave the '*':
			if  (_nameMatch(pPat, pNamNext, (nest+1)))
				{
DBGD(9);
				result = TRUE;			// alternate path succeeded
				goto exit;
				}

			// And, we continue with the primary thread
			// Try subsume no name path character(s) and complete the '*':
			pPat  = pPatNext;
//			pNam  = pNam;
			}

		else // (chPat == '?')
			{
DBGD(10);	// For the '?' case, the algorithm is:
			// Try subsuming one name path characters and keep the '*')
			// Try subsuming no more name path characters and complete the '*')

DBGD(11);	// Try subsuming one name path characters and keep the '*'):
			if ((chNam != NULCH)
			&&  (_nameMatch(pPat, (pNam+1), (nest+1))))
				{
DBGD(12);
				result = TRUE;		// alternate path succeeded
				goto exit;
				}

			// Continue with the primary thread
			// Try subsuming no more name path characters and complete the '*'):
			pPat  = pPatNext;
//			pNam  = pNam;
			}
		}

exit:	// All exits pass through the exit message

#ifdef DEBUG
printf("_nameMatch: %s\n", (result ? "Match" : "No match"));
#endif

	return (result);
	}

// --------------------------------------------------------------------------
	static int				// Return TRUE if the paths match
_pathMatch (				// Test two filename fragments for filename match
	int		patIndex,		// Pattern index into the pattern table
	int		namIndex,		// Name index into the name table
	int		nest)			// Nesting level (for debug use)

	{
	// Match a physical pathspec to a pattern pathspec.
	// The pattern pathspec may contain the recursive wild (RWILD) specifier
	// "**", which signifies skipping over zero or more path segments within
	// the name pathspec.  The name pathspec must be physical.
	// The name pathspec is tested for compatibility with the pattern pathspec.
	// The algorithm is recursive at choice points.

	int result = FALSE;

	for (;;)				// Loop through all the segments
		{
		SEGMENT *pPatTbl = PointPattern(patIndex);	// Pointer to the current pattern segment
		SEGMENT *pNamTbl = PointName(namIndex);		// Pointer to the current name segment

#ifdef DEBUG
		char *pPat = ((patIndex < numPatIndex) ? (pPatTbl->pBuffer) : "");
		char *pNam = ((namIndex < numNamIndex) ? (pNamTbl->pBuffer) : "");
printf("_pathMatch: Pattern and Name: [%d]  (%d, %d) \"%s\"   \"%s\"\n",
	nest, patIndex, namIndex, pPat, pNam);
#endif
		if ((patIndex >= numPatIndex)		// Check if done success
		&&  (namIndex >= numNamIndex))
			{
			result = TRUE;
			goto exit;
			}

		if ((patIndex == numPatIndex)		// Check if done fail
		||  (namIndex == numNamIndex))
			{
			result = FALSE;
			goto exit;
			}

DBGP(1);
		if ( ! pPatTbl->rwild)
			{
			if (_nameMatch(pPatTbl->pBuffer, pNamTbl->pBuffer, 0))
				{
				++patIndex;				// Advance to the next pair of segments
				++namIndex;
				continue;
				}
			else
				{
				result = FALSE;
				goto exit;
				}
			}

		// Now processing an RWILD pattern segment
		// Try skipping one name segment, and keep the RWILD
		// Try NOT skipping over the current name segment, and close the RWILD

		int nextPatIndex = (patIndex+1);
		int nextNamIndex = (namIndex+1);
			
DBGP(2);// Try skipping over the current name segment:
		if ((nextNamIndex <= numNamIndex)
		&&  (_pathMatch(patIndex, nextNamIndex, (nest+1))))	// Keep the RWILD
			{
			result = TRUE;
			goto exit;
			}

DBGP(3);
		// Continue with the primary thread
		// Try NOT skipping over the current name segment:
		if (nextPatIndex > numPatIndex)
			{
			result = FALSE;
			goto exit;
			}

		patIndex = nextPatIndex;	// Close the RWILD
//		namIndex = namIndex;		// Keep the same name segment
		}

exit:	// All exits pass through the exit message

#ifdef DEBUG
printf("_pathMatch: %s\n", (result ? "Match" : "No match"));
#endif

	return (result);
	}

// --------------------------------------------------------------------------
	static int				// Returns number of pattern buffer segments
PathCompiler (				// Builds the SEGMENT table from the path data
	SEGMENT  *pSegTbl,		// Pointer to the SEGMENT table
	char	 *pPath)		// Pointer to the path buffer

	{
	int index    = 0;		// Running segment index

	// Compile the pathspec segments into a SEGMENT table.
	// The SEGMENT tables do not include the pathspec prefixes.
	// Install the first path segment

	if (*pPath == NULCH)					// Check if first segment empty
		return (index);						// No first segement

	pSegTbl->index		= index++;			// Install the index
	pSegTbl->wild		= FALSE;			// Init the WILD flag
	pSegTbl->rwild		= FALSE;			// Init the RWILD flag
	pSegTbl->pBuffer	= pPath;			// Install the path segment pointer

	// Install the remaining segments, by finding each PATHCH

	while ((pPath = strchr((pPath), PATHCH)) != NULL)
		{
		*pPath			= NULCH;			// NUL the found PATHCH
		if (*(++pPath) == NULCH)			// If the path segment is empty
			break;							//   terminate compilation

		++pSegTbl;							// Advance the segment table pointer
		pSegTbl->index		= index++;		// Install the index
		pSegTbl->wild		= FALSE;		// Init the WILD flag
		pSegTbl->rwild		= FALSE;		// Init the RWILD flag
		pSegTbl->pBuffer	= pPath;		// Install the path segment pointer
		}

	return (index);			// Returns number of pattern buffer segments
	}

// --------------------------------------------------------------------------
	static int				// Return TRUE if the name fragments match
prefixMatch (				// Test two pathspec prefixes for pathspec match
	const char *pPat,		// Ptr to the Pattern pathspec
	const char *pNam)		// Ptr to the Name pathspec

	{
	int result = FALSE;		// The returned result, assume no match or error

	// Match the prefixes of the two pathspecs.
	// Compile the SEGMENT tables for the two pathspecs.
	
#ifdef DEBUG
printf("prefixMatch Entry:  \"%s\"    \"%s\"\n", pPat, pNam);
#endif

	if ((pPat == NULL)				// NULL pattern pointer
	||  (strlen(pPat) > MAX_PATH)	// Oversize
	||  (pNam == NULL)				// NULL name pointer
	||  (strlen(pNam) > MAX_PATH)	// Oversize
	||  (isWild(pNam)))				// Wild name is disallowed
		goto exit;

	// Copy both paths; make both absolute, ensure case insensitivity
	// and ensure consistent path chars

	_fnabspth(PatternBuffer, pPat);
	strupr(PatternBuffer);
	strsetp(PatternBuffer, PATHCH);

	_fnabspth(NameBuffer, pNam);
	strupr(NameBuffer);
	strsetp(NameBuffer, PATHCH);
	
#ifdef DEBUG
printf("prefixMatch 1     \"%s\"    \"%s\"\n", PatternBuffer, NameBuffer);
#endif

	// Get the pointers to the path bodies (if any)
	// (The prefixes contain the root separator, if any)
	// Matching prefixes must be identical in length and content

	char *pPatBody  = PointPastPrefix(PatternBuffer);
	int   lengthPat = (int)(pPatBody - PatternBuffer);
	char *pNamBody  = PointPastPrefix(NameBuffer);
	int   lengthNam = (int)(pNamBody - NameBuffer);

	result = ((lengthPat == lengthNam)
		  &&  (strncmp(PatternBuffer, NameBuffer, lengthPat) == 0));

	if (result == FALSE)	// If the prefixes don't match,
		goto exit;			//   we are done

#ifdef DEBUG
printf("prefixMatch 2     \"%s\"    \"%s\"\n", pPatBody, pNamBody);
#endif

	// The two prefixes are matched, and we have pointers to both path bodies
	// Compile the two path bodies into the respective SEGMENT structures
	// Each body now contains some number of directories and (possibly) a file
	// and we have already skipped over any root separators
	// NUL each further separator to make each segment NUL terminated
	// (except possible initial root separators)
	// Keep lists of the body segments and whether each is WILD or RWILD
	// Count number of segments in each body

	numPatIndex = PathCompiler(&patSegTable[0], pPatBody);
	numNamIndex = PathCompiler(&namSegTable[0], pNamBody);
	
	// Configure (only) the pattern table RWILD and WILD indicators
	// A valid name path has no wild content

	for (int index = 0; (index < numPatIndex); (++index))
		{
		SEGMENT *pSegTbl    = PointPattern(index);

		if ((strchr(pSegTbl->pBuffer, '*') != NULL)		// Flag WILD segment
		||  (strchr(pSegTbl->pBuffer, '?') != NULL))
			pSegTbl->wild   = TRUE;

		if (strcmp(pSegTbl->pBuffer, "**") == 0)
			{
			if (strcmp(pSegTbl->pBuffer, "**") == 0)	// Flag RWILD segment
			pSegTbl->rwild  = TRUE;
			}
		}

	result = TRUE;			// Because the prefixes did match

exit:	// All exits pass through the exit message

#ifdef DEBUG
	dump();					// Data structure diagnostic dump
printf("prefixMatch Exit: %s\n", (result ? "Match" : "No match"));
#endif

	return (result);
	}

// --------------------------------------------------------------------------
	int						// Return TRUE if the name fragments match
fnMatch (					// Test two pathspecs for pathspec match
	const char *pPat,		// Ptr to the Pattern pathspec
	const char *pNam)		// Ptr to the Name pathspec

	{
	int  result;			// The returned result

	// Match a physical pathspec segment to a pattern pathspec segment.

#ifdef DEBUG
printf("\nfnMatch Entry:  \"%s\"    \"%s\"\n", pPat, pNam);
#endif

	if ((pPat == NULL)				// NULL pattern pointer
	||  (pNam == NULL)				// NULL name pointer
	||  (strcmp(pNam, ".") == 0)	// "."  is unmatchable
	||  (strcmp(pNam, "..") == 0))	// ".." is unmatchable
		{
		result = FALSE;		// No possibility of overlap
		goto exit;
		}

	if ((strcmp(pPat, "*")  == 0)	// "*"  matches everything
	||  (strcmp(pPat, "**") == 0))	// "**" matches everything
		{
		result = TRUE;		// No possibility of overlap
		goto exit;
		}

	// Copy both paths; ensure case insensitivity and consistent path chars
	
	pathCopy(PatternBuffer, pPat, MAX_COPY);
	strupr(PatternBuffer);
	strsetp(PatternBuffer, PATHCH);
	pathCopy(NameBuffer, pNam, MAX_COPY);
	strupr(NameBuffer);
	strsetp(NameBuffer, PATHCH);
	
	result = _nameMatch(PatternBuffer, NameBuffer, 0);

exit:	// All exits pass through the exit message

#ifdef DEBUG
printf("\nfnMatch Exit:  %s\n", result ? "Match" : "No match");
#endif

	return (result);
	}

// --------------------------------------------------------------------------
	int						// Return TRUE if the name fragments match
pnMatch (					// Test two pathspecs for pathspec match
	const char *pPat,		// Ptr to the Pattern pathspec
	const char *pNam)		// Ptr to the Name pathspec

	{
	int  result;			// The returned result

	// Match a physical pathspec to a pattern pathspec
	// If they match, the SEGMENT structures will also be built

#ifdef DEBUG
printf("\npnMatch Entry:  \"%s\"    \"%s\"\n", pPat, pNam);
#endif

	if (! prefixMatch(pPat, pNam))	// Check for prefix mismatch
		{
		result = FALSE;		// No possibility of overlap
		goto exit;
		}

	// Test the two pathspecs for pathspec match

	result = _pathMatch(0, 0, 0);

exit:	// All exits pass through the exit message

#ifdef DEBUG
printf("\npnMatch Exit:  %s\n", result ? "Match" : "No match");
#endif

	return (result);
	}

// --------------------------------------------------------------------------
	int						// Return TRUE if the name fragments match
pnOverlap (					// Test two pathspecs for pathspec overlap
	const char *pPat,		// Ptr to the Pattern pathspec
	const char *pNam)		// Ptr to the Name pathspec

	{
	int  result = FALSE;	// The returned result, assume no overlap

	// Check for overlap between two pathspecs.
	// The pathspecs are matched only up to a wild segment.
	// If they match, the SEGMENT structures will also be built

#ifdef DEBUG
printf("\npnOverlap Entry:  \"%s\"    \"%s\"\n", pPat, pNam);
#endif

	if (! prefixMatch(pPat, pNam))	// Check for prefix mismatch
		{
		result = FALSE;		// No match, no structures, no overlap
		goto exit;
		}

	// For the overlap test, we want to compare the two paths
	// only up to but not including the first RWILD directory
	// Count the number of non-wild pattern segments, and
	// truncate the pattern SEGMENT table count to that number
 
	int numPat     = numPatIndex;	// Number of non-RWILD segments
	int numRwild   = 0;		// TRUE if there is any RWILD segment
	int plus1wild  = 0;		// TRUE if WILD at the name length plus 1 position

	for (int index = 0; (index < numPatIndex); (++index))
		{
		SEGMENT *pSegTbl = PointPattern(index);

		if (pSegTbl->rwild)
			{
			++numRwild;
			numPat = pSegTbl->index;
			}

		if (pSegTbl->index == numNamIndex)
			++plus1wild;
		}

#ifdef DEBUG
printf("Pat [%d]  Nam [%d]\n\n", numPat, numNamIndex);
#endif

	// There are three cases of possible overlap.  If all of
	// these cases can be ruled out, return no overlap now.

	if ((numRwild == 0)
	&&  (! (plus1wild  &&  (numPatIndex == numNamIndex + 1)))
	&&  (! (numNamIndex == numPatIndex)))
		{
		result = FALSE;		// No possibility of overlap
		goto exit;
		}

	// Overlap is still possible if the two commomn paths match.
	// Determine the number of segments to test for overlap
	// as the segment count of the shortest non-RWILD path.
	// Then,test the two filespecs for common pathspec match.

	int commonLen = min(numNamIndex, numPat);
	numNamIndex = numPatIndex = commonLen;

	result = (_pathMatch(0, 0, 0));

exit:	// All exits pass through the exit message

#ifdef DEBUG
printf("\npnOverlap Exit:  %s\n", result ? "Overlap" : "No overlap");
#endif

	return (result);
	}

//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#if 0	// Notes on the overlap algorithm
There is possible overlap

if:		RWILD anywhere in the pattern
	and (no requirements about WILD segments)
	and (no requirements about pattern and name counts)
	and common non-RWILD segments match

or if:	no RWILD
	and WILD segment at (name count +1) position
	and pattern count == (name count + 1)
	and common non-RWILD segments match

or if:	no RWILD
	and (no requirements about WILD segments)
	and pattern count == name count
	and common non-RWILD segments match
#endif

// --------------------------------------------------------------------------
#ifdef DEBUG
	static void
dump (void)				/* Data structure dump test program */

	{
	SEGMENT *pSegTbl;						// Working segment table pointer

	printf("\nPattern structure: [%d]\n", numPatIndex);
	for (int index = 0; (index < numPatIndex); (++index))
		{
		pSegTbl = PointPattern(index);
		printf("Index:   %d\n", pSegTbl->index);
		printf("Wild:    %s\n", (pSegTbl->wild  ? "WILD" : "-----"));
		printf("Rwild:   %s\n", (pSegTbl->rwild ? "RWILD" : "-----"));
		printf("Buffer:  \"%s\"\n\n", pSegTbl->pBuffer);

		pSegTbl->rwild = (strcmp(pSegTbl->pBuffer, "**") == 0);
		}

	printf("   Name structure: [%d]\n", numNamIndex);
	for (int index = 0; (index < numNamIndex); (++index))
		{
		pSegTbl = PointName(index);
		printf("Index:   %d\n", pSegTbl->index);
		printf("Wild:    %s\n", "-----");
		printf("Rwild:   %s\n", "-----");
		printf("Buffer:  \"%s\"\n\n", pSegTbl->pBuffer);
		}
	printf("\n");
	}

#endif
// --------------------------------------------------------------------------
#ifdef TEST
    void
main ()					/* Test program for match */

//#define FNMATCH
//#define PNMATCH
#define PNOVER
//#define DUMP

	{
	char  s1 [1024];
	char  s2 [1024];

	while (TRUE)
		{
		printf("\nPattern string: ");
		gets(s1);
		printf("   Name string: ");
		gets(s2);
#ifdef FNMATCH
		if (fnmatch2(s1, s2))
#endif		
#ifdef PNMATCH
		if (pnMatch(s1, s2))
#endif		
#ifdef PNOVER
		if (pnOverlap(s1, s2))
#endif		
			printf("\"%s\" Matched \"%s\"\n", s1, s2);
		else
			printf("\"%s\"  Failed \"%s\"\n", s1, s2);
#ifdef DUMP
	    dump();
#endif
		}
	}

#endif
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
