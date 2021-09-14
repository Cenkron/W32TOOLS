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

//#undef _DLL

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

// BWJ
#define  ESC		"\033"
#define  BACKSPACE	'\010'

/* ----------------------------------------------------------------------- */

char   *usagedoc [] =
{
"Usage: pr [%c?abcglnopr]  [%cdh name]  [%cmnst NNN]  [file_list]",
"",
"pr prints files to the printer, with options",
"",
// BWJ
"    %cb  paginate using a boxed title",
// BWJ
"    %cp  paginate using a simple title",
// BWJ
"    %co  do not paginate (default)",
"    %cg  uses \"|+-\" graphics characters for title boxes (default IBM)",
"    %ca  print to an ASCII-only printer          (default PCL printer)",
"",
// BWJ
"    %cc  compresses text for the LaserJet (130 col, 80 line)",
"",
"    %cd        specifies double-sided printing        (default single)",
"    %cf <val>  specifies the font width (10, 12, 15)  (default 10)",
// BWJ
"    %ch <name> specifies the page header name         (default <filename>)",
"    %cl        lists file names as they are processed",
"    %cm <val>  specifies the left margin width        (default 0)",
"    %cn <NNN>  specifies NNN copies to be printed     (default 1)",
"    %co        specifies landscape orientation        (default portrait)",
"    %cs <val>  specifies the right margin width       (default 0)",
"    %ct <val>  specifies the tab expansion width      (default 4)",
"    %cw        specifies enable overlong line Wrap    (default no)",
"",
"Copyright (c) 1990, 1993 by Brian W Johnson, Lucas TX - All Rights Reserved",
NULL
};

/* ----------------------------------------------------------------------- */

#define  true		  1
#define  false		  0

#define  N_COLS		 80	// Normal number of columns
#define  C_COLS		133	// Compressed number of lines
#define  N_LINES	 59	// Normal number of columns - 1
#define  C_LINES	 79	// Compressed number of lines - 1

#define  COMPRESSED	  0
#define  NORMAL		  1


/* ----------------------------------------------------------------------- */

// BWJ check all
int			b_flag     = false;		// Box flag
int			c_flag     = false;		// Compress flag
int			d_flag     = false;		// Double-sided flag (iff true)
int			h_flag     = false;		// Alternate header name flag
FONT_CPI	f_font     = FONT_10;	// Font width
int			l_flag     = false;		// List file names flag
ORIENT		o_flag     = PORTRAIT;	// Landscape mode flag
int			p_flag     = false;		// Paginate flag

int			title_flag = false;		// true if title needs to be printed
// BWJ
int			form_flag  = false;		// true if pre-FF is required before printing
int			tabwidth   = 4;			// The tab width
int			lmargin    = 0;			// The left margin
int			rmargin    = 0;			// The right margin
int			line       = 0;			// The current line number
// BWJ
int			maxline    = N_LINES;	// The maximum line number
int			col        = 0;			// The current column number
int			left_marg  = true;		// true when left margin processing required
int			copies     = 1;			// The number of copies
int			MaxCol     = 0;			// The maximum column number (set while running)
int			wrap_flag  = false;		// true when line wrapping enabled

// BWJ
char	hdrname [81];		// The alternate header name
char   *hp         = NULL;	// Pointer to the wild file data block

/* ----------------------------------------------------------------------- *\
|  Box character definitions
\* ----------------------------------------------------------------------- */

#define  IBM_VER_LINE	(char)(179)	// IBM graphics
#define  IBM_UR_CORNER	(char)(191)
#define  IBM_LL_CORNER	(char)(192)
#define  IBM_HOR_LINE	(char)(196)
#define  IBM_LR_CORNER	(char)(217)
#define  IBM_UL_CORNER	(char)(218)

#define  CHR_VER_LINE	'|'		// Text graphics
#define  CHR_UR_CORNER	'+'
#define  CHR_LL_CORNER	'+'
#define  CHR_HOR_LINE	'-'
#define  CHR_LR_CORNER	'+'
#define  CHR_UL_CORNER	'+'

typedef
	struct
		{
		char  v;
		char  ur;
		char  ll;
		char  h;
		char  lr;
		char  ul;
		}  BOXC;

BOXC	boxchar [2] =
    {
		{
		IBM_VER_LINE,	IBM_UR_CORNER,	IBM_LL_CORNER,
		IBM_HOR_LINE,	IBM_LR_CORNER,	IBM_UL_CORNER
		},
		{
		CHR_VER_LINE,	CHR_UR_CORNER,	CHR_LL_CORNER,
		CHR_HOR_LINE,	CHR_LR_CORNER,	CHR_UL_CORNER
		}
    };

BOXC   *boxp = &boxchar[0];		// Pointer to the box set

#define  VER_LINE	boxp->v
#define  UR_CORNER	boxp->ur
#define  LL_CORNER	boxp->ll
#define  HOR_LINE	boxp->h
#define  LR_CORNER	boxp->lr
#define  UL_CORNER	boxp->ul

