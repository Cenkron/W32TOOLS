/* ----------------------------------------------------------------------- *\
|
|	fnValidName () - Syntactically validate a filename path string
|
|	Copyright (c) 1991, all rights reserved, Brian W Johnson
|
|	1-Oct-22	New file
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <ctype.h>

#include  "fwild.h"

/* ----------------------------------------------------------------------- */

#define  S_NONE			(0)		// No path characters scanned yet
#define  S_ELEMENT		(1)		// Path element last scanned
#define  S_SEPARATOR	(2)		// Separator last scanned

#define isValidSeparator(ch)	(((ch) == '\\') || ((ch) == '/'))

/* ----------------------------------------------------------------------- *\
|  fnValidName () - Scan and validate a path as a potential filename
\* ----------------------------------------------------------------------- */
	int					// Return TRUE for success, FALSE for failure
fnValidName(
	char* pathname)		// Pointer to the test pathname

	{
	char* p;						// Pointer into the string
	char	ch;						// The current character
	int		state = S_NONE;		// Initialize the FSM state to the last recognized type

	// Scan and detect any prefix

	if ((p = QueryUNCPrefix(pathname)) != NULL)	// Check for UNC prefix
		{
		// Require initial element
		state = S_SEPARATOR;		// Because UNC is inherently rooted		
		}
	else if ((p = QueryDrivePrefix(pathname, TRUE)) != NULL)	// Check for Drive prefix
		{
		state = S_NONE;		// Permit either root separator, or element
		}
	else	// No prefix found (or perhaps ill formed)
		{
		p = pathname;		// No prefix, just point the string
		state = S_NONE;		// Permit either root separator, or element
		}

	// Scan through the remaining string, performing the pattern match algorithm
	// 1. No root separator following a UNC prefix
	// 2. No repeated separators
	// 3. Path element characters are all valid.
	// 4. Ends in valid path element character(s)

	while (ch = *(p++))
		{
		if (isValidSeparator(ch))
			{
			if (state == S_SEPARATOR)
				return (FALSE);				// Invalid repeated separators
			else
				state = S_SEPARATOR;
			continue;
			}

		else if (isValidPath(ch))
			state = S_ELEMENT;

		else
			return (FALSE);					// Invalid character encountered
		}

	return (state == S_ELEMENT);			// Success if ended in a non-zero length path element
	}

// ------------------------------------------------------------------------------------------------
