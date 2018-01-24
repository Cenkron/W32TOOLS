/* ----------------------------------------------------------------------- *\
|
|				   CAT Filter
|
|		    Copyright (c) 1985, 1990, 2011, All rights reserved
|				Brian W Johnson
|				   18-Aug-90
|				   20-Feb-93
|				   31-Oct-93
|				   13-Aug-97
|				   20-Jan-11 - Add switch to entab only leading white space
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <stdlib.h>
#include  <ctype.h>
#include  <fcntl.h>
#include  <io.h>

#include  "fwild.h"

#ifndef TRUE
#define FALSE	  0
#define TRUE	  1
#endif

char	copyright [] =
"Copyright (c) 1985, 1993 by J & M Software, Dallas TX - All Rights Reserved";

char	buffer [BUFSIZ];	/* Buffer for stdout */

char	swch = '-';		/* The switch character */

int	d_flag     = FALSE;	/* Detab flag */
int	e_flag     = FALSE;	/* Entab flag */
int	f_flag     = FALSE;	/* Formfeed flag */
int	h_flag     = FALSE;	/* Trim head lines flag */
int	l_flag     = FALSE;	/* List name flag */
int	p_flag     = FALSE;	/* Prefix flag */
int	t_flag     = FALSE;	/* Trim tail lines flag */
int	q_flag     = FALSE;	/* En/detab within quoted strings */
int	lw_flag    = FALSE;	/* En/detab only leading white space flag */
int	lc_flag    = FALSE;	/* Convert to lower case */
int	uc_flag    = FALSE;	/* Convert to upper case */
int	os9_flag   = FALSE;	/* Convert to OS9 format */

int	prefixsize = 0;		/* Prefix string length */
int	itabsize   = 8;		/* The incoming tab size */
int	otabsize   = 8;		/* The outgoing tab size */
int	runlength  = 1;		/* The maximum space run length */

int	mask   = 0xFF;		/* WordStar mask */

static	void	help ();
static	void	cantopen (char *);
static	void	dprint (char **);
static	void	process (FILE *, char *);
static	void	usage ();

/* ----------------------------------------------------------------------- */
    void
main (argc, argv)
    int    argc;
    char  *argv [];

    {
    int    smode = FW_FILE;		/* File search mode attributes */
    long   ltemp;			/* Used by optvalue() */
    char   ch;				/* Parser temporary */
    char  *s;				/* Parser temporary */
    void  *hp  = NULL;			/* Pointer to wild file data block */
    char  *fnp = NULL;			/* Input file name pointer */
    FILE  *fp  = NULL;			/* Input file descriptor */


    setbuf(stdout, buffer);
    swch = egetswch();

    while (--argc > 0 && (*++argv)[0] == swch)
	for (s = argv[0] + 1; ch = *(s++); )
	    switch (tolower(ch))
		{
		case 'c':
		    if (*s == 'l')
			{
			++s;
			lc_flag = TRUE;
			}
		    else if (*s == 'u')
			{
			++s;
			uc_flag = TRUE;
			}
		    else
			usage();
		    break;

		case 'd':
		    ++d_flag;
		    break;

		case 'e':
		    ++d_flag;
		    ++e_flag;
		    break;

		case 'f':
		    ++f_flag;
		    break;

		case 'h':
		    ++h_flag;
		    break;

		case 'i':
		    if (optvalue(s, &ltemp, 1, 80))
			{
			printf("Input tab parm error - %s\n", optvalerror());
			usage();
			}
		    *s = '\0';
		    itabsize = (int)(ltemp);
		    break;

		case 'l':
		    if (*s == 'w')
			{
			++s;
			++lw_flag;
			}
		    else
			++l_flag;
		    break;

		case 'o':
		    if (optvalue(s, &ltemp, 1, 80))
			{
			printf("Output tab parm error - %s\n", optvalerror());
			usage();
			}
		    *s = '\0';
		    otabsize = (int)(ltemp);
		    break;

		case 'p':
		    if (optvalue(s, &ltemp, 1, 80))
			{
			printf("Prefix parm error - %s\n", optvalerror());
			usage();
			}
		    *s = '\0';
		    ++p_flag;
		    ++t_flag;
		    prefixsize = (int)(ltemp);
		    break;

		case 'q':
		    ++q_flag;
		    break;

		case 'r':
		    if (optvalue(s, &ltemp, 0, 80))
			{
			printf("Run length parm error - %s\n", optvalerror());
			usage();
			}
		    *s = '\0';
		    runlength = (int)(ltemp);
		    break;

		case 't':
		    ++t_flag;
		    break;

		case 'w':
		    mask = 0x7F;
		    break;

		case '9':
                    ++os9_flag;
		    break;

		case 'u':
                    setmode(fileno(stdout), O_BINARY);
		    break;

		case '?':
		    help();

		default:
		    usage();
		}

    if (argc == 0)
	process(stdin, "<stdin>");
    else
	{
	do  {
	    hp = fwinit(*argv, smode);		/* Process the input list */
	    if ((fnp = fwild(hp)) == NULL)
		cantopen(*argv);
	    else
		{
		do  {				/* Process one filespec */
		    if (fp = fopen(fnp, "r"))
			{
			process(fp, fnp);
			fclose(fp);
			}
		    else
			cantopen(fnp);
		    } while ((fnp = fwild(hp)));
		}
	    } while (*++argv);
	}
    }

