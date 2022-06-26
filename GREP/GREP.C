/* ----------------------------------------------------------------------- *\
|
|									 GREP
|
|								Brian W Johnson
|								   18-Aug-90
|								   27-Dec-92
|								   20-Feb-93
|									6-Nov-93
|									3-Feb-96
|								   17-Aug-97 (Conversion to NT)
|								   14-Sep-97 (Addition of highlighting)
|								   30-Sep-97 (Addition of reverse (-r) option)
|								   31-Oct-97 (Long/short match (-m) modes)
|								   31-Oct-97 (Pattern from stdin or file)
|									4-Jan-98 (Region lines (-R) capability)
|								   14-Aug-00 (Long filename support)
|									4-Aug-10 (Add OR operator support)
|								   15-Sep-10 (Add multi-line pattern file support)
|								   16-Sep-10 (Separate -R [-/+]NN (before/after))
|								   29-Sep-10 (Add ability to mask unmatched text)
|								   29-Sep-10 (Add AND operator support, and AND/OR negation)
|									4-Oct-10 (Add :A#, :O#, :g operator support)
|									6-Oct-10 (Add directed output support)
|								   26-Oct-11 (Add tab metacharacter support)
|								   22-Nov-11 (Add match announcement line)
|								   18-Oct-13 (Add hex code match)
|
\* ----------------------------------------------------------------------- */

#ifdef _WIN32
#include  <windows.h>
#include  <wincon.h>
#endif

#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <ctype.h>
#include  <signal.h>
#include  <io.h>

#include  "fwild.h"
#include  "case.h"

/* ----------------------------------------------------------------------- */

char  *usagedoc [] =
{
"Usage:  grep  [%cac?cCefhilLmnprX] [-P path] [-R [-/+]NN] [-s NNN] [-t N]",
"              [%ct N] [-v+] [-w NNN] regexpr  [input_file_list]",
"",
"grep searches one or more files for a given regular expression pattern.",
"",
"    %ca      Print the match announcement line        (toggles, default off)",
"    %cc      Print only a count of matching lines     (toggles, default off)",
"    %cC      Observe upper and lower case             (toggles, default off)",
"    %cf      Print file names                         (toggles, default on )",
"    %ch      Highlight matched fields                 (toggles, default on )",
"    %ci      Indirectly read (possibly wild) pathnames from the input file(s)",
"                                                     (toggles, default off)",
"    %cl      List only the names of files containing a pattern match",
"                                                     (toggles, default off)",
"    %cL      List only the names of files NOT containing a pattern match",
"                                                     (toggles, default off)",
"    %cm      Use longest (shortest) match mode        (toggles, default short)",
"    %cM      Mask out unmatched text                  (toggles, default no masking)",
"    %cn      Precede each line by its line number     (toggles, default off)",
"    %cO <pathname> Write the output to the named file",
"    %cp      Read the regular expression from stdin",
"    %cP <pathname> Read the regular expression from the named file",
"    %cr      Print only NON-matching lines            (toggles, default off)",
"    %cR [-/+]NN Print NN lines before/after match lines  (max 99, default 0)",
#ifdef _WIN32
"    %cs NNN  Set the matched field text color         (default yellow)",
#else
"    %cs NNN  Set the matched field text attribute     (default bold)",
#endif
"    %cS      Print region separators (if -R regioned) (toggles, default off)",
"    %ct N    Set the tab width increment              (default 8)",
"    %cvvv    Print verbose internal information       (cumulative)",
"    %cw NNN  Trim output to NNN columns               (default no limit)",
"    %cX <pathspec> e/X/clude (possibly wild) paths matching pathspec",
"    %cX @<xfile>   e/X/clude files that match pathspec(s) in xfile",
"    %cX-      Disable default file exclusion(s)",
"    %cX+      Show exclusion path(s)",
"",
"Input is from the file list, or standard input.  Output is to standard output.",
"The input file list path names accept the wildcards '?', '*', and '**'.",
"Upper and lower case are ignored by default.  Blank lines never match.",
"The concatenation of regular expressions is a regular expression.",
"",
"    x   Any character not mentioned below matches that character;",
"    \\   quotes any non-white character; \"\\.\" matches a \".\";",
"    .   matches any character;",
"    :a  matches any alphabetic character;",
"    :b  matches any binary digit character;",
"    :c  matches any control character;",
"    :d  matches any decimal digit character;",
"    :g  matches any graphic (non-white) character;",
"    :n  matches any alphanumeric character;",
"    :p  matches any punctuation character;",
"    :s  matches any space, tab, or other white space character;",
"    :t  matches the tab character;",
"    :x  matches any hexadecimal digit character;",
"    :Xnn  matches the specified hexadecimal character code;",
"    :[  beginning an expression matches the beginning of a line;",
"    :]  ending an expression matches the end of a line;",
"    *   following an expression matches zero or more occurences",
"        of that expression: \"fo*\" matches \"f\", \"fo\", \"foo\"",
"    +   following an expression matches one or more occurences",
"        of that expression: \"fo+\" matches \"fo\", \"foo\", etc;",
"    -   following an expression optionally matches the expression;",
"  [xyz] A string enclosed in square brackets matches any character in that",
"        string, but no others.  If the first character in the string is a",
"        \"!\", the expression matches any character except the characters",
"        in the string.  For example, \"[xyz]\" matches \"xx\" and \"zyx\",",
"        while \"[!xyz]\" matches \"abc\" but not \"xzy\".  A range of",
"        characters may be specified by two characters separated by \"-\".",
"        Note that \"[a-z]\" matches alphas, while \"[z-a]\" matches nothing.",
"    :A  begins an \"AND\"     clause;",
"    :A# begins an \"AND\"     clause (but do not print);",
"    :A! begins an \"AND NOT\" clause;",
"    :O  begins an \"OR\"      clause;",
"    :O# begins an \"OR\"      clause (but do not print);",
"    :O! begins an \"OR NOT\"  clause;",
"",
"Copyright (c) 1985-2010 by J & M Software, Dallas TX - All Rights Reserved",
NULL
};

// ----------------------------------------------------------------------------
// General global definitions
// ----------------------------------------------------------------------------

#define MAX_LINE_LENGTH (16384)			// Maximum input file line length
#define MAX_EXT_PATTERN	 (1024)			// Maximum external pattern expression
#define PATHNAMESIZE	 (1024)			// Size of a pathname buffer

#ifdef _WIN32
#define PRTFLUSH()						// 32 bit windows needs no flushing
#else
#define PRTFLUSH()	(fflush(fout))		// Only 16 bit windows needs flushing
#endif

// ----------------------------------------------------------------------------
// Global control flags
// ----------------------------------------------------------------------------

int		announceflag  = 0;		/* TRUE to print announcement line */
int		caseflag	  = 1;		/* TRUE to ignore case */
int		cflag		  = 0;		/* TRUE to print match count only */
int		fonly		  = 0;		/* TRUE to list only filenames */
int		fronly		  = 0;		/* TRUE to list only non-match filenames */
int		fflag		  = 1;		/* TRUE to list filenames */
int		iflag		  = 0;		/* Read pathnames from stdin */
int		nflag		  = 0;		/* TRUE to list line numbers */
int		rflag		  = 0;		/* TRUE to list non-matching lines */
int		width		  = 0;		/* The trim width */
int		tty_flag	  = 0;		/* TRUE if output is to the console */
int		tab_width	  = 8;		/* The tab width increment */
int		maskflag	  = 0;		/* TRUE to mask out unmatched text */
int		highlightflag = 1;		/* TRUE to enable highlighting */
int		shortestmatch = 1;		/* TRUE to use shortest match */
int		debug		  = 0;		/* Increment for debug output */

