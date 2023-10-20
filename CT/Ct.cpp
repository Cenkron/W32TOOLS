/* ----------------------------------------------------------------------- *\
|
|				       CT
|
|				 Brian W Johnson
|		    Copyright (c) 1997; All rights reserved
|				   22-Aug-97
|				   15-Sep-97 (Coloration added)
|				   29-Sep-00 (White space between files)
|
\* ----------------------------------------------------------------------- */

#define  WINVER  0x0A00

#include  <afx.h>
#include  <string.h>
#include  <stdio.h>
#include  <stdlib.h>
#include  <ctype.h>
#include  <io.h>
#include  <signal.h>

#include  <cdiff.h>
#include  "fWild.h"

// #define  BUFFEREDOUT		// Define this to buffer stdout

/* ----------------------------------------------------------------------- */

char  *usagedoc [] =
{
"Usage:  ct  [%c?<nltw>] [T[ab]N]  file A  [file/path B] >output_file",
"",
"ct contrasts two files, listing the differences",
"If path A is wild, multiple file comparisons will be performed",
"If path B is a directory, the filename is taken from path A",
"If path B is missing, the current directory is assumed",
"Line numbers in common regions represent file A.",
"",
"ct returns an exit code as follows:",
"",
"     2  File not found, or parameter error",
"     1  Files do NOT match according to the options supplied",
"     0  Files DO match according to the options supplied",
"",
"    %ca        Lists only lines from file A       (toggles, default off)",
"    %cb        Lists only lines from file B       (toggles, default off)",
"    %cc        Include common code in the listing (toggles, default on )",
"    %ch        Highlight matched fields           (toggles, default on )",
"    %cn        Lists line numbers                 (toggles, default off)",
"    %cL        Shows regions as \"A\" and \"B\" rather than filenames",
"    %cl        Does not consider leading  white space for match purposes",
"    %cs NNN    Set the matched field text color   (default yellow)",
"    %ct        Does not consider trailing white space for match purposes",
"    %cw        Does not consider any      white space for match purposes",
"    %cT N      Sets the tab interpretation size for both files",
//??? "    %cTa N     Sets the tab interpretation size for file A",
//??? "    %cTb N     Sets the tab interpretation size for file B",
"    %cv        Requests verbose output",
"    %cq        Does not list, but still sets the exit code",
"    %cz        Always returns a zero exit code, regardless of match",
"    %c?        Produces this help information",
"",
"Copyright (c) 1997 by Brian Johnson, Lucas TX - All Rights Reserved",
NULL
};

/* ----------------------------------------------------------------------- */

#define   FW_NORM      (FW_FILE | FW_DSTAR | FW_SUBD | FW_HIDDEN | FW_SYSTEM)

/* ----------------------------------------------------------------------- */

BOOL	a_flag = FALSE;			// TRUE to list file A components
BOOL	b_flag = FALSE;			// TRUE to list file B components
BOOL	c_flag = FALSE;			// TRUE to list common code in both files
BOOL	h_flag = TRUE;			// TRUE to enable highlighting
BOOL	n_flag = FALSE;			// TRUE to list line numbers
BOOL	w_flag = FALSE;			// TRUE to exclude all      white space
BOOL	l_flag = FALSE;			// TRUE to exclude leading  white space
BOOL	t_flag = FALSE;			// TRUE to exclude trailing white space
BOOL	L_flag = TRUE;			// TRUE to list filenames instead of A, B
BOOL	v_flag = 0;				// Verbosity count
BOOL	q_flag = FALSE;			// TRUE to quietly set the exit code
BOOL	z_flag = FALSE;			// TRUE to always return a zero exit code
BOOL	tty_flag = FALSE;		// TRUE if stdout is a console

char   *fnpA   = NULL;			// Input file A name pointer
char   *fnpB   = NULL;			// Input file B name pointer

BOOL	Match  = FALSE;			// TRUE it the files match

int	exitcode = 0;				// Exit code - assume success

CDiff  Difference;				// The CDiff class instance

/* ----------------------------------------------------------------------- */

#define  SETEXIT(n)   {if (exitcode < (n)) exitcode = (n);}

extern	void	f_err    (char *);
extern	void	filepair (char *fnp1, char *fnp2);
extern	void	Contrast (void);
extern  void	List     (void);
extern	void	Prepare  (void);

extern	void	InitializeColors (void);	/* Color initialization system */
extern	void	SetHighlight (int Flag);	/* Set highlighted field text */
extern	void	SetHighlightColor (int Color);	/* Set highlighted field text */

/* ----------------------------------------------------------------------- *\
|  Ctrl-c and Ctrl-Break handler
\* ----------------------------------------------------------------------- */
	void
Terminated (
	int signal)

	{
	SetHighlight(FALSE);
	exit(255);
	}

/* ----------------------------------------------------------------------- */
	int
