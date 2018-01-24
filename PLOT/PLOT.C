/* ----------------------------------------------------------------------- *\
|
|			  Sailplane Design LaserJet Plotter
|
|		    Copyright (c) 1987, 1990, all rights reserved
|				Brian W Johnson
|				   20-Oct-90
|				   13-May-91
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <ctype.h>

#include  "fwild.h"

#define  ESC	"\033"

/* ----------------------------------------------------------------------- */

char   *usagedoc [] =
{

"Usage:  plot  [%c?lr]  [%cd name]  [input_file_list]",
"",
"plot plots HPGL files to the LaserJet in HPGL mode.",
"",
"    %cl  lists file names as they are processed",
"    %cr  resets (only) the LaserJet",
"    %cd <name> specifies the print device or filename (default PRN)",

"",
"Copyright (c) 1990 by Brian W Johnson, Lucas TX - All Rights Reserved",
NULL
};

/* ----------------------------------------------------------------------- */

#define  TRUE	1
#define  FALSE	0

/* ----------------------------------------------------------------------- */

int	l_flag      = FALSE;	// List file names flag
int	r_flag      = FALSE;	// Reset LaserJet flag

int	set_flag    = TRUE;	// Set LaserJet flag

char	device [81] = "prn";	// The default printer file name
void   *hp          = NULL;	// Pointer to the wild file data block
FILE   *prfp        = NULL;	// The printer FILE structure pointer

/* ----------------------------------------------------------------------- *\
|  Function prototypes
\* ----------------------------------------------------------------------- */

extern	void	process    (FILE *, char *);
extern	void	lj_set     (void);
extern	void	lj_reset   (void);

extern	void	parse      (int ch);
extern	int	nextstate  (void);
extern	void	translate  (void);

/* ----------------------------------------------------------------------- */
    void
main (
    int    argc,			// Argument count
    char  *argv [])			// Argument list pointer

    {
    int    smode = FW_FILE;		// File search mode attributes
    int    option;			// Option character
    char  *ap;				// Argument pointer
    char  *fnp = NULL;			// Input file name pointer
    FILE  *fp  = NULL;			// Input file descriptor


    optenv = getenv("PLOT");

    while ((option = getopt(argc, argv, "?d:D:lLrR")) != EOF)
	{
	switch (tolower(option))
	    {
	    case 'd':
		if (optarg == NULL)
		    usage();
		strncpy(&device[0], optarg, 80);
		break;

	    case 'l':
		++l_flag;
		break;

	    case 'r':
		++r_flag;
		break;

	    case '?':
		help();

	    default:
		usage();
	    }
	}


    if ( ! (prfp = fopen(&device[0], "w")))
	{
	printf("Unable to access printer: %s\n", &device[0]);
	exit(1);
	}

    if (r_flag)
	goto done;

    if (optind >= argc)
	process(stdin, "<stdin>");
    else
	{
	while (optind < argc)
	    {
	    ap = argv[optind++];
	    hp = finit(ap, smode);		// Process the input list
	    if ((fnp = fwild(hp)) == NULL)
		cantopen(ap);
	    else
		{
		do  {				// Process one filespec
		    if (fp = fopen(fnp, "r"))
			{
			lj_set();
			set_flag = FALSE;
			process(fp, fnp);
			fclose(fp);
			}
		    else
			cantopen(fnp);
		    } while ((fnp = fwild(hp)));
		}
	    }
	}

done:
    lj_reset();

    fflush(prfp);
    fclose(prfp);
    }

/* ----------------------------------------------------------------------- *\
|  Perform HP LaserJet Setup
\* ----------------------------------------------------------------------- */
    void
lj_set (void)

    {
    lj_reset();			// Do reset

    fputs(ESC "&l1O", prfp);	// Set landscape mode
    fputs(ESC "%0B",  prfp);	// Set HPGL mode
    }

/* ----------------------------------------------------------------------- *\
|  Perform HP LaserJet Reset
\* ----------------------------------------------------------------------- */
    void
lj_reset (void)

    {
    fputs(ESC "E",     prfp);	// Reset LaserJet
    fputs(ESC "(10U",  prfp);	// Select IBM symbol set
    }

/* ----------------------------------------------------------------------- *\
|  Process one file
\* ----------------------------------------------------------------------- */
    static void
