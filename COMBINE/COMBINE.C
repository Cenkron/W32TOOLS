/* ----------------------------------------------------------------------- *\
|
|			      File Combiner Program
|
|		  Copyright (c) 1988, 1995  all rights reserved
|				Brian W Johnson
|				    8-Dec-95
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <stdlib.h>
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
void	FixPaths      (void);
void	FileOpen      (void);
void	ProcessFile   (void);

/* ----------------------------------------------------------------------- *\
|  Private variables
\* ----------------------------------------------------------------------- */

int		l_flag       = 0;	// TRUE to list the file only
int		s_flag       = 0;	// TRUE to list the search paths
char	delimiter;			// The filename delimiter
char   *pfn;				// Pointer to the prospective file name
int		nesting      = 0;	// The nesting level

FILE   *fpArray [64];		// The filename array
FILE  **pfp = &fpArray[0];	// Stack pointer into the path array

char   *SearchList [64] = { "." };	// The null path
char  **pSearchEntry = &SearchList[1];	// Pointer to the next search path

char	buffer [16384];		// The file text line buffer

char	copyright [] =
"Copyright (c) 1988, 1995 by Brian W. Johnson, Dallas TX - All Rights Reserved";

/* ----------------------------------------------------------------------- *\
|  main () - The main program
\* ----------------------------------------------------------------------- */
	void
main (
	int    argc,
	char  *argv [])

	{
	char   ch;			// Parser temporary
	char  *s;			// Parser temporary
	char  *s1;			// Parser temporary


	while (--argc > 0 && (*++argv)[0] == SWCH)
		{
		for (s = argv[0] + 1; ch = *(s++); )
			{
			switch (tolower(ch))
				{
				case 'i':
					s1 = s;
					while (isgraph(*s))
						{
						*(pSearchEntry++) = s;
						while (isgraph(*s))
							{
							if (*s == ';')
								{
								*(s++) = '\0';
								break;
								}
							++s;
							}
						}
					if (s == s1)
						{
						fprintf(stderr, "Missing search path\n");
						usagei(1);
						}
					break;

				case 'l':
					++l_flag;
					break;

				case 's':
					++s_flag;
					break;

				default:
					if (ch != '?')
						fprintf(stderr, "Invalid switch '%c'\n", ch);
					usagei(ch != '?');
					break;
				}
			}
		}

	FixPaths();

	if (argc == 0)
		{
		fprintf(stderr, "A primary input file name is required\n");
		usagei(1);
		}
	else if (argc > 1)
		{
		fprintf(stderr, "Excess arguments on the command line\n");
		usagei(1);
		}

	if (s_flag)
		{
		char **p;

		if (&SearchList[0] == pSearchEntry)
			printf("\nNo search paths\n");
		else
			{
			printf("\nSearch paths:\n");
			for (p = &SearchList[0]; (p < pSearchEntry); ++p)
				printf("    %s\n", *p);
			}
		}

	else
		{
		pfn = *argv;
		delimiter = '\0';
		FileOpen();
		ProcessFile();
		}

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
	"usage:  combine  [-?ls] [-i<path>]  infile  >outfile]",
	"",
	"combine reads the input file, recursively expanding lines",
	"of the form:",
	"",
	"        \"$include <file>\"",
	"",
	"Expanded files are found using the current directory and any",
	"specified search paths.",
	"",
	"    -i<path>  provide a search path for the included file(s)",
	"    -l        list (only) the included path/filename(s)",
	"    -s        list (only) the search path(s)",
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
dprint (dp)				// Print documentation text
	char  **dp;			// Document array pointer

	{
	while (*dp)
		{
		printf(*(dp++));
		putchar('\n');
		}
	}

/* ----------------------------------------------------------------------- *\
|  FixPaths () - Delete any trailing "\" characters from the search paths
\* ----------------------------------------------------------------------- */
	void
FixPaths (void)

	{
	char  **p;			// Pointer to the current search path

	for (p = &SearchList[1]; (p < pSearchEntry); ++p)
		{
		int     n = strlen(*p);		// Length of the search path
		char  *px = *p + (n - 1);	// Pointer to the possible '\'

		if ((n > 0)  &&  (*px == '\\'))	// Check for it...
			*px = '\0';			// ...yes, truncate it
		}
	}

/* ----------------------------------------------------------------------- *\
|  FileOpen () - Attempt to open the file
\* ----------------------------------------------------------------------- */
	void
FileOpen (void)

	{
	int     a_flag;			// Absolute filename flag
	char  **p;				// Pointer into the search array
	FILE   *fp;				// The file pointer
	char    filename [1024];		// The filename buffer


	a_flag = ((*pfn == '\\')  ||  (*(pfn+1) == ':'));

	if (delimiter == '<')
		p = &SearchList[1];
	else
		p = &SearchList[0];

	for ( ; (p < pSearchEntry); ++p)
		{
		if (a_flag)
			strcpy(filename, pfn);	// Use the filename
		else
			{
			strcpy(filename, *p);	// Construct the filename
			strcat(filename, "\\");
			strcat(filename, pfn);
			}

// printf("Trying \"%s\"\n", filename);

		if ((fp = fopen(filename, "r")) != NULL)
			{
			*(++pfp) = fp;		// Stack the file entry
			if (l_flag)
				{
				int n = 2 * (++nesting);
				printf("    %d%*c%s\n", nesting, n, ' ', filename);
				}
			return;
			}

	if (a_flag)
		break;
	}

	fprintf(stderr, "Unable to open file \"%s\"\n", pfn);
	exit(1);
	}

/* ----------------------------------------------------------------------- *\
|  CheckInclude () - Check for an included file
\* ----------------------------------------------------------------------- */
	int
CheckInclude (void)

	{
	char  *p = buffer;		// Point the buffer

// printf("Checking \"%s\"\n", p);

	while (isspace(*p))		// Skip leading white space
		++p;

	if ((*p != '$')  ||  (strncmp(p, "$include", 8) != 0))
		return (0);		// Not a "$include" request

	while (isgraph(*p))		// Skip over the "$include"
		++p;

	while (isspace(*p))		// Skip intermediate white space
		++p;

	if ((*p == '"')  ||  (*p == '<'))
		{
		delimiter = *p;
		++p;
		}
	else
		delimiter = '\0';

	pfn = p;			// Point the filename portion

	while (isgraph(*p))		// Skip over the filename
		++p;

	if (pfn == p)
		{
		fprintf(stderr, "No file name in \"%s\"", buffer);
		exit(1);
		}

	if (delimiter != '\0')
		{
		--p;
		if (((delimiter == '"')  &&  (*p != '"'))
		||  ((delimiter == '<')  &&  (*p != '>')))
			{
			fprintf(stderr, "Delimiter pair error in \"%s\"", buffer);
			exit(1);
			}
		}

	*p = '\0';			// NUL terminate the filename

	return (1);
	}

/* ----------------------------------------------------------------------- *\
|  ProcessFile () - Process a file
\* ----------------------------------------------------------------------- */
	void
ProcessFile (void)

	{
	while (fgets(buffer, sizeof(buffer), *pfp) != NULL)
		{
		if (CheckInclude())
			{
			FileOpen();
			ProcessFile();
			--pfp;
			--nesting;
			}
		else if ( ! l_flag)
			fputs(buffer, stdout);
		}
	}

/* --------------------------------- EOF --------------------------------- */
