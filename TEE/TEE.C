/* ----------------------------------------------------------------------- *\
|
|				  TEE Filter
|
|		    Copyright (c) 1985, 1990, all rights reserved
|				Brian W Johnson
|				   18-Aug-90
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <stdlib.h>
#include  <ctype.h>

#include  "fwild.h"

char	copyright [] =
"Copyright (c) 1985 by J & M Software, Dallas TX - All Rights Reserved";

char	 buffer [BUFSIZ];	/* Buffer for stdout */

char	swch = '-';		/* The switch character */

static void cantopen (char *);
void usage (void);
void help (void	);
void dprint (char **);

/* ----------------------------------------------------------------------- */
    void
main (argc, argv)
    int    argc;
    char  *argv [];

    {
    FILE  *teeout;
    char   record [512];
    char  *s;


    setbuf(stdout, buffer);
    swch = egetswch();

    while (--argc > 0 && (*++argv)[0] == swch)
	for (s = argv[0] + 1; *s; s++)
	    switch (tolower(*s))
		{
		case '?':
		    help();

		default:
		    usage();
		}

    if (argc == 0)
	usage();

    if ((teeout = fopen(*argv, "w")) == NULL)
	cantopen(*argv);

    while (fgets(record, sizeof record, stdin))
	{
	fputs(record, teeout);
	fputs(record, stdout);
	fflush(stdout);
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
	"Usage:  <command>  |  tee  [%c?]  out_file",
	"        <command>  |  tee  [%c?]  out_file  |  <command>",
	"        tee  %c?  for help",
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
	"Usage:  <command>  |  tee  [%c?]  out_file",
	"        <command>  |  tee  [%c?]  out_file  |  <command>",
	"",
	"tee (pipe fitting) diverts a copy of data to out_file",
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
	printf(*(dp++), swch);
	putchar('\n');
	}
    }

/* ----------------------------------------------------------------------- */
