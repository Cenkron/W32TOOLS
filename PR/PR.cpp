/* ----------------------------------------------------------------------- *\
|
|						Line Print Processor
|
|		    Copyright (c) 1987, 1990, all rights reserved
|						Brian W Johnson
|						11-Apr-92
|						27-Mar-93
|						 9-Apr-93
|						 4-Aug-94
|						13-Sep-21
|
\* ----------------------------------------------------------------------- */

#define  WINVER  0x0A00

#include  "afx.h"
#include  <wingdi.h>
#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <ctype.h>
#include  <limits.h>

#include  "fwild.h"

#include  "GMLineprint.h"
#include  "LPRC.h"


/* ----------------------------------------------------------------------- */

char   *usagedoc [] =
{
"Usage: pr [%c?abcglnopr]  [%cdh name]  [%cmnst NNN]  [file_list]",
"",
"pr prints files to the printer, with options",
"",
// BWJ Maybe later
//"    %cb        specifies using title footer           (default none)",

"    %cd        specifies double-sided printing        (default single)",
"    %cf <val>  specifies the font CPI (10, 12, 15)    (default 10)",

// BWJ Maybe later
//"    %ch <name> specifies the page header name         (default <filename>)",

"    %cl        lists file names as they are processed",
"    %cm <val>  specifies the left margin col width    (default 0)",
"    %cn <NNN>  specifies NNN copies to be printed     (default 1)",
"    %co        specifies landscape orientation        (default portrait)",
"    %cr <val>  specifies the right margin col width   (default 0)",
"    %ct <val>  specifies the tab expansion width      (default 4)",
"    %cw        specifies enable overlong line Wrap    (default no)",
"",
"Copyright (c) 2021 by Brian W Johnson, Lucas TX - All Rights Reserved",
NULL
};

/* ----------------------------------------------------------------------- */

#define  BACKSPACE	'\010'

/* ----------------------------------------------------------------------- */

// BWJ check all
int			d_flag		= false;		// Double-sided flag (iff true)
FONT_CPI	font		= FONT_10;		// Font width

// BWJ Maybe later
//int			hdr_flag	= false;		// Alternate header name flag

int			l_margin	= 0;			// The left margin
int			list_flag	= false;		// List file names flag
int			MaxCol		= 0;			// The maximum column number (set while running)
int			n_copies	= 1;			// The number of copies printed
int			r_margin	= 0;			// The right margin
ORIENT		o_flag		= PORTRAIT;		// Landscape mode flag
int			tabwidth	= 4;			// The tab width
int			title_flag	= false;		// true if title needs to be printed
int			wrap_flag	= false;		// true when line wrapping enabled

// BWJ Maybe later
//char	hdrname [81];					// The alternate header name

char   *hp = NULL;						// Pointer to the wild file data block

/* ----------------------------------------------------------------------- */

extern	void	process   		(FILE *, char *);
extern	void	print_one_file	(FILE *fp, char *fnp);
extern	void	CalculateMargins (void);

/* ----------------------------------------------------------------------- */
	void
