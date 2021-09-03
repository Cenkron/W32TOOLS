/*********************************************************************\
                                PROCESS
 ---------------------------------------------------------------------
                        Extended COPY utility
 ---------------------------------------------------------------------
 Copyright (c) 1986-2018 Miller Micro Systems - All Rights Reserved
                    Written by Michael S. Miller
\*********************************************************************/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dtypes.h>
#include <fwild.h>
#include "xcopy.h"

/* ----------------------------------------------------------------------- */
#define VERSION "940602.092146"
/* ----------------------------------------------------------------------- */

PRIVATE void    move (char *src, char *dst, char *path);

/* ----------------------------------------------------------------------- */
	EXPORT void
process (
	char *src,
	char *hp,
	char *dst,
	char *path)

	{
	fnreduce(src);
	fnreduce(dst);
	fnreduce(path);
    
	char *      xsrc = fnabspth(src);

	strcpy(temp_name, path);
	catpth(temp_name, "xcopy.$$$");
	fnreduce(temp_name);

	if (!azFlags.n && azFlags.p)
		{
		unlink(temp_name);
		if (rename(dst,temp_name) == (-1))
			error(dst,"unsuccessful protect");
		}

	if (should_copy(src, hp, dst)
	&&  may_copy(src, dst, path)
	&&  can_copy(src, dst, path))
		{
		if (azFlags.k && (xsrc[0] == dst[0]))   /* if kill flag set */
			{                                   /* and src drive = dst drive */
			move(src, dst, path);               /* then just move file */
			}
		else
			{
			if ((copy(src, dst, path) == 0)
			&&  (verify(src, dst, path) == 0))
				{
				kill(src);
				}
			}
		}

	if (xsrc)
		free(xsrc);
	}

/* ----------------------------------------------------------------------- */

/* ----------------------------------------------------------------------- */
	PRIVATE void
move (
	char *src,
	char *dst,
	char *pth)

	{
	char    path  [1024];
	char    drive [1024];
	char    dir   [1024];
	char    fname [1024];
	char    ext   [1024];

	if (!azFlags.l)
		{
		notify(MOVING, src, pth, dst);
		}

	if (!azFlags.n)
		{
		unlink(dst);

		_splitpath(dst, drive, dir, fname, ext);
		_makepath(path, drive, dir, NULL, NULL);
		pathmake(path);

		if (rename(src, dst) != 0)
			{
			error(src, "Unable to rename file");

			if (azFlags.p)
				{
				if (rename(temp_name, dst) == (-1))
					error(dst, "unsuccessful unprotect");
				}
			}
		else
			unlink(temp_name);
		}

//	if (!azFlags.l && !azFlags.q)
	if (!azFlags.l)
		putchar('\n');
	}

/* ----------------------------------------------------------------------- */

/* ----------------------------------------------------------------------- */
	int                         /* Return the index to the filename part */
suffix (                        /* Point the non-directory tail of path s */
	char  *s)                   /* Pointer to the pathname string */

	{
	int    index = 0;           /* Index to filename part of path s */
	char   ch;                  /* Temporary character variable */
	char  *temp;                /* Pointer to a temporary string buffer */
	char  *p;                   /* Pointer to a temporary string buffer */

	if ((p = temp = strdup(s)) != NULL)
		{
		fnreduce(temp);

		index = 0;
		do  {
			if ((ch = *p) != '\0')
				++p;
			if ((ch == '\0') || (ch == ':') || (ch == '/') || (ch == '\\'))
				{
				const char cs = *p;
				*p = '\0';
				if (fnchkdir(temp))
					index = (p - temp);
//				else				// This code breaks UNC copies
//					break;
// printf("test(%2d): \"%s\"\n", index, temp);
				*p = cs;
				}
			} while (ch);

		free(temp);
		}

	return index;
	}

/*--------------------------------------------------------------------*/
	EXPORT void
catpth (
	char *s,
	char *t)

	{
	char * p;

	if ((p=fncatpth(s,t)) != NULL)
		{
		strcpy(s,p);
		free(p);
		}
	}

/*--------------------------------------------------------------------*/
	EXPORT void
notfound (
	char *fn)

	{
	if ((!azFlags.z)  &&  (!azFlags.w))
		printf("\n\aFile not found: %s\n",fn);

	if (azFlags.x)
		{
		if (azFlags.z)
			exit(0);
		else
			exit(1);
		}
	}

/*--------------------------------------------------------------------*/
	EXPORT void
error (
	char *filename,
	char *message)

	{
	if (!azFlags.z)
		printf("\n\aFile error: %s - %s\n", filename, message);

	if (azFlags.x)
		exit(azFlags.z ? 0 : 1);
	}

/*--------------------------------------------------------------------*/
	EXPORT void
fatal (
	char *s)

	{
	if (!azFlags.z)
		printf("\n\aFatal error: %s\n",s);

	exit(azFlags.z ? 0 : 1);
	}

/**********************************************************************\
                                EOF
\**********************************************************************/