char	pattern [MAX_EXT_PATTERN+1];  /* Pattern, if read from a file */

// ----------------------------------------------------------------------------
// Pattern compiler object types
// ----------------------------------------------------------------------------

typedef enum matchclass
	{
	C_ENDARRAY = 0,		// Not really needed
	C_ENDPAT   = 65536,	// Rest of classes are in high word
	C_CHR,
	C_BOL,
	C_EOL,
	C_ANY,
	C_CLASS,
	C_NCLASS,
	C_STAR,
	C_PLUS,
	C_MINUS,
	C_ALPHA,
	C_BINARY,
	C_DIGIT,
	C_ALPHANUM,
	C_PUNCT,
	C_SPACE,
	C_TAB,
	C_CTRL,
	C_HEX,
	C_GRAPHIC,
	C_RANGE
	} MatchClass, *PMatchClass, **PPMatchClass;

// ----------------------------------------------------------------------------
// Match descriptor buffers
// ----------------------------------------------------------------------------

#define	 MatchItemLimit (256)					// Number of match buffers supported

typedef
	struct _match
		{
		int	   PatIndex;
		char  *Begin;
		char  *End;
		} MATCHITEM;

MATCHITEM	MatchItems [MatchItemLimit];		// Array of match item structures
int			MatchItemCount;						// Count of match item structures

// ----------------------------------------------------------------------------
// Match pattern buffers
// ----------------------------------------------------------------------------

#define MAX_PATTERN_COUNT	  (10)				// Maximum number of patterns
#define MAX_PATTERN_LENGTH (16384)				// Max length of a pattern

typedef enum matchtype
	{
	M_FIRST,	// First clause
	M_OR,		// OR  clause
	M_ORNOT,	// OR  not clause
	M_AND,		// AND clause
	M_ANDNOT	// AND not clause
	} MTYPE;

int			PatCount = 0;											// Number of OR'ed patterns
MatchClass	PatBuff	 [MAX_PATTERN_COUNT] [MAX_PATTERN_LENGTH];		// The pattern buffer array
MTYPE		PatType	 [MAX_PATTERN_COUNT];							// Pattern type array
BOOL		PatPrint [MAX_PATTERN_COUNT];							// Print on :A and :O

// ----------------------------------------------------------------------------
// File handling
// ----------------------------------------------------------------------------

extern	void	InitializeColors (void);	/* Color initialization system */
extern	int		SetHighlight (int Flag);	/* Set highlighted field text */
extern	void	SetHighlightColor (int Color);	/* Set highlighted field text */

FILE   *fout;							// The output file, defaults to stdout

char	swch = '-';				/* The switch character */

char	buffer [BUFSIZ];		/* Buffer for stdout */

// ----------------------------------------------------------------------------
// Region line buffers
// ----------------------------------------------------------------------------

#define	 MAXREGION		 (99)					// Maximum region buffer count

int		RegionSeparator = FALSE;				// TRUE to print separator lines
int		RegionBefore	= 0;					// Number of match preceding lines
int		RegionAfter		= 0;					// Number of match succeeding lines

int		Remaining;								// Number of remaining lines to show
int		OldestLine;
int		CurrentLine;
char   *pCurrentLine;							// Ptr to the current input line
char   *LineArray [MAXREGION + 1];				// Ptrs to region line buffers

// ----------------------------------------------------------------------------
// Function prototypes
// ----------------------------------------------------------------------------

		void	 procwild		(char *pp);
		void	 compile		(char *);
		char	*cclass			(char *, char *, PPMatchClass, PMatchClass);
		void	 store			(MatchClass, PPMatchClass, PMatchClass);
		void	 process		(FILE *, char *);
		void	 file			(char *);
		void	 trim			(char *);
		BOOL	 linematch		(char *pLine, int PatIndex);
		char	*positionmatch	(char *, PMatchClass);
		char	*fgetss			(char *, int, FILE *);
		void	 badpat			(char *, char *, char *);
		char	*typename		(int type);
		char	*patname		(int type);
		int		match			(char *pLine);
		void	OutputLine		(int lno, BOOL RegionFlag, PCHAR pLine);
		void	debugpatterns	(void);
		void	debugmatches	(char *pLine);

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

/* ----------------------------------------------------------------------- *\
|  getname () - Get the next pathname
\* ----------------------------------------------------------------------- */
	char *								/* Returns ptr to pathname */
getname (FILE *fp)						/* File descriptor pointer */

	{
	int	   ch;							/* Working char */
	char  *p;							/* Pointer into the path name buffer */

static char	 namebuffer [PATHNAMESIZE]; /* The path name buffer */


	p = &namebuffer[0];					/* Point the name buffer */
	namebuffer[0] = '\0';				/* Show the buffer empty */

	while (((ch = fgetc(fp)) != EOF)	/* Skip leading whitespace */
	&&	  (isspace(ch)))
		;

	while ((ch != EOF)					/* Construct the pathname */
	&&	  ( ! isspace(ch)))
		{
		if (p < &namebuffer[PATHNAMESIZE-1])/* Don't overflow the buffer */
			*(p++) = ch;
		ch = fgetc(fp);
		}

	*p = '\0';							/* Terminate the pathname */
	if (namebuffer[0] != '\0')			/* Return only valid path names */
		return (&namebuffer[0]);

	return (NULL);						/* There is no name */
	}

/* ----------------------------------------------------------------------- *\
|  Read the regular expression from the specified file
\* ----------------------------------------------------------------------- */
	void
ReadExpression (
	char  *pFilename)	// Ptr to the path/filename

	{
	FILE  *fp;

	if (pFilename == NULL)
		fp = stdin;
	else if ((fp = fopen(pFilename, "rt")) == NULL)
		{
		fprintf(fout, "Pattern file not found\n");
		usage();
		}

	memset(pattern, 0, sizeof(pattern));
	if (fread(pattern, 1, (MAX_EXT_PATTERN), fp) == 0)
		{
		fprintf(fout, "Pattern file read error\n");
		usage();
		}

	if (pFilename != NULL)
		fclose(fp);
	}

/* ----------------------------------------------------------------------- *\
|  Open the named output file
\* ----------------------------------------------------------------------- */
	void
OpenOutputFile (
	char  *pFilename)	// Ptr to the path/filename

	{
	FILE  *fp;

	if ((pFilename == NULL)
	|| ((fp = pfopen(pFilename, "wt")) == NULL))
		{
		printf("Can't open output file \"%s\"\n", pFilename);
		usage();
		}

	fout = fp;	// OK, adopt it as the output file descriptor
	}

/* ----------------------------------------------------------------------- *\
|  Configure the output file as required
\* ----------------------------------------------------------------------- */
	void
