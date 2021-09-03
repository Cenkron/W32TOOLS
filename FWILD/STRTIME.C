/* ----------------------------------------------------------------------- *\
|
|				    STRTIME
|
|		    Copyright (c) 1985, 1992, all rights reserved
|				Brian W Johnson
|				    7-Dec-86
|				   27-Dec-92
|
|	    char *		Return a pointer to a time string
|	p = strtime (dt);	Convert a unix timedate to a time string
|	    time_t  dt;		The unix timedate
|
|	    char *		Return a pointer to a time string
|	p = strtime2 (dt);	Convert a unix timedate to a time string
|	    time_t  dt;		The unix timedate
|
|	The pointed time string is static, and must not be altered.
|
|	strtime2() includes the seconds, strtime() does not.
|
\* ----------------------------------------------------------------------- */

#include  <time.h>
#include  <stdio.h>

static	char	stime [32];	/* The returned time string */
static	struct tm  *timestr;	/* Pointer to the tm structure */

#if defined STRTIME_SECONDS
#define  STRTIME  strtime2
#else
#define  STRTIME  strtime
#endif

/* ------------------------------------------------------------------------ */
	char *				/* Return a pointer to a time string */
STRTIME (				/* Convert a unix timedate to a time string */
    __time32_t dt)		/* The unix timedate */

	{
	char  ampm;

	timestr = _localtime32(&dt);	/* Convert to a tm structure */

	if (timestr->tm_hour > 11)	/* Convert to 12 hour time */
		ampm = 'p';
	else
		ampm = 'a';

	if (timestr->tm_hour > 12)
		timestr->tm_hour -= 12;

#if defined STRTIME_SECONDS

	sprintf(			/* Build the time string */
		&stime[0],
		"%2d:%02d:%02d%c",
		timestr->tm_hour,
		timestr->tm_min, 
		timestr->tm_sec,
		ampm);

#else

	sprintf(			/* Build the time string */
		&stime[0],
		"%2d:%02d%c",
		timestr->tm_hour,
		timestr->tm_min, 
		ampm);

#endif

	return  (&stime[0]);	/* Return the date string */
	}

/* ----------------------------------------------------------------------- */
