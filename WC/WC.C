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

#include  "fwild.h"

#ifndef TRUE
#define FALSE	  0
#define TRUE	  1
#endif

char	copyright [] =
"Copyright (c) 1993 by J & M Software, Dallas TX - All Rights Reserved";

char	buffer [BUFSIZ];	/* Buffer for stdout */

int	d_flag = FALSE;		/* List detail results flag */
int	i_flag = FALSE;		/* Take pathnames from stdin */
int	l_flag = FALSE;		/* List file names flag */

long	tc = 0L;		/* Total character count */
long	tw = 0L;		/* Total word count */
long	tl = 0L;		/* Total line count */

char	swch = '-';		/* The switch character */

static  void    process (FILE *, char *);
static  char   *stdpath (void);

/* ----------------------------------------------------------------------- */

char  *usagedoc [] =
{
"Usage:  wc  [%c?dil]  [input_file_list]  [>output_file]",
"",
"wc counts the characters, words, and lines in the input_file_list",
"",
"    %ci  takes the input_file_list from stdin",
"    %cd  lists detailed results",
"    %cl  lists file names as they are processed",
"",
copyright,
NULL
};

/* ----------------------------------------------------------------------- */
	void
main (
	int    argc,
	char  *argv [])

	{
	int    smode = FW_FILE;	/* File search mode attributes */
	int    option;		/* Option character */
	char  *ap;			/* Argument pointer */
	void  *hp  = NULL;		/* Pointer to wild file data block */
	char  *fnp = NULL;		/* Input file name pointer */
	FILE  *fp  = NULL;		/* Input file descriptor */

static	char   *optstring = "?dDiIlL";


	setbuf(stdout, buffer);
	optenv = getenv("WC");
	swch = egetswch();

	while ((option = getopt(argc, argv, optstring)) != EOF)
		{
		switch (tolower(option))
			{
			case 'd':
				++d_flag;
				++l_flag;
				break;

			case 'i':
				i_flag = TRUE;
				break;

			case 'l':
				++l_flag;
				break;

			case '?':
				help();

			default:
				usage();
			}
		}


	if (i_flag)
		{
		while (ap = stdpath())			/* Process the stdin list */
			{
			hp = fwinit(ap, smode);		/* Process the input list */
			if ((fnp = fwild(hp)) == NULL)
				{
				hp = NULL;
				cantopen(ap);
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
			}
		}

	else if (optind >= argc)
		process(stdin, "<stdin>");

	else
		{
		while (optind < argc)
			{
			ap = argv[optind++];
			hp = fwinit(ap, smode);		/* Process the input list */
			if ((fnp = fwild(hp)) == NULL)
				{
				hp = NULL;
				cantopen(ap);
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
			}
		}

	if (d_flag)
		printf("    Total:      ");
	printf("%8ld characters  %8ld words  %8ld lines\n", tc, tw, tl);
	exit(0);
	}

/* ----------------------------------------------------------------------- */
	static void
process (				/* Process one input file */
	FILE  *fp,			/* Input file descriptor */
	char  *fnp)			/* Input file name */ 

	{
	int  ch;			/* Temporary character */
	int  inword;		/* TRUE when parsing a word */
	long  nc = 0;		/* Character count */
	long  nw = 0;		/* Word count */
	long  nl = 0;		/* Line count */

	if (d_flag)
		printf("%-16s", fnp);
	else if (l_flag)
		printf("%s", fnp);

	inword = FALSE;
	while ((ch = getc(fp)) != EOF)
		{
		++nc;
		if (ch == '\n')
			{
			++nl;
			inword = FALSE;
			}

		if ((ch == '\n') || (ch == ' ') || (ch == '\t'))
			inword = FALSE;
		else if (inword == FALSE)
			{
			inword = TRUE;
			++nw;
			}
		}

	tc += nc;		/* Update the totals */
	tw += nw;
	tl += nl;

	if (d_flag)
		printf("%8ld characters  %8ld words  %8ld lines", nc, nw, nl);
	if (d_flag || l_flag)
		putchar('\n');
	fflush(stdout);
	}

/* ----------------------------------------------------------------------- */
	char *
stdpath (void)		/* Parse pathnames from stdin */

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