ConfigureOutputFile (void)

	{
	tty_flag = ((fout == stdout)  &&  isatty(fileno(stdout)));
#ifdef _WIN32
	if ( ! tty_flag)
#endif
		if (fout != stdout)
			setbuf(fout, buffer);
	}

/* ----------------------------------------------------------------------- *\
|  Initialize the necessary number of region buffers
\* ----------------------------------------------------------------------- */
	void
InitRegionBuffers (void)

	{
	int	 i;

	for (i = 0; (i <= RegionBefore); ++i)		// Build the region buffer array
		{
		if ((LineArray[i] = malloc(MAX_LINE_LENGTH + 4)) == NULL)
			{
			fprintf(fout, "Unable to allocate region buffer\n");
			exit(255);
			}
		}
	}

/* ----------------------------------------------------------------------- *\
|  main () - Program main
\* ----------------------------------------------------------------------- */
	void
main (argc, argv)
	int	   argc;
	char  *argv [];

	{
	int	   option;				/* Option character */
	long   ltemp;				/* optvalue() temporary */
	char  *ap;					/* Argument pointer */
	FILE  *ifp;					/* Indirect input file descriptor pointer */
	char  *ip;					/* Indirect input pathname */
	char  *pPattern;			/* Ptr to the pattern buffer */

static	char   *optstring = "?acCfFhHiIlLmMnNo:O:pP:rR:s:St:T:vVw:W:X:";


	fout	 = stdout;			// The output file defaults to stdout
	swch	 = egetswch();
	optenv	 = getenv("GREP");

// BWJ
//fprintf(fout, "Starting\n");
//printf("Starting\n");
	while ((option = getopt(argc, argv, optstring)) != EOF)
		{
		switch (tolower(option))
			{
			case 'a':
				announceflag = !announceflag;
				break;

			case 'c':
				if (option == 'C')
					caseflag = !caseflag;
				else /* (option == 'c') */
					cflag = !cflag;
				break;

			case 'f':
				fflag = !fflag;
				break;

			case 'h':
				highlightflag = !highlightflag;
				break;

			case 'i':
				++iflag;
				break;

			case 'l':
				if (option == 'L')
					fronly = !fronly;
				else /* (option == 'l') */
					fonly = !fonly;
				break;

			case 'm':
				if (option == 'M')
					{
					if ((maskflag = !maskflag) != FALSE);
						highlightflag = 0;		// When masking, default highlighting off
					}
				else /* (option == 'm') */
					shortestmatch = !shortestmatch;
				break;

			case 'n':
				nflag = !nflag;
				break;

			case 'o':
				OpenOutputFile(optarg);
				break;

			case 'p':
				if (option == 'P')
					ReadExpression(optarg);
				else /* (option == 'p') */
					ReadExpression(NULL);
				break;

			case 'r':
				if (option == 'r')
					rflag = !rflag;
				else /* (option == 'R') */
					{
					int RegionType;

					switch (*optarg)
						{
						case '-':  RegionType = -1;	 ++optarg;	break;
						case '+':  RegionType =	 1;	 ++optarg;	break;
						default:   RegionType =	 0;				break;
						}

					if (optvalue(optarg, &ltemp, 0, MAXREGION))
						{
						fprintf(fout, "Region error - %s\n", optvalerror());
						usage();
						}

					if (RegionType	<= 0)
						RegionBefore = (int)(ltemp);

					if (RegionType	>= 0)
						RegionAfter	 = (int)(ltemp);
					}
				break;

			case 's':
				if (option == 's')
					{
#ifdef _WIN32
					if (optvalue(optarg, &ltemp, 1, 255))
#else
					if (optvalue(optarg, &ltemp, 1, 47))
#endif
						{
						fprintf(fout, "Color error - %s\n", optvalerror());
						usage();
						}
					SetHighlightColor((int)(ltemp));
					}
				else /* (option == 'S') */
					RegionSeparator = !RegionSeparator;
				break;

			case 't':
				if (optvalue(optarg, &ltemp, 1, 8))
					{
					fprintf(fout, "Tab size error - %s\n", optvalerror());
					usage();
					}
				tab_width = (int)(ltemp);
				break;

			case 'v':
				++debug;
				break;

			case 'w':
				if (optvalue(optarg, &ltemp, 1, MAX_LINE_LENGTH))
					{
					fprintf(fout, "Width error - %s\n", optvalerror());
					usage();
					}
				width = (int)(ltemp);
				break;

			case 'x':
				if (option == 'x')
					usage();

				if      (optarg[0] == '-')
					fexcludeDefEnable(FALSE);		/* Disable default file exclusion(s) */
				else if (optarg[0] == '+')
					fexcludeShowExcl(TRUE);			/* Enable stdout of exclusion(s) */
				else if (fexclude(optarg))
					{
					printf("\7Exclusion string fault: \"%s\"\n", optarg);
					usage();
					}
				break;

			case '?':
				help();

			default:
				usage();
			}
		}

	ConfigureOutputFile();				// Handle file buffering and tty_flag
	InitRegionBuffers();				// Allocate the Region buffers
	build_case_table(caseflag);			// Build the character case table

	if (pattern[0] != '\0')				// Process the expression pattern
		pPattern = pattern;				// ...from a pattern file
	else if (optind < argc)
		pPattern = argv[optind++];		// ...from the command line
	else
		{
		fprintf(fout, "Grep pattern missing\n");
		usage();
		}

	if (announceflag)
		fprintf(fout, "--- Matching \"%s\" ---\n", pPattern);

	compile(pPattern);

	if (tty_flag)
		{
		InitializeColors();
		signal(SIGINT, Terminated);
		signal(SIGBREAK, Terminated);
		}

	if (fronly)
		{
		fflag = FALSE;
		fonly = TRUE;
		}
	
	if (optind >= argc)			/* check for input from stdin */
		{
		if (iflag)
			{
			while ((ap = getname(stdin)) != NULL)
				procwild(ap);
			}
		else
			{
			fflag = FALSE;
			process(stdin, "<stdin>");
			}
		}
	else		/* taking pathnames from the command line list */
		{
		while (optind < argc)
			{
			ap = argv[optind++];
			if (iflag)
				{
				if (iswild(ap))
					fprintf(fout, "Indirect file can't be wild: %s\n", ap);
				else if (ifp = fopen(ap, "r"))
					{
					while ((ip = getname(ifp)) != NULL)
						procwild(ip);
					fclose(ifp);
					}
				else
					fprintf(fout, "Unable to open indirect file: %s\n", ap);
				}
			else
				procwild(ap);
			}
		}

#ifdef _WIN32
	if (tty_flag)
		SetHighlight(FALSE);
#endif

	if (fout != stdout)
		fclose(fout);

	exit(0);
	}

/* ----------------------------------------------------------------------- *\
|  procwild () - Process the (possibly wild) pathname
\* ----------------------------------------------------------------------- */
	void
procwild (
	char  *pp)					/* Pointer to pathname specifier */

	{
	void  *hp;					/* Pointer to wild file data block */
	FILE  *fp;					/* Input file descriptor */
	char  *fnp;					/* The translated file name */
	int	   smode = FW_FILE;		/* File search mode attributes */


	hp = fwinit(pp, smode);				/* Process the input list */
	if ((fnp = fwildexcl(hp)) == NULL)
		cantopen(pp);
	else
		{
		do	{							/* Process one filespec */
			if (fp = fopen(fnp, "r"))
				{
				process(fp, fnp);
				fclose(fp);
				}
			else
				cantopen(fnp);
			} while ((fnp = fwildexcl(hp)));
		}
	}

