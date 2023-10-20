/* ----------------------------------------------------------------------- *\
|
|				      WC
|
|		    Copyright (c) 1985, 1990, all rights reserved
|				Brian W Johnson
|				   18-Aug-90
|				   12-Jul-92
|				   18-Feb-93
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <stdlib.h>
#include  <ctype.h>
#include  <io.h>

#include  "fWild.h"

#ifndef TRUE
#define FALSE	  0
#define TRUE	  1
#endif

/* ----------------------------------------------------------------------- */

char	copyright [] =
"Copyright (c) 1993 by J & M Software, Dallas TX - All Rights Reserved";

char	buffer [BUFSIZ];	/* Buffer for stdout */

typedef enum
	{
	Mixed = 0,
	Unix,
	Windows
	}  ENDTYPE;

ENDTYPE	type;					// Type of file
static char typeName [3][15] =	// Name of file type
	{ "Mixed", "Unix", "Windows" };

int	smode = FW_FILE;	/* File search mode attributes */

int	a_flag = FALSE;		/* Analyze the line ending type (default off) */
int	i_flag = FALSE;		/* Take pathnames from stdin (automatic) */
int	d_flag = TRUE;		/* List detail results flag (default on) */
int	t_flag = TRUE;		/* List only total results flag (default on)  */
int	f_flag = TRUE;		/* List only files, not stats (default off)  */
int	m_flag = FALSE;		/* Show only mixed type files (default off) */
int	w_flag = FALSE;		/* Show only Unix type files (default off) */
int	u_flag = FALSE;		/* Show only Windows type files (default off) */

INT64	tc		= 0L;	/* Total character count */
INT64	tw		= 0L;	/* Total word count */
INT64	tl		= 0L;	/* Total LF (line) count */
INT64	tr		= 0L;	/* Total CR count */
INT64	files	= 0;	// Total number of files
INT64	ufiles	= 0;	// Total number of unix (LF) files
INT64	wfiles	= 0;	// Total number of Windows (CRLF) files
INT64	mfiles	= 0;	// Total number of mixed line ending files

char	swch = '-';		/* The switch character */

void   *hp  = NULL;		// Pointer to the fWild instance

static  void    Process (FILE *, char *);
static	void	ProcessLoop (const char *pPattern);
static	void	ListTotals (void);
static  char   *UseStdin (void);

/* ----------------------------------------------------------------------- */

char  *usagedoc [] =
{
"Usage:  wc  [%c?dil]  [input_file_list]  [>output_file]",
"",
"wc counts the characters, words, and lines in the input_file_list",
"",
"    %ca  analyze the file ending type  (default off)",
"    %cf  lists only selected files, no stats (default off)",
"    %ct  lists (only) total results    (default all)",
"    %cm  lists only Mixed text files   (default all types)",
"    %cw  lists only Windows text files (default all types)",
"    %cu  lists only Unix text files    (default all types)",
"",
copyright,
NULL
};

/* ----------------------------------------------------------------------- */
//	Main program
/* ----------------------------------------------------------------------- */
	void
main (
	int    argc,
	char  *argv [])

	{
	int    option;			/* Option character */
	char  *pPattern;		/* Pattern pointer */

static	char   *optstring = "?aAfFiImMuUwW";


	if ((hp = fwOpen()) == NULL)
		exit(1);

	setbuf(stdout, buffer);
	optenv = getenv("WC");
	swch = egetswch();

	while ((option = getopt(argc, argv, optstring)) != EOF)
		{
		switch (tolower(option))
			{
			case 'f':
				f_flag = FALSE;
				break;

			case 'a':
				a_flag = TRUE;
				m_flag = TRUE;
				u_flag = TRUE;
				w_flag = TRUE;
				break;

			case 't':
				d_flag = FALSE;
				t_flag = TRUE;
				break;

			case 'm':
				a_flag = TRUE;
				m_flag = TRUE;
				u_flag = FALSE;
				w_flag = FALSE;
				break;

			case 'u':
				a_flag = TRUE;
				m_flag = FALSE;
				u_flag = TRUE;
				w_flag = FALSE;
				break;

			case 'w':
				a_flag = TRUE;
				m_flag = FALSE;
				u_flag = FALSE;
				w_flag = TRUE;
				break;

			case '?':
				help();

			default:
				usage();
			}
		}


	i_flag = ( ! isatty(fileno(stdin)));

	if (i_flag)
		{
		while (pPattern = UseStdin())		// Expand the stdin list
			ProcessLoop (pPattern);			// Expand pathspec
		}

	else
		{
		while (optind < argc)
			{
			pPattern = argv[optind++];
			ProcessLoop (pPattern);			// Expand pathspec
			}
		}

	ListTotals();

	hp = fwClose(hp);
	exit(0);
	}

