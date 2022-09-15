/* ----------------------------------------------------------------------- *\
|
|			      Envelope Processor
|
|		    Copyright (c) 1990, all rights reserved
|				Brian W Johnson
|				   20-Oct-90
|				   13-Mar-93
|				   25-Nov-95
|				    1-Mar-09
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

"Usage:  env  [%c?l9]  [%cd name]  [%cr ra_name]  [%cn NNN]  [file_list]",
"",
"env prints envelopes, using either the default return address, or a specified",
"return address file, for each address file specified.",
"",
"    %cl        lists address file names as they are processed",

"    %cn <NNN>  specifies the number of copies",
"               (only if not using <stdin>)",
"    %cd <name> specifies the print device or filename",
"               (default PRN)",
"    %cr <name> specifies the return address filename",
"               (default d:\\bwjfiles\\envelope\\ra.adr)",
"    %c9        specifies a #9 envelope size",
"",
"Copyright (c) 1990 by Brian W Johnson, Lucas TX - All Rights Reserved",
NULL
};

/* ----------------------------------------------------------------------- */

#define  TRUE	1
#define  FALSE	0

/* ----------------------------------------------------------------------- */

int	l_flag      = FALSE;
	// List file names flag
int	n9_flag     = FALSE;	// Specify a number 9 size envelope
int	copies      = 1;	// The number of envelope copies

char	raname [81] =		// The return address file name
	"d:\\bwj\\envelope\\ra.adr";

char	device [81] = "prn";	// The default printer file name

FILE   *rafp;			// The return address FILE structure pointer
FILE   *prfp;			// The printer FILE structure pointer

/* ----------------------------------------------------------------------- */

extern	void	process    (FILE *fp, char *fnp);
extern	void	print_env  (FILE *fp);
extern	void	lj_vert    (void);
extern	void	lj_horiz   (void);
extern	void	lj_ravert  (void);
extern	void	lj_rahoriz (void);
extern	void	lj_prefix  (void);
extern	void	lj_suffix  (void);

/* ----------------------------------------------------------------------- */
	void
main (
	int    argc,			// Argument count
	char  *argv [])			// Argument list pointer

	{
	int    smode = FW_FILE;		// File search mode attributes
	int    option;			// Option character
	long   ltemp;			// Used by optvalue()
	char  *ap;				// Argument pointer
	char  *fnp = NULL;			// Input file name pointer
	FILE  *fp  = NULL;			// Input file descriptor
	void  *hp  = NULL;			// Pointer to the wild file data block


	optenv = getenv("ENV");

	while ((option = getopt(argc, argv, "9?d:D:lLn:N:r:R:")) != EOF)
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

			case '9':
				++n9_flag;
				break;

			case 'n':
				if (optvalue(optarg, &ltemp, 1, 256))
					{
					printf("Invalid copies parameter: %s\n", optarg);
					usage();
					}
				copies = (int)(ltemp);
				break;

			case 'r':
				if (optarg == NULL)
					usage();
				strncpy(&raname[0], optarg, 80);
				break;

			case '?':
				help();

			default:
				usage();
			}
		}


	if ( ! (rafp = fopen(&raname[0], "r")))
		{
		printf("Unable to open return address file: %s\n", &raname[0]);
		exit(1);
		}

	if ( ! (prfp = fopen(&device[0], "w")))
		{
		printf("Unable to access printer: %s\n", &device[0]);
		exit(1);
		}

	if (optind >= argc)
		{
		copies = 1;
		process(stdin, "<stdin>");
		}
	else
		{
		while (optind < argc)
			{
			ap = argv[optind++];
			hp = finit(ap, smode);		// Process the input list
			if ((fnp = fwild(hp)) == NULL)
				{
				hp = NULL;
				cantopen(ap);
				}
			else
				{
				do  {				// Process one filespec
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

    fflush(prfp);
    fclose(prfp);
    fclose(rafp);
    }

/* ----------------------------------------------------------------------- *\
|  process ()  -  Print "copies" envelope(s)
\* ----------------------------------------------------------------------- */
    void
process (			// Process one input file
    FILE  *fp,			// The FILE pointer
    char  *fnp)			// The file name

    {
    int    i;			// Copy counter


    if (l_flag)
	printf("Printing: %s\n", fnp);

    for (i = 0; i < copies; ++i)
	{
	fseek(rafp, 0L, SEEK_SET);	// Rewind the RA file
	fseek(  fp, 0L, SEEK_SET);	// Rewind the destination file
	lj_prefix();
	print_env(fp);
	lj_suffix();
	}
    }

/* ----------------------------------------------------------------------- *\
|  print_env ()  -  Print one envelope
\* ----------------------------------------------------------------------- */
    void
print_env (FILE *fp)			// The FILE pointer

    {
    char   buffer [1024];


    lj_ravert();			// Position to the vertical pos
    while (fgets(&buffer[0], (sizeof(buffer) - 2), rafp))
	{
	lj_rahoriz();			// Position to the horizontal pos
	fputs(&buffer[0], prfp);	// Print the return address line
	}

    lj_vert();				// Position to the vertical pos
    while (fgets(&buffer[0], (sizeof(buffer) - 2), fp))
	{
	lj_horiz();			// Position to the horizontal pos
	fputs(&buffer[0], prfp);	// Print the destination address line
	}
    }

/* ----------------------------------------------------------------------- *\
|  lj_vert ()  -  Set the vertical position for the destination address
\* ----------------------------------------------------------------------- */
    void
lj_vert (void)

    {
    fputs(ESC "&a1440V", prfp);	// Position to 2 inches from top margin
    }

/* ----------------------------------------------------------------------- *\
|  lj_horiz ()  -  Set the horizontal position for the destination address
\* ----------------------------------------------------------------------- */
    void
lj_horiz (void)

    {
    fputs(ESC "&a3600H", prfp);	// Position to 5 inches from left margin
    }

/* ----------------------------------------------------------------------- *\
|  lj_ravert ()  -  Set the vertical position for the return address
\* ----------------------------------------------------------------------- */
    void
lj_ravert (void)

    {
//  fputs(ESC "&a0V", prfp);	// Position to 0 inches from top margin
    }

/* ----------------------------------------------------------------------- *\
|  lj_rahoriz ()  -  Set the horizontal position for the return address
\* ----------------------------------------------------------------------- */
    void
lj_rahoriz (void)

    {
    if (n9_flag)
	fputs(ESC "&a72H", prfp);  // Position to 0.1 inches from left margin
    else
	fputs(ESC "&a288H", prfp); // Position to 0.4 inches from left margin
    }

/* ----------------------------------------------------------------------- *\
|  lj_prefix ()  -  Output HP LaserJet prefix
\* ----------------------------------------------------------------------- */
    void
lj_prefix (void)

    {
	// Set for COM10 envelopes, Manual envelope feed,
	// Landscape orientation, and 4.8 lpi

    if (n9_flag)
	fputs(ESC "&l90a3h1o10C", prfp);
    else
	fputs(ESC "&l81a3h1o10C", prfp);

	// Set the PC-8 character set

    fputs(ESC "(10U", prfp);

	// Set the universe font, 13 point

    fputs(ESC "(s1p13v0s0b4148T", prfp);
    }

/* ----------------------------------------------------------------------- *\
|  lj_suffix ()  -  Output HP LaserJet suffix
\* ----------------------------------------------------------------------- */
    void
lj_suffix (void)

    {
    fputs(ESC "E", prfp);	// Eject, reset the LaserJet
    }

/* ----------------------------------------------------------------------- */