/* ----------------------------------------------------------------------- *\
|  store () - Store the compiled elements into the current PatBuff[]
\* ----------------------------------------------------------------------- */
	void
store (
	MatchClass	    opchar,		// The operator/character to be stored
	PPMatchClass  ppCurrent,	// Pptr into		 the current pattern buffer
	PMatchClass    pEnd)		// Ptr to the end of the current pattern buffer

	{
	if (*ppCurrent >= pEnd)
		{
		fprintf(fout, "Grep pattern too complex");
		exit(1);
		}
	**ppCurrent = opchar;		// Store caller's operator or character into the pattern
	++*ppCurrent;				// Advance caller's pattern buffer pointer
	}

/* ----------------------------------------------------------------------- *\
|  compile () - Compile the pattern(s) into the PatBuff and PatType arrays
\* ----------------------------------------------------------------------- */
	void
compile (
	char  *pSrcPattern)			/* Ptr to pattern to compile */

	{
	char *pSrc = pSrcPattern;	/* Ptr into source pattern */
	PMatchClass  pPatBegin;		/* Ptr to the beginning of the current pattern buffer */
	PMatchClass  pPatEnd;		/* Ptr to the end		of the current pattern buffer */
	PMatchClass  pPatCur;		/* Ptr into				   the current pattern buffer */
	PMatchClass  pPatPrev;		/* Last pattern pointer */
	PMatchClass  pPatSaved;		/* Save beginning of pattern */
	int	   c;					/* Current character */
	int	   cmod;				/* Current modifier character for :A and :O */
	int	   o;					/* Temporary */
	MTYPE  matchType = M_FIRST; /* Type for the current pattern */
	BOOL   printType = TRUE;	/* Print type for the current pattern */

	PatCount = 0;

	if (debug)
		{
		fprintf(fout, "Pattern = \"%s\"\n", pSrc);
		PRTFLUSH();
		}

	do	{						/* Compile each OR'ed or AND'ed pattern */
		if (PatCount >= MAX_PATTERN_COUNT)
			{
			badpat("too many patterns", pSrcPattern, pSrc);
			break;
			}

		PatType[PatCount]  = matchType;	// (and initialized to M_FIRST for the initial clause)
		PatPrint[PatCount] = printType;	// (and initialized to TRUE for the initial clause)
		pPatBegin = PatBuff[PatCount];
		pPatEnd	  = pPatBegin + (MAX_PATTERN_LENGTH - 1);
		pPatCur   = pPatBegin;
		pPatPrev  = pPatCur;

		while ((c = *pSrc++) != '\0')
			{
			if ( ! isprint(c))			// Ignore CR, LF, and all other control characters
				continue;

			/* STAR, PLUS, and MINUS are special operators */

			if ((c == '*')  ||  (c == '+')  ||  (c == '-'))
				{
				if ((pPatCur == pPatBegin)  ||  ((o = pPatCur[-1]) == C_BOL)  ||  (o == C_EOL)
				||	 (o == C_STAR)  ||  (o == C_PLUS)  ||  (o == C_MINUS))
					{
					badpat("illegal occurence of pattern opcode", pSrcPattern, pSrc);
					break;
					}
				store(C_ENDPAT, &pPatCur, pPatEnd);
				store(C_ENDPAT, &pPatCur, pPatEnd);
				pPatSaved = pPatCur;					/* Save current pattern end */
				while (--pPatCur > pPatPrev)			/* Move pattern down */
					*pPatCur = pPatCur[-1];				/* one byte */
				*pPatCur     = (((char)(c) == '*')
					? C_STAR : (((char)(c) == '-')
					? C_MINUS
					: C_PLUS));
				pPatCur = pPatSaved;					/* Restore pattern end */
				continue;
				}

			pPatPrev = pPatCur;							// All the others, remember the start
			switch (c)
				{
#if 0
				case '^':								// The Microsoft shell escape character
					store(C_BOL, &pPatCur, pPatEnd);
					break;

				case '$':
					store(C_EOL, &pPatCur, pPatEnd);
					break;
#endif
				case '.':
					store(C_ANY, &pPatCur, pPatEnd);
					break;

				case '[':
					pSrc = cclass(pSrcPattern, pSrc, &pPatCur, pPatEnd);
					break;

				case ':':								// Special operator sequence
					if ( ! isprint(*pSrc))
						{
						badpat(":x type missing", pSrcPattern, pSrc);
						break;
						}
					c = *pSrc++;						// Get the colon operator type code

					if (c == 'A')			// Process the 'AND' operators
						{
						cmod = *pSrc;					// Get any possible modifier
						if		(cmod == '!')			// Check for NAND
							{
							++pSrc;
							matchType = M_ANDNOT;
							printType = FALSE;
							}
						else if (cmod == '#')			// Check for no print option
							{
							++pSrc;
							matchType = M_AND;
							printType = FALSE;
							}
						else							// else must be a plain M_AND
							{
							matchType = M_AND;
							printType = TRUE;
							}
						goto patterndone;
						}

					if (c == 'O')			// Process the 'OR' operators
						{
						cmod = *pSrc;					// Get any possible modifier
						if		(cmod == '!')			// Check for NOR
							{
							++pSrc;
							matchType = M_ORNOT;
							printType = FALSE;
							}
						else if (cmod == '#')			// Check for no print option
							{
							++pSrc;
							matchType = M_OR;
							printType = FALSE;
							}
						else							// else must be a plain M_OR
							{
							matchType = M_OR;
							printType = TRUE;
							}
						goto patterndone;
						}

					switch (c)				// Process the pattern operators
						{
						case ':':
							store(C_CHR, &pPatCur, pPatEnd);	// Secondary escape for ':'
							store((MatchClass)(':'), &pPatCur, pPatEnd);
							break;

						case 'a':
						case 'A':
							store(C_ALPHA, &pPatCur, pPatEnd);
							break;

						case 'b':
						case 'B':
							store(C_BINARY, &pPatCur, pPatEnd);
							break;

						case 'c':
						case 'C':
							store(C_CTRL, &pPatCur, pPatEnd);
							break;

						case 'd':
						case 'D':
							store(C_DIGIT, &pPatCur, pPatEnd);
							break;

						case 'g':
						case 'G':
							store(C_GRAPHIC, &pPatCur, pPatEnd);
							break;

						case 'n':
						case 'N':
							store(C_ALPHANUM, &pPatCur, pPatEnd);
							break;

						case 'p':
						case 'P':
							store(C_PUNCT, &pPatCur, pPatEnd);
							break;

						case 's':
						case 'S':
							store(C_SPACE, &pPatCur, pPatEnd);
							break;

						case 't':
						case 'T':
							store(C_TAB, &pPatCur, pPatEnd);
							break;

						case 'x':
							store(C_HEX, &pPatCur, pPatEnd);
							break;

						case 'X':
							{
							int  c;		// Working character
							char code;	// Assembled hex code byte

							c = toupper(*(pSrc++));
//							printf("char %02X, pSrc: %08lX\n", c, pSrc); // BWJ
//							printf("pattern: %08lx, string %s\n", pSrcPattern, pSrcPattern); // BWJ
							if (isxdigit(c))
								code = (((c <= '9') ? (c - '0') : (c - 'A' + 10)) << 4);
							else
								{
//								printf("char %02X, pSrc: %0xlX\n", c, pSrc); // BWJ
								badpat("invalid hex character", pSrcPattern, pSrc);
								break;
								}

							c = toupper(*(pSrc++));
//							printf("char %02X, pSrc: %08lX\n", c, pSrc); // BWJ
//							printf("pattern: %08lx, string %s\n", pSrcPattern, pSrcPattern); // BWJ
							if (isxdigit(c))
								code |= ((c <= '9') ? (c - '0') : (c - 'A' + 10));
							else
								{
//								printf("char %02X\n", c); // BWJ
								badpat("invalid hex character", pSrcPattern, pSrc);
								break;
								}
							store(C_CHR,	&pPatCur, pPatEnd);
//							printf("char %02X\n", code); // BWJ
							store((MatchClass)(code), &pPatCur, pPatEnd);
							break;
							}

						case '[':
							store(C_BOL, &pPatCur, pPatEnd);
							break;

						case ']':
							store(C_EOL, &pPatCur, pPatEnd);
							break;

						default:
							badpat(":x type unknown", pSrcPattern, pSrc);
							break;
						}
					break;

				case '\\':						// The grep escape character
					if ( ! isprint(*pSrc))
						{
						badpat("\\x escape missing", pSrcPattern, pSrc);
						break;
						}
					store(C_CHR, &pPatCur, pPatEnd);
					store((MatchClass)(adjust_case(*pSrc++)), &pPatCur, pPatEnd);
					break;

				default:						// Ordinary character
					store(C_CHR, &pPatCur, pPatEnd);
					store((MatchClass)(adjust_case(c)), &pPatCur, pPatEnd);
				}
			}

patterndone:
		if (pPatCur != pPatBegin)
			++PatCount;							// Count the pattern, if not empty
		store(C_ENDPAT, &pPatCur, pPatEnd);
		store(C_ENDARRAY, &pPatCur, pPatEnd);
		} while (c != '\0');	// Not NUL signifies an additional OR'ed or AND'ed pattern */

	if (debug)
		debugpatterns();
	}

