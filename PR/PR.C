/* ----------------------------------------------------------------------- *\
|
|				Print Processor
|
|		    Copyright (c) 1987, 1990, all rights reserved
|				Brian W Johnson
|				   11-Apr-92
|				   27-Mar-93
|				    9-Apr-93
|				    4-Aug-94
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <ctype.h>
#include  <limits.h>

#include  "fwild.h"

#define  ESC		"\033"
#define  BACKSPACE	'\010'

/* ----------------------------------------------------------------------- */

char   *usagedoc [] =
{
"Usage: pr [%c?abcglnopr]  [%cdh name]  [%cmnst NNN]  [file_list]",
"",
"pr prints files to the printer, optionally paginating and titling",
"",
"    %cb  paginate using a boxed title",
"    %cp  paginate using a simple title",
"    %co  do not paginate (default)",
"    %cg  uses \"|+-\" graphics characters for title boxes (default IBM)",
"    %ca  print to an ASCII-only printer          (default PCL printer)",
"",
"    %cc  compresses text for the LaserJet (130 col, 80 line)",
"    %cl  lists file names as they are processed",
"    %cr  resets (only) the LaserJet",
"",
"    %cd <name> specifies the print device or filename (default PRN)",
"    %ce <val>  specifies the ending page printed      (default inf)",
"    %cf <val>  specifies the first page printed       (default 1)",
"    %ch <name> specifies the page header name         (default <filename>)",
"    %ci <file> specifies the initialization file      (default none)",
"    %cn <NNN>  specifies NNN copies to be printed     (default 1)",
"    %cm <val>  specifies the left margin width        (default 0)",
"    %cs <val>  specifies the right margin width       (default 0)",
// "    %cS <eo>   specifies even/odd side printing       (default eo)",
"    %ct <val>  specifies the tab expansion width      (default 8)",

"",
"Copyright (c) 1990, 1993 by Brian W Johnson, Lucas TX - All Rights Reserved",
NULL
};

/* ----------------------------------------------------------------------- */

#define  TRUE		  1
#define  FALSE		  0

#define  N_COLS		 80	// Normal number of columns
#define  C_COLS		133	// Compressed number of lines
#define  N_LINES	 59	// Normal number of columns - 1
#define  C_LINES	 79	// Compressed number of lines - 1

#define  COMPRESSED	  0
#define  NORMAL		  1

#define  SIDENABLE()  ((page & 1) ? (odd_side) : (even_side))
#define  PRINTFLAG()  {printing = ((page >= firstpage) && (page <= endpage) && SIDENABLE());}
#define  PRINTCH(ch)  {if (printing) fputc((ch), prfp);}

/* ----------------------------------------------------------------------- */

int	b_flag     = FALSE;	// Box flag
int	c_flag     = FALSE;	// Compress flag
int	h_flag     = FALSE;	// Alternate header name flag
int	l_flag     = FALSE;	// List file names flag
int	p_flag     = FALSE;	// Paginate flag
int	r_flag     = FALSE;	// Reset LaserJet flag
int	pcl_flag   = TRUE;	// Issue PCL commands flag

int	title_flag = FALSE;	// TRUE if title needs to be printed
int	form_flag  = FALSE;	// TRUE if pre-FF is required before printing
int	tabwidth   = 8;		// The tab width
int	lmargin    = 0;		// The left margin
int	rmargin    = 0;		// The right margin
int	line       = 0;		// The current line number
int	maxline    = N_LINES;	// The maximum line number
int	col        = 0;		// The current column number
int	left_marg  = TRUE;	// TRUE when left margin processing required
int	copies     = 1;		// The number of copies
int	maxcol     = N_COLS;	// The maximum column number
int	textcol    = N_COLS;	// The total number of text columns
int	page       = 0;		// The current page number
int	firstpage  = 0;		// The first page to print
int	endpage    = 32767;	// The ending page to print
int	barcount   = 0;		// The horizontal bar count (pagination)
int	spccount   = 0;		// The horizontal space count (pagination)
int	printing   = FALSE;	// TRUE when printing enabled
int	odd_side   = TRUE;	// TRUE when odd  side printing enabled
int	even_side  = TRUE;	// TRUE when even side printing enabled

char	device [81] = "prn";	// The default printer file name
char	hdrname [81];		// The alternate header name
FILE   *prfp       = NULL;	// The printer FILE structure pointer
void   *hp         = NULL;	// Pointer to the wild file data block
char   *ifilename  = NULL;	// The initialization file name

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

