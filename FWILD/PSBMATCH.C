/* ----------------------------------------------------------------------- *\
|
|				   PSBMATCH
|		           Wild Card Path Name Test
|
|		   Copyright (c) 1993, all rights reserved
|				Brian W Johnson
|				   30-Jan-93
|				   17-Aug-97
|
|	Drive specifications, if any, are matched before matching the rest
|
|	This file is deprecated.  It is no longer in use, left here for historical access.
|	This file has been removed from the build.
|
\* ----------------------------------------------------------------------- */

#include  <windows.h>
#include  <ctype.h>
#include  <stdlib.h>

// #include  <stdio.h>

#define  FWILD_INTERNAL		// Enable internal definitions

#include  "fWild.h"

/* ----------------------------------------------------------------------- */
	int			// Return TRUE if the paths match
psubmatch (
	char  *pattern,	// Pattern path (may include wild cards)
	char  *target,	// Target name (must be a non-wild pathname)
	int    eflag)	// If TRUE, missing ext matches any file ext

	{
	int    dflag;
	int    result  = FALSE;
	char  *abspath = NULL;

	if ((pattern == NULL)		// Validate both pointers
	||  (target  == NULL))
		goto exit;

	if ((abspath = fnabspth(target)) == NULL)	// Canonicalize the target path
		goto exit;

	target  = abspath;

	// If the pattern specifies a drive, match the drives

	dflag = (isalpha(*pattern)  &&  (*(pattern + 1) == ':'));
	if (dflag  &&  (toupper(*pattern) != *target))
		goto exit;

	target += 2;			// Strip the drive(s)
	if (dflag)
		pattern += 2;

	// if the pattern specified only a drive, return a match
	// if the pattern is empty, return a mismatch

	if (*pattern == '\0')
		{
		result = dflag;
		goto exit;
		}

	// if the pattern specifies a root, match only against the root

	if ((*pattern == '\\')  ||  (*pattern == '/'))
		{
		result = pmatch(pattern, target, eflag);
		goto exit;
		}

	// Match the pattern against the first character of the target
	// path following each path separator in the target, starting
	// with the first character following the root specification

	do  {
		if (result = pmatch(pattern, ++target, eflag))
			goto exit;

		while (*target  &&  ((*target != '\\')  &&  (*target != '/')))
			++target;
		}  while (*target);

	// If we haven't already matched, return no match

exit :
	if (abspath != NULL)
		free(abspath);		// Free the canonical path
	return (result);
	}

/* ----------------------------------------------------------------------- */
