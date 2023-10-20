/* ------------------------------------------------------------------------ *\
|
|							  fwLastFound.c
|
|  This function uses the fWild system to return a pointer to the first
|  filename found matching the supplied fWild pattern pathname.
|
|  The fWild system is gracefully terminated with all memory freed.
|
| - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
|
|	16-Jan-93	Copyright (c) 1993, Brian W. Johnson,
|			All rights reserved
|	17-Dec-94	Buffer increased
|
\* ------------------------------------------------------------------------ */

#include  <windows.h>
#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>

#define FWILD_INTERNAL

#include  <fWild.h>

// ---------------------------------------------------------------------------
//	fwLastFound - Return the name of the found file
// ---------------------------------------------------------------------------
	char *				// Ptr to the last found filespec
fwLastFound (
	PFWH  hp)			// Pointer to the fWild instance

	{
	if (fwActive(hp))
		return (hp->file_name);

	return (NULL);	
	}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
