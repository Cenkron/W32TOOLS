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
#include <fWild.h>

#include "xcopy.h"

/* ----------------------------------------------------------------------- */
#define VERSION "940602.092146"
/* ----------------------------------------------------------------------- */

PRIVATE void    move (char *src, char *dst, char *path);

/* ----------------------------------------------------------------------- */
	EXPORT void
process (
	void *hp,
	char *pSrcName,
	char *pDstName,
	char *pDstPath)

	{
	if (v_flag >= 2)
		{
printf("PR src: \"%s\"\n", pSrcName);
printf("PR dst: \"%s\"\n", pDstName);
printf("PR pth: \"%s\"\n", pDstPath);
		}

	fnreduce(pDstName);
	fnreduce(pDstPath);

	if ((! should_copy(pSrcName, hp, pDstName))					// Decide whether to do the copy
	||  (! may_copy(pSrcName, pDstName, pDstPath))
	||  (! can_copy(pSrcName, pDstName, pDstPath)))
		return;													// No, just return


	if (!azFlags.p)												// Not protect mode
		{
		if ((azFlags.k)											// If kill flag set,
		&&  (isSameDevice(pSrcName, pDstName)))					// and (src device == dst device),
			move(pSrcName, pDstName, pDstPath);					// then just move the file

		else // Do actual copy 									// Not protected, or moving
			{
			do	{
				if (copy(pSrcName, pDstName, pDstPath) != 0)	// Copy the file
					break;

				if (((azFlags.v) || (azFlags.k))
				&&  (verify(pSrcName, pDstName, pDstPath) != 0))
					{
					unlink(pDstName);							// Delete the failed verify
					break;
					}

				if (azFlags.k)									// If kill flag set,
					{
					kill(pSrcName);								// Delete the src file
					break;
					}

				} while (FALSE);
			}
		}

	else if (! azFlags.n) // Protect and execute				// Protect mode copy
		{
		strcpy(temp_name, pDstPath);							// Make a tempFile name
		catpth(temp_name, "xcopy.$$$");
		fnreduce(temp_name);

		unlink(temp_name);										// Ensure tempFile doesn't exist
		do	{
			if (copy(pSrcName, temp_name, pDstPath) != 0)		// Copy the file
				break;

			if (verify(pSrcName, temp_name, pDstPath) != 0)
				{
				unlink(temp_name);								// Delete the failed copy
				break;
				}
				
			if (rename(temp_name, pDstName) != 0)				// Rename to requested name
				{
				unlink(temp_name);								// Delete the failed rename
				break;
				}

			if (azFlags.k)										// If kill flag set,
				{
				kill(pSrcName);								// Delete the src file
				break;
				}

			} while (FALSE);

		unlink(temp_name);										// Ensure tempFile is gone
		}

	return;
	}

/* ----------------------------------------------------------------------- */

/* ----------------------------------------------------------------------- */
	PRIVATE void
move (
	char *pSrcName,
	char *pDstName,
	char *pDstPath)

	{
static char path  [MAX_PATH];
	char    drive [1024];
	char    dir   [1024];
	char    fname [1024];
	char    ext   [1024];

	// This code reqires that isSameDevice() has already been called

	if (!azFlags.l)
		{
		notify(MOVING, pSrcName, pDstPath, pDstName);
		}

	if (!azFlags.n)
		{
#ifdef DEBUG
printf("\nunlink process move1 \"%s\"\n", pDstName);
#endif
		unlink(pDstName);				// In case it already existd

		_splitpath(pDstName, drive, dir, fname, ext);
		_makepath(path, drive, dir, NULL, NULL);

		if (rename(pSrcName, pDstName) != 0)
			{
			error(pSrcName, "Unable to rename file");

			if (azFlags.p)
				{
				if (rename(temp_name, pDstName) == (-1))
					error(pDstName, "unsuccessful unprotect");
				}
			}
		else
			{
#ifdef DEBUG
printf("\nunlink process move2 \"%s\"\n", temp_name);
#endif
			unlink(temp_name);
			}
		}

//	if (!azFlags.l && !azFlags.q)
	if (!azFlags.l)
		putchar('\n');
	}

/* ----------------------------------------------------------------------- */

/*--------------------------------------------------------------------*/
	EXPORT void
catpth (
	char *s,
	char *t)

	{
	char *p = fncatpth(s, t);

	if (p)
		{
		strcpy(s, p);
		free(p);
		}
	else
		error(s, "fncatpth error");
	}

/*--------------------------------------------------------------------*/
	EXPORT void
notfound (
	char *fn)

	{
	if ((!azFlags.z)  &&  (!azFlags.w))
		printf("\nFile not found: %s\n",fn);

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
		printf("\nFile error: %s - %s\n", filename, message);

	if (azFlags.x)
		exit(azFlags.z ? 0 : 1);
	}

/*--------------------------------------------------------------------*/
	EXPORT void
fatal (
	char *s)

	{
	if (!azFlags.z)
		printf("\nFatal error: %s\n", s);

	exit(azFlags.z ? 0 : 1);
	}

/**********************************************************************\
                                EOF
\**********************************************************************/
