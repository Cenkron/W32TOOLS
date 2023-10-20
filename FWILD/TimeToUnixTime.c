/* ----------------------------------------------------------------------- *\
|
|						TimeToUnixTime
|
|			    Copyright (c) 2023, all rights reserved
|					Brian W Johnson
|					    25-Oct-23
|
|		time_t		Return the unix time
|	TimeToUnixTime (
|		time_t  time	// Windows time
|
|		time_t		Return the Windows time
|	UnixTimeToTime (
|		time_t  time	// unix time
|
\* ----------------------------------------------------------------------- */

#include  <time.h>

#include  "fWild.h"

#define	 RESOLUTION	(10000000)			// Accounts for resolution ratio
#define	 DELTA	(116444736000000000)	// Accounts for origin date difference

// ---------------------------------------------------------------------------
//	#define TEST		// Define to build the test program
//	#define DEBUG		// Define to include debug output

#if defined(TEST)
#define DEBUG
#endif

#if defined(DEBUG)
#include <stdio.h>
#endif

// ---------------------------------------------------------------------------
	time_t			// Returns the timestamp in unix format
TimeToUnixTime (
	time_t	wTime)	// Timestamp in Windows format

	{
	return ((wTime - DELTA) / RESOLUTION);
	}

// ---------------------------------------------------------------------------
	time_t			// Returns the timestamp in Windows format
UnixTimeToTime (
	time_t	uTime)	// Timestamp in unix format

	{
	return  ((uTime * RESOLUTION) + DELTA); 
	}

// ---------------------------------------------------------------------------
//	Test program
// ---------------------------------------------------------------------------
#ifdef TEST

#include <stdio.h>
#include <stdlib.h>

	void
main ()

	{
	char   in [100];
	time_t time;
	
	
	for (;;)
		{
		printf("\nTime: ");
		gets(in);
		time = strtoll(in, NULL, 10);
		printf("Utime: %lld\n", time);
		time = UnixTimeToTime(time);
		printf("Wtime:  %lld\n", time);
		time = TimeToUnixTime(time);
		printf("Utime: %lld\n\n", time);
		}
	}

#endif // TEST
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
