/* ----------------------------------------------------------------------- *\
|
|				    ZEROTIME
|
|		    Copyright (c) 1990, all rights reserved
|				Brian W Johnson
|				    3-Jun-90
|
|	    long		Return the long UNIX timedate
|	dt = zerotime (void);	Convert 1-1-80, 00:00:02 to UNIX time
|
\* ----------------------------------------------------------------------- */

#include  <time.h>

#include  "fwild.h"

static	struct tm	t =
	{
	0,		/* Seconds after the minute - [0,59] */
	0,		/* Minutes after the hour - [0,59] */
	0,		/* Hours since midnight - [0,23] */
	1,		/* Day of the month - [1,31] */
	0,		/* Months since January - [0,11] */
	80,		/* Years since 1900 */
	0,		/* Days since Sunday - [0,6]      (ignored) */
	0,		/* Days since January 1 - [0,365] (ignored) */
	0		/* Daylight savings time flag     (ignored) */
	};

/* ------------------------------------------------------------------------ */
    long			/* Return the long UNIX timedate word */
zerotime (void)

    {
    return ((long)(_mktime32(&t)));
    }

/* ------------------------------------------------------------------------ */