/* ----------------------------------------------------------------------- */
    static void
cantopen (fnp)			/* Inform user of input failure */
    char  *fnp;			/* Input file name */

    {
    fprintf(stderr, "\7Unable to open input file: %s\n", fnp);
    }

/* ----------------------------------------------------------------------- */
    void
usage ()			/* Display usage documentation */

    {
    static char  *udoc [] =
	{
	"Usage:  cat  [%c?cdefhlqt9uw]  [%ciopr<n>]  [input_file_list]  [>output_file]",
	"        cat  %c?  for help",
	NULL
	};

    dprint(udoc);
    exit(1);
    }

/* ----------------------------------------------------------------------- */
    void
help ()				/* Display help documentation */

    {
    static char  *hdoc [] =
	{
	"Usage:  cat  [%c?cdefhlqt9uw]  [%ciopr<n>]  [input_file_list]  [>output_file]",
	"",
	"cat concatenates the files in the input_file_list (or stdin) to stdout",
	"(defaults / ranges in parentheses)",
	"",
	"    %ccl   converts to lower case (off)",
	"    %ccu   converts to upper case (off)",
	"    %cf    interposes formfeeds between files (off)",
	"    %ch    trims off all leading white space from each line (off)",
	"    %cl    lists file names as they are concatenated (off)",
	"    %cp<n> prefixes <n> (1 - 80) columns of white space to each line (0)",
	"    %ct    trims off all trailing white space from each line (off)",
	"    %c9    outputs in OS-9 format (off)",
	"    %cu    outputs in Unix format (off)",
	"    %cw    clears WordStar high bits (off)",
	"",
	"(de/en)tabbing:",
	"",
	"    %cd    detabs the file according to the incoming tab size (off)",
	"    %ce    entabs the file according to the outgoing tab size (implies -d) (off)",
	"    %ci<n> sets the incoming tab size to <n> (default 8, 1 - 80)",
	"    %co<n> sets the outgoing tab size to <n> (default 8, 1 - 80)",
	"    %cr<n> sets the maximum space run length to <n> (default 1, 0 - 80)",
	"    %clw   en/detab only leading white space (off)",
	"    %cq    en/detab even inside quoted strings (off)",
	"",
	copyright,
	NULL
	};

    dprint(hdoc);
    exit(0);
    }

/* ----------------------------------------------------------------------- */
    void
dprint (dp)			/* Print documentation text */
    char  **dp;			/* Document array pointer */

    {
    while (*dp)
	{
	printf(*(dp++), swch, swch);
	putchar('\n');
	}
    }

/* ----------------------------------------------------------------------- *\
|  Private variables for the process() algorithm
\* ----------------------------------------------------------------------- */

#define  BSIZE       (32768)		     /* Size of the whitespace buffer */

static  int	icolcnt = 0;		     /* Current input column count */
static  int	wcolcnt = 0;		     /* Beginning of whitespace buffer */
static  int	scolcnt = 0;		     /* Beginning of whitespace count */
static  int	ocolcnt = 0;		     /* Current output column count */

static  char	quoted  = 0;		     /* The current quote character */

static  char	whiteflag = FALSE;	     /* TRUE when whitespace is enqueued */
static	char	whitebuf [BSIZE];	     /* Whitespace buffer */
static	char   *whiteptr = &whitebuf[0];     /* Pointer into the whitespace buffer */
static	char   *whiteend = &whitebuf[BSIZE]; /* Pointer to buffer end */

static  void	flush      (int, int);
static  void	out        (int, int);

/* ----------------------------------------------------------------------- */
    static void
process (fp, fnp)		/* Process one input file */
    FILE  *fp;			/* Input file descriptor */
    char  *fnp;			/* Input file name */

    {
    int   ch;
    int   prevch;
    int   lw_space;		/* TRUE when in leading whitespace */
static char  notfirst = FALSE;	/* Not first input file flag */

    if (l_flag)
	fprintf(stderr, "Catting file: %s\n", fnp);

    if (f_flag && notfirst)
	{
	notfirst = TRUE;
	fputc('\f', stdout);
	}

    lw_space = TRUE;
    icolcnt  = 0;
    wcolcnt  = 0;
    scolcnt  = 0;
    ocolcnt  = 0;
    quoted   = 0;
    prevch   = -1;
    whiteptr = whitebuf;

    while ((ch = getc(fp)) != EOF)
	{
	ch &= mask;			/* Apply the bit mask */

	if (lc_flag)			/* Apply case conversions */
	    ch = tolower(ch);
	else if (uc_flag)
	    ch = toupper(ch);

	if (ch == '\r')			/* Convert OS-9 files */
	    ch = '\n';

	if ((ch == '\n')  ||  (ch == '\f'))
	    {
	    flush(ch, TRUE);		/* Process line terminators */
	    icolcnt  = 0;
	    wcolcnt  = 0;
	    fflush(stdout);
	    quoted   = 0;
	    lw_space = TRUE;
	    }

	else if ((ch == ' ')  &&  (quoted == 0)  &&  (lw_space  ||  ! lw_flag))
	    {
	    if (whiteptr == whiteend)	/* Process space characters */
		{
		flush(EOF, FALSE);
		wcolcnt = icolcnt;
		}
	    whiteflag = TRUE;
	    *(whiteptr++) = ' ';
	    ++icolcnt;
	    }

	else if ((ch == '\t')  &&  (quoted == 0)  &&  (lw_space  ||  ! lw_flag))
	    {
	    if (whiteptr == whiteend)	/* Process tab characters */
		{
		flush(EOF, FALSE);
		wcolcnt = icolcnt;
		}
	    whiteflag = TRUE;
	    *(whiteptr++) = '\t';
	    icolcnt += (itabsize - (icolcnt % itabsize));
	    }

	else
	    {
	    lw_space = FALSE;		/* No longer in white space */
	    flush(ch, FALSE);		/* Process normal characters */
	    if (ch > ' ')
		++icolcnt;

	    if ( ! q_flag)		/* Process string quoting */
		{
		if ((ch == '"')  &&  (prevch != '\'')  &&  (prevch != '\\'))
		    quoted ^= 1;
		}

	    wcolcnt = icolcnt;
	    }
	prevch = ch;
	}

    flush(EOF, TRUE);
    fflush(stdout);
    }

/* ----------------------------------------------------------------------- */
    static void
flush (				/* Perform whitespace operations */
    int  ch,			/* The character to be sent */
    int  lineend)		/* TRUE if at the end of the line */

    {
    int    i;
    char  *p;


    if (p_flag  &&  ( ! lineend)  &&  (ocolcnt == 0))
	for (i = prefixsize; (i != 0); --i)
	    out(' ', FALSE);

    if (whiteflag)
	{
	if ((( ! h_flag)  ||  (ocolcnt != 0))
	&&  (( ! t_flag)  ||  ( ! lineend)))
	    {
	    if (d_flag)
		{
		for (i = (icolcnt - wcolcnt); (i > 0); --i)
		    out(' ', FALSE);
		}
	    else
		{
		for (p = whitebuf; p != whiteptr; ++p)
		    out(*p, FALSE);
		}
	    }
	whiteflag = FALSE;
	whiteptr  = whitebuf;
	}

    if (ch != EOF)
	out(ch, lineend);
    }

/* ----------------------------------------------------------------------- */
    static void
out (				/* Perform output operations */
    int  ch,			/* The character to be sent */
    int  lineend)		/* TRUE if at the end of the line */

    {
static int  lw_space = TRUE;	/* TRUE when in leading white space */
    int  recoding;		/* TRUE when should be recoding white space */

    recoding = (e_flag  &&  (lw_space  ||  ! lw_flag));

    if (recoding  &&  (ch == ' ')  &&  (quoted == 0))
	++ocolcnt;

    else
	{
	if (recoding)
	    {
	    int  i;

	    while (ocolcnt > (scolcnt + runlength))
		{
		i = (otabsize - (scolcnt % otabsize));
		if ((scolcnt + i) <= ocolcnt)
		    {
		    putc('\t', stdout);
		    scolcnt += i;
		    continue;
		    }
		break;
		}

	    for (i = (ocolcnt - scolcnt); (i > 0); --i)
		putc(' ', stdout);
	    }

	if (ch == '\n')
	    {
	    if (os9_flag)
		ch = '\r';
	    }
	else if ((ch != ' ')  &&  (ch != '\t'))
	    lw_space = FALSE;

	putc(ch, stdout);

	if (lineend)
	    {
	    ocolcnt  = 0;
	    lw_space = TRUE;
	    }
	else
	    ++ocolcnt;
	scolcnt = ocolcnt;
	}
    }

/* ----------------------------------------------------------------------- */