process (			// Process one input file
    FILE  *fp,			// Input file descriptor
    char  *fnp)			// Input file name

    {
    int   ch;			// Input character


    if (l_flag)
	printf("Plotting: %s\n", fnp);

    do  {
	ch = fgetc(fp);
	parse(ch);
	} while (ch != EOF);
    fputc(';', prfp);
    }

/* ----------------------------------------------------------------------- *\
|  Global translator data
\* ----------------------------------------------------------------------- */

#define  FIRST		0	// Process the first command character
#define  SECOND		1	// Process the second command character
#define  STRING		2	// Copy string operand
#define  COPY		3	// Copy numeric operands
#define  DATA		4	// Buffer numeric operands
#define  SKIP		5	// Skip numeric operands

int	state = FIRST;		// The state variable

char	cmd  [3];		// Command buffer
char   *cp         = &cmd[0];	// Command buffer pointer
char	data [81];		// Data buffer
char   *dp         = &data[0];	// Data buffer pointer

char	lblterm    = '\3';	// Label terminator character

/* ----------------------------------------------------------------------- *\
|  parse ()  -  Parse HPGL commands
\* ----------------------------------------------------------------------- */
    void
parse (int ch)

    {
    switch (state)
	{
	case FIRST:
	    if (ch == EOF)
		fputs(";\n", prfp);
	    else if (isalpha(ch))
		{
		cmd[0] = (char)(tolower(ch));
		state  = SECOND;
		}
	    else
		fputc((char)(ch), prfp);
	    break;

	case SECOND:
	    if (ch == EOF)
		fputs(";\n", prfp);
	    else
		{
		cmd[1] = (char)(tolower(ch));
		dp     = &data[0];
		state  = nextstate();
		}
	    break;

	case STRING:
	    if (ch == EOF)
		{
		fputc(lblterm, prfp);
		fputs(";\n", prfp);
		}
	    else
		{
		fputc(ch, prfp);
		if (ch == lblterm)
		    {
		    state = FIRST;
		    fputc('\n', prfp);
		    }
		}
	    break;

	case COPY:
	    if (ch == EOF)
		fputs(";\n", prfp);
	    else if (isalpha(ch))
		{
		cmd[0] = (char)(tolower(ch));
		state  = SECOND;
		}
	    else
		{
		fputc(ch, prfp);
		if (ch == ';')
		    state = FIRST;
		}
	    break;

	case DATA:
	    if (ch == EOF)
		{
		translate();
		fputs(";\n", prfp);
		}
	    else if (isalpha(ch))
		{
		translate();
		cmd[0] = (char)(tolower(ch));
		state  = SECOND;
		}
	    else if (ch == ';')
		{
		translate();
		state = FIRST;
		}
	    else
		*dp++ = (char)(ch);
	    break;

	case SKIP:
	    if (ch == EOF)
		fputs(";\n", prfp);
	    else if (isalpha(ch))
		{
		cmd[0] = (char)(tolower(ch));
		state  = SECOND;
		}
	    else if (ch == ';')
		state = FIRST;
	    break;
	}
    }

/* ----------------------------------------------------------------------- *\
|  nextstate ()  -  Test the command to determine the operand category
\* ----------------------------------------------------------------------- */
    int
nextstate (void)

    {
    int  result;


    if (strncmp(&cmd[0], "in", 2) == MATCH)
	{
	if (set_flag)
	    lj_set();
	set_flag = TRUE;
	fputs(&cmd[0], prfp);
	result = COPY;
	}
    else if (strncmp(&cmd[0], "lb", 2) == MATCH)
	{
	fputs(&cmd[0], prfp);
	result = STRING;
	}
    else if (strncmp(&cmd[0], "sp", 2) == MATCH)
	{
	fputs(&cmd[0], prfp);
	fputs("1;\n", prfp);
	result = SKIP;
	}
    else if (strncmp(&cmd[0], "vs", 2) == MATCH)
	result = SKIP;
    else
	{
	fputs(&cmd[0], prfp);
	result = COPY;
	}

    return (result);
    }

/* ----------------------------------------------------------------------- *\
|  translate ()  -  Translate an HPGL command
\* ----------------------------------------------------------------------- */
    void
translate (void)

    {
    }

/* ----------------------------------------------------------------------- */