/* ----------------------------------------------------------------------- *\
|  cclass () - Compile a character class (within the [] construct)
\* ----------------------------------------------------------------------- */
	char *
cclass (
	char         *pClassSrc,		// Ptr to src class pattern start (for error message)
	char         *pClassDef,		// Ptr to class definition text
	PPMatchClass  ppPatBegin,		// Pptr to beginning of current pattern buffer
	PMatchClass    pPatEnd)			// Ptr	to end		 of current pattern buffer

	{
	int           c;				// Current character
	int           Length;			// Length of the class
	MatchClass    MatchType;		// Determined pattern object type
	PMatchClass  pPatLengthField;	// Ptr into compiled pattern, length byte field

	MatchType = C_CLASS;
	if (*pClassDef == '!')
		{
		++pClassDef;
		MatchType = C_NCLASS;
		}
	store(MatchType, ppPatBegin, pPatEnd);								// Store the object type

	pPatLengthField = *ppPatBegin;										// (N)CLASS is delimited by byte count, not by ENDPAT
	Length			= 0;												// Init the class pattern byte count to zero
	store((MatchClass)(Length), ppPatBegin, pPatEnd);

	while (((c = *pClassDef++) != '\0')  &&  (c != ']'))
		{
		if (c == '\\')													// Store quoted char
			{
			if ((c = *pClassDef++) == '\0')								// Gotta get something
				badpat("pattern class terminates badly", pClassSrc, pClassDef);
			else
				store((MatchClass)(adjust_case(c)), ppPatBegin, pPatEnd);
			}

		else if ((c == '-')  &&  ((*ppPatBegin - pPatLengthField) > 1)  &&  (*pClassDef != ']')  &&  (*pClassDef != '\0'))
			{
			c = (*ppPatBegin)[-1];										// Insert RANGE type before the range start character
			(*ppPatBegin)[-1] = C_RANGE;
			store((MatchClass)(c), ppPatBegin, pPatEnd);// Re-store the range start character
			c = *pClassDef++;											// Store the range end char
			store((MatchClass)(adjust_case(c)), ppPatBegin, pPatEnd);
			}
		else
			store((MatchClass)(adjust_case(c)), ppPatBegin, pPatEnd);	// Store normal char for class
		}

	if (c != ']')
		badpat("pattern class unterminated", pClassSrc, pClassDef);
	if ((Length = (*ppPatBegin - pPatLengthField)) >= 256)
		badpat("pattern class too large", pClassSrc, pClassDef);
	if (Length == 0)
		badpat("pattern class empty", pClassSrc, pClassDef);

	*pPatLengthField = (char)(Length);
	return	(pClassDef);
	}

/* ----------------------------------------------------------------------- *\
|  badpat () - Report a fatal pattern error
\* ----------------------------------------------------------------------- */
	void
badpat (message, source, stop)
	char  *message;				/* Error message */
	char  *source;				/* Pattern start */
	char  *stop;				/* Pattern end */

	{
	fprintf(fout, "Pattern error (%s), pattern is \"%s\"\n", message, source);
	fprintf(fout, "  stopped at character %d: '%c'\n", stop-source, stop[-1]);
	exit(1);
	}

/* ----------------------------------------------------------------------- *\
|  process () - Process one file
\* ----------------------------------------------------------------------- */
	void
process (fp, fnp)				/* Scan the file for the patterns in PatBuff[][] */
	FILE  *fp;					/* File to process */
	char  *fnp;					/* Path/file name (for -f option) */

	{
	int		lno;
	int		count;
	int		matched;
	BOOL	Showit;
	BOOL	Shown;


	lno			 = 0;
	count		 = 0;
	Shown		 = 0;
	Remaining	 = -1;
	OldestLine	 = CurrentLine = 0;
	pCurrentLine = LineArray[CurrentLine];
	while (fgetss(pCurrentLine, MAX_LINE_LENGTH, fp))
		{
		++lno;
		Showit = FALSE;
		matched = match(pCurrentLine);
		if ((matched && !rflag)	 ||	 ((!matched) && rflag))
			{
			++count;
			if (!cflag)
				{
				if (fflag && fnp)
					{
					file(fnp);
					fnp = NULL;
					}
				Showit = (! fonly);
				}
			}

		if (Showit)
			{
			while (OldestLine != CurrentLine)			// Flush the region buffer
				{
				int	 Delta = (CurrentLine - OldestLine);

				if (Delta < 0)
					Delta += (RegionBefore + 1);

				if (Shown == 0)							// Check if region separator needed
					Shown = 1;
				else if (RegionSeparator  &&  (Delta == RegionBefore))
					OutputLine(-1, TRUE, "--------");

				OutputLine((lno - Delta), TRUE, LineArray[OldestLine]);
				if (++OldestLine > RegionBefore)		// Advance oldest that we just showed
					OldestLine = 0;
				}

			OutputLine(lno, FALSE, pCurrentLine);		// Show the current (matched) line

			Remaining = RegionAfter;					// Establish any needed remaining count
			}
		else if (Remaining > 0)							// Only show (unmatched) if remaining not zero
			{
			Showit = TRUE;
			OutputLine(lno, TRUE, pCurrentLine);		// Show the region line
			--Remaining;
			}

		if (RegionBefore > 0)							// If regioning before...
			{
			if (++CurrentLine > RegionBefore)			// Advance current to the next line
				CurrentLine = 0;

			if (Showit)									// If line was shown, make it stay empty
				OldestLine = CurrentLine;

			else if ((OldestLine == CurrentLine)		// Else if overflow, push oldest to the next line
			&&	(++OldestLine > RegionBefore))
				OldestLine = 0;

			pCurrentLine = LineArray[CurrentLine];		// Also advance the line buffer pointer
			}
		}

	if (fronly && (count == 0) && (fnp != NULL))
		file(fnp);

	if (cflag)
		{
		if (fflag && (fnp != NULL))
			file(fnp);
		fprintf(fout, "%d\n", count);
		PRTFLUSH();
		}
	}

