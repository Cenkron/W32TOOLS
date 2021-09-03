/* ----------------------------------------------------------------------- *\
|
|				     SORT
|
|		   Copyright (c) 1993, all rights reserved
|				Brian W Johnson
|				   11-Apr-93
|				   31-Aug-93
|				   17-Aug-97
|				   27-Aug-10 (Fix to compile under VS2008)
|
\* ----------------------------------------------------------------------- */

/* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <stdlib.h>
#include  <stdarg.h>
#include  <string.h>
#include  <malloc.h>
#include  <ctype.h>
#include  <fcntl.h>
#include  <limits.h>

#include  "fwild.h"
#include  "sort.h"

/* ----------------------------------------------------------------------- *\
|  Usage strings
\* ----------------------------------------------------------------------- */

char	copyright [] =
"Copyright (c) 1993 by J & M Software, Dallas TX - All Rights Reserved";

char  *usagedoc [] =
{
"Usage:  sort [%cbeiloqrsvwz?] [sort_controls] input_file_list >output_file",
"",
"Sort sorts all of the input_file_list files together and writes the",
"result to the standard output.  if no file names are supplied,",
"the standard input is sorted.  Sort may be used as a filter.",
"",
"The default sort key is the entire line.  The default ordering",
"is lexicographic in the ASCII collating sequence with upper",
"and lower case treated as distinct.",
"",
"    %cb      includes blank lines in the sorted output",
"    %cc      detab the input (and output) to use column counts",
"    %ce      don't quit on minor errors",
"    %ci      ignores case; upper and lower case are equivalent",
"    %cl      lists the filenames as they are sorted",
"    %co      write administrative output and errors to stdout",
"    %cq      quiet mode: minor errors and unfound files are not reported",
"    %cr      reverses the sort output order (for the next field only)",
"    %cs      uses a stable (slow) sort algorithm, else quicksort",
"    %ct N    sets the tab size to N (2..16, default 8)",
"    %cv      enables the display of informational messages which",
"            identify the input files, temporary file usage, errors,",
"            and other miscellaneous information",
"    %cw      excludes leading white space from the sort keys",
"    %cy filename   uses \"filename\" for the temporary file",
"    %cz      returns a zero exit code, even if there are errors",
"",
"Sort control options can be multiple, ordered by sorting precedence,",
"most significant first.  Input lines are limited to 1024 characters.",
"Fields are white-space delimited, but field 0 denotes the entire line.",
"",
"    %cf F[,C[,N]] sorts on a collation key beginning in field F,",
"            optionally column C (default 1), for N columns (default all).",
"    %cd M[,N] sorts on a date/time beginning in field M",
"            (and optionally, in field N).",
"    %cd l    sorts on the date and time from the \"l\" program.",
"    %cn F[,C[,N[,B]] sorts on a numeric key beginning in field F,",
"            optionally column C (default 1), for N columns (default all),",
"            and using base B (default c-language convention).",
"",
copyright,
NULL
};

/* ----------------------------------------------------------------------- *\
|  Definitions
\* ----------------------------------------------------------------------- */

#define	 INBUFSIZ	(1024)		/* Size of the input buffer */

/* ----------------------------------------------------------------------- *\
|  Variables
\* ----------------------------------------------------------------------- */

char	swch  = '-';	/* The switch character */

int	bflag = 0;	/* Includes blank lines in the output */
int	cflag = 0;	/* Detab the input (and output) */
int	eflag = 1;	/* Quit on minor errors */
int	iflag = 0;	/* Ignores case */
int	lflag = 0;	/* Lists files being sorted */
int	oflag = 0;	/* Outputs verbosity and errors to stdout */
int	qflag = 0;	/* Quiet mode */
int	rflag = 0;	/* Reverses the sort order (for one field only) */
int	sflag = 0;	/* Uses a stable (slow) sort method */
int	vflag = 0;	/* Lists verbose information levels */
int	wflag = 0;	/* Excludes leading white space from the sort */
int	zflag = 0;	/* Returns a zero exit code, even for errors */

int	tabsize	   = 8;		/* The tab size */
int	exitcode   = 0;		/* The exit code */
int	errorlevel = FWEXIT_MAJOR_ERROR;	/* The fatal error level */

FILE   *fperr; // = stderr;		/* The error output fp */

