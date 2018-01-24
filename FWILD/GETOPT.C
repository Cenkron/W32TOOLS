/* ----------------------------------------------------------------------- *\
|
|				   GETOPT ()
|
|		    Copyright (c) 1985, 1990, all rights reserved
|				Brian W Johnson
|				   16-Jun-90
|
|	    int
|	opt = getopt (argc, argv, optstr);
|	    int  argc;
|	    char  *argv [];
|	    char  *optstr;
|
|	    char   optswch;	(Global variable, declared in getopt() )
|	    char   optchar;	(Global variable, declared in getopt() )
|	    int    optind;	(Global variable, declared in getopt() )
|	    char  *optarg;	(Global variable, declared in getopt() )
|	    char  *optenv;	(Global variable, declared in getopt() )
|
|	getopt() parses command line arguments, looking for valid options.
|	For each valid option, getopt() returns the option character.
|	If there are no more valid options, getopt() returns EOF (-1).
|	If an option character is invalid, getopt() returns NUL (0).
|	If an option passes a parameter, optarg points to it.
|	If an option parameter is missing, getopt() returns NUL (0).
|	If an option does not pass a parameter, optarg is NULL.
|	After parsing of options is complete, the main program can find
|	the next command line argument pointed by argv[optind].
|
|	To recognize the options  -a -A -b -C -Ffilename -f filename
|	the optstr is formed as follows:
|
|	    char  *optstr = "aAbCF:f:";
|
|	"--" can be used to terminate option parsing, and returns EOF.
|	The alternate option form -a-C is also accepted.
|
|	If optenv is not NULL, that string is searched first before searching
|	the argv[] array.  The application can place a pointer to an option
|	string into optenv, prior to the first call to getopt().  This string
|	is usually obtained from the environment.
|
|	The current switch character is obtained from the environment
|	or operating system via egetswch().
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <ctype.h>
#include  <string.h>

#include  "ascii.h"
#include  "fwild.h"

/* ----------------------------------------------------------------------- */

	char	optswch = '-';		/* The option switch character */
	char	optchar = NUL;		/* The last option character */
	int	optind 	= 0;		/* The current argv[] index */
	char   *optarg	= NULL;		/* Pointer to option parameter */
	char   *optenv	= NULL;		/* Pointer to environment options */

#define  ST_INITIAL	 (0)		/* The initial state */
#define  ST_ENV1	 (1)		/* Get next env option */
#define  ST_ENV2	 (2)		/* Process env option */
#define  ST_ENV3	 (3)		/* Select next state */
#define  ST_ARGV0	 (4)		/* Get next option string */
#define  ST_ARGV1	 (5)		/* Get next option character (- req) */
#define  ST_ARGV2	 (6)		/* Validate option character */
#define  ST_ARGV3	 (7)		/* Process option with no argument */
#define  ST_ARGV4	 (8)		/* Process option with argument */
#define  ST_ERROR	 (9)		/* Error in scan, return NUL */
#define  ST_DONE	(10)		/* Scan complete, return EOF */

#define  LOOPING	(0x100)		/* Not a valid return value */

static	int	state	= ST_INITIAL;	/* The state variable */
static	char	chsave	= NUL;		/* The saved env character */
static	char   *ap	= NULL;		/* Pointer to the current option */

/* ----------------------------------------------------------------------- */
    int