/* ----------------------------------------------------------------------- *\
|  OutputLine () - Output a line that is matched, or is region included
\* ----------------------------------------------------------------------- */
	void
OutputLine (
	int		Line,				// The line number
	BOOL	RegionFlag,			// TRUE if it is a region line
	PCHAR  pLine)				// Ptr to the line buffer

	{
	int	 Highlighted = FALSE;

	if (nflag  &&  (Line > 0))
		fprintf(fout, "%7d:", Line);

	trim(pLine);
	if (RegionFlag)
		fputs(pLine, fout);		// Just output the entire line

	else						// Output with highlighted/masked regions
		{
		char		 ch;
		int			 Highlight;
		int			 Column		 = 0;
		MATCHITEM  *pSentinel	 = &MatchItems[MatchItemCount];
		char	   *p			 = pLine;


		while ((ch = *p) != '\0')
			{
			MATCHITEM  *pMatchItem = &MatchItems[0];

			Highlight = FALSE;
			for (pMatchItem = &MatchItems[0]; (pMatchItem < pSentinel); ++pMatchItem)
				{
				if ((p >= pMatchItem->Begin)  &&  (p < pMatchItem->End))
					{
					Highlight = TRUE;
					break;
					}
				}

			if (Highlighted != Highlight)
				Highlighted	 = SetHighlight(Highlight);

			if (ch == '\t')
				{
				if (Highlighted)
					fputc(0x7F, fout);
				else
					fputc(' ', fout);
				while ((++Column % tab_width) != 0)
					fputc(' ', fout);
				}
			else if ((! maskflag)  ||  Highlighted)
				{
				fputc(ch, fout);	/* Output the character when not masking (highlighted or maskflag is FALSE) */
				++Column;
				}
			else						/* Output a blank when masking (maskflag TRUE and not in a match phrase) */
				{
				fputc(' ', fout);
				++Column;
				}
			++p;
			}
		}

	if (Highlighted)
		SetHighlight(FALSE);
	fputc('\n', fout);
	PRTFLUSH();
	}

/* ----------------------------------------------------------------------- *\
|  file () - Print the file name when requested
\* ----------------------------------------------------------------------- */
	void
file (fnp)						/* Print the file identification */
	char  *fnp;					/* Path/file name (for -f option) */

	{
	if (fonly || fronly)
		fprintf(fout, "%s\n", fnp);
	else
		fprintf(fout, "\nFile %s:\n\n", fnp);
	PRTFLUSH();
	}

/* ----------------------------------------------------------------------- *\
|  trim () - Trim the output lines to a maximum length
\* ----------------------------------------------------------------------- */
	void
trim (s)						/* Trim the output line to 80 columns */
	char  *s;					/* Pointer to the string */

	{
	int	 col = 0;

	if (width == 0)
		return;

	if (nflag)					/* Allow for line numbers */
		col = 8;
	while (*s)
		{
		if (*s == '\t')			/* Handle tabs */
			{
			col = (col + 8) & ~7;
			}
		else
			++col;

		if (col >= width)
			{
			*s = '\0';
			return;
			}
		++s;
		}
	}

/* ----------------------------------------------------------------------- *\
|  match () - Perform full matching on one line
\* ----------------------------------------------------------------------- */
	int							// Return nonzero if match successful, else 0
match (
	char  *pLine)				// Ptr to line to match in

	{
	int	   PatIndex;			// Pattern index counter
	int	   StartingMatchIndex;	// Match item count before scanning each pattern
	BOOL   Matched;				// TRUE to show the match
	BOOL   ShowIt;				// TRUE to cause a match to be shown
	BOOL   MatchResult;			// Match result


	if (debug > 1)
		{
		fprintf(fout, "LINE(\"%s\")\n", pLine);
		PRTFLUSH();
		}

	MatchResult	   = FALSE;
	MatchItemCount = 0;
	for (PatIndex = 0; (PatIndex < PatCount); ++PatIndex)
		{
		BOOL PrintMask = PatPrint[PatIndex];

		StartingMatchIndex = MatchItemCount;

		Matched = linematch(pLine, PatIndex);

		switch (PatType[PatIndex])
			{
			case M_FIRST:
			default:
				MatchResult = Matched;
				ShowIt		= Matched;
				break;

			case M_OR:
				MatchResult = (MatchResult ||  Matched);
				ShowIt		= PrintMask && Matched;
				break;

			case M_ORNOT:
				MatchResult = (MatchResult || !Matched);
				ShowIt		= PrintMask;
				break;

			case M_AND:
				MatchResult = (MatchResult &&  Matched);
				ShowIt		= PrintMask && Matched;
				break;

			case M_ANDNOT:
				MatchResult = (MatchResult && !Matched);
				ShowIt		= PrintMask;
				break;
			}

		if ( ! MatchResult)
			MatchItemCount = 0;							// Discard all matches from all patterns
		else if ( ! ShowIt)
			MatchItemCount = StartingMatchIndex;		// Discard the matches from this pattern
		}

	if ( ! MatchResult)
		MatchItemCount = 0;

	if (debug > 1)
		debugmatches(pLine);

	return	(MatchResult);
	}

/* ----------------------------------------------------------------------- *\
|  linematch () - Match one pattern against one input line
\* ----------------------------------------------------------------------- */
	BOOL						/* Return TRUE if match found, else FALSE */
linematch (
	char        *pLine,			/* Ptr to line to match in */
	int	          PatIndex)		/* Index of the pattern buffer */

	{
	char        *pTryMatch;		/* Ptr into the line */
	char        *pEndMatch;		/* The positionmatch() result */
	PMatchClass  pPattern;		/* Ptr to the pattern buffer */
	BOOL          MatchResult;	/* Match result */


	MatchResult = FALSE;
	pPattern	= PatBuff[PatIndex];
	for (pTryMatch = pLine; *pTryMatch; ++pTryMatch)
		{
		if ((pEndMatch = positionmatch(pTryMatch, pPattern)) != NULL)
			{
			MatchResult = TRUE;
			if (MatchItemCount >= MatchItemLimit)
				break;	/* Too many matches */

			else
				{
				MATCHITEM  *pMatchItem = &MatchItems[MatchItemCount++];

				pMatchItem->PatIndex   = PatIndex;
				pMatchItem->Begin	   = pTryMatch;
				pMatchItem->End		   = pEndMatch;
				}
			}
		}

	if (debug > 2)
		{
		if (MatchResult)
			fprintf(fout, "Matched pattern %d\n", PatIndex);
		else
			fprintf(fout, "Match failed\n");
		PRTFLUSH();
		}

	return	(MatchResult);
	}

