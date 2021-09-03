/* ----------------------------------------------------------------------- *\
|
|				   OPTVALUE ()
|
|		    Copyright (c) 1993, all rights reserved
|				Brian W Johnson
|				   20-Feb-93
|
|	    int
|	rc = optvalue (pstr, presult, minvalue, maxvalue);
|	    char  *pstr;
|           long  *presult;
|           long   minvalue;
|           long   maxvalue;
|
|	optvalue() parses the passed arithmetic expression string,
|       converts it to a numerical value, and range checks it.
|	Expressions can include the following symbols:
|
|               Decimal, Octal, and Hexadecimal constants
|               + - * / % ( )
|
|       See expression() for the parse rules
|
|       The return value is zero (0) for success, nonzero for failure
|
|	    char *
|	s = optvalerror (void);
|
|	optvalerror() returns the text string describing the most recent
|	optvalue() error.
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <stdlib.h>

#include  "fwild.h"

/* ----------------------------------------------------------------------- */

#define  ERROR_NONE		( 0)
#define  ERROR_RANGE	(-1)

static	int	result;		/* The most recent returned result */
static	char	errmsg [50] = "value too ";

/* ----------------------------------------------------------------------- */
	int
optvalue (
	char   *pstr,			/* Pointer to the input string */
	long   *presult,		/* Pointer to returned result */
	long    minvalue,		/* Minimum valid expression value */
	long    maxvalue)		/* Maximum valid expression value */

	{
	long   value;			/* The determined value */


	result = expression(pstr, &value);

	if (result == ERROR_NONE)
		{
		if (value < minvalue)
			{
			result = ERROR_RANGE;
			sprintf(&errmsg[10], "small: %ld (min %ld)", value, minvalue);
			}
		else if (value > maxvalue)
			{
			result = ERROR_RANGE;
			sprintf(&errmsg[10], "large: %ld (max %ld)", value, maxvalue);
			}
		else
			*presult = value;
		}

	return (result);
	}

/* ----------------------------------------------------------------------- */
	char *
optvalerror (void)

	{
	return ((result == ERROR_RANGE) ? (errmsg) : (expression_error()));
	}

/* ----------------------------------------------------------------------- */