main (
	int    argc,			// Argument count
	char  *argv [])			// Argument list pointer

	{
	int    option;			// Option character
	long   ltemp;			// Used for optvalue()
	char  *fnp1 = NULL;		// Ptr to the first pattern
	char  *fnp2 = NULL;		// Ptr to the second pattern

static	char   *fargv [] = { "*" };	// Fake argv array
static	char   *optstring = "?aAbBcChHlLnNs:S:t:T:vVwWqQzZ";
//???    %cT  N     Sets the tab interpretation size for both files",
//???    %cTa N     Sets the tab interpretation size for file A",
//???    %cTb N     Sets the tab interpretation size for file B",


	tty_flag = isatty(fileno(stdout));
#ifdef  BUFFEREDOUT
	if ( ! tty_flag)
	setbuf(stdout, buffer);
#endif
	optenv = getenv("CT");

	while ((option = getopt(argc, argv, optstring)) != EOF)
		{
		switch (tolower(option))
			{
			case 'a':
			a_flag = ! a_flag;
			break;

			case 'b':
			b_flag = ! b_flag;
			break;

			case 'c':
			c_flag = ! c_flag;
			break;

			case 'h':
			h_flag = ! h_flag;
			break;

			case 'n':
			n_flag = ! n_flag;
			break;

			case 'l':
			if (option == 'l')
				Difference.SetIgnoreLeadingBlanks(l_flag = ! l_flag);
			else // (option == 'L')
				L_flag = ! L_flag;
			break;

			case 's':
			if (optvalue(optarg, &ltemp, 1, 255))
				{
				printf("Color parm error - %s\n", optvalerror());
				usage();
				}
			SetHighlightColor((int)(ltemp));
			break;

			case 't':
			if (option == 't')
				Difference.SetIgnoreTrailingBlanks(t_flag = ! t_flag);
			else if (optvalue(optarg, &ltemp, 1, 21))
				{
				printf("Tab size parm error - %s\n", optvalerror());
				usage();
				}
			else // (valid tab size)
				{
				Difference.SetTabSize(0, (int)(ltemp));
				Difference.SetTabSize(1, (int)(ltemp));
				}
			break;

			case 'v':
			++v_flag;
			break;

			case 'w':
			Difference.SetIgnoreAllBlanks(w_flag = ! w_flag);
			break;

			case 'q':
			q_flag = ! q_flag;
			break;

			case 'z':
			z_flag = TRUE;
			break;

			case '?':
			help();

			default:
			usage();
			}
		}


	if (tty_flag)
		{
		InitializeColors();
		signal(SIGINT, Terminated);
		signal(SIGBREAK, Terminated);
		}

	if (a_flag  &&  b_flag)
	q_flag = TRUE;		// Can't list only both

	if (optind < argc)
		fnp1 = argv[optind++];
	else
		f_err("At least one pathname is required");

	if (optind < argc)
		fnp2 = argv[optind++];
	else
		fnp2 = ".";

	if (optind < argc)
		f_err("Only two pathnames are allowed");

	if (v_flag > 1)
		printf("Patterns:  %s  and  %s\n", fnp1, fnp2);

	filepair(fnp1, fnp2);

	if (v_flag > 0)
		printf("Returning %d\n", exitcode);
	return ((z_flag) ? (0) : (exitcode));
	}

/* ----------------------------------------------------------------------- */
	void				// Report a fatal error
f_err (char *s)			// Pointer to the message string

	{
	printf("\7%s\n", s);
	usage();
	}

/* ----------------------------------------------------------------------- */
	void
filepair (
	char  *fnp1,			// Ptr to the first  file pattern
	char  *fnp2)			// Ptr to the second file pattern

	{
	BOOL   ListFiles;		// TRUE to list filenames
	int    retval;			// Return from fp_init(), fp_pair()
	int    Count = 0;		// Count of file pairs found
	void  *fp = NULL;		// Pointer to the FP structure


	ListFiles = (isWild(fnp1)  ||  ( ! fnchkfil(fnp1)));
	if ((retval = fp_init(&fp, TRUE, fnp1, fnp2)) == 0)
		{
		while (fp_pair(fp, &fnpA, &fnpB) == 0)
			{
			if (ListFiles)
				{
				printf("%-30s  and  %s\n", fnpA, fnpB);
#ifdef  BUFFEREDOUT
				fflush(stdout);
#endif
				}

			if (Count++ > 0)
				printf("\n\n");

			Contrast();
#ifdef  BUFFEREDOUT
			fflush(stdout);
#endif
			}
		if (Count == 0)
			cantfind(fnp1);
		}
	else
		{
		switch (retval)
			{
			case FPR_NOFILE:	f_err("Path 2 not found");        break;
			case FPR_P2WILD:	f_err("Path 2 cannot be wild");   break;
			case FPR_P2FILE:	f_err("Path 2 cannot be a file"); break;
			case FPR_MEMORY:	f_err("Insufficient memory");     break;
			case FPR_FWERROR:	f_err("fWild() error");           break;
			default:			f_err("Unknown error");           break;
			}
		}
	}

