/* ------------------------------------------------------------------------ *\
|
|								  showFileAttr
|
|				Copyright (c) 1985, 2023, all rights reserved
|								Brian W Johnson
|									1-Oct-23  (New)
|
|		char *				Returns string showing attributes
|	showFileAttr (
|		int attr)			Attribute bitmap
|
\* ------------------------------------------------------------------------ */

#include  <windows.h>
#include  <stdio.h>
#include  <stdlib.h>

#include  "fWild.h"

//----------------------------------------------------------------------------
//	Translate file and search attributes to a diagnostic string
//----------------------------------------------------------------------------
	char *
showFileAttr (
	int mode)

	{
static char  modeStr [40];	// The returned mode string

	strcpy(modeStr, "{ ");
	if (mode & ATT_FILE)
		strcat(modeStr, "FILE ");
	if (mode & ATT_DIR)
		strcat(modeStr, "DIR ");
	if (mode & ATT_RONLY)
		strcat(modeStr, "RONLY ");
	if (mode & ATT_HIDDEN)
		strcat(modeStr, "HIDDEN ");
	if (mode & ATT_SYSTEM)
		strcat(modeStr, "SYSTEM ");
	if (mode & ATT_ARCH)
		strcat(modeStr, "AR ");
	if (mode & FW_AD)
		strcat(modeStr, "AD");
	strcat(modeStr, " }");

	return (modeStr);
	}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