/* ----------------------------------------------------------------------- */

extern	void	process   		(FILE *, char *);
extern	void	print_one_file	(FILE *fp, char *fnp);
extern	void	CalculateMargins (void);

/* ----------------------------------------------------------------------- */
	void
main (
	int    argc,			// Argument count
	char  *argv [])			// Argument list pointer

	{
	int    smode = FW_FILE;		// File search mode attributes
	int    option;			// Option character
	long   ltemp;			// Used for optvalue()
	char  *ap;				// Argument pointer
	char  *fnp = NULL;			// Input file name pointer
	FILE  *fp  = NULL;			// Input file descriptor


	optenv = getenv("PR");

	while ((option = getopt(argc, argv,
			"?aAbBcCdDf:F:gGh:H:lLm:M:n:N:oOpPt:T:vVwW")) != EOF)
		{
		switch (tolower(option))
			{
			case 'a':
				c_flag   = false;	// Compress flag
				break;

			case 'b':
				p_flag = true;
				b_flag = true;
				title_flag = true;
				break;

			case 'c':
				++c_flag;
				break;

			case 'd':
				++d_flag;
				break;

			case 'f':
				optvalue(optarg, &ltemp, 10L, 15L);
				switch (ltemp)
					{
					case 10:
						f_font = FONT_10;
						break;
					case 12:
						f_font = FONT_12;
						break;
					case 15:
						f_font = FONT_15;
						break;
					default:
						printf("Invalid font width\n");
						usage();
					}
				break;

// BWJ
			case 'g':
				boxp = &boxchar[1];
				break;

			case 'h':
				if (optarg == NULL)
					{
					printf("Missing header name\n");
					usage();
					}
				strncpy(&hdrname[0], optarg, 80);
				h_flag = true;
				break;

			case 'l':
				++l_flag;
				break;

			case 'm':
				if (optvalue(optarg, &ltemp, 0L, 256L))
					{
					printf("Invalid left margin spec: %s\n", optarg);
					usage();
					}
				lmargin = (int)(ltemp);
				break;

			case 'n':
				if (optvalue(optarg, &ltemp, 0L, 256L))
					{
					printf("Invalid copies spec: %s\n", optarg);
					usage();
					}
				copies = (int)(ltemp);
				break;

			case 'o':
				o_flag = LANDSCAPE;
				break;

			case 'p':
// BWJ
				p_flag = true;
				b_flag = false;
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

//done:;
#if 0 // BWJ
	if (form_flag)
		ff();
#endif // 0
#if 0
	lj_reset();
#endif

//	fflush(prfp);
//	fclose(prfp);
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
	char  title [255];	// Title string


	if (h_flag)			// Build the title line
		strcpy(&title[0], &hdrname[0]);
	else
		strcpy(&title[0], fnp);
#if 1 // BWJ
	strcat(&title[0], "    ");
	strcat(&title[0], fwdate(hp));
	strcat(&title[0], "  ");
	strcat(&title[0], fwtime(hp));
#endif


	if ( ! LprGetReady())
		{
		printf("Unable to access printer\n");
		exit(1);
		}

	// Printer Init was successful

// BWJ this needs work
// BWJ	LprSetPunchMargin(0.5);	// BWJ needs a definition set somewhere
	LprSetPunchMargin(0.0);

// BWJ this needs work
	LprSetWriteHeaders(false);

	LprSetPageBreaks(true);

	if (title_flag)
		LprSetTitle(title);

	LprSetOrient(o_flag ? LANDSCAPE : PORTRAIT);

	LprSetFont(f_font);

	if (o_flag == PORTRAIT)
		LprSetDuplex(d_flag ? DOUBLE_SHORT : SINGLE);
	else // (o_flag == LANDSCAPE)        ...
		LprSetDuplex(d_flag ? DOUBLE_LONG : SINGLE);

	CalculateMargins();

	// Do the printing

	for (n = 0; n < copies; ++n)
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

	if (l_flag)
		printf("Printing: %s\n", fnp);

	while (fgets(&InBuff[0], sizeof(InBuff), fp))
		{
		InCol		= 0;
		OutCol		= 0;
		ValidLine	= true;

//		printf("Line1: \"%s\"\n", InBuff);

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
		switch (f_font)
			{
			case FONT_10:	cols = 84;		break;
			case FONT_12:	cols = 100;		break;
			case FONT_15:	cols = 117;		break;
			}
		}
	else // (o_flag == LANDSCAPE)
		{
		switch(f_font)
			{
			case FONT_10:	cols =	113;	break;
			case FONT_12:	cols =	132;	break;
			case FONT_15:	cols =	154;	break;
			}
		}

// BWJ have not handled left margin settings

	MaxCol = cols - lmargin - rmargin;
	}

// -------------------------------------------------------------------
//								EOF
// -------------------------------------------------------------------
