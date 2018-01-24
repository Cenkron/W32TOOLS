/* ----------------------------------------------------------------------- *\
|
|				    STRDATE
|
|		    Copyright (c) 1985, 1992, all rights reserved
|				Brian W Johnson
|				    7-Dec-86
|				   27-Dec-92
|				   10-Sep-98
|
|	    char *		Return a pointer to a date string
|	p = strdate (dt);	Convert a unix timedate to a date string
|	    time_t  dt;		The unix timedate
|
|	The pointed time string is static, and must not be altered.
|
\* ----------------------------------------------------------------------- */

#include  <time.h>
#include  <stdio.h>

static	char	sdate [32];	/* The returned date string */
static	struct tm  *timestr;	/* Pointer to the tm structure */

static	char   *month [] =	/* The month name array [0..11] */
    {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };

/* ----------------------------------------------------------------------- */
    char *			/* Return a pointer to a date string */
strdate (dt)			/* Convert a unix timedate to a date string */
    __time32_t  dt;		/* The unix timedate */

    {
    timestr = _localtime32(&dt);	/* Convert to a tm structure */

    while (timestr->tm_year >= 100)	/* Convert to 2 digits */
	timestr->tm_year -= 100;

    if ((timestr->tm_mon < 0)		/* Validate the month */
    ||  (timestr->tm_mon > 12))
	timestr->tm_mon = 0;

    sprintf(				/* Build the date string */
	&sdate[0],
	"%d-%s-%02d",
	timestr->tm_mday,
	month[timestr->tm_mon], 
	timestr->tm_year);

    return  (&sdate[0]);		/* Return the date string */
    }

/* ----------------------------------------------------------------------- */
