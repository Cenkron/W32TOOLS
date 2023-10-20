/* ----------------------------------------------------------------------- *\
|
|	getcols () - Report the number of columns on the console
|
|	Copyright (c) 1993, all rights reserved, Brian W Johnson
|
|	23-Feb-93
|	28-May-94  Accept environment override
|	16-Aug-97  WIN32
|
\* ----------------------------------------------------------------------- */

#include <windows.h>

#include <stdlib.h>

#include  "fWild.h"

/* ----------------------------------------------------------------------- */
	int
getcols (void)

	{
	CONSOLE_SCREEN_BUFFER_INFO  csbi;
	int     cols;
	ULONG   ltemp;
	char   *p;


	if (p = getenv("GETCOLS"))
		{
		ltemp = strtoul(p, NULL, 0);
		cols  = (USHORT)(min(ltemp, 255L));
		}
	else
		{
		if (GetConsoleScreenBufferInfo(
				GetStdHandle(STD_OUTPUT_HANDLE),
				&csbi))
			cols = csbi.dwSize.X;
		else
			cols = 80;
		}

	return (max(cols, 80));
	}

/* ----------------------------------------------------------------------- */