/* ----------------------------------------------------------------------- *\
|  positionmatch () - Match one pattern against one input line starting position
\* ----------------------------------------------------------------------- */
	char *						/* Returns ptr to next char, or NULL */
positionmatch (
	char          *pLine,		/* (partial) line to match */
	PMatchClass    pattern)		/* (partial) pattern to match */

	{
	char         *pCurChar;		/* Current pointer into the line */
	char         *pPrevChar;	/* Saved previous current pointer */
	char           ch;			/* Current character */
	char        *spMatchEnd;	/* End for STAR and PLUS match */
	MatchClass     matchType;	/* Pattern operation */
	int	           nClass;		/* Class counter */
	PMatchClass   pPatCur;		/* Current pattern pointer */
	PMatchClass   pNextPat;		/* Ptr to next pattern */


	if (debug > 2)
		{
		fprintf(fout, "positionmatch(\"%s\")\n", pLine);
		PRTFLUSH();
		}

	pCurChar = pLine;
	pPatCur  = pattern;
	while ((matchType = *pPatCur++) != C_ENDPAT)
		{
		if (debug > 3)
			{
			fprintf(fout, "byte[%d] = 0x%02X, '%c', matchType = %s\n", pCurChar - pLine, *pCurChar, *pCurChar, patname(matchType));
			PRTFLUSH();
			}

		switch (matchType)
			{
			case C_CHR:
				ch = *pCurChar++;
//				printf("ch %02X\n", ch); // BWJ
				if (adjust_case(ch) != *pPatCur++)
					return	(NULL);
				break;

			case C_BOL:
				if (pCurChar != pCurrentLine)
					return	(NULL);
				break;

			case C_EOL:
				if (*pCurChar != '\0')
					return	(NULL);
				break;

			case C_ANY:
				if (*pCurChar++ == '\0')
					return	(NULL);
				break;

			case C_BINARY:
				ch = *pCurChar++;
				if ((ch != '0')  &&  (ch != '1'))
					return	(NULL);
				break;

			case C_DIGIT:
				ch = *pCurChar++;
				if ( ! isdigit(ch))
					return	(NULL);
				break;

			case C_ALPHA:
				ch = *pCurChar++;
				if ( ! isalpha(ch))
					return	(NULL);
				break;

			case C_ALPHANUM:
				ch = *pCurChar++;
				if ( ! isalnum(ch))
					return	(NULL);
				break;

			case C_PUNCT:
				ch = *pCurChar++;
				if ( ! ispunct(ch))
					return	(NULL);
				break;

			case C_SPACE:
				ch = *pCurChar++;
				if ( ! isspace(ch))
					return	(NULL);
				break;

			case C_TAB:
				ch = *pCurChar++;
				if (ch != '\t')
					return	(NULL);
				break;

			case C_CTRL:
				ch = *pCurChar++;
				if ( ! iscntrl(ch))
					return	(NULL);
				break;

			case C_HEX:
				ch = *pCurChar++;
				if ( ! isxdigit(ch))
					return	(NULL);
				break;

			case C_GRAPHIC:
				ch = *pCurChar++;
				if ( ! isgraph(ch))
					return	(NULL);
				break;

			case C_CLASS:
			case C_NCLASS:
				ch = *pCurChar++;
				ch = adjust_case(ch);
				nClass = *pPatCur++ & 0xFF;
				do	{
					if (*pPatCur == C_RANGE)
						{
						pPatCur += 3;
						nClass -= 2;
						if ((ch >= pPatCur[-2])  &&  (ch <= pPatCur[-1]))
							break;
						}
					else if (ch == *pPatCur++)
						break;
					} while (--nClass > 1);

				if ((matchType == C_CLASS) == (nClass <= 1))
					return	(NULL);
				if (matchType == C_CLASS)
					pPatCur += nClass - 2;
				break;

			case C_MINUS:
				spMatchEnd = positionmatch(pCurChar, pPatCur);	/* Look for a match		*/
				while (*pPatCur++ != C_ENDPAT);					/* Skip over pattern	*/
					if (spMatchEnd)								/* Got a match?			*/
						pCurChar = spMatchEnd;					/* Yes, update string	*/
				break;											/* Always succeeds		*/

			case C_PLUS:										/* One or more ...		*/
				if ((pCurChar = positionmatch(pCurChar, pPatCur)) == NULL)
					return (NULL);								/* Gotta have a match	*/
			// NOTE: Intentional fallthrough to the STAR subcase

			case C_STAR:										/* Zero or more ...		*/
				{
				if (shortestmatch)
					{	// (looking for shortest match)
					for (pNextPat = pPatCur; (*pNextPat++ != C_ENDPAT); )
						;										/* Skip over the STAR pattern */
					do	{										/* Try to match rest */
						if (spMatchEnd = positionmatch(pCurChar, pNextPat))
							return (spMatchEnd);				/* Success */
						}  while (*pCurChar  &&  (pCurChar = positionmatch(pCurChar, pPatCur))); /* Advance STAR by one  */
					}
				else /* (looking for longest match) */
					{
					pPrevChar = pCurChar;						/* Remember the line position start */
					while (*pCurChar  &&  (spMatchEnd = positionmatch(pCurChar, pPatCur)))
						pCurChar = spMatchEnd;					/* Get longest match */
					while (*pPatCur++ != C_ENDPAT)				/* Skip over pattern */
						;
					while (pCurChar >= pPrevChar)				/* Try to match rest */
						{
						if (spMatchEnd = positionmatch(pCurChar, pPatCur))
							return (spMatchEnd);
						--pCurChar;								/* Nope, try earlier */
						}
					}
				return (NULL);									/* Nothing worked */
				}

			default:
				fprintf(fout, "Bad grep pattern opcode (program error) %d\n", matchType);
				exit(1);
			}
		}

	return (pCurChar);
	}

/* ----------------------------------------------------------------------- *\
|  fgetss () - Read and prepare one line of an input file
\* ----------------------------------------------------------------------- */
	char *
fgetss (buf, len, fp)
	char  *buf;
	int	   len;
	FILE  *fp;

	{
	char *p;
	char *q;

	if (p = fgets(buf, len, fp))
		{
		q = buf + strlen(buf) - 1;		/* Remove final newline, if any */
		if (*q == '\n')
			*q = '\0';
		}
	return (p);
	}

/* ----------------------------------------------------------------------- *\
|  typename () - Translate a pattern clause type to its text name
\* ----------------------------------------------------------------------- */
	char *
typename (
	int	   type)

	{
	char  *p;

	switch (type)
		{
		case M_FIRST:		p = "First  ";			break;
		case M_OR:			p = "OR     ";			break;
		case M_ORNOT:		p = "OR NOT ";			break;
		case M_AND:			p = "AND    ";			break;
		case M_ANDNOT:		p = "AND NOT";			break;
		default:			p = "<UNK>  ";			break;
		}

	return (p);
	}