getopt (argc, argv, optstring)
    int    argc;		/* argc passed from main() */
    char  *argv [];		/* argv passed from main() */
    char  *optstring;		/* Pointer to the option selector string */

    {
    int    result;		/* The returned result */
    char  *sp;			/* Pointer into selector string */


    if (chsave != NUL)
	{
	*ap = chsave;
	chsave = NUL;
	}

    for (result = LOOPING; result == LOOPING; )
	{
	switch (state)
	    {
	    case ST_INITIAL:			/* Initial execution */
		if (optenv != NULL)
		    {
		    ap     = stpblk(optenv);
		    optenv = NULL;
		    state  = ST_ENV1;
		    }
		else
		    state  = ST_ARGV0;
		break;


	    case ST_ENV1:
		if ((*ap++ == optswch)
		&&  (*ap != optswch)		/* Check for "--" */
		&&  (*ap != NUL))		/* Check for "-NUL" */
		    state = ST_ENV2;		/* Candidate option char */
		else
		    state = ST_ARGV0;
		break;


	    case ST_ENV2:
		optchar = *ap++;
		if ((sp = strchr(optstring, optchar)) == NULL)
		    {
		    state = ST_ERROR;
		    break;
		    }
		else if (*(sp + 1) != ':')	/* Check if parm required */
		    {
		    optarg = NULL;
		    result = optchar;
		    }
		else if (*(ap = stpblk(ap)) != NUL)
		    {
		    optarg = ap;		/* Argument */
		    result = optchar;
		    while (isgraph(*ap))
			++ap;
		    chsave = *ap;		/* Temporary arg term */
		    *ap = NUL;
		    }
		else
		    {
		    state = ST_ERROR;		/* No argument */
		    break;
		    }
		state = ST_ENV3;		/* Defer next state choice */
		break;


	    case ST_ENV3:
		if (*ap == NUL)			/* Select the next state */
		    state = ST_ARGV0;		/* End of env string */
		else if (*ap == optswch)
		    state = ST_ENV1;		/* Allow "-x-y" */
		else if ( ! isspace(*ap))
		    state = ST_ENV2;		/* Allow "-xy" */
		else if (*(ap = stpblk(ap)) == optswch)
		    state = ST_ENV1;		/* Allow "-x -y" */
		else
		    state = ST_ERROR;		/* "-x y" */
		break;


	    case ST_ARGV0:			/* Checking for new string */
		if ((++optind < argc)		/* Ensure no argument overflow */
		&& ((ap = argv[optind])) != NULL)
		    state = ST_ARGV1;
		else
		    state = ST_DONE;
		break;


	    case ST_ARGV1:			/* Starting new string */
		ap = stpblk(ap);
		if (*ap++ != optswch)		/* Check for switch char */
		    state = ST_DONE;		/* No, terminate scan */
		else if ((*ap == optswch)	/* Check for "--" */
		     ||  (*ap == NUL))		/* Check for "-NUL" */
		    {
		    ++optind;			/* Yes, terminate scan */
		    state = ST_DONE;
		    }
		else
		    state = ST_ARGV2;		/* Candidate option char */
		break;


	    case ST_ARGV2:			/* Validating option char */
		optchar = *ap++;
		if ((sp = strchr(optstring, optchar)) == NULL)
		    {
		    ++optind;
		    state = ST_ERROR;
		    }
		else if (*(sp + 1) != ':')	/* Check if parm required */
		    state = ST_ARGV3;		/* No */
		else
		    state = ST_ARGV4;		/* Yes */
		break;


	    case ST_ARGV3:			/* Processing simple option */
		optarg = NULL;
		result = optchar;

		if (*ap == NUL)			/* Select the next state */
		    state = ST_ARGV0;		/* End of string */
		else if (*ap == optswch)
		    state = ST_ARGV1;		/* Allow "-x-y" */
		else
		    state = ST_ARGV2;		/* Allow "-xy" */
		break;


	    case ST_ARGV4:			/* Processing complex option */
		if (*(ap = stpblk(ap)) != NUL)
		    {
		    optarg = ap;		/* Concatenated argument */
		    result = optchar;
		    state  = ST_ARGV0;
		    }
		else if ((++optind < argc)	/* Ensure no arg overflow */
		     && ((ap = argv[optind]) != NULL)
		     && (*(ap = stpblk(ap)) != NUL))
		    {
		    optarg = ap;		/* Next string argument */
		    result = optchar;
		    state  = ST_ARGV0;
		    }
		else
		    state = ST_ERROR;		/* No argument */
		break;


	    case ST_DONE:			/* Finished, return EOF */
		optchar = NUL;
		optarg  = NULL;
		result  = EOF;
		break;


	    case ST_ERROR:			/* Error, return NUL */
	    default:	
		optarg  = NULL;
		result  = NUL;
		state   = ST_DONE;		/* Report finished next time */
		break;
	    }
	}
    return (result);
    }

/* ----------------------------------------------------------------------- */
