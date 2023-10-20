/* ----------------------------------------------------------------------- *\
|
|			      COMM File Comparator
|
|				Brian W Johnson
|				   18-Aug-90
|				   27-Dec-92
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <ctype.h>

#include  "fWild.h"

#ifndef TRUE
#define FALSE	  0
#define TRUE	  1
#endif

char	 buffer [BUFSIZ];	/* Buffer for stdout */

/* ----------------------------------------------------------------------- */
/*
** The  information  in  this  document  is  subject  to  change
** without  notice  and  should not be construed as a commitment
** by Digital Equipment Corporation or by DECUS.
** 
** Neither Digital Equipment Corporation, DECUS, nor the authors
** assume any responsibility for the use or reliability of  this
** document or the described software.
** 
** 	Copyright (C) 1980, DECUS
** 
** General permission to copy or modify, but not for profit,  is
** hereby  granted,  provided that the above copyright notice is
** included and reference made to  the  fact  that  reproduction
** privileges were granted by DECUS.
*/
/* ----------------------------------------------------------------------- */
/*
** modified for msdos 2.x and lattice c 2.x msm 9 feb 84
*/
/* ----------------------------------------------------------------------- */

#define	LINESIZE	512

struct area
    {
    FILE * fd;
    char    line[LINESIZE];
    }           area[2];

FILE   *outfd;

char   *head[3] =
    {
    "", "\t", "\t\t"
    };				/* Heading vector */
int	flag[3];		/* What's set */

int	w_flag = FALSE;		/* The trim flag */

char	swch = '-';		/* The switch character */

void fill (int);
void fillboth (void);
void flushout (int);
void output (char *, int);
void trim (char *, int);
void finis (void);
void dprint (char **);
int	equals (char *str1, char *str2);
int	input (int into);

/* ----------------------------------------------------------------------- */
    void
main (argc, argv)
    int    argc;
    char  *argv [];

    {
    register int    temp;
    register int    c;


    setbuf(stdout, buffer);
    swch = egetswch();

    temp = 0;
    if (argc <= 1)
	usage();
    if (argv[1][0] == swch && argv[1][1] != 0)
	{
	while ((c = *++argv[1]))
	    {
	    switch (tolower(c))
		{
		case '1': 
		case '2': 
		case '3': 
		    c -= '1';	/* '1..3' => 0..2	 */
		    if (!flag[c])
			{
			flag[c]++;
			temp++;
			}
		    break;

		case 'w':
		    w_flag = TRUE;
		    break;

		case '?':
		    help();

		default: 
		    usage();
		}
	    }
	head[2] = head[2 - temp];
	argc--;
	argv++;
	}

    /*
    ** Open files
    */

    if (argc < 3)
	usage();
    for (temp = 0; temp < 2; temp++)
	{
	if ((area[temp].fd = fopen(argv[1 + temp], "r")) == NULL)
		{
	    cantopen(argv[1 + temp]);
	 	exit(1);
	 	}
	}
    if (argc > 3)
	{
	if ((outfd = fopen(argv[3], "w")) == NULL)
		{
	    cantopen(argv[3]);
	 	exit(1);
	 	}
	}
    else
	outfd = stdout;

    /*
    ** Fill both buffers and off we go
    */

    fillboth();
    for (;;)
	{
	switch (equals(area[0].line, area[1].line))
	    {

	    case 0: 		/* File 1 == file 2	 */
		output(area[0].line, 2);
		fillboth();
		break;

	    case 1: 		/* File 1 < file 2	 */
		output(area[0].line, 0);
		fill(0);
		break;

	    case 2: 		/* File 1 > file 2	 */
		output(area[1].line, 1);
		fill(1);
		break;
	    }
	}
    }

/* ----------------------------------------------------------------------- */
    void
fill (into)			/* buffer filler.  Just does one of them. */
    int     into;		/* Fill this buffer		 */

    {
    if (input(into))
	flushout(1 - into);
    }

/* ----------------------------------------------------------------------- */
    void
fillboth ()			/* Fill both buffers */

    {
    if (input(0))
	{
	if (input(1))
	    finis();		/* Both end filed	 */
	else
	    flushout(1);	/* Just file 0		 */
	}
    if (input(1))
	flushout(0);
    }

/* ----------------------------------------------------------------------- */
    int
input (into)		/* Read a line into this buffer, return 1 on eof */
    int  into;			/* Read into this buffer */

    {
    return  (fgets(area[into].line, LINESIZE, area[into].fd) == NULL);
    }

/* ----------------------------------------------------------------------- */
    void
flushout (other)		/* Flush out a line */
    int  other;			/* Flush this one */

    {
    do	{
	output(area[other].line, other);
	} while (input(other) == 0);
    finis();
    }

/* ----------------------------------------------------------------------- */
    void
output (text, which)		/* Output lines */
    char  *text;		/* Line to output */
    int    which;		/* Which column; 1, 2, or 3 */

    {
    char  tmpline [LINESIZE];

    if (!flag[which])
	{
	if (w_flag)
	    {
	    strcpy(&tmpline[0], text);
	    trim(&tmpline[0], which);
	    text = &tmpline[0];
	    }
	fprintf(outfd, "%s%s", head[which], text);
	}
    }

/* ----------------------------------------------------------------------- */
    void
trim (s, n)			/* Trim the output line to 80 columns */
    char  *s;			/* Pointer to the string */
    int    n;			/* Tab number */

    {
    int  col = 8 * n;			/* Offset for the column */

    while (*s)
	{
	if (*s == '\t')			/* Handle tabs */
	    {
	    col = (col + 8) & ~7;
	    }
	else
	    ++col;
	if (col > 79)
	    {
	    *(s++) = '\n';
	    *s = '\0';
	    return;
	    }
	++s;
	}
    }

/* ----------------------------------------------------------------------- */
    int
equals (str1, str2)		/* Compare two strings */
    char  *str1;
    char  *str2;

    {
    register char  *p1;
    register char  *p2;

    p1 = str1 - 1;
    p2 = str2 - 1;
    while (*++p1 == *++p2)
	{
	if (*p1 == 0)
	    return  (0);
	}
    return  ((p1 < p2) ? 1 : 2);
    }

/* ----------------------------------------------------------------------- */
    void
finis ()			/* Exit from comm */

    {
    fflush(outfd);
    fclose(outfd);
    fclose(area[0].fd);
    fclose(area[1].fd);
    exit(0);
    }

/* ----------------------------------------------------------------------- */
    void
usage ()			/* Display usage documentation */

    {
    static char  *udoc [] =
	{
	"Usage:  comm  [%c?w123]  file1  file2  [out_file]",
	"        comm  %c?  for help",
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
	"Usage:  comm  [%c?w123]  file1  file2  [out_file]",
	"",
	"comm prints lines common to two files",
	"",
	"Lines in file 1 only are printed in the leftmost column.",
	"Lines in file 2 only are printed in the second column.",
	"Lines in both files are printed in the third column.",
	"Flags 1, 2, or 3 suppress printing of the corresponding column",
	"",
	"    %cw    trims output lines to 80 columns",
	"    %c12   prints only lines common to both files",
	"    %c23   prints lines in the first file, but not the second",
	"    %c123  does nothing",
	"",
	"If the third file is not present, output goes to stdout",
	"Stdout may be redirected.",
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
	printf(*(dp++), swch);
	putchar('\n');
	}
    }

/* ----------------------------------------------------------------------- */