main (
	int    argc,				// Argument count
	char  *argv [])				// Argument list pointer

	{
	int    smode = FW_FILE;		// File search mode attributes
	int    option;				// Option character
	long   ltemp;				// Used for optvalue()
	char  *ap;					// Argument pointer
	char  *fnp = NULL;			// Input file name pointer
	FILE  *fp  = NULL;			// Input file descriptor


	optenv = getenv("PR");

	while ((option = getopt(argc, argv,
			"?bBdDf:F:h:H:lLm:M:n:N:oOr:R:t:T:wW")) != EOF)
		{
		switch (tolower(option))
			{
// BWJ Maybe later
//			case 'b':
//				title_flag = true;
//				break;

			case 'd':
				++d_flag;
				break;

			case 'f':
				optvalue(optarg, &ltemp, 10L, 15L);
				switch (ltemp)
					{
					case 10:
						font = FONT_10;
						break;
					case 12:
						font = FONT_12;
						break;
					case 15:
						font = FONT_15;
						break;
					default:
						printf("Invalid font width\n");
						usage();
					}
				break;

// BWJ Maybe later
//			case 'h':
//				if (optarg == NULL)
//					{
//					printf("Missing header name\n");
//					usage();
//					}
//				strncpy(&hdrname[0], optarg, 80);
//				hdr_flag = true;
//				break;

			case 'l':
				++list_flag;
				break;

			case 'm':
				if (optvalue(optarg, &ltemp, 0L, 256L))
					{
					printf("Invalid left margin spec: %s\n", optarg);
					usage();
					}
				l_margin = (int)(ltemp);
				break;

			case 'n':
				if (optvalue(optarg, &ltemp, 0L, 256L))
					{
					printf("Invalid copies spec: %s\n", optarg);
					usage();
					}
				n_copies = (int)(ltemp);
				break;

			case 'o':
				o_flag = LANDSCAPE;
				break;

			case 'r':
				if (optvalue(optarg, &ltemp, 0L, 256L))
					{
					printf("Invalid right margin spec: %s\n", optarg);
					usage();
					}
				r_margin = (int)(ltemp);
				break;

			case 't':
				if (optvalue(optarg, &ltemp, 1L, 16L))
					{
					printf("Invalid tab size spec: %s\n", optarg);
					usage();
					}
				tabwidth = (int)(ltemp);
				break;

			case 'w':
				++wrap_flag;
				break;

			case '?':
				help();

			default:
				usage();
			}
		}

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
						process(fp, fnp);
						fclose(fp);
						}
					else
						cantopen(fnp);
					} while ((fnp = fwild(hp)));
				}
			}
		}
	}

/* ----------------------------------------------------------------------- *\
|  Process one file
\* ----------------------------------------------------------------------- */
	static void
process (				// Process one input file
	FILE  *fp,			// Input file descriptor
	char  *fnp)			// Input file name

	{
	int   n;			// Copy counter

// BWJ Maybe later
#if 0
	char  title [255];	// Title string

	if (hdr_flag)			// Build the title line
		strcpy(&title[0], &hdrname[0]);
	else
		strcpy(&title[0], fnp);
#if 0
	strcat(&title[0], "    ");
	strcat(&title[0], fwdate(hp));
	strcat(&title[0], "  ");
	strcat(&title[0], fwtime(hp));
#endif
#endif

	if ( ! LprGetReady())
		{
		printf("Unable to access printer\n");
		exit(1);
		}

	// Printer Init was successful

	double punchMargin;
	double margin = ((double)(l_margin));

	switch (font)
		{
		case FONT_10: 
			punchMargin = (margin * (1.0 / 10.0));		break;
		case FONT_12: 
			punchMargin = (margin * (1.0 / 12.0));		break;
		case FONT_15: 
			punchMargin = (margin * (1.0 / 15.0));		break;
		}

	LprSetPunchMargin(punchMargin);

// BWJ Maybe later
	LprSetWriteHeaders(false);

	LprSetPageBreaks(true);

// BWJ Maybe later
//	if (title_flag)
//		LprSetTitle(title);

	LprSetOrient(o_flag ? LANDSCAPE : PORTRAIT);

	LprSetFont(font);

	if (o_flag == PORTRAIT)
		LprSetDuplex(d_flag ? DOUBLE_SHORT : SINGLE);
	else // (o_flag == LANDSCAPE)        ...
		LprSetDuplex(d_flag ? DOUBLE_LONG : SINGLE);

	CalculateMargins();

	// Do the printing

	for (n = 0; n < n_copies; ++n)
		{
		fseek(fp, 0L, SEEK_SET);	// Rewind the print file
		print_one_file(fp, fnp);	// Print the file
		}
	}

/* ----------------------------------------------------------------------- *\
|  Print one file
\* ----------------------------------------------------------------------- */
	void
