/* ----------------------------------------------------------------------- *\
|
|	getrows () - Report the number of rows on the console
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

#include  "fwild.h"

/* ----------------------------------------------------------------------- */
	int
getrows (void)

	{
	CONSOLE_SCREEN_BUFFER_INFO  csbi;
	int     rows;
	ULONG   ltemp;
	char   *p;

	if (p = getenv("GETROWS"))
		{
		ltemp = strtoul(p, NULL, 0);
		rows  = (USHORT)(min(ltemp, 255L));
		}
	else
		{
		if (GetConsoleScreenBufferInfo(
				GetStdHandle(STD_OUTPUT_HANDLE),
				&csbi))
			rows = csbi.dwSize.Y;
		else
			rows = 25;
		}

	return (max(rows, 25));
	}

/* ----------------------------------------------------------------------- */
