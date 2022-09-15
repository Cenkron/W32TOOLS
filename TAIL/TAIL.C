/* ----------------------------------------------------------------------- *\
|
|				  TAIL Filter
|
|		Copyright (c) 1985, 1990, 1993, all rights reserved
|				Brian W Johnson
|				   18-Aug-90
|				   20-Feb-93
|				    7-Aug-93
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <ctype.h>
#include  <limits.h>

#include  "fwild.h"

/* ----------------------------------------------------------------------- *\
|  Definitions
\* ----------------------------------------------------------------------- */

#ifndef TRUE
#define FALSE	  0
#define TRUE	  1
#endif

#define  VPRINTF(x)  if (v_flag) printf x

/* ----------------------------------------------------------------------- *\
|  Variables
\* ----------------------------------------------------------------------- */

char	copyright [] =
"Copyright (c) 1985, 1993 by J & M Software, Dallas TX - All Rights Reserved";

int	f_flag   = FALSE;	/* Formfeed flag */
int	l_flag   = FALSE;	/* List name flag */
int	p_flag   = FALSE;	/* Prefix flag */
int	v_flag   = FALSE;	/* Verbose flag */

long	tsize   = 10L;		/* Tail size */
long	prefix  = 0L;		/* Prefix size */

char	swch = '-';		/* The switch character */

char	obuffer [BUFSIZ];	/* Buffer for stdout */

#define	MBUFSIZ   (50000)	/* Size of main data buffer */

char	buffer [MBUFSIZ] = { 0 };  /* Main data buffer */

char   *pbstart = &buffer[0];	/* Start of buffer */
char   *pbend   = &buffer[MBUFSIZ - 1]; /* End of buffer */
char   *pbhead  = &buffer[0];	/* Head of buffer */
char   *pbtail  = &buffer[0];	/* Tail of buffer */

#define	MAXLINES  (2001)	/* Maximum tail count */

char   *table [MAXLINES] = { 0 };  /* Line pointer table */

char  **ptstart = &table[0];	/* Start of table */
char  **ptend   = &table[MAXLINES - 1]; /* End of table */
char  **pthead  = &table[0];	/* Head of table */
char  **pttail  = &table[0];	/* Tail of table */

/* ----------------------------------------------------------------------- *\
|  Private function prototypes
\* ----------------------------------------------------------------------- */

void cantopen (char *);
void usage (void);
void help (void);
void dprint (char **);
void process (FILE *, char *);
void procin (FILE *);
void procout (void);

/* ----------------------------------------------------------------------- *\
|  main () - The main program
\* ----------------------------------------------------------------------- */
    void
main (
	int    argc,
	char  *argv [])

	{
	int    smode = FW_FILE;		/* File search mode attributes */
	char   ch;				/* Parser temporary */
	char  *s;				/* Parser temporary */
	void  *hp  = NULL;			/* Pointer to wild file data block */
	char  *fnp = NULL;			/* Input file name pointer */
	FILE  *fp  = NULL;			/* Input file descriptor */

	setbuf(stdout, obuffer);
	swch = egetswch();

	while (--argc > 0 && (*++argv)[0] == swch)
		{
		for (s = argv[0] + 1; ch = *(s++); )
			{
			switch (tolower(ch))
				{
				case 'f':
					++f_flag;
					break;

				case 'l':
					++l_flag;
					break;

				case 'p':
					++p_flag;
					if (optvalue(s, &prefix, 1, 80))
						{
						printf("Prefix parm error - %s\n", optvalerror());
						usage();
						}
					*s = '\0';
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
					if (optvalue(s-1, &tsize, 1, (MAXLINES - 1)))
						{
						printf("Tail size parm error - %s\n", optvalerror());
						usage();
						}
					*s = '\0';
					break;

				case 'v':
					++v_flag;
					break;

				case '?':
					help();

				default:
					usage();
				}
			}
		}

	if (argc == 0)
		process(stdin, "<stdin>");
	else
		{
		do  {
			hp = fwinit(*argv, smode);		/* Process the input list */
			if ((fnp = fwild(hp)) == NULL)
				{
				hp = NULL;
				cantopen(*argv);
				}
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
				hp = NULL;
				}
			} while (*++argv);
		}
	}

/* ----------------------------------------------------------------------- *\
|  cantopen () - Error report function for non-found files
\* ----------------------------------------------------------------------- */
	void
cantopen (fnp)			/* Inform user of input failure */
	char  *fnp;			/* Input file name */

	{
	printf("Unable to open input file: %s\n", fnp);
	}

/* ----------------------------------------------------------------------- *\
|  usage () - Usage message output
\* ----------------------------------------------------------------------- */
	void
usage ()			/* Display usage documentation */

	{
	static char  *udoc [] =
	{
"Usage:  tail  [%c?flv]  [%cp<n>]  [%c<n>]  [input_file_list]  [>output_file]",
	"        tail  %c?  for help",
	NULL
	};

	dprint(udoc);
	exit(4);
	}

/* ----------------------------------------------------------------------- *\
|  help () - Help message output
\* ----------------------------------------------------------------------- */
	void
