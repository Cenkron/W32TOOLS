/* ----------------------------------------------------------------------- *\
|
|				   EXPRESSION ()
|
|		    Copyright (c) 1993, all rights reserved
|				Brian W Johnson
|				   20-Feb-93
|
|	    int
|	rc = expression (pstr, presult);
|	    char  *pstr;
|           long  *presult;
|
|	expression() parses the passed arithmetic expression string, and
|       converts it to a numerical value.  Expressions can include the
|       following symbols:
|
|               Decimal, Octal, and Hexadecimal constants
|               + - * / % ( )
|
|	A pointer is returned to the string symbol terminating the parse.
|
|       The return value is zero (0) for success, non-zero for failure
|
|       This function does not attempt to detect numerical overflow.
|
|	    char *
|	p = expression_error ();
|
|	expression_error() returns a pointer to a static text string
|	describing the most recent error.
|
| ------------------------------------------------------------------------
|
|       Parse rules:
|
|       result      =   expression
|       expression  =   [ +- ] term [ +- term ]*
|       term        =   factor [ *%/ factor ]*
|       factor      =   number
|                   or  ( expression )
|
|	where [ x ]   symbolizes one optional occurrence of x
|	and   [ x ]*  symbolizes 0 or more occurrences of x
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <stdlib.h>
#include  <ctype.h>
// #include  <string.h>

#include  "fwild.h"

/* ----------------------------------------------------------------------- *\
|  Private definitions
\* ----------------------------------------------------------------------- */

#define  ERROR_NONE		(0)
#define  ERROR_DIV_BY_ZERO	(1)
#define  ERROR_MOD_BY_ZERO	(2)
#define  ERROR_MISSING_RPAREN	(3)
#define  ERROR_BAD_NUMBER	(4)
#define  ERROR_SYNTAX		(5)

/* ----------------------------------------------------------------------- *\
|  Private variables
\* ----------------------------------------------------------------------- */

static	int		errorcode;	/* The returned error result */
static	int		ch;		/* The current scan character */
static	char   *s_current;	/* The current scan pointer */
static	char   *s_initial;	/* The initial scan pointer */
static	char	errmsg [40];	/* The error message string */

/* ----------------------------------------------------------------------- *\
|  Private function prototypes
\* ----------------------------------------------------------------------- */

static	int		expr	(long *pret_value);
static	int		term	(long *pret_value);
static	int		factor	(long *pret_value);
static	int		number	(long *pret_value);
static	void	advance (void);
static	void	skip    (void);
static	void	error	(int   err_value);

/* ----------------------------------------------------------------------- *\
|  Main entry
\* ----------------------------------------------------------------------- */
	int
expression (
	char   *pstr,				/* Pointer to the input string */
	long   *pret_value)			/* Pointer to returned result value */

	{
	s_initial = pstr;			/* Set the global variables */
	s_current = pstr;
	ch = tolower(*s_current);

	if ((expr(pret_value) == ERROR_NONE)/* Evaluate the string */
	&&  (ch != '\0'))			/* Ensure we used the string up */
		error(ERROR_SYNTAX);

	return (errorcode);
	}

/* ----------------------------------------------------------------------- *\
|  Process a required expression
\* ----------------------------------------------------------------------- */
	static int					/* Return ERROR_* */
expr (
	long  *pret_value)			/* Pointer to the returned value */

	{
	int    first     = TRUE;
	long   cum_value = 0L;
	long   value;

	for (;;)
		{
//		printf("expr: %s\n", s_current);
		skip();
		if (ch == '-')
			{
			advance();
			if (term(&value))
				break;
			cum_value -= value;
			}

		else if (ch == '+')
			{
			advance();
			if (term(&value))
				break;
			cum_value += value;
			}

		else if (first)			/* (Implied leading '+') */
			{
			if (term(&cum_value))
				break;
			}

		else
			break;

		first = FALSE;
		}

	*pret_value = cum_value;	/* Return the value */
	return (errorcode);
	}

/* ----------------------------------------------------------------------- *\
|  Process a required term
\* ----------------------------------------------------------------------- */
	static int				/* Return ERROR_* */