/* ----------------------------------------------------------------------- *\
|  patname () - Translate a pattern compiler type to its text name
\* ----------------------------------------------------------------------- */
	char *
patname (
	int	   type)

	{
	char  *p;

	switch (type)
		{
		case C_ENDARRAY:	p = "<NUL>";			break;
		case C_ENDPAT:		p = "<ENDPAT>";			break;
		case C_CHR:			p = "<CHAR>";			break;
		case C_BOL:			p = "<BOL>";			break;
		case C_EOL:			p = "<EOL>";			break;
		case C_ANY:			p = "<ANY>";			break;
		case C_CLASS:		p = "<CLASS>";			break;
		case C_NCLASS:		p = "<NCLASS>";			break;
		case C_STAR:		p = "<STAR>";			break;
		case C_PLUS:		p = "<PLUS>";			break;
		case C_MINUS:		p = "<MINUS>";			break;
		case C_ALPHA:		p = "<ALPHA>";			break;
		case C_BINARY:		p = "<BINARY>";			break;
		case C_DIGIT:		p = "<DIGIT>";			break;
		case C_ALPHANUM:	p = "<ALPHANUM>";		break;
		case C_PUNCT:		p = "<PUNCT>";			break;
		case C_SPACE:		p = "<SPACE>";			break;
		case C_TAB:			p = "<TAB>";			break;
		case C_CTRL:		p = "<CTRL>";			break;
		case C_HEX:			p = "<HEX>";			break;
		case C_GRAPHIC:		p = "<GRAPHIC>";		break;
		case C_RANGE:		p = "<RANGE>";			break;
		default:			p = "<unknown>";		break;
		}

	return (p);
	}

/* ----------------------------------------------------------------------- *\
|  debugpatterns () - Print the pattern table
\* ----------------------------------------------------------------------- */
	void
debugpatterns (void)

	{
	int	   PatIndex;		// Pattern index
	MatchClass	 c;			// Current class code
	PMatchClass  pp;		// Ptr into				   the current pattern buffer
	PMatchClass  pPatBegin;	// Ptr to the beginning of the current pattern buffer
	PMatchClass  pPatEnd;	// Ptr to the end		of the current pattern buffer

	fprintf(fout, "%d pattern(s) compiled\n", PatCount);
	for (PatIndex = 0; (PatIndex < PatCount); ++PatIndex)
		{
		pPatBegin = PatBuff[PatIndex];
		pPatEnd	  = pPatBegin + (MAX_PATTERN_LENGTH - 1);

		// Show the pattern index and type
		fprintf(fout, "PATTERN %d: [%s] ", PatIndex, typename(PatType[PatIndex]));
		for (pp = pPatBegin; ((pp < pPatEnd)  &&  (c = *pp) != C_ENDARRAY); ++pp)
			{
			if ((c >= C_ENDPAT)  ||  (c == C_ENDARRAY))
				fprintf(fout, "%s ", patname(c));		// Show an operator
			else
				fprintf(fout, "%c ", c);				// Show a character
			}
		fprintf(fout, " [end]\n");
		}
	PRTFLUSH();
	}

/* ----------------------------------------------------------------------- *\
|  debugmatches () - Print the match item list
\* ----------------------------------------------------------------------- */
	void
debugmatches (
	char  *pLine)				// Ptr to current line

	{
	int	 MatchIndex;

	fprintf(fout, "%d match item(s)\n", MatchItemCount);
	for (MatchIndex = 0; (MatchIndex < MatchItemCount); ++MatchIndex)
		{
		MATCHITEM  *pMatchItem = &MatchItems[MatchIndex];

		fprintf(fout, "MATCH %d: Pattern %d, offset %d, length %d\n",
			MatchIndex,
			pMatchItem->PatIndex,
			(pMatchItem->Begin - pLine),
			(pMatchItem->End - pMatchItem->Begin));
		}
	PRTFLUSH();
	}

/* ----------------------------------------------------------------------- *\
|  Highlight subsystem - 16 bit
\* ----------------------------------------------------------------------- */
#ifndef _WIN32

static	char	*pLoLite	 = "\033[0m";  /* The unhighlight request string */
static	char	pHiLite [10] = "\033[1m";  /* The highlight	  request string */

/* ----------------------------------------------------------------------- *\
|  InitializeColors () - Initialize the color system
\* ----------------------------------------------------------------------- */
	void
InitializeColors (void)

	{
	}

/* ----------------------------------------------------------------------- *\
|  SetHighlightColor () - Set the highlight color
\* ----------------------------------------------------------------------- */
	void
SetHighlightColor (
	int	 Color)

	{
	if (tty_flag)
		sprintf(pHiLite, "\033[%um", Color);
	}

/* ----------------------------------------------------------------------- *\
|  SetHighlight () - 
\* ----------------------------------------------------------------------- */
	int
SetHighlight (
	int	 Highlight)

	{
	if (highlightflag  &&  tty_flag)
		{
		fputs((Highlight ? pHiLite : pLoLite), stdout);
		fputc('\n', stdout);
		}
	return (Highlight);
	}

#endif
/* ----------------------------------------------------------------------- *\
|  Highlight subsystem - 32 bit
\* ----------------------------------------------------------------------- */
#ifdef _WIN32

static	HANDLE	hConsole;				/* The stdout console handle */
static	WORD	HiLite;					/* The highlighted color value */
static	WORD	LoLite;					/* The unhighlighted color value */

/* ----------------------------------------------------------------------- *\
|  SetHighlightColor () - Set the highlight color
\* ----------------------------------------------------------------------- */
	void
InitializeColors (void)

	{
	if (tty_flag)
		{
		CONSOLE_SCREEN_BUFFER_INFO	ScreenInfo;		/* Current screen buffer info */

		hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

		GetConsoleScreenBufferInfo(hConsole, &ScreenInfo);
		LoLite = ScreenInfo.wAttributes;

//		if (debug)
//			fprintf(fout, "Screen Colors: %04X\n", LoLite);

		if (HiLite == 0)
			HiLite = (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);

#if 0	// FOR SOME REASON THE BACKGROUND COLOR ISN'T AVAILABLE HERE
		if (LoLite & (BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE))
			HiLite = (FOREGROUND_RED | FOREGROUND_INTENSITY);
		else
			HiLite = (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
//			HiLite = 0xAC;
//			HiLite = (BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE);
//			HiLite = (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY | BACKGROUND_BLUE);
//			HiLite = (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
#endif
		}
	}

/* ----------------------------------------------------------------------- *\
|  SetHighlightColor () - Set the highlight color
\* ----------------------------------------------------------------------- */
	void
SetHighlightColor (
	int	 Color)

	{
	HiLite = Color;
	}

/* ----------------------------------------------------------------------- *\
|  SetHighlight () - 
\* ----------------------------------------------------------------------- */
	int
SetHighlight (
	int	 Highlight)

	{
	if (highlightflag  &&  tty_flag)
		SetConsoleTextAttribute(hConsole, (WORD)(Highlight ? HiLite : LoLite));
	return (Highlight);
	}
#endif
/* --------------------------------- EOF --------------------------------- */