/* ----------------------------------------------------------------------- */
	void
Contrast (void)

	{
	if ( ! Difference.LoadFiles(fnpA, fnpB))
		{
		printf("%s\n", (LPCTSTR)(Difference.GetError()));
		Difference.Reset();
		SETEXIT(2);
		return;
		}

	Match = Difference.GetFileMatch();
	if ( ! q_flag)
		{
		Prepare();
		List();
		}

	if ( ! Match)
		SETEXIT(1);
	Difference.Reset();
	}

/* ----------------------------------------------------------------------- */

FileDesc  LogicalFile = C;		// The logical file to be listed

char   CommonAB [_MAX_PATH] = {0};	// Common to both files
char   OnlyInA  [_MAX_PATH] = {0};	// Only  in file A header
char   OnlyInB  [_MAX_PATH] = {0};	// Only  in file B header
char   MovedInA [_MAX_PATH] = {0};	// Moved in file A header
char   MovedInB [_MAX_PATH] = {0};	// Moved in file B header
char   EndMsg   [_MAX_PATH] = {0};	// End of the listing

/* ----------------------------------------------------------------------- */
	void
Prepare (void)

	{
	sprintf(CommonAB, "******* Common");
	sprintf(OnlyInA,  "******* Only in  %s",  (L_flag) ? (fnpA) : "A");
	sprintf(OnlyInB,  "******* Only in  %s",  (L_flag) ? (fnpB) : "B");
	sprintf(MovedInA, "******* Moved in  %s", (L_flag) ? (fnpA) : "A");
	sprintf(MovedInB, "******* Moved in  %s", (L_flag) ? (fnpB) : "B");
	sprintf(EndMsg,   "******* End - ");

	LogicalFile = (a_flag) ? A : (b_flag) ? B : C;
	}

/* ----------------------------------------------------------------------- */
	BOOL
ShowHeader (
	RegType  Type)	// The region type

	{
	PSTR  s;		// Ptr to the string to be printed
	BOOL  Result = TRUE;// The returned result

	switch (Type)
		{
		default:		Result = FALSE;		break;
		case Common:	s = CommonAB; Result = (c_flag); break;
		case OnlyA:		s = OnlyInA;		break;
		case OnlyB:		s = OnlyInB;		break;
		case MovedA:	s = MovedInA;		break;
		case MovedB:	s = MovedInB;		break;
		}

	if (Result)
		{
		if (tty_flag)
			SetHighlight(TRUE);
		puts(s);
		if (tty_flag)
			SetHighlight(FALSE);
		}

	return (Result);
	}

/* ----------------------------------------------------------------------- */
	void
List (void)

	{
	int       LogicalLineNumber = 1;
	int       ActualLineNumber;
	BOOL      LinesPrinted = FALSE;
	BOOL      ListEnable = FALSE;
	PSTR      pLine;
	RegType   Type;
	RegType   PrevType = (RegType)(-1);


	while ((pLine = Difference.GetTextLine (LogicalFile, LogicalLineNumber, &ActualLineNumber, &Type)) != NULL)
		{
		if (PrevType != Type)
			ListEnable = ShowHeader(PrevType = Type);

		if (ListEnable)
			{
			if (n_flag)
			printf("%6u  ", ActualLineNumber);
			puts(pLine);
			LinesPrinted = TRUE;
			}
		++LogicalLineNumber;
		}

	if (tty_flag)
		SetHighlight(TRUE);

	if (LinesPrinted)
		fputs(EndMsg, stdout);

	if (Match)
		printf("Files are identical\n");
	else
		printf("Files are different\n");

	if (tty_flag)
		SetHighlight(FALSE);
	}

/* ----------------------------------------------------------------------- *\
|  Highlight subsystem - 32 bit
\* ----------------------------------------------------------------------- */

static	HANDLE  hConsole;		// The stdout console handle
static	WORD	HiLite;			// The highlighted color value
static	WORD	LoLite;			// The unhighlighted color value

/* ----------------------------------------------------------------------- */
	void
InitializeColors (void)

	{
	CONSOLE_SCREEN_BUFFER_INFO  ScreenInfo;	    // Current screen buffer info

	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	GetConsoleScreenBufferInfo(hConsole, &ScreenInfo);
	LoLite = ScreenInfo.wAttributes;

	if (HiLite == 0)
		HiLite = (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	}

/* ----------------------------------------------------------------------- */
	void
SetHighlightColor (
	int  Color)

	{
	HiLite = Color;
	}

/* ----------------------------------------------------------------------- */
	void
SetHighlight (
	int  Highlight)

	{
	if (h_flag)
		{
#ifdef  BUFFEREDOUT
		fflush(stdout);
#endif
		SetConsoleTextAttribute(hConsole, (WORD)(Highlight ? HiLite : LoLite));
		}
	}

/* --------------------------------- EOF --------------------------------- */
