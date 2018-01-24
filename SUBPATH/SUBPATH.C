/* ----------------------------------------------------------------------- *\
|
|			      PVCS Path Builder Program
|
|		  Copyright (c) 1988, 1996  all rights reserved
|				Brian W Johnson
|				   15-Aug-96
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <stdlib.h>
#include  <direct.h>
#include  <ctype.h>
#include  <string.h>

/* ----------------------------------------------------------------------- *\
|  Definitions
\* ----------------------------------------------------------------------- */

#define  SWCH '-'		/* The switch character */

#ifndef FALSE
#define FALSE	  0
#define TRUE	  1
#endif

/* ----------------------------------------------------------------------- *\
|  Function prototypes
\* ----------------------------------------------------------------------- */

void	usagei        (int);
void	dprint        (char **);

/* ----------------------------------------------------------------------- *\
|  Private variables
\* ----------------------------------------------------------------------- */

char	cwd [1024];		// The current working directory buffer

/* ----------------------------------------------------------------------- *\
|  main () - The main program
\* ----------------------------------------------------------------------- */
    void
main (
    int    argc,
    char  *argv [])

    {
    char  *subpath;		// Pointer to the current base path
    char  *rootpath;		// Pointer to the root path variable
    char  *prefix;		// Pointer to the prefix string
    int    count = 0;		// Subpathing count


    getcwd(&cwd[0], sizeof(cwd));
    subpath = &cwd[0];
    strupr(&cwd[0]);

    if (argc > 1)
	{
	if (*(argv[1]) == '-')
	    usagei(0);

	strupr(argv[1]);
	rootpath = getenv(argv[1]);
	if (rootpath != NULL)
	    strupr(rootpath);
	else
	    rootpath = argv[1];

#if 0
	printf("%s\n", cwd);
	printf("%s\n", argv[1]);
	printf("%s\n", rootpath);
#endif

#if 0
	if ((strlen(rootpath) >= 2)
	&&  (*(rootpath + 1) == ':')
	&&  (strlen(subpath) >= 2)
	&&  (*(subpath + 1) == ':')
	&&  (tolower(*rootpath) == tolower(*subpath)))
	    {
	    subpath  += 2;
	    rootpath += 2;
	    }
#endif

	while (*subpath == *rootpath)
	    {
	    if (*subpath == ':')
		count = 0;
	    else
		++count;
	    ++subpath;
	    ++rootpath;
	    }

	if ((count > 0)
	&& ((*subpath == '/')  ||  (*subpath == '\\')))
	    ++subpath;
	}

    if (argc > 2)
	{
	strupr(argv[2]);
	prefix = getenv(argv[2]);
	if (prefix != NULL)
	    strupr(prefix);
	else
	    prefix = argv[2];
	printf("%s", prefix);
	}

    printf("%s", subpath);

    exit(0);
    }

/* ----------------------------------------------------------------------- *\
|  usagei () - Display usage information and exit
\* ----------------------------------------------------------------------- */
    void
usagei (int value)		// Display help documentation

    {
    static char  *hdoc [] =
	{
	"usage:  subpath  [rootname]  [prefix]  >outfile]",
	"",
	"subpath outputs (to stdout) the current working directory, optionally",
	"modified by deleting the leading portion of the path that matches the",
	"rootname argument, and also optionally prefixed by the prefix argument.",
	"",
	"rootname is the optional (quoted if multi-word) root path,",
	"or the name of an environment variable containing the root path.",
	"",
	"prefix is the optional (quoted if multi-word) prefix string,",
	"or the name of an environment variable containing the prefix string.",
//	"",
//	copyright,
	NULL
	};

    dprint(hdoc);
    exit(value);
    }

/* ----------------------------------------------------------------------- *\
|  dprint () - Display usage data
\* ----------------------------------------------------------------------- */
    void
dprint (dp)			// Print documentation text
    char  **dp;			// Document array pointer

    {
    while (*dp)
	{
	printf(*(dp++));
	putchar('\n');
	}
    }

/* --------------------------------- EOF --------------------------------- */