/* ----------------------------------------------------------------------- */
//	Generate the file list from the search pattern
/* ----------------------------------------------------------------------- */
	static void
ProcessLoop (	
	const char *pPattern)	// Expand the filespec

	{
	char  *fnp = NULL;		/* Input file name pointer */
	FILE  *fp  = NULL;		/* Input file descriptor */


	if (fwInit(hp, pPattern, smode) != FWERR_NONE)	// Process the pattern
		fwInitError(pPattern);
	if ((fnp = fWild(hp)) == NULL)
		{
		cantopen(pPattern);
		}
				
	else
		{
		do  {				// Process one filespec
			if (fp = fopen(fnp, "rb"))
				{
				Process(fp, fnp);
				fclose(fp);
				}
			else
				cantopen(fnp);
			} while ((fnp = fWild(hp)));
		}
	}

/* ----------------------------------------------------------------------- */
//	Process one file
/* ----------------------------------------------------------------------- */
	static void
Process (				// Process one input file
	FILE  *fp,			// Input file descriptor
	char  *fnp)			// Input file name

	{
	int		ch;					// Temporary character
	int		inWord   = FALSE;	// TRUE when parsing a word
	int		selected = FALSE;	// Selected by the type analysis

	INT64	nc = 0;		// Character count
	INT64	nw = 0;		// Word count
	INT64	nl = 0;		// LF (line) count
	INT64	nr = 0;		// CR count

	while ((ch = getc(fp)) != EOF)
		{
		++nc;
		if (ch == '\r')
			++nr;
		
		if (ch == '\n')
			++nl;

		if ((ch == '\r') || (ch == '\n') || (ch == ' ') || (ch == '\t'))
			inWord = FALSE;
		else if (inWord == FALSE)
			{
			inWord = TRUE;
			++nw;
			}
		}

	if (a_flag)
		{
		if (nl > 0)		// Classify by line ending type
			{
			if (nr == nl)
				{
				type = Windows;
				if (w_flag)
					{
					++wfiles;
					selected = TRUE;
					}
				}
			else if (nr == 0)
				{
				type = Unix;
				if (u_flag)
					{
					++ufiles;
					selected = TRUE;
					}
				}
			else
				{
				type = Mixed;
				if (m_flag)
					{
					++mfiles;
					selected = TRUE;
					}
				}
			}
		}

	if (selected || (! a_flag))
		{
		tc += nc;		// Update the totals
		tw += nw;
		tl += nl;
		tr += nr;
		++files;

		if (d_flag)
			{
			printf("%s\n", fnp);
			if (f_flag)
				{
				printf("%8lld characters  %8lld words       %8lld lines\n", nc, nw, nl);
				if (a_flag)
					{
					printf("%8lld LF count    %8lld CR count    %s\n", nl, nr, &typeName[type][0]);
					}
				}
			fflush(stdout);
			}
		}
	}

/* ----------------------------------------------------------------------- */
//	Process stdin to generate the input filespecs
/* ----------------------------------------------------------------------- */
	char *
UseStdin (void)		/* Parse pathnames from stdin */

	{
	int  ch;
	char  *p;
static	int   eofflag = FALSE;
static	char  line [81];

	line[0] = '\0';
	if ( ! eofflag)
		{
		for (;;)
			{
			ch = getchar();
			if (ch == EOF)
				{
				eofflag = TRUE;
				break;
				}
			if (isgraph(ch))
				break;
			}
		}

	if ( ! eofflag)
		{
		for (p = &line[0]; ; )
			{
			if (p >= &line[80])
				{
				line[0] = '\0';
				break;
				}
			*(p++) = (char)(ch);
			*p = '\0';
			ch = getchar();
			if ( ! isgraph(ch))
				{
				if (ch == EOF)
					eofflag = TRUE;
				break;
				}
			}
		}

	p = (line[0] != '\0') ? (&line[0]) : (NULL);
	return (p);
	}

/* ----------------------------------------------------------------------- */
//	List total stats
/* ----------------------------------------------------------------------- */
	static void
ListTotals (void)

	{
	if (t_flag && f_flag)
		{
		printf("-----------------------------------------------------------------\n");
		printf("Total: %6lld files\n", files);
		printf("%8lld characters  %8lld words       %8lld lines\n", tc, tw, tl);
		if (a_flag)
			{
			printf("%8lld Unix files  %8lld Win files   %8lld Mixed files\n",
				ufiles, wfiles, mfiles);
			}
		}
	}

/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */
