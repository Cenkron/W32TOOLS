/*********************************************************************\
				XCOPY
 ---------------------------------------------------------------------
                        Extended COPY utility
 ---------------------------------------------------------------------
			     KILL routine
 ---------------------------------------------------------------------
 Copyright (c) 1986-2018 Miller Micro Systems - All Rights Reserved
                    Written by Michael S. Miller
\*********************************************************************/

#include <dos.h>
#include <stdio.h>
#include <dtypes.h>

#include "xcopy.h"

/**********************************************************************/
#define VERSION "940602.092145"
/**********************************************************************/
	EXPORT void
kill (
	char *src)

	{
	if (azFlags.k && !azFlags.n)
		{
		if (azFlags.r && is_readonly(src))
			clr_readonly(src);

		if (!azFlags.l)
			notify(DELETING, src, NULL, NULL);
#ifdef DEBUG
printf("\nunlink kill \"%s\"\n", src);
#endif
		if (unlink(src) == 0)
			printf(" OK\n");
		else
			printf(" failed\n");
		}
    }

/*--------------------------------------------------------------------*/
/*---------------------------EOF--------------------------------------*/
/*--------------------------------------------------------------------*/