char	input_buffer [INBUFSIZ];	/* Buffer for input */
char	extra_buffer [INBUFSIZ];	/* Buffer for detabbing */

char	output_buffer [BUFSIZ];		/* Buffer for stdout */

/* ----------------------------------------------------------------------- *\
|  Private function prototypes
\* ----------------------------------------------------------------------- */

static	int	collationfield	(char *optionstring);
static	int	datefield	(char *optionstring);
static	int	numericfield	(char *optionstring);
static	void	process_input	(FILE *fp, char *fnp);
static	void	process_output	(void);
static	void	initial_verbosity (void);
static	void	final_verbosity (void);
static	void	detab		(void);

extern	void	exitmsg		(int code, char *fmt, ... );

/* ----------------------------------------------------------------------- *\
|  main () - The program main
\* ----------------------------------------------------------------------- */
	void
main (
	int	   argc,
	char  *argv [])

	{
	int	   smode = FW_FILE;		/* File search mode attributes */
	int	   option;			/* Option character */
	long   ltemp;			/* Used for optvalue() */
	void  *hp  = NULL;			/* Pointer to wild file data block */
	char  *ap  = NULL;			/* Argument pointer */
	char  *fnp = NULL;			/* Input file name pointer */
	FILE  *fp  = NULL;			/* Input file descriptor */

static	char   *optstring = "?bBcCd:D:eEf:F:iIlLn:N:oOqQrRsSt:T:vVwWy:Y:zZ";


	fperr  = stderr;			// Init the stderr output fp
	swch   = egetswch();
	optenv = getenv("SORT");
	setbuf(stdout, output_buffer);
	sort_init();

	while ((option = getopt(argc, argv, optstring)) != EOF)
		{
		switch (tolower(option))
			{

	/* Sort control options */

			case 'd':  datefield(optarg);	break;
			case 'f':  collationfield(optarg);	break;
			case 'n':  numericfield(optarg);	break;

	/* Administrative options */

			case 'b':	bflag = ! bflag;	break;
			case 'c':	cflag = ! cflag;	break;
			case 'e':	eflag = ! eflag;	break;
			case 'i':	iflag = ! iflag;	break;
			case 'l':	lflag = ! lflag;	break;
			case 'o':	oflag = ! oflag;	break;
			case 'q':	qflag = ! qflag;	break;
			case 'r':	rflag = 1;			break;
			case 's':	sflag = ! sflag;	break;
			case 'v':	++vflag;			break;
			case 'w':	wflag = ! wflag;	break;
			case 'z':	zflag = ! zflag;	break;
			case '?':	fperr = stdout; help(); break;


			case 't':
				if (optvalue(optarg, &ltemp, 2L, 16L))
					exitmsg(FWEXIT_COMMAND_LINE, "Tab size parm error - %s", optvalerror());
				tabsize = (int)(ltemp);
				break;

			case 'y':
				sort_set_temp(optarg);
				break;

			default:
				exitmsg(FWEXIT_COMMAND_LINE, "Unknown option '%c'", option);
				break;
			}
		}

	initial_verbosity();

	if (optind >= argc)
		process_input(stdin, "<stdin>");
	else
		{
		do  {
			ap = argv[optind++];
			hp = fwinit(ap, smode);		/* Process the input list */
			if ((fnp = fwild(hp)) == NULL)
				cantopen(ap);
			else
				{
				do  {				/* Process one filespec */
					if (fp = fopen(fnp, "r"))
						{
						process_input(fp, fnp);
						fclose(fp);
						}
					else
						cantopen(fnp);
					} while ((fnp = fwild(hp)));
				}
			} while (optind < argc);
		}

	process_output();

	final_verbosity();

	exitmsg(-1, NULL);
	}

/* ----------------------------------------------------------------------- *\
|  cantopen () - Report a file that can't be found/opened
\* ----------------------------------------------------------------------- */
	void
cantopen (			/* Fatal error, print a message and die */
	char  *fnp)			/* Pointer to the error message */

	{
	exitmsg(FWEXIT_FILE_NOT_FOUND, "Unable to find/open file: \"%s\"", fnp);
	}

/* ----------------------------------------------------------------------- *\
|  exitmsg () - Optionally report an error and optionally exit
\* ----------------------------------------------------------------------- */
	void