help ()				/* Display help documentation */

	{
	static char  *hdoc [] =
	{
	"Usage:  tail  [%c?flv]  [%cp<n>]  [%c<n>]  [input_file_list]  [>output_file]",
	"",
	"tail concatenates the tails of the files in the input_file_list",
	"(or stdin) to stdout (or to the redirected output file).",
	"",
	"    %cf    interposes formfeeds between files",
	"    %cl    lists file names as they are concatenated",
	"    %cp<n> prefixes <n> columns of white space to each line of text",
	"    %c<n>  determines the tail size; the default is 10",
	"",
	copyright,
	NULL
	};

	dprint(hdoc);
	exit(0);
	}

/* ----------------------------------------------------------------------- *\
|  dprint () - Message printing utility
\* ----------------------------------------------------------------------- */
    void
dprint (			/* Print documentation text */
	char  **dp)

	{
	while (*dp)
		{
		printf(*(dp++), swch, swch, swch);
		putchar('\n');
		}
	}

/* ----------------------------------------------------------------------- *\
|  process () - Process one input file
\* ----------------------------------------------------------------------- */
	void
process (fp, fnp)		/* Process one input file */
	FILE  *fp;			/* Input file descriptor */
	char  *fnp;			/* Input file name */ 

	{
static int  notfirst = FALSE;	/* Not first input file flag */


	if (l_flag)
		fprintf(stderr, "Tailing file: %s\n", fnp);

	if (f_flag  &&  notfirst)
		fputc('\f', stdout);

	procin(fp);

VPRINTF(("pthead: %d\n", pthead - ptstart));
VPRINTF(("pttail: %d\n", pttail - ptstart));
VPRINTF(("pbhead: %d\n", pbhead - pbstart));
VPRINTF(("pbtail: %d\n", pbtail - pbstart));

	if (pbhead != pbtail)
		procout();

	fflush(stdout);
	notfirst = TRUE;
	}

/* ----------------------------------------------------------------------- *\
|  procin () - Read one input file to the queue
\* ----------------------------------------------------------------------- */
	void
procin (					/* Read in one input file to the queue */
	FILE  *fp)				/* Input file descriptor */

	{
	int   ch;				/* Temporary character */
	int   first = TRUE;		/* First character flag */
	int   eol   = TRUE;		/* End of line flag */


	pbhead  = &buffer[0];	/* Initialize the queues */
	pbtail  = &buffer[0];
	pthead  = &table[0];
	pttail  = &table[0];
	*pthead = NULL;

	while ((ch = getc(fp)) != EOF)
		{
		if (ch == '\0')
			continue;

/* If we are currently at end of line, enter a new line pointer */

		if (eol)
			{
VPRINTF(("processing eol\n"));
			eol     = FALSE;
			*pthead = pbhead;
			if (pthead == ptend)
			pthead =  ptstart;
			else
			++pthead;
			*pthead = NULL;

			if (pthead == pttail)
				{
VPRINTF(("processing eol, pushing pttail forward\n"));
				if (pttail == ptend)
					pttail =  ptstart;
				else
					++pttail;
				}
			}

/* If we are about to overrun the next line, bump the line table pointer */

		if ((pbhead == *pttail)  &&  (pbhead != pbtail))
			{
VPRINTF(("pushing pttail forward because of pbhead collision\n"));
			if (pttail == ptend)
				pttail =  ptstart;
			else
				++pttail;
			}

/* Enqueue this character; if we overrun the queue, bump the tail buffer pointer */

		*pbhead = (char)(ch);
		if (pbhead == pbend)
			pbhead =  pbstart;
		else
			++pbhead;

VPRINTF(("character: %02X\n", ch));

		if (pbhead == pbtail)
			{
			if (pbtail == pbend)
				pbtail =  pbstart;
			else
				++pbtail;
			}

/* If this character terminates a line, remember it */

		if ((ch == '\n')  ||  (ch == '\f'))
			{
VPRINTF(("setting eol\n"));
			eol = TRUE;
			}
		}
    }

/* ----------------------------------------------------------------------- *\
|  procout () - Write one output file to stdout
\* ----------------------------------------------------------------------- */
	void
procout ()			/* Write the queue segment to stdout */

	{
	int    eol = TRUE;		/* End of line flag */
	int    n;			/* General purpose counter */
	char   ch;			/* The current character */
	char  *pb;			/* Pointer into the character queue */
	char **pt;			/* Pointer into the line table */


    /* Determine the starting offset within the character queue */

	n = (int)(tsize);
	pt    = pthead;
	while (n-- > 0)
		{
		if (pt == pttail)
			break;
		if (pt == ptstart)
			pt =  ptend;
		else
			--pt;
		}

/* Output all characters remaining in the queue */

	pb = *pt;
	while (pb != pbhead)
		{
		if (p_flag  &&  eol)	/* If necessary, write the prefix */
			{
			for (n = (int)(prefix); n > 0; --n)
			fputc(' ', stdout);
			eol = FALSE;
			}

		ch = *pb;		/* Output the character */
		putc(ch, stdout);

		if (pb == pbend)	/* Advance the queue pointer */
			pb =  pbstart;
		else
			++pb;

		if ((ch == '\n')  ||  (ch == '\f'))
			eol = TRUE;		/* End of line reached */
		}
	}

/* ----------------------------------------------------------------------- */