print_one_file (
	FILE  *fp,			// The FILE pointer
	char  *fnp)			// The file name

	{
	bool	ValidLine;
	int		InCol;
	int		OutCol;
	char	ch;
	char	InBuff [1024];
	char	OutBuff [1024];

#define LeftMarginTest  (OutCol == 0)
#define RightMarginTest (OutCol >= MaxCol)

	LprErase(RS_ALL);

	if (list_flag)
		printf("Printing: %s\n", fnp);

	while (fgets(&InBuff[0], sizeof(InBuff), fp))
		{
		InCol		= 0;
		OutCol		= 0;
		ValidLine	= true;

//		printf("Line: \"%s\"\n", InBuff);

		while ((ch = InBuff[InCol++]) != '\0')
			{
			if (ch == '\f')				// Form feed
				{
				if ( ! LeftMarginTest)				// if OutBuff is not empty,
					{
					OutBuff[OutCol] = '\0';		// Terminate OutBuff, and
					LprWrite(LT_BODY, OutBuff);	// flush OutBuff to the printer, and.
					OutCol = 0;					// reset OutBuff
					}

				LprWrite(LT_NEWPAGE, 0);		// then, write the form feed to the printer,

				ch = InBuff[InCol++];			// and possibly continue with InBuff as if a new line.
				if ((ch == '\0')				// if InBuff is now exhausted,
				||  (ch == '\n')				// or has reached EOL,
				||  (ch == '\r'))				// then begin processing the next line
					{
					ValidLine = false;			// Treat EOL immediately after FF as not a line.
					goto nextline;				// without writing an empty line.
					}
				}								// Else, continue processing this line


			if ((ch == '\n')			// EOL
			||  (ch == '\r'))
				{
				goto nextline;
				}


			if (ch == BACKSPACE)		// Backspace
				{
				if (! LeftMarginTest)
					OutBuff[OutCol--] = '\0';
				goto nextchar;
				}


			if (ch == '\t')				// Tab
				{
				do  {
					if (RightMarginTest)
						{
						OutBuff[OutCol] = '\0';		// Terminate OutBuff
						if (wrap_flag)
							{
							LprWrite(LT_BODY, OutBuff);	// flush OutBuff to the printer, and.
							OutCol = 0;					// reset OutBuff
							}
						else
							{
							goto nextline;
							}
						}
					else
						{
						OutBuff[OutCol++] = ' ';
						}
					}  while ((OutCol % tabwidth) != 0);
				goto nextchar;
				}


			if (isprint(ch))			// Printable character
				{
				OutBuff[OutCol++] = ch;
				ValidLine = true;
				}

nextchar:
			if (RightMarginTest)			// Check for line overflow
				{
				OutBuff[OutCol] = '\0';		// Terminate OutBuff
				if (wrap_flag)
					{
					LprWrite(LT_BODY, OutBuff);	// flush OutBuff to the printer, and.
					OutCol = 0;					// reset OutBuff
					}
				else
					{
					goto nextline;
					}
				}
			} // end of process chars of this input line loop

nextline:
		if (ValidLine)
			{
			OutBuff[OutCol] = '\0';
			LprWrite(LT_BODY, OutBuff);
			}

		} // end of process lines loop

	LprPrint(DEFAULT);
	}

/* ----------------------------------------------------------------------- *\
|  Calculate the margins based on font, orientation, left margin, and right margin
\* ----------------------------------------------------------------------- */
	void
CalculateMargins ()

	{
	int cols;

	// Calculate the maximum possible character count

	if (o_flag == PORTRAIT)
		{
		switch (font)
			{
			case FONT_10:	cols =  83;		break;
			case FONT_12:	cols = 100;		break;
			case FONT_15:	cols = 117;		break;
			}
		}
	else // (o_flag == LANDSCAPE)
		{
		switch(font)
			{
			case FONT_10:	cols =	113;	break;
			case FONT_12:	cols =	132;	break;
			case FONT_15:	cols =	154;	break;
			}
		}

	MaxCol = cols - l_margin - r_margin;
	}

// -------------------------------------------------------------------
//								EOF
// -------------------------------------------------------------------