exitmsg (			/* Print a message and conditionally die */
	int    code,		/* The exit code to assign */
	char  *fmt,			/* Pointer to the exit message format string */
	...)			/* Optional arguments */

	{
	fflush(stdout);
	if ((fmt != NULL)  &&  ((qflag == 0)  ||  (code >= FWEXIT_MAJOR_ERROR)))
		{
		va_list  marker;
		va_start(marker, fmt);
		vfprintf(fperr, fmt, marker);
		va_end(marker);
		fputc('\n', fperr);
		}

	if (code == FWEXIT_COMMAND_LINE)
		usage();

	if (exitcode < code)
		exitcode = code;

	if ((exitcode >= errorlevel)  ||  (code < 0))
		exit((zflag) ? (exitcode) : (0));

	fflush(fperr);
	}

/* ----------------------------------------------------------------------- *\
|  docprint () - Print the program documentation (low level)
\* ----------------------------------------------------------------------- */
	void
docprint (			/* Print documentation text */
	char  **dp)			/* Document array pointer */

	{
	while (*dp)
		{
		fprintf(fperr, *(dp++), swch);
		fputc('\n', fperr);
		}
	}

/* ----------------------------------------------------------------------- *\
|  Prepare a string for diagnostic printing
\* ----------------------------------------------------------------------- */
	char *
strip (			/* Sanitize the string for error listing */
	char  *pstr)	/* Pointer to the string */

	{
	char   ch;		/* Temporary char variable */
	char  *p;		/* Pointer into the string */


	for (p = pstr; ((ch = *p) != '\0'); ++p)
		{
		if ((ch == '\n')  ||  (ch == '\f'))
			{
			*p = '\0';
			break;
			}
		}

	return (pstr);
	}

/* ----------------------------------------------------------------------- *\
|  collationfield () - Decode and install a collation field option
\* ----------------------------------------------------------------------- */
	static int
collationfield (
	char  *s)

	{
	int    field;
	int    column;
	int    ncolumns;
	int    result;
	long   ltemp;
	char  *p;


	if ((p = strtoken(s, NULL, NULL)) == NULL)
		return (-1);

	if (optvalue(p, &ltemp, 0L, SHRT_MAX))
		exitmsg(FWEXIT_COMMAND_LINE, optvalerror());
	field = (int)(ltemp);

	if ((p = strtoken(NULL, NULL, NULL)) != NULL)
		{
		if (optvalue(p, &ltemp, 0L, SHRT_MAX))
			exitmsg(FWEXIT_COMMAND_LINE, optvalerror());
		column = (int)(ltemp);
		}
	else
		column = 0;

	if ((p = strtoken(NULL, NULL, NULL)) != NULL)
		{
		if (optvalue(p, &ltemp, 0L, SHRT_MAX))
			exitmsg(FWEXIT_COMMAND_LINE, optvalerror());
		ncolumns = (int)(ltemp);
		}
	else
		ncolumns = SHRT_MAX;

	result = sort_string(field, column, ncolumns, rflag);
	rflag  = 0;
	return (result);
	}

/* ----------------------------------------------------------------------- *\
|  numericfield () - Decode and install a numeric field option
\* ----------------------------------------------------------------------- */
	static int
numericfield (
    char *s)

	{
	int    field;
	int    column;
	int    ncolumns;
	int    base;
	int    result;
	long   ltemp;
	char  *p;


	if ((p = strtoken(s, NULL, NULL)) == NULL)
		return (-1);

	if (optvalue(p, &ltemp, 0L, SHRT_MAX))
		exitmsg(FWEXIT_COMMAND_LINE, optvalerror());
	field = (int)(ltemp);

	if ((p = strtoken(NULL, NULL, NULL)) != NULL)
		{
		if (optvalue(p, &ltemp, 0L, SHRT_MAX))
			exitmsg(FWEXIT_COMMAND_LINE, optvalerror());
		column = (int)(ltemp);
		}
	else
		column = 0;

	if ((p = strtoken(NULL, NULL, NULL)) != NULL)
		{
		if (optvalue(p, &ltemp, 0L, SHRT_MAX))
			exitmsg(FWEXIT_COMMAND_LINE, optvalerror());
		ncolumns = (int)(ltemp);
		}
	else
		ncolumns = SHRT_MAX;

	if ((p = strtoken(NULL, NULL, NULL)) != NULL)
		{
		if (optvalue(p, &ltemp, 2L, 16L))
			exitmsg(FWEXIT_COMMAND_LINE, optvalerror());
		base = (int)(ltemp);
		}
	else
		base = 0;

	result = sort_numeric(field, column, ncolumns, base, rflag);
	rflag  = 0;
	return (result);
	}

