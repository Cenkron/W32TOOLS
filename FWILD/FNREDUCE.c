/* ----------------------------------------------------------------------- *\
|
|					    FNREDUCE
|
|			    Copyright (c) 1985, 1990, all rights reserved
|					Brian W Johnson
|					   26-May-90
|					   25-Sep-97 UNC
|					   10-Aug-00 .xxxx filenames
|					   19-Sep-22 corrected UNC skipover
|					   10-Sep-23 rewritten
|					   17-Sep-23 reorganized
|
|	    int				Returns effective path depth, non-negative if OK, or negative for bad
|	fnreduce (s);		Eliminate pathname redundancy
|	    char  *s;		Pointer to the pathname
|
|	    int				Returns relative depth of the tested path
|	fnCondense (s);		Eliminate pathname redundancy
|	    char  *s;		Pointer to the pathname
|
|	Both functions work similarly, but only fnreduce can handle a path prefix.
|
|	Both functions return -1 if a NULL pointer passed
|	The return string is reduced to minimal form,
|	and the path characters are standardized to '\\'.
|
|	The return values are the depth of the passed pathspec pointer.
|	Depth refers to the degree of ''..'ing enountered during the parse.
|	Negative total depth is physically impossible to access.
|	The depth must be non-negative for physical access.
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
|	NOTE:	Some input strings will produce an empty result string !
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <ctype.h>
#include  <string.h>

#include  "fwild.h"

// ------------------------------------------------------------------------------------------------

//#define TEST	// Define this to include the test main

//#define DEBUG	// Define this to include the diagnostics

#ifdef  TEST
#define DEBUG
#endif

#define	PATHCH		('\\')
#define	NULCH		('\0')
#define	DOTCH		('.')
#define	DOUBLEDOT	("..")
#define ispath(ch)	(ch == PATHCH)

#define ERROR   (-1)		// Returned if a NULL ptr to the path string

#define DRIVESPEC(ch) ((int)(tolower(ch) - 'a' + 1))

#define	MAX_RECORD (100)	// Maximum parseable depth (very generous)

typedef
	struct
		{
		char   *pSep;		// Pointer to the preceding separator (else NULL)
		char   *pField;		// Pointer to the field
		int		length;		// Length of the name part of the field (excludes preceding path)
		int		isName;		// True iff a named field (i.e., not "..")
		} Element, *PElement;

Element	Etable [MAX_RECORD];

#define	isFirst(pElement)	(pElement->pSep == NULL)

// BWJ issue of fnCondenseDot()

// ------------------------------------------------------------------------------------------------
// Condense the path pointed by s (uses a recursive algorithm)
// Condense should be called pointing after the prefix and the possible root path char.
// ------------------------------------------------------------------------------------------------
	int								// Returns path depth
fnCondense (						// Condense '.' and '..' from the path name
	char  *s)						// Ptr to the path suffix (follows any root character)

	{
	char		ch;					// Current working character
	PElement	pElement = NULL;	// Pointer to the current field element
	PElement	pPrevElement;		// Ptr to the previous element (when valid)
	int			minDepth = 0;		// Minimum directory depth reached
	int			depth    = 0;		// Current directory depth
	int			Running;			// TRUE while parsing
	char	   *p;					// Working pointer into path

	if (s == NULL)					// Null string ptr is illegal
		return (ERROR);

	if (strlen(s) == 0)				// Empty string is legal
		return (0);

	pElement = &Etable[0];									// Init and use the first element
		pElement->pSep   = (NULL);							// First element; no initial separator
		pElement->pField = s;								// Point the element first char
		pElement->length = 0;								// Element length defaults to zero
		pElement->isName = FALSE;							// No name characters yet

	p = s;													// Point the first element name field
	for (Running = TRUE; (Running); )						// Process all fields and characters
		{
		ch = *p;											// p points the current field character
		if (ch == '/')
            ch = PATHCH;									// Replace old style path characters

		pElement->length = (int)(p - pElement->pField);		// Update the current field length
		switch (ch)
			{
			case NULCH:										// We have reached the end of the parsed string
				{
				Running = FALSE;							// EOL found; finish this section, then we are done
#ifdef DEBUG
printf("(%2d) \"%s\"\n", depth, s);
printf("NUL E(%d)\n", (int)(pElement - &Etable[0]));
fflush(stdout);
#endif
				if (pElement->length == 0)					// If the element is empty
					{
#ifdef DEBUG
printf("NUL Empty (%2d) \"%s\"\n", depth, p);
fflush(stdout);
#endif
// Can't happen		if (isFirst(pElement))					// If the first element, the string is empty
//						{
//						strcpy(s, ".");						// Convert to "." path
//						break; // and return				// The path is empty
//						}
							
					if (pElement->pSep != NULL)				// If there is a preceding separator,
						*pElement->pSep = NULCH;			// Delete this element and the remaining trailing separator

					break; // and return
					} // End (element length == 0)

				else if ((pElement->length == 1) && (*(pElement->pField) == DOTCH))
					{
#ifdef DEBUG
printf("NUL Dot (%2d) \"%s\"\n", depth, p);
fflush(stdout);
#endif
					if (! isFirst(pElement)					// If not the first element,
					&&  ((pElement - 1)->isName))			// and the previous element is a name
						*pElement->pSep = NULCH;			// Delete the (not first element) "\." element

					break; // and return					// Else (first element), leave the initial dot
					} // End (element length == 1)

				else if ((pElement->length == 2) && (strncmp(pElement->pField, DOUBLEDOT, 2) == 0))
					{
//					pElement->isName = FALSE;				// Not a name (but doesn't matter)
#ifdef DEBUG
printf("NUL DD  (%2d) \"%s\"\n", depth, p);
fflush(stdout);
#endif
					if (isFirst(pElement))					// If this is the first element
						{
#ifdef DEBUG
printf("1\n");
fflush(stdout);
#endif
						--depth;
						break; // and return				// Accept the initial ".."
						}

					pPrevElement = (pElement - 1);			// Not the first element; check the previous element
					if (pPrevElement->isName)				// If the preceding element is a name element
						{
#ifdef DEBUG
printf("2\n");
fflush(stdout);
#endif
						if (! isFirst(pPrevElement))		// If the prev name element is NOT the first element
							{
							*pPrevElement->pSep = NULCH;	// Excise both elements and the previous separator
							--depth;						// Cancels the previous element depth
							}

						else // Previous element IS the first element
							{
							strcpy(s, ".");					// Convert to "." path
							depth = 0;						// Cancels the previous element depth
							}
						}
					else
						{
						--depth;							// Previous element is not a name, accept the ".."
						}

					break; // and return		
					} // End (element length == 2)

				// else we were parsing a name field (and length > 0) (proven above)

				++depth;
//				if (pElement->length > 2)
//					pElement->isName = TRUE;				// but doesn't matter

				break;	// Its a good place to end
				} // End case NULCH


			case PATHCH:									// We have reached the end of an element
				{
#ifdef DEBUG
printf("(%2d) \"%s\"\n", depth, s);
printf("PTH E(%d)\n", (int)(pElement - &Etable[0]));
fflush(stdout);
#endif
				if (pElement->length == 0)
					{
#ifdef DEBUG
printf("PTH Dup (%2d) \"%s\"\n", depth, p);
fflush(stdout);
#endif
					// Excise this "\\" element, reusing the same element and preceding separator (if any)
						
//					pElement = pElement;					// Reuse this element record for the next element
//					pElement->pSep;							// Pointer to the separator stays the same
//					pElement->pField; stays the same		// Pointer to the element field stays the same
					pElement->length = 0;					// Reset the length
//					pElement->isName = FALSE;				// Still not a name

					// Do the actual excision copy, and readjust the current char pointer

					strcpy(pElement->pField, (p + 1));		// Excise this "\\" element
					p = pElement->pField;					// Point first character of the new field (no real change)
					break;									// Accept it
					} // End (element length == 0)

				else if ((pElement->length == 1) && (*(pElement->pField) == DOTCH))
					{
#ifdef DEBUG
printf("PTH Dot (%2d) \"%s\"\n", depth, p);
printf("(%2d) \"%s\"\n", depth, s);
printf("(%2d) \"%s\"\n", depth, p);
fflush(stdout);
#endif
					if ((isFirst(pElement))
					&&  (*(p+1) == NULCH))
						{
                        *p = NULCH;							// Leave only the dot
						break;
						}

					else
						{
						// Excise this ".\" element, reusing the same element and preceding separator
						
//						pElement = pElement;				// Reuse this element for the next element
//						pElement->pSep;						// Pointer to the separator stays the same
//						pElement->pField; stays the same	// Pointer to the element field stays the same
						pElement->length = 0;				// Reset the length
//						pElement->isName = FALSE;			// Still not a name

						// Do the actual excision copy, and readjust the current char pointer

						strcpy(pElement->pField, (p + 1));	// Excise this ".\" element (first element or not)
						p = pElement->pField;				// Point first character of the new field
#ifdef DEBUG
printf("(%2d) \"%s\"\n", depth, s);
printf("(%2d) \"%s\"\n", depth, p);
fflush(stdout);
#endif
						}
					break;									// Accept it
					} // End (element length == 1) (and a ".")
				
				else if ((pElement->length == 2) && (strncmp(pElement->pField, DOUBLEDOT, 2) == 0))
					{
					pElement->isName = FALSE;				// Not a name
#ifdef DEBUG
printf("PTH DD  (%2d) \"%s\"\n", depth, p);
fflush(stdout);
#endif
					// If either this is the first element, or the previous element is not a name, we must accept it

					pPrevElement = (pElement - 1);			// This pointer is valid only if not on the first element
					if ((isFirst(pElement))					// If this is the first element,
					||  (! pPrevElement->isName))			// or the preceding element is NOT a name,
						{
#ifdef DEBUG
printf("1 %d\n", (int)(pElement - &Etable[0]));
fflush(stdout);
#endif
						--depth;							// Adjust the depth
						++pElement;							// Advance to the next element
						pElement->pSep = p;					// Pointer to the separator
						pElement->pField = (p + 1);			// Pointer to the field
						pElement->length = 0;				// Clear the length
//						pElement->isName = FALSE;			// Not yet a name

						p = pElement->pField;				// Advance to the next first character of the new field
						break;								// Accept the ".."
						}

					else // We can excise both this element and the previous element, because they cancel
						{
#ifdef DEBUG
printf("2\n");
fflush(stdout);
#endif
						--depth;							// Adjust the depth (foe loss of the named element)
						--pElement;							// Back up to the previous element
//						pElement->pSep;						// Pointer to the separator stays the same
//						pElement->pField					// Pointer to the element field stays the same
						pElement->length = 0;				// Clear the length
						pElement->isName = FALSE;			// No longer a name

						// Do the actual excision copy, and readjust the current char pointer

						strcpy(pElement->pField, (p+1));	// Excise this and the previous (named) element
						p = pElement->pField;				// Advance to the next first character of the new field
						break;								// Accept the excision
						}

					break;									// (Dummy)
					} // End (element length == 2) (and a "..")

				else // We have reached the end of the current (named) element (and length > 0 proven above)
					{
					++depth;
#ifdef DEBUG
printf("PTH Nam (%2d) \"%s\"\n", depth, p);
fflush(stdout);
#endif
					// So, we need a new element for the next element

					++pElement;								// Advance to the next element
					pElement->pSep = p;						// Pointer to the separator
					pElement->pField = (p + 1);				// Pointer to the element field
					pElement->length = 0;					// Clear the length
					pElement->isName = FALSE;				// Not yet a name

					p = pElement->pField;					// Advance to the next first character of the new field
					break;
					}

				break;
				} // End case PATHCH


			default: // element name character				// Processing a character of an element
				{
#ifdef DEBUG
printf("ch      (%2d) \"%s\"\n", depth, p);
fflush(stdout);
#endif
				if ((ch != '.')
				||  (pElement->length > 2))
					pElement->isName = TRUE;				// We have a name
				++p;										// Advance to the next character
				break;
				}
			} // End switch

		if (depth < minDepth)								// Update the minimum depth
			minDepth = depth;

		if (Running == FALSE)								// Finished the inner loop
			break; // and return to the outer loop

		} // End of loop

#ifdef DEBUG
printf("fnCondense Return (%2d, %2d) \"%s\"\n", depth, minDepth, s);
fflush(stdout);
#endif
	return (minDepth);				// Return the minimum reached depth of the directory
	}

// ------------------------------------------------------------------------------------------------
// fnreduce - Remove any redundancies to get the shortest possible path
// ------------------------------------------------------------------------------------------------
	int					// Returns minimum depth of path (negative if physically unreachable)
fnreduce (				// Eliminate pathname redundancy
    char  *s)			// Pointer to the pathname string

	{
	char  *p = s;		// Pointer to the path following any prefix and root separator
	char  *pTemp;		// Returned result from prefix queries
	int    CWDdepth;	// The CWD depth
	int    depth;		// The relative depth of the tested path


	if (s == NULL)					// Null string ptr is illegal
		return (ERROR);

#ifdef DEBUG
printf("fnreduce Entry: \"%s\"\n", s);
fflush(stdout);
#endif

	strsetp(s, PATHCH);				// Normalize the standard path character in the string

	if ((pTemp = QueryDrivePrefix(s, TRUE)) != NULL)	// Single mode
		{
		p = pTemp;					// We have a drive spec; point the root
		if (ispath(*p))				// If we now find a root separator
			{
			CWDdepth = 0;			// rooted, so the CWD depth is zero
			++p;					// Skip over the root separator
			}
		else						// Drive spec, unrooted
			CWDdepth = getdepth(DRIVESPEC(*s));
		}

	else if ((pTemp = QueryUNCPrefix(s)) != NULL)
		{
		p = pTemp;					// - UNC spec, rooted (by definition)
		CWDdepth = 0;
		}

	else if (ispath(*p))			// No prefix found; if we have a root separator
		{
		CWDdepth = 0;				// No Drive/UNC spec, rooted
		++p;						// Skip over the root separator
		}
	else							// No Drive/UNC spec, unrooted
		CWDdepth = getdepth(0);		// Request for the default drive

	depth = fnCondense(p);			// Condense the path elements

	// If the reported effective depth is negative, this is a phsically impossible path.

#ifdef DEBUG
printf("fnreduce Return: \"%s\"\n", s);
fflush(stdout);
#endif

	return (CWDdepth + depth);		// Report the effective depth of the tested path
	}

/* ----------------------------------------------------------------------- */
#ifdef TEST
main ()					/* Test main program */

	{
	char  s [1024];
	int result;

	for (;;)
		{
		printf("\nPattern: ");
		gets(s);
		printf("\n");
		result = fnCondense(s);
		printf("\nResult:  %d      \"%s\"\n\n", result, s);
		}
	}

#endif // TEST
/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */

