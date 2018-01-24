/* ----------------------------------------------------------------------- *\
|
|				  HEAD Filter
|
|		    Copyright (c) 1985, 1990, all rights reserved
|				Brian W Johnson
|				   18-Aug-90
|				   20-Feb-93
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <stdlib.h>
#include  <ctype.h>
#include  <limits.h>

#include  "fwild.h"

#ifndef TRUE
#define FALSE	  0
#define TRUE	  1
#endif

char	copyright [] =
"Copyright (c) 1985 by J & M Software, Dallas TX - All Rights Reserved";

char	 buffer [BUFSIZ];	/* Buffer for stdout */

#define  RECLEN		1024	/* MAximum line length */

int	f_flag   = FALSE;	/* Formfeed flag */
int	l_flag   = FALSE;	/* List name flag */
int	p_flag   = FALSE;	/* Prefix with a tab flag */
int	t_flag   = FALSE;	/* Trim lines flag */
int	notfirst = FALSE;	/* Not first input file flag */

char	prefix [32];		/* Prefix string */

long	reccnt  =  0L;		/* Record counter */
long	hsize   = 10L;		/* Head size */

char	swch = '-';		/* The switch character */

void cantopen (char *);
void usage (void);
void help (void);
void dprint (char **);
void build (char *);
void process (FILE *, char *);
void proc (FILE *, char *);
void trim (char *, int, char);

/* ----------------------------------------------------------------------- */
    void
main (argc, argv)
    int    argc;
    char  *argv [];

    {
    int    smode = FW_FILE;		/* File search mode attributes */
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
		case 'f':
		    ++f_flag;
		    break;

		case 'l':
		    ++l_flag;
		    break;

		case 'p':
		    ++t_flag;
		    ++p_flag;
		    build(s);
		    *s = '\0';
		    break;

		case 't':
		    ++t_flag;
		    break;

		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		    if (optvalue(s-1, &hsize, 1, LONG_MAX))
			{
			printf("Head size parm error - %s\n", optvalerror());
			usage();
			}
		    *s = '\0';
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
    void
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
"Usage:  head  [%c?flt]  [%cp<n>]  [%c<n>]  [input_file_list]  [>output_file]",
	"        tail  %c?  for help",
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
"Usage:  head  [%c?flt]  [%cp<n>]  [%c<n>]  [input_file_list]  [>output_file]",
	"",
	"head concatenates the heads of the files in the input_file_list",
	"(or stdin) to stdout (or to the redirected output file).",
	"",
	"    %cf    interposes formfeeds between files",
	"    %cl    lists file names as they are concatenated",
	"    %cp<n> prefixes <n> columns of white space to each line of text",
	"    %ct    trims off all trailing white space from each line",
	"    %c<n>  determines the head size; the default is 10",
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
	printf(*(dp++), swch, swch, swch);
	putchar('\n');
	}
    }

/* ----------------------------------------------------------------------- */
    void
build (s)			/* Build the white space prefix string */
    char  *s;			/* Pointer to the size parameter string */

    {
    int   n;
    long  ltemp;


    if (optvalue(s, &ltemp, 1, 80))
	{
	printf("Prefix parm error - %s\n", optvalerror());
	usage();
	}

    n = (int)(ltemp);
    s = &prefix[0];
    while (n > 7)
	{
	n -= 8;
	*(s++) = '\t';
	}
    while (n-- > 0)
	*(s++) = ' ';
    *s = '\0';
    }

/* ----------------------------------------------------------------------- */
    void
process (fp, fnp)		/* Process one input file */
    FILE  *fp;			/* Input file descriptor */
    char  *fnp;			/* Input file name */ 

    {
    if (l_flag)
	fprintf(stderr, "Heading file: %s\n", fnp);

    if (f_flag && notfirst)
	fputc('\f', stdout);

    proc(fp, fnp);

    fflush(stdout);
    notfirst = TRUE;
    }

/* ----------------------------------------------------------------------- */
    void
proc (fp, fnp)			/* Process one input file */
    FILE  *fp;			/* Input file descriptor */
    char  *fnp;			/* Input file name */ 

    {
    int   n;			/* Character counter */
    int   ch;			/* Temporary character */
    char  line [RECLEN];	/* Text line buffer */


    n = 0;
    reccnt = 0L;
    while ((ch = getc(fp)) != EOF)
	{
	if (ch == '\0')
	    ;
	else if ((ch == '\n') || (ch == '\f'))
	    {
	    trim(&line[0], n, (char) ch);
	    n = 0;
	    if (++reccnt >= hsize)
		break;
	    }
	else
	    line[n++] = (char)(ch);
	}

    if (n > 0)
	trim(&line[0], n, (char) '\0');
    }

/* ----------------------------------------------------------------------- */
    void
trim (s, n, ch)		/* Store a text line into the line buffer */
    char  *s;			/* Pointer to the line */
    int    n;			/* Number of characters in the line */
    char   ch;			/* The line terminator */

    {
    char  *p;			/* Pointer to the line end */


    p = s + n - 1;		/* Point the last line character */
    if (t_flag && (n > 0))	/* If necessary, trim the line */
	{
	while ((isspace(*p)) && (p-- != s))
	    ;
	}
    if (ch != '\0')
	*(++p) = ch;		/* Append the terminator character */
    *(++p) = '\0';		/* Append the terminator NUL */

    if ((p - s) > 0)		/* If not empty, store the line */
	{
	ch = *s;
	if ((ch == '\n') || (ch == '\f'))
	    putc(ch, stdout);
	else
	    {
	    if (p_flag)			/* If necessary, write the prefix */
		fputs(prefix, stdout);
	    fputs(s, stdout);
	    }
	}
    }

/* ----------------------------------------------------------------------- */