/* ----------------------------------------------------------------------- *\
|  datefield () - Decode and install a date field option
\* ----------------------------------------------------------------------- */
	static int
datefield (
	char  *s)

	{
	int    field1, field2;
	int    result;
	long   ltemp;
	char  *p;


	if (tolower(*s) == 'l')
		{
		field1 = 3;
		field2 = 4;
		goto exxit;
	}

	if ((p = strtoken(s, NULL, NULL)) == NULL)
		return (-1);

	if (optvalue(p, &ltemp, 0L, SHRT_MAX))
		exitmsg(FWEXIT_COMMAND_LINE, optvalerror());
	field1 = (int)(ltemp);

	if ((p = strtoken(NULL, NULL, NULL)) != NULL)
		{
		if (optvalue(p, &ltemp, 0L, SHRT_MAX))
			exitmsg(FWEXIT_COMMAND_LINE, optvalerror());
		field2 = (int)(ltemp);
		}
	else
		field2 = 0;

exxit:
	result = sort_date(field1, field2, rflag);
	rflag  = 0;
	return (result);
	}

/* ----------------------------------------------------------------------- *\
|  process_input () - Enter the appropriate lines from the file for sorting
\* ----------------------------------------------------------------------- */
	static void
process_input (
	FILE  *fp,
	char  *fnp)

	{
	if ((vflag > 0)  ||  lflag)
		fprintf(fperr, "Reading file: %s\n", fnp);

	while (fgets(input_buffer, sizeof(input_buffer), fp) != NULL)
		{
		char *p = input_buffer;

		while ((*p != '\0')  &&  (*p != '\n')  &&  (*p != '\f'))
			++p;
		*p = '\0';

	/* Conditionally skip blank lines */

		if ((bflag == FALSE)  &&  *stpblk(input_buffer) == '\0')
			continue;

	/* Conditionally convert tabs to spaces */

		if (cflag)
			{
			detab();
			sort_put_line(extra_buffer);
			}
		else
			sort_put_line(input_buffer);
		}
	}

/* ----------------------------------------------------------------------- *\
|  detab () - Detab the input buffer to the extra buffer
\* ----------------------------------------------------------------------- */
	static void
detab (void)

	{
	char   ch;
	int    count;
	int    column = 0;
	char  *p      = input_buffer;
	char  *q      = extra_buffer;

	while ((ch = *(p++)) != '\0')
		{
		if (ch == '\t')
			{
			ch = ' ';
			count = tabsize - (column % tabsize);
			}
		else
			count = 1;

		do  {
			if (column >= (INBUFSIZ - 1))
				{
				exitmsg(FWEXIT_MINOR_ERROR, "Line too long: \"%s\"\n",
					strip(input_buffer));
				break;
				}
			else
				{
				*(q++) = ch;
				++column;
				}
			} while (--count > 0);
		}
	*q = '\0';
	}

/* ----------------------------------------------------------------------- *\
|  process_output () - Output the sorted lines to stdout
\* ----------------------------------------------------------------------- */
	static void
process_output (void)

	{
	char  *p;

	while (sort_get_line(&p) == 0)
		{
		fputs(p, stdout);
		fputc('\n', stdout);
			fflush(stdout);
		}
	}

/* ----------------------------------------------------------------------- *\
|  initial_verbosity () - Output any requested verbosity before starting
\* ----------------------------------------------------------------------- */
	static void
initial_verbosity (void)

	{
	if (eflag)
		errorlevel = FWEXIT_MINOR_ERROR;
	if (oflag)
		fperr = stdout;

	sort_parms(sflag, vflag, wflag, iflag, rflag, tabsize);
	rflag = 0;

//	if (vflag > 1)
//		{
//		}
	}

/* ----------------------------------------------------------------------- *\
|  final_verbosity () - Output any requested verbosity before exiting
\* ----------------------------------------------------------------------- */
	static void
final_verbosity (void)

	{
	fflush(stdout);
	sort_terminate();

	if (vflag > 1)
		{
		sort_stats();
		fprintf(stderr, "Sort exit code: %d\n", exitcode);
		}
	}

/* ----------------------------------------------------------------------- */