extern	void	process    (FILE *, char *);
extern	void	print_file (FILE *fp, char *fnp, char *titlep);
extern	void	margin     (void);
extern	void	ff         (void);
extern	void	put_title  (char *titlep);
extern	void	lj_set     (int flag);
extern	void	lj_reset   (void);
extern	void	copyinit   (void);
extern	void	sides      (char *s);

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
		"?aAbBcCd:D:e:E:f:F:gGh:H:i:I:lLm:M:n:N:oOpPrRs:S:t:T:")) != EOF)
	{
	switch (tolower(option))
	    {
	    case 'a':
		c_flag   = FALSE;	// Compress flag
		r_flag   = FALSE;	// Reset LaserJet flag
		pcl_flag = FALSE;	// PCL flag
		break;

	    case 'b':
		p_flag = TRUE;
		b_flag = TRUE;
		break;

	    case 'c':
		++c_flag;
		break;

	    case 'd':
		if (optarg == NULL)
		    {
		    printf("Missing device name\n");
		    usage();
		    }
		strncpy(&device[0], optarg, 80);
		break;

	    case 'e':
		if (optvalue(optarg, &ltemp, 0L, (long)(unsigned int)(INT_MAX)))
		    {
		    printf("Invalid ending page number: %s\n", optarg);
		    usage();
		    }
		endpage = (int)(ltemp);
		break;

	    case 'f':
		if (optvalue(optarg, &ltemp, 0L, (long)(unsigned int)(INT_MAX)))
		    {
		    printf("Invalid starting page number: %s\n", optarg);
		    usage();
		    }
		firstpage = (int)(ltemp);
		break;

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
		h_flag = TRUE;
		break;

	    case 'i':
		ifilename = optarg;
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
		p_flag = FALSE;
		b_flag = FALSE;
		break;

	    case 'p':
		p_flag = TRUE;
		b_flag = FALSE;
		break;

	    case 'r':
		++r_flag;
		break;

	    case 's':
		if (option == 'S')
		    sides(optarg);
		else
		    {
		    if (optvalue(optarg, &ltemp, 0L, 256L))
			{
			printf("Invalid right margin spec: %s\n", optarg);
			usage();
			}
		    rmargin = (int)(ltemp);
		    }
		break;

	    case 't':
		if (optvalue(optarg, &ltemp, 1L, 16L))
		    {
		    printf("Invalid tab size spec: %s\n", optarg);
		    usage();
		    }
		tabwidth = (int)(ltemp);
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

    if (c_flag)
	{
	maxcol  = C_COLS;
	maxline = C_LINES;
	lj_set(COMPRESSED);
	}
    else
	{
	maxcol  = N_COLS;
	maxline = N_LINES;
	lj_set(NORMAL);
	}

    textcol  = maxcol - lmargin - rmargin - 1;
    barcount = textcol - 2;
    spccount = textcol - 16;

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

done:
    if (form_flag)
	ff();

    lj_reset();

    fflush(prfp);
    fclose(prfp);
    }

/* ----------------------------------------------------------------------- *\
|  Process one file
\* ----------------------------------------------------------------------- */
    static void
process (			// Process one input file
    FILE  *fp,			// Input file descriptor
    char  *fnp)			// Input file name

    {
    int   n;			// Copy counter
    char  title [255];		// Title string


    if (h_flag)				// Build the title line
	strcpy(&title[0], &hdrname[0]);
    else
	strcpy(&title[0], fnp);
    strcat(&title[0], "    ");
    strcat(&title[0], fwdate(hp));
    strcat(&title[0], "  ");
    strcat(&title[0], fwtime(hp));

    for (n = 0; n < copies; ++n)
	{
	fseek(fp, 0L, SEEK_SET);	// Rewind the print file
	print_file(fp, fnp, &title[0]);	// Print the file
	}
    }

/* ----------------------------------------------------------------------- *\
|  Print one file
\* ----------------------------------------------------------------------- */
    void
print_file (
    FILE  *fp,			// The FILE pointer
    char  *fnp,			// The file name
    char  *titlep)		// The title string

    {
    int    ch;
    char  *p;
    char   buffer [1024];


    if (l_flag)
	printf("Printing: %s\n", fnp);

    line = 0;
    page = (form_flag) ? (0) : (1);
    PRINTFLAG();
    title_flag = p_flag;

    while (fgets(&buffer[0], sizeof(buffer), fp))
	{
	p         = &buffer[0];
	col       = 0;
	left_marg = TRUE;

	while ((ch = *(p++)) != '\0')
	    {
	    if (ch == '\f')
		{
		form_flag = TRUE;
		continue;	/* Avoid double FF at end */
		}

	    /* At this point, we have something to print */

	    if (p_flag  &&  (line >= maxline))
//	    if (line >= maxline)
		form_flag = TRUE;

	    if (form_flag)
		ff();

	    if (title_flag)
		put_title(titlep);

	    if (ch == '\n')
		{
// printf("Line %d, Page %d, flag = %d\n", line, page, printing);
// fflush(stdout);
		PRINTCH(ch);
		col       = 0;
		left_marg = TRUE;
		++line;
		continue;
		}

	    if (ch == '\r')
		{
		PRINTCH(ch);
		col       = 0;
		left_marg = TRUE;
		continue;
		}

	    if (ch == BACKSPACE)
		{
		PRINTCH(ch);
		if (col > 0)
		    --col;
		continue;
		}

	    if (left_marg  &&  (lmargin != 0))
		margin();

	    if (ch == '\t')
		{
		do  {
		    if (col >= textcol)
			{
			PRINTCH('\n');
			col       = 0;
			left_marg = TRUE;
			++line;
			--p;
			break;
			}
		    else
			PRINTCH(' ');
		    }  while ((++col % tabwidth) != 0);
		}
	    else if (col >= textcol)
		{
		PRINTCH('\n');
		col       = 0;
		left_marg = TRUE;
		++line;
		--p;
		continue;
		}
	    else
		{
		PRINTCH(ch);
		++col;
		}
	    }
	}
    form_flag = TRUE;			// Terminal form feed
    }

/* ----------------------------------------------------------------------- *\
|  Print the left margin
\* ----------------------------------------------------------------------- */
    void
margin (void)

    {
    int  i;

    for (i = lmargin; i; --i)
	PRINTCH(' ');
    left_marg = FALSE;
    }

/* ----------------------------------------------------------------------- *\
|  Skip the printer to a new page
\* ----------------------------------------------------------------------- */
    void
ff (void)

    {
    PRINTCH('\f');
    PRINTCH('\r');
    fflush(prfp);
    col       = 0;
    left_marg = TRUE;
    line      = 0;
    ++page;
    PRINTFLAG();
    title_flag = p_flag;
    form_flag  = FALSE;
// printf("Page %d, flag = %d\n", page, printing);
// fflush(stdout);
    }

/* ----------------------------------------------------------------------- *\
|  Print the title block on a new page
\* ----------------------------------------------------------------------- */
    void
put_title (char *titlep)	// Pointer to the title string

    {
    int  col;			// Column counter

    if (p_flag)
	{
	if (b_flag)
	    {
	    if (margin != 0)
		margin();
	    PRINTCH(UL_CORNER);
	    for (col = barcount; col; --col)
		PRINTCH(HOR_LINE);
	    PRINTCH(UR_CORNER);
	    PRINTCH('\n');
	    ++line;
	    }

	if (lmargin != 0)
	    margin();

	if (b_flag)
	    PRINTCH(VER_LINE);

	if (printing)
	    fprintf(prfp, "  %-*s Page %-3d   ", spccount, titlep, page);

	if (b_flag)
	    PRINTCH(VER_LINE);

	PRINTCH('\n');
	++line;

	if (b_flag)
	    {
	    if (lmargin != 0)
		margin();
	    PRINTCH(LL_CORNER);
	    for (col = barcount; col; --col)
		PRINTCH(HOR_LINE);
	    PRINTCH(LR_CORNER);
	    PRINTCH('\n');
	    ++line;
	    }

	PRINTCH('\n');
	++line;
	left_marg = TRUE;
	}

    title_flag = FALSE;
    }

/* ----------------------------------------------------------------------- *\
|  Perform HP LaserJet Setup
\* ----------------------------------------------------------------------- */
    void
lj_set (int flag)

    {
    lj_reset();					// Reset the LaserJet

    if (pcl_flag)
	{
	if (flag == COMPRESSED)
	    {
	    fputs(ESC "(10U",  prfp);		// Set compressed LP, PC-8
	    fputs(ESC "(s0p16.67h8.5v0s0b0T",  prfp);
	    fputs(ESC "&l8D",  prfp);		// Set 8 lines per inch
	    }
	else  // (flag == NORMAL)
	    {
	    fputs(ESC "(10U",  prfp);		// Set normal Courier, PC-8
	    fputs(ESC "(s0p10.00h12.0v0s0b3T",  prfp);
	    fputs(ESC "&l6D",  prfp);		// Set 6 lines per inch
	    }
	fputs(ESC "&k0G",  prfp);		// Disable CR, LF, FF translation
	}

    if (ifilename != NULL)
	copyinit();
    }

/* ----------------------------------------------------------------------- *\
|  Perform HP LaserJet Reset
\* ----------------------------------------------------------------------- */
    void
lj_reset (void)

    {
    if (pcl_flag)
	fputs(ESC "E",  prfp);			// Reset the LaserJet
    }

/* ----------------------------------------------------------------------- *\
|  Process "sides" argument
\* ----------------------------------------------------------------------- */
    void
sides (
    char  *arg)			// Pointer to the option string

    {
    char  *p = arg;

    odd_side  = FALSE;
    even_side = FALSE;
    while (*p)
	{
	if (tolower(*p) == 'e')
	    even_side = TRUE;
	else if (tolower(*p) == 'o')
	    odd_side  = TRUE;
	else
	    {
	    printf("Unexpected \"sides\" arguments: %s\n", arg);
	    usage();
	    }
	++p;
	}
    }

/* ----------------------------------------------------------------------- *\
|  Copy any optional initialization file to the printer
\* ----------------------------------------------------------------------- */
    void
copyinit (void)

    {
    int    ch;
    char  *ifnp;
    FILE  *fp;


    if ((ifnp = fwfirst(ifilename)) == NULL)
	{
	printf("Unable to find initialization file: %s\n", ifilename);
	usage();
	}

    if ((fp = fopen(ifnp, "r")) == NULL)
	{
	printf("Unable to open initialization file: %s\n", ifnp);
	usage();
	}

    while ((ch = fgetc(fp)) != EOF)
	fputc((ch), prfp);

    fclose(fp);
    }

/* ----------------------------------------------------------------------- */