term (
	long  *pret_value)		/* Pointer to the returned value */

	{
	long   cum_value;
	long   value;


	skip();
//	printf("term 1: %s\n", s_current);
	if (factor(&cum_value) == ERROR_NONE)
		{
		for (;;)
			{
//			printf("term 2: %s\n", s_current);
			skip();
			if (ch == '*')
				{
				advance();
				if (factor(&value))
					break;
				cum_value *= value;
				}

			else if (ch == '/')
				{
				advance();
				if (factor(&value))
					break;
				if (value == 0L)
					{
					error(ERROR_DIV_BY_ZERO);
					break;
					}
				cum_value /= value;
				}

			else if (ch == '%')
				{
				advance();
				if (factor(&value))
					break;
				if (value == 0L)
					{
					error(ERROR_MOD_BY_ZERO);
					break;
					}
				cum_value %= value;
				}

			else
				break;
			}
		}

	*pret_value = cum_value;	/* Return the value */
	return (errorcode);
	}

/* ----------------------------------------------------------------------- *\
|  Process a required factor
\* ----------------------------------------------------------------------- */
    static int					/* Return ERROR_* */
factor (
	long  *pret_value)			/* Pointer to the returned value */

	{
	long   value;


	skip();
//	printf("factor: %s\n", s_current);
	if (ch == '(')
		{
		advance();
		if (expr(&value) == ERROR_NONE)
			{
			skip();
			if (ch == ')')
				advance();
			else
				error(ERROR_MISSING_RPAREN);
			}
		}
	else
		number(&value);

	*pret_value = value;		/* Return the value */
	return (errorcode);
	}

/* ----------------------------------------------------------------------- *\
|  Process a required numeric field
\* ----------------------------------------------------------------------- */
	static int					/* Return ERROR_* */
number (
	long  *pret_value)			/* Pointer to the returned value */

	{
	char  *s_saved = s_current;	/* Saved current string pointer */


//	printf("number: %s\n", s_current);
	*pret_value = strtol(s_current, &s_current, 0);
	ch = *s_current;

	if ((s_current == s_saved)  ||  isalnum(ch))
		error(ERROR_BAD_NUMBER);

	return (errorcode);
	}

/* ----------------------------------------------------------------------- *\
|  Advance to the next character
\* ----------------------------------------------------------------------- */
	static void
advance (void)

	{
	++s_current;
	ch = tolower(*s_current);
	}

/* ----------------------------------------------------------------------- *\
|  Skip over white space
\* ----------------------------------------------------------------------- */
	static void
skip (void)

	{
	while (isspace(ch))
		advance();
	}

/* ----------------------------------------------------------------------- *\
|  Process an error
\* ----------------------------------------------------------------------- */
	static void
error (
	int  err_value)				/* The error code */

    {
    char  *p;					/* Pointer to the error text */

	switch (err_value)
		{
		case ERROR_DIV_BY_ZERO:		p = "divide by zero";	break;
		case ERROR_MOD_BY_ZERO:		p = "modulus by zero";	break;
		case ERROR_MISSING_RPAREN:	p = "missing ')'";	break;
		case ERROR_BAD_NUMBER:		p = "bad number";	break;
		case ERROR_SYNTAX:			p = "bad syntax";	break;
		}

	sprintf(errmsg, "%s at character %d", p, (1 + s_current - s_initial));
	errorcode = err_value;		/* Remember the error code */
	}

/* ----------------------------------------------------------------------- *\
|  Return the error message
\* ----------------------------------------------------------------------- */
	char *						/* Return a pointer to the error message */
expression_error (void)

	{
	return ((errorcode) ? (errmsg) : ("no error"));
	}

/* ----------------------------------------------------------------------- *\
|  Test main
\* ----------------------------------------------------------------------- */
#ifdef  TEST_EXPR

	int
main (
	int    argc,
	char  *argv[])

	{
	int    result;
	long   value;
	char  *s = argv[1];


	if (argc < 2)
		printf("expr <expression>\n");

	else
		{
		result = expression(s, &value);
		printf("arg:    \"%s\"\n", s);
		printf("value:  %ld (0x%04lX)\n", value, value);
		printf("result: (%d) \"%s\"\n", result, expression_error());
		}
	}

#endif
/* ----------------------------------------------------------------------- */
