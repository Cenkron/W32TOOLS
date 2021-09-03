/*********************************************************************\
				XCOPY
 ---------------------------------------------------------------------
                        Extended COPY utility
 ---------------------------------------------------------------------
			    MAYCOPY routine
 ---------------------------------------------------------------------
 Copyright (c) 1986-2018 Miller Micro Systems - All Rights Reserved
                    Written by Michael S. Miller
\*********************************************************************/

#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <dtypes.h>

#include "xcopy.h"

/**********************************************************************/
#define VERSION "940602.092146"
/**********************************************************************/

char get_key (void);

/**********************************************************************/
	EXPORT int
may_copy (
	char *src,
	char *dst,
	char *dstpath)

	{
	int         c;
	int		retval;

	if (azFlags.q)
		{
		notify(MAYCOPY, src, dstpath, dst);

		if ( (c=tolower(get_key())) == 'y' )
			retval = TRUE;
		else if (c == 'r')
			{
			azFlags.q = AZ_Flags.q = FALSE;
			retval = TRUE;
			}
		else if ( (c == 'q') || (c == 'c') )
			{
			printf("XCopy terminated\n");
			exit(0);
			}
		else
			retval = FALSE;
		}
	else
		retval = TRUE;

	return retval;
	}

/*--------------------------------------------------------------------*/
	char
get_key (void)
	{
	char	c;

	if (AZ_Flags.q)
		c = (char)(getchar());

	else
		{
		for (;;)
			{
			while (((c = (char)(_getch())) == 0)  ||  (c == (char)(0xE0)))
				_getch();
			c = tolower(c);
			if ((c == 'y')
			||  (c == 'n')
			||  (c == 'r')
			||  (c == 'q')
			||  (c == 'c'))
			break;
			}
		}

	putchar(c);
	putchar('\n');
	return (c);
	}

/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
