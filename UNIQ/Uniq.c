/* ----------------------------------------------------------------------- *\
|
|				     UNIQ
|
|		    Copyright (c) 1985, 1990, all rights reserved
|				Brian W Johnson
|				   18-Aug-90
|				   22-Mar-05
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <ctype.h>

#include  "fwild.h"

#ifndef TRUE
#define FALSE	  0
#define TRUE	  1
#endif

#ifndef MATCH
#define MATCH	  0
#endif

char	copyright [] =
"Copyright (c) 1985 by J & M Software, Dallas TX - All Rights Reserved";

char	 buffer [BUFSIZ];		/* Buffer for stdout */

int	defflg = TRUE;			/* Use default flag */
int	c_flag = FALSE;			/* Count flag */
int	d_flag = FALSE;			/* Duplicate flag */
int	i_flag = TRUE;			/* Ignore case flag */
int	l_flag = FALSE;			/* List file names flag */
int	u_flag = FALSE;			/* Unique flag */

int	w_cnt  = 0;			/* Word skip count */
int	c_cnt  = 0;			/* Character skip count */

char	swch = '-';			/* The switch character */

static void cantopen (char *);
void usage (void);
void help (void);
void dprint (char **);
static void process (FILE *, char *);

/* ----------------------------------------------------------------------- */
    void
main (argc, argv)
    int    argc;
    char  *argv [];

    {
    int    smode = FW_FILE;		/* File search mode attributes */
    char   sw;				/* Parser temporary */
    char  *s;				/* Parser temporary */
    void  *hp  = NULL;			/* Pointer to wild file data block */
    char  *fnp = NULL;			/* Input file name pointer */
    FILE  *fp  = NULL;			/* Input file descriptor */


    setbuf(stdout, buffer);
    swch = egetswch();

    while (--argc > 0)
	{
	sw = **(++argv);
	if ((sw != swch)
	&&  (sw != '-')
	&&  (sw != '+'))
	    break;

	for (s = *argv + 1; *s; ++s)
	    {
	    if (isdigit(*s))
		{
		if (sw == '-')
		    {
		    w_cnt = atoi(s);
		    break;
		    }
		else if (sw == '+')
		    {
		    c_cnt = atoi(s);
		    break;
		    }
		else
		    usage();
		}
	    else if (sw == swch)
		{
		switch (tolower(*s))
		    {
		    case 'c':
			++c_flag;
			++d_flag;
			++u_flag;
			defflg = FALSE;
			break;

		    case 'd':
			++d_flag;
			defflg = FALSE;
			break;

		    case 'i':
			i_flag = !i_flag;
			break;

		    case 'l':
			++l_flag;
			break;

		    case 'u':
			++u_flag;
			defflg = FALSE;
			break;

		    case '?':
			help();

		    default:
			usage();
		    }
		}
	    else
		usage();
	    }
	}

    if (defflg)
	{
	d_flag = TRUE;
	u_flag = TRUE;
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
	"Usage:  uniq  [%c?clnu]  [+NNN]  [-NNN]  [input_file_list]",
	"        uniq  %c?  for help",
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
	"Usage:  uniq  [%c?clnu]  [+NNN]  [-NNN]  [input_file_list]",
	"",
	"uniq reads the input_file_list (or stdin), comparing",
	"adjacent lines, and listing the results as follows:",
	"",
	"    %cc  lists repeat counts (sets %cdu)",
	"    %cd  lists only lines which are duplicate (default)",
	"         (Repeated lines are listed only once)",
	"    %ci  ignores case when matching (default on)",
	"    %cl  lists file names as they are processed",
	"    %cu  lists only lines which are unique (default)",
	"",
	"    -N  skips N \"words\" before testing",
	"    +N  skips N characters after the word skip",
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

/* ----------------------------------------------------------------------- */
    static void
process (fp, fnp)		/* Process one input file */
    FILE  *fp;			/* Input file descriptor */
    char  *fnp;			/* Input file name */ 

    {
    int    i;			/* Temporary field/character counter */
    int    count;		/* Line match count */
    char  *oldp;		/* Pointer to compare point */
    char  *newp;		/* Pointer to compare point */
    char   old [512];		/* Buffer for previous line */
    char   new [512];		/* Buffer for current line */


    if (l_flag)
	printf("\nProcessing file: %s\n", fnp);

    count = 0;				/* Zero until successful read */

    while (fgets(&new[0], sizeof new, fp))
	{
	newp = &new[0];			/* Skip words and/or characters */
	if (w_cnt)
	    {
	    newp = stpblk(newp);
	    for (i = w_cnt; ((i > 0) && (*newp)); --i)
		{
		while ((*newp) && ( ! isspace(*newp)))
		    ++newp;
		newp = stpblk(newp);
		}
	    }
	for (i = c_cnt; ((i > 0) && (*newp)); --i)
	    ++newp;


	if (count == 0)			/* If the first line... */
	    {
	    strcpy(&old[0], &new[0]);
	    oldp  = &old[0] + (newp - &new[0]);
	    count = 1;
	    }
	else				/* Not the first line */
	    {
	    if (((i_flag)  &&  (stricmp(newp, oldp) == MATCH))
	    ||                 (strcmp (newp, oldp) == MATCH))
		++count;		/* Increment the match count */
	    else
		{
		if ((u_flag && (count == 1)) || (d_flag && (count > 1)))
		    {
		    if (c_flag)
			fprintf(stdout, "%6d\t", count);
		    fputs(&old[0], stdout);
		    fflush(stdout);
		    }
		strcpy(&old[0], &new[0]);
		oldp  = &old[0] + (newp - &new[0]);
		count = 1;
		}
	    }
	}

					/* Process the last line, if any */

    if ((u_flag && (count == 1)) || (d_flag && (count > 1)))
	{
	if (c_flag)
	    fprintf(stdout, "%6d\t", count);
	fputs(&old[0], stdout);
	fflush(stdout);
	}
    }

/* ----------------------------------------------------------------------- */
