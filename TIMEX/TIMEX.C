/*--------------------------------------------------------------------*\
|  TIMEX - Time the execution of a command
\*--------------------------------------------------------------------*/

#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <process.h>

#include <fWild.h>

/*--------------------------------------------------------------------*/
	char * 
usagedoc [] = {
	"Usage:  timex  command arguments",
	"",
	"Version 1.0 Copyright (c) 1993 J & M Software, Dallas TX - All Rights Reserved",
	NULL};

/*--------------------------------------------------------------------*/

long	SystemTimeDiff (SYSTEMTIME *dtStart, SYSTEMTIME *dtEnd, SYSTEMTIME *dtDiff);

/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
	int
main (
	int   argc,
	char *argv[])

	{
	int		c;
	SYSTEMTIME	dtStart;
	SYSTEMTIME	dtEnd;
	SYSTEMTIME	dtElapsed;

	optenv = getenv("TIMEX");

	while ( (c=getopt(argc,argv,"?")) != EOF )
		{
		switch (tolower(c))
			{
			case '?':
			help();

		default:
			fprintf(stderr, "invalid option '%c'\n", optchar);
			usage();
			}
		}

	if (optind < argc)
		{
		GetSystemTime(&dtStart);

		spawnvp(P_WAIT, argv[optind], &argv[optind]);

		GetSystemTime(&dtEnd);

		SystemTimeDiff(&dtStart, &dtEnd, &dtElapsed);

		printf("\nElapsed time: %d:%02d.%03d\n",
			(int) dtElapsed.wMinute,
			(int) dtElapsed.wSecond,
			(int) dtElapsed.wMilliseconds);
		}

	return(0);
	}

/*--------------------------------------------------------------------*/
	long
SystemTimeDiff (
	SYSTEMTIME *dtStart,
	SYSTEMTIME *dtEnd,
	SYSTEMTIME *dtDiff)

	{
	long	lStart;
	long	lEnd;
	long	lElapsed;
    
	lStart  = dtStart->wMilliseconds
			+ dtStart->wSecond * 1000L
			+ dtStart->wMinute * 1000L * 60L
			+ dtStart->wHour   * 1000L * 60L * 60L;

	lEnd    = dtEnd->wMilliseconds
			+ dtEnd->wSecond   * 1000L
			+ dtEnd->wMinute   * 1000L * 60L
			+ dtEnd->wHour     * 1000L * 60L * 60L;

	lElapsed = lEnd - lStart;

	if (dtDiff)
		{
		*dtDiff = *dtEnd;

		dtDiff->wMilliseconds = (UCHAR) ((lElapsed) % 1000L);
		dtDiff->wSecond       = (UCHAR) ((lElapsed / (1000L)) % 60L);
		dtDiff->wMinute       = (UCHAR) ((lElapsed / (1000L*60L)) % 60L);
		dtDiff->wHour         = (UCHAR) ((lElapsed / (1000L*60L*60L)) % 24L);

		//dtDiff->day = ;
		//dtDiff->month = ;
		//dtDiff->year = ;
		}

	return (lElapsed);
	}

/*--------------------------------------------------------------------*/
/*-------------------------- EOF -------------------------------------*/
/*--------------------------------------------------------------------*/
