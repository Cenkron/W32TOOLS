/* ----------------------------------------------------------------------- *\
|
|				      MVD
|
|		    Copyright (c) 1985, 1990, all rights reserved
|				Brian W Johnson
|				   30-May-92
|				   17-Aug-97
|
|	mvdir(p1, p2, flag, mflag);	Move directory p1 to p2
|
|	p1 and p2 are absolute pathnames of directories.  p2 is created
|	(if necessary), all of the contents of p1 are (recursively) moved
|	to p2, and p1 is removed.  This is equivalent to simply moving
|	p1 to another place in the directory tree.  If flag is TRUE,
|	a message is printed to stdout for each move attempted.  If mflag
|	is TRUE, the old style create/copy/delete mode is used regardless
|	of the environment.
|
\* ----------------------------------------------------------------------- */

#include  <windows.h>
#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <direct.h>

#include  "fWild.h"
#include  "mvd.h"

#ifndef MATCH
#define MATCH	  0
#endif

#define  MODE	(FW_FILE | FW_HIDDEN | FW_SYSTEM | FW_DIR)

extern	void	mvdir_copy (char *p1, char *p2, int l_flag);

/* ----------------------------------------------------------------------- */
	void
mvdir (					/* Move a directory and its contents */
	char  *p1,			/* Pointer to the origin directory */
	char  *p2,			/* Pointer to the destination directory */
    int    l_flag,		/* TRUE if progress is to be printed */
    int    m_flag)		/* TRUE if old mode is to be forced */

	{
	USHORT  rc;			// The FAPI return code
	char    errmsg [80];	// The constructed error message


//??? MAY NOT WANT TO KEEP THE COPY MODE
	if (m_flag)
		mvdir_copy(p1, p2, l_flag);
	else if (rc = rename(p1, p2) != 0)
		{
		sprintf(errmsg, "Unable to move directory: %d", rc);
		fatalerr(errmsg);
		}
	else if (l_flag)
		printf("%-20s  to  %s\n", p1, p2);

#if 0
	if (m_flag  ||  ((_osmode != OS2_MODE)  &&  (_osmajor < 4)))
		mvdir_copy(p1, p2, l_flag);
	else if (rc = DosMove((PCHAR)(p1), (PCHAR)(p2), 0L) != 0)
		{
		sprintf(errmsg, "Unable to move directory: %d", rc);
		fatalerr(errmsg);
		}
	else if (l_flag)
		printf("%-20s  to  %s\n", p1, p2);
#endif
	}

/* ----------------------------------------------------------------------- */
	void
mvdir_copy (			/* Move a directory and its contents */
    char  *p1,			/* Pointer to the origin directory */
    char  *p2,			/* Pointer to the destination directory */
    int    l_flag)		/* TRUE if progress is to be printed */

	{
	char  *p   = NULL;		/* Pointer to temporary pathname */
	void  *hp  = NULL;		/* Pointer to wild file data block */
	char  *fnp = NULL;		/* Input file name pointer */


	if ((hp = fwOpen()) == NULL)
		exit(1);

	if ( ! fnchkdir(p2))	/* Create the destination directory */
		{
		if (l_flag)
			printf("%-20s  to  %s\n", p1, p2);
		if (mkdir(p2))
			fatalerr("Unable to move directory");
		}

	if (fwInit(hp, p1, MODE) != FWERR_NONE)	/* Move all of the files and directories */
		fwInitError(p1);
	while (fnp = fWild(hp))
		{
		if ((stricmp(fnp, p1) != MATCH)
		&&  ( ! fndot(fnp)))
			{
			if ((p = fncatpth(p2, fntail(fnp))) < 0)
				fatalerr("dst filespec error");
			
			if (fwtype(hp) & ATT_DIR)
				mvdir_copy(fnp, p, l_flag);
			else
				{
				if (l_flag)
					printf("%-20s  to  %s\n", fnp, p);
				if (rename(fnp, p))
					fatalerr("Unable to move file");
				}
			free(p);
			}
		}

	rmdir(p1);			/* Remove the source directory */
	hp = fwClose(hp);
	}

/* ----------------------------------------------------------------------- */
