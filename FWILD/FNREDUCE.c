/* ----------------------------------------------------------------------- *\
|
|				    FNREDUCE
|
|		    Copyright (c) 1985, 1990, all rights reserved
|				Brian W Johnson
|				   26-May-90
|				   25-Sep-97 UNC
|				   10-Aug-00 .xxxx filenames
|				   19-Sep-22 corrected UNC skipover
|				   10-Sep-23 rewrote the fnCondense function (checks depth)
|
|	    int				Returns positive effective depth, or -1 for error
|	fnreduce (s);		Eliminate pathname redundancy
|	    char  *s;		Pointer to the pathname
|
|	The return string is reduced to minimal form,
|	and the path characters are standardized to '\\'.
|
|	Rules:
|	Initial drive identification is always preserved.
|	Works with drivespec, UNC and defaultdrive paths
|	Initial root slash is always preserved.
|	Initial parent directory notation ".." is always preserved (if legal).
|	".." following real path components are always reduced.
|	".." preceding real path components are always retained.
|	".." elements in a relative path are validated re the CWD.
|	"." components are always excised, unless trailing.
|	Trailing "/" is always removed, unless it was initial.
|	Trailing "." is left if the path does not end in a name.
|	Reports minimum depth parsed, else -1 if too deep.
| 
|	NOTE:	Input string "." will produce an empty result string !
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <ctype.h>
#include  <string.h>

#include  "fwild.h"


//#define TEST	// Define this to include the test main and diagnostics

#define	 PATHCH		('\\')
#define	 NULCH		('\0')
#define	 DOTCH		('.')
#define	 DOUBLEDOT	("..")
#define  ispath(ch)	(ch == PATHCH)

#define C_PATH	(256) // arbitrary

#define  DRIVESPEC(ch) ((int)(tolower(ch) - 'a' + 1))

static	int		rooted = FALSE;

#define ERROR   (-1)
#define SUCCESS (0)
#define	MAX_RECORD (64)

typedef
	struct
		{
		char   *pSep;		// Pointer to the preceding separator (else NULL)
		char   *pField;		// Pointer to the field
		int		isNamed;	// True iff a named field (not "." or "..")
		} RTABLE, *PRTABLE;

RTABLE	Rtable [MAX_RECORD];

// ------------------------------------------------------------------------------------------------
// Condense the path pointed by s (uses a recursive algorithm)
//
// Condense should be called pointing after the prefix and the possible root path char.
// ------------------------------------------------------------------------------------------------
    static int						// Returns 0 for success, else depth required (depth >= 0)
fnCondense (						// Condense '.' and '..' from the path name
	int    CWDdepth,				// Depth of the effective CWD
	int    rooted,					// True if the path is rooted
    char  *s)						// Ptr to the path suffix

    {
	char	ch;						// Current working character
	PRTABLE	pRecord = NULL;			// Pointer to the current field record
	PRTABLE	pPrevRecord;			// Ptr to the previous record (when valid)
	int     minDepth = CWDdepth;	// Minimum depth reached
	int     depth    = CWDdepth;	// Current directory depth (cannot go negative)
	int     length;					// Length of the current record
	int     Running;				// TRUE while parsing
	char   *p;						// Working pointer into path

	if (s == NULL)					// Null string ptr is illegal
		return (ERROR);

	if (strlen(s) == 0)				// Empty string is legal
		return (CWDdepth);

	strsetp(s, PATHCH);				// Normalize all path characters to '\'


	Running = TRUE;
	p = s;															// Point the first name field
//	pRecord = NULL;													// Indicate no pRecord yet
	while (Running)	// Enter outer loop								// Process all fields
		{
#ifdef TEST
printf("New Rec (%2d) \"%s\"\n", depth, p);
fflush(stdout);
#endif
		if (pRecord == NULL)
			{
			pRecord = &Rtable[0];									// Point the first record
			pRecord->pSep = (NULL);
			}
		else
			{
			++pRecord;												// Point the next record
			pRecord->pSep = (p - 1);
			}
		pRecord->pField  = p;
		pRecord->isNamed = FALSE;

#ifdef TEST
printf("NEW R(%d)\n", (int)(pRecord - &Rtable[0]));
fflush(stdout);
#endif
		while (Running)	// Enter inner loop							// Process all field characters
			{
			ch = *p;												// p points the current field character

			switch (ch)
				{
				case NULCH:											// We have reached the end of the parsed string
					{
#ifdef TEST
printf("(%2d) \"%s\"\n", depth, s);
printf("NUL R(%d)\n", (int)(pRecord - &Rtable[0]));
fflush(stdout);
#endif
					Running = FALSE;								// EOL found; finish this section, then done
					length = (int)(p - pRecord->pField);
					break; // and return success

					if (length == 0)
						{
#ifdef TEST
printf("NUL Dup (%2d) \"%s\"\n", depth, p);
fflush(stdout);
#endif
						if (pRecord->pSep)							// If not the first element, (trailing "\")
							*pRecord->pSep = NULCH;					// Delete the (not first) element
						else
							*pRecord->pField = NULCH;				// Delete the first element (likeley redundant)
						break; // and return success
						}

					else if ((length == 1) && (*pRecord->pField == DOTCH))
						{
#ifdef TEST
printf("NUL Dot (%2d) \"%s\"\n", depth, p);
fflush(stdout);
#endif
						if ((pRecord->pSep)							// If not the first element,
						&&  ((pRecord - 1)->isNamed))				// and the previoous record is a name
							*pRecord->pSep = NULCH;					// Delete the (not first) element
						break; // and return success				// Else, leave the trailing dot
						}

					else if ((length == 2) && (strncmp(pRecord->pField, DOUBLEDOT, 2) == 0))
						{
#ifdef TEST
printf("NUL DD  (%2d) \"%s\"\n", depth, p);
fflush(stdout);
#endif
						if (pRecord->pSep == NULL)					// If this is the first record
							{
#ifdef TEST
printf("1\n");
fflush(stdout);
#endif
							if (--depth < 0)
								return (ERROR);						// Depth error
							else
								break; // and return success		// OK, accept the ".."
							}

						pPrevRecord = (pRecord - 1);				// Not the first record; check the previous record

						if (pPrevRecord->isNamed)					// If the preceding record is named,
							{
#ifdef TEST
printf("2\n");
fflush(stdout);
#endif
							if (pPrevRecord->pSep != NULL)
								*pPrevRecord->pSep = NULCH;			// Excise both records and the previous separator
							else
								*pPrevRecord->pField = NULCH;		// Excise both (all) records

							if (--depth < 0)						// Previous field is not named
								return (ERROR);						// Depth error

							if (strlen(s) == 0)						// Minimal valid path is "."
								strcat_s(s, 2, ".");
							break; // and return success
							}

						else if (--depth < 0)						// Previous field is not named
							return (ERROR);							// Depth error
						else
							{
#ifdef TEST
printf("3\n");
fflush(stdout);
#endif
							break; // and return success			// OK, accept the ".."
							}
						}
					} // End case NULCH


				case PATHCH:										// We have reached the end of a field
					{
#ifdef TEST
printf("(%2d) \"%s\"\n", depth, s);
printf("PTH R(%d)\n", (int)(pRecord - &Rtable[0]));
fflush(stdout);
#endif
					length = (int)(p - pRecord->pField);

					if (length == 0)
						{
#ifdef TEST
printf("PTH Dup (%2d) \"%s\"\n", depth, p);
fflush(stdout);
#endif
						return (ERROR);								// Name character required, else illegal "\\"
						}

					else if ((*(p+1) == NULCH)						// Check if this is a trailing path
						 &&  (p > s))								// and not the first string character
						{
						*p = NULCH;									// Yes, truncate it, leave the previous name
						Running = FALSE;
						break;	// and return success				// Leave the trailing name
						}

					else if ((length == 1) && (*pRecord->pField == DOTCH))
						{
#ifdef TEST
printf("PTH Dot (%2d) \"%s\"\n", depth, p);
printf("(%2d) \"%s\"\n", depth, s);
printf("(%2d) \"%s\"\n", depth, p);
fflush(stdout);
#endif
						strcpy(pRecord->pField, (p + 1));	// Excise this ".\" element
#ifdef TEST
//printf("(%2d) \"%s\"\n", depth, s);
//printf("(%2d) \"%s\"\n", depth, p);
//fflush(stdout);
#endif
						p       -= 2;								// Next record is now here
						pRecord -= 1;								// Backup to the previous record (allow for the ++pRecord)
#ifdef TEST
printf("(%2d) \"%s\"\n", depth, s);
printf("(%2d) \"%s\"\n", depth, p);
fflush(stdout);
#endif
						break;										// (and allow for the ++p) BWJ
						}

					else if ((length == 2) && (strncmp(pRecord->pField, DOUBLEDOT, 2) == 0))
						{
#ifdef TEST
printf("PTH DD  (%2d) \"%s\"\n", depth, p);
fflush(stdout);
#endif
						if (pRecord->pSep == NULL)					// If this is the first record
							{
#ifdef TEST
printf("1 %d\n", (int)(pRecord - &Rtable[0]));
fflush(stdout);
#endif
							if (--depth < 0)
								return (ERROR);						// Depth error
							else
								break;								// OK, accept the ".."
							}

						pPrevRecord = (pRecord - 1);				// Not the first record; check the previous record

						if (pPrevRecord->isNamed)					// If the preceding record is named,
							{
#ifdef TEST
printf("2\n");
fflush(stdout);
#endif
							strcpy(pPrevRecord->pField, (p+1));	// Excise both records

							pRecord = pPrevRecord;					// Back up to the previous record
							pRecord->isNamed = FALSE;				// Reinit the record
							p = (pRecord->pField -1);				// Next record is now here (allow for the ++p)
							if (--depth < 0)
								return (ERROR);						// Depth error
							break;
							}

						--depth;									// Preceding not named
#ifdef TEST
printf("3\n");
fflush(stdout);
#endif
						if (depth < 0)								// Preceding not named
							return (ERROR);							// Depth error
						else
							break;									// OK, accept the ".."
						}
					
					else // Declare this element named
						{
						++depth;									// Valid named field increases depth margin
#ifdef TEST
printf("PTH Nam (%2d) \"%s\"\n", depth, p);
fflush(stdout);
#endif
						pRecord->isNamed = TRUE;					// Accept the name record
						break;
						}
					} // End case PATHCH

				default:											// Processing a name field
					{
#ifdef TEST
printf("ch      (%2d) \"%s\"\n", depth, p);
fflush(stdout);
#endif
					break;											// Dummy default
					}
				} // End switch

			if ((ch == PATHCH)										// End of name; start a new record
			|| (Running == FALSE))									// Finished; return SUCCESS
				{
				++p;												// Point the next (name) character, if any
				break; // and return SUCCESS
				}

			++p;													// Advance to the next character
			} // End of inner loop

		if (depth < minDepth)
			minDepth = depth;

		if (Running == FALSE)
			break; // and return SUCCESS
		} // End of outer loop

#ifdef TEST
printf("fnreduce Return (%2d) \"%s\"\n", minDepth, s);
fflush(stdout);
#endif
	return (minDepth);				// Return the minimum reached depth of the directory
	}

// ------------------------------------------------------------------------------------------------
// fnreduce - Remove any redundancies to get the shortest possible path
// ------------------------------------------------------------------------------------------------
	int					// Returns minimum depth of path (negative if an error)
fnreduce (				// Eliminate pathname redundancy
    char  *s)			// Pointer to the pathname string

	{
	char  *p = s;		// Pointer to the path following any prefix and root separator
	char  *pTemp;		// Returned result from prefix queries
	int    CWDdepth;	// The effective CWD depth


	if (s == NULL)
		return (ERROR);

#ifdef TEST
printf("fnreduce Entry: \"%s\"\n", s);
fflush(stdout);
#endif

	strsetp(s, PATHCH);				// Normalize the standard path character in the string

	if ((pTemp = QueryDrivePrefix(s, TRUE)) != NULL)	// Single mode
		{
		p = pTemp;					// We have a drive spec; point the root
		if (ispath(*p))				// If we now find a root separator
			{						// - Drive spec, rooted
			rooted   = TRUE;		// Yes, we are rooted (absolute path)
			CWDdepth = 0;			// so the CWD depth is zero
			++p;					// Skip over the root separator
			}
		else // Unrooted			// - Drive spec, unrooted
			CWDdepth = getdepth(DRIVESPEC(*s));
		}

	else if ((pTemp = QueryUNCPrefix(s)) != NULL)
		{
		p = pTemp;					// - UNC spec, rooted (by definition)
		rooted   = TRUE;			// UNC header is by definition rooted (absolute path)
		CWDdepth = 0;
		}

	else if (ispath(*p))			// No prefix found; if we have a root separator
		{							// - No Drive/UNC spec, rooted
		rooted   = TRUE;			// Yes, we are rooted (absolute path)
		CWDdepth = 0;
		++p;						// Skip over the root separator
		}
	else							// - No Drive/UNC spec, unrooted
		{
		rooted   = FALSE;			// We are not rooted (relative path)
		CWDdepth = getdepth(0);		// Request for the default drive
		}

	return (fnCondense(CWDdepth, rooted, p));	// Condense the path elements
	}

/* ----------------------------------------------------------------------- */
#ifdef TESTM
main(					/* Test main program */
	int    argc,
	char* argv[])

	{
	int	result;

	if (argc < 2)
		printf("No input\n");
	else
		{
		char *pTest = argv[1];
		printf("Input:  \"%s\"\n", pTest);
		result = fnreduce(pTest);
		printf("Output: \"%s\"\n", pTest);
		printf("Result:  %d\n", result);
		}
	}
#endif
/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */
