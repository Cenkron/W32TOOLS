/* ----------------------------------------------------------------------- *\
|
|				     GREP
|
|		      Modified for MS-DOS 2.X, 3.X, 5.0
|			        Brian W Johnson
|				   18-Aug-90
|				   27-Dec-92
|				   20-Feb-93
|				    6-Nov-93
|				    3-Feb-96
|				   17-Aug-97 (Conversion to NT)
|				   14-Sep-97 (Addition of highlighting)
|
|  -----------------------------------------------------------------------
|
|	The  information  in  this  document  is  subject  to  change
|	without  notice  and  should not be construed as a commitment
|	by Digital Equipment Corporation or by DECUS.
| 
|	Neither Digital Equipment Corporation, DECUS, nor the authors
|	assume any responsibility for the use or reliability of  this
|	document or the described software.
| 
|	Copyright (C) 1980, DECUS
|
|	General permission to copy or modify, but not for profit,  is
|	hereby  granted,  provided that the above copyright notice is
|	included and reference made to  the  fact  that  reproduction
|	privileges were granted by DECUS.
|
\* ----------------------------------------------------------------------- */

#include  <windows.h>
#include  <wincon.h>
#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <ctype.h>
#include  <signal.h>
#include  <io.h>

#include  "fWild.h"
#include  "case.h"

#define  OK	 0
#define  FAIL	-1

#define SHORTESTMATCH	    // New match method, looking for shortest match

/* ----------------------------------------------------------------------- */

char  *usagedoc [] =
{
"Usage:  grep  [%c?cefihlnrsvwx]  expr  [input_file_list]",
"",
"grep searches one or more files for a given regular expression pattern.",
"",
"    %cc      Print only a count of matching lines   (toggles, default off)",
"    %cC      Observe upper and lower case           (toggles, default off)",
"    %ce      Extract only the matching symbols      (toggles, default off)",
"    %cf      Print file names                       (toggles, default on )",
"    %ch      Highlight matched fields               (toggles, default on )",
"    %ci      Indirectly read (possibly wild) pathnames from the input file(s)",
"                                                   (toggles, default off)",
"    %cl      List only the names of files containing a pattern match",
"                                                   (toggles, default off)",
"    %cn      Precede each line by its line number   (toggles, default off)",
"    %cr      Print only NON-matching lines          (toggles, default off)",
"    %cs NNN  Set the matched field text color       (default yellow)",
"    %ct N    Set the tab width increment            (default 8)",
"    %cvvv    Print verbose internal information     (cumulative)",
"    %cw NNN  Limit output to NNN columns            (default no limit)",
"    %cX <pathname> excludes matching (possibly wild) pathnames from the grep",
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
"    :c  matches any control character;",
"    :d  matches any decimal digit character;",
"    :n  matches any alphanumeric character;",
"    :p  matches any punctuation character;",
"    :s  matches any space, tab, or other white space character;",
"    :x  matches any hexadecimal digit character;",
"    :(  beginning an expression matches the beginning of a line;",
"    :)  ending an expression matches the end of a line;",
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
"",
"Copyright (c) 1985-1997 by J & M Software, Dallas TX - All Rights Reserved",
NULL
};

/* ----------------------------------------------------------------------- */

#define ENDARRAY  0
#define CHAR      1
#define BOL       2
#define EOL       3
#define ANY       4
#define CLASS     5
#define NCLASS    6
#define STAR      7
#define PLUS      8
#define MINUS     9
#define ALPHA    10
#define DIGIT    11
#define ALPHANUM 12
#define PUNCT    13
#define SPACE    14
#define CTRL     15
#define HEX      16
#define RANGE    17
#define ENDPAT   18

#define LMAX	16384
#define PMAX	16384

#ifndef TRUE
#define FALSE	  0
#define TRUE	  1
#endif

int	nfile;
char   *pp;

char	lbuf [LMAX];
char	pbuf [PMAX];

int	case_flag = 1;		/* TRUE to ignore case */
int	cflag     = 0;		/* TRUE to print match count only */
int	debug     = 0;		/* Increment for debug output */
int	fonly     = 0;		/* TRUE to list only filenames */
int	eflag     = 0;		/* Extract the matching symbol */
int	fflag     = 1;		/* TRUE to list filenames */
int	hflag     = 1;		/* TRUE to enable highlighting */
int	iflag     = 0;		/* Read pathnames from stdin */
int	nflag     = 0;		/* TRUE to list line numbers */
int	rflag     = 0;		/* TRUE to list non-matching lines */
int	width     = 0;		/* The trim width */
int     tty_flag  = 0;		/* TRUE if output is to the console */
int     tab_width = 8;		/* The tab width increment */

typedef
    struct _match
	{
	char  *Begin;
	char  *End;
	} MATCHITEM;

#define     MatchItemLimit (256)	    /* Number of matches supported */
MATCHITEM   MatchItems [MatchItemLimit];    /* Array of match item structures */
int         MatchItemCount;		    /* Count of match item structures */

extern	void	InitializeColors (void);    /* Color initialization system */
extern	void	SetHighlight (int Flag);    /* Set highlighted field text */
extern	void	SetHighlightColor (int Color);  /* Set highlighted field text */

char	swch = '-';		/* The switch character */

char	buffer [BUFSIZ];	/* Buffer for stdout */
	
	PHP		hp = NULL;		// FWILD instance pointer
	PEX		xp = NULL;		// FEX instance pointer

	void     procwild	(char *pp);
	void	 compile	(char *);
	char    *cclass		(char *, char *);
	void	 store		(int);
	void	 process	(FILE *, char *);
	void	 file		(char *);
	void	 trim		(char *);
	char    *patmatch	(char *, char *);
	char    *fgetss		(char *, int, FILE *);
	void	 badpat		(char *, char *, char *);
	char    *patname	(int type);
	int	match		(void);

#define  NAMESIZE  (1024)	/* Size of the pathname buffer */

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
    char *				/* Returns ptr to pathname */
getname (FILE *fp)			/* File descriptor pointer */

    {
    int    ch;				/* Working char */
    char  *p;				/* Pointer into the path name buffer */

static char  namebuffer [NAMESIZE];	/* The path name buffer */


    p = &namebuffer[0];			/* Point the name buffer */
    namebuffer[0] = '\0';		/* Show the buffer empty */

    while (((ch = fgetc(fp)) != EOF)	/* Skip leading whitespace */
    &&    (isspace(ch)))
	;

    while ((ch != EOF)			/* Construct the pathname */
    &&    ( ! isspace(ch)))
	{
	if (p < &namebuffer[NAMESIZE-1])/* Don't overflow the buffer */
	    *(p++) = ch;
	ch = fgetc(fp);
	}

    *p = '\0';				/* Terminate the pathname */
    if (namebuffer[0] != '\0')		/* Return only valid path names */
	return (&namebuffer[0]);

    return (NULL);			/* There is no name */
    }

/* ----------------------------------------------------------------------- *\
|  Program main
\* ----------------------------------------------------------------------- */
    void
main (argc, argv)
    int    argc;
    char  *argv [];

    {
    int    option;		/* Option character */
    long   ltemp;		/* optvalue() temporary */
    char  *ap;			/* Argument pointer */
    FILE  *ifp;			/* Indirect input file descriptor pointer */
    char  *ip;			/* Indirect input pathname */

static	char   *optstring = "?cCeEfFhHiIlLnNrRs:S:t:T:vVw:W:x:X:";


	if ((hp = fwOpen()) == NULL)
		exit(1);
	if ((xp = fExcludeOpen()) == NULL)
		exit(1);

    swch     = egetswch();
    optenv   = getenv("GREP");
    tty_flag = isatty(fileno(stdout));
    if ( ! tty_flag)
		setbuf(stdout, buffer);

    while ((option = getopt(argc, argv, optstring)) != EOF)
	{
	switch (tolower(option))
	    {
	    case 'c':
		if (option == 'C')
		    case_flag = !case_flag;
		else
		    cflag = !cflag;
		break;

	    case 'e':
		eflag = !eflag;
		break;

	    case 'f':
		fflag = !fflag;
		break;

	    case 'h':
		hflag = !hflag;
		break;

	    case 'i':
		++iflag;
		break;

	    case 'l':
		fonly = !fonly;
		break;

	    case 'n':
		nflag = !nflag;
		break;

	    case 'r':
		rflag = !rflag;
		break;
	    case 's':
		if (optvalue(optarg, &ltemp, 1, 255))
		    {
		    printf("Color error - %s\n", optvalerror());
		    usage();
		    }
		SetHighlightColor((int)(ltemp));
		break;

	    case 't':
		if (optvalue(optarg, &ltemp, 1, 8))
		    {
		    printf("Tab size error - %s\n", optvalerror());
		    usage();
		    }
		tab_width = (int)(ltemp);
		break;

	    case 'v':
		++debug;
		break;

	    case 'w':
		if (optvalue(optarg, &ltemp, 1, LMAX))
		    {
		    printf("Width error - %s\n", optvalerror());
		    usage();
		    }
		width = (int)(ltemp) + 1;
		break;

	    case 'x':
		if (fExclude(xp, optarg))
		    {
		    printf("Exclusion string fault: \"%s\"\n", optarg);
		    usage();
		    }
		break;

	    case '?':
		help();

	    default:
		usage();
	    }
	}

    build_case_table(case_flag);

    if (optind >= argc)
	{
	printf("Grep pattern missing\n");
	usage();
	}

    else
	compile(argv[optind]);


    if (tty_flag)
	{
	InitializeColors();
	signal(SIGINT, Terminated);
	signal(SIGBREAK, Terminated);
	}

    if (++optind >= argc)	/* check for input from stdin */
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
    else	/* taking pathnames from the command line list */
	{
	while (optind < argc)
	    {
	    ap = argv[optind++];
	    if (iflag)
		{
		if (isWild(ap))
		    printf("Indirect file can't be wild: %s\n", ap);
		else if (ifp = fopen(ap, "r"))
		    {
		    while ((ip = getname(ifp)) != NULL)
			procwild(ip);
		    fclose(ifp);
		    }
		else
		    printf("Unable to open indirect file: %s\n", ap);
		}
	    else
		procwild(ap);
	    }
	}

    if (tty_flag)
		SetHighlight(FALSE);

	xp = fExcludeClose(xp);		// Close the Exclusion instance
	hp = fwClose(hp);			// Close the fWild instance
    exit(0);
    }

/* ----------------------------------------------------------------------- *\
|  procwild () - Process the (possibly wild) pathname
\* ----------------------------------------------------------------------- */
    void
procwild (char *pp)		/* Pointer to pathname specifier */

	{
	FILE  *fp;				/* Input file descriptor */
	char  *fnp;				/* The translated file name */
	int    smode = FW_FILE;	/* File search mode attributes */


	if (fwInit(hp, pp, smode) != FWERR_NONE)	// Process the input list
		fwInitError(pp);
	fExcludeConnect(xp, hp);					// Connect the exclusion instance
    if ((fnp = fWild(hp)) == NULL)
		{
		cantopen(pp);
		}
    else
		{
		do  {				/* Process one filespec */
		    if (fp = fopen(fnp, "r"))
			{
			process(fp, fnp);
			fclose(fp);
			}
	    else
			cantopen(fnp);
		    } while ((fnp = fWild(hp)));
		}
    }

/* ----------------------------------------------------------------------- */
    void
compile (source)		/* Compile the pattern into global pbuf[] */
    char  *source;		/* Pattern to compile */

    {
    char  *s;			/* Source string pointer */
    char  *lp;			/* Last pattern pointer	*/
    int    c;			/* Current character */
    int    o;			/* Temporary */
    char  *spp;			/* Save beginning of pattern */
    char  *cclass();		/* Compile class routine */

    s = source;
    if (debug)
	{
	printf("Pattern = \"%s\"\n", s);
	fflush(stdout);
	}

    pp = pbuf;
    lp = pp;
    while (c = *s++)			/* STAR, PLUS and MINUS are special */
	{
	if ((c == '*')  ||  (c == '+')  ||  (c == '-'))
	    {
	    if ((pp == pbuf)  ||  ((o = pp[-1]) == BOL)  ||  (o == EOL)
	    ||   (o == STAR)  ||  (o == PLUS)  ||  (o == MINUS))
		badpat("illegal occurence of pattern opcode", source, s);
	    store(ENDPAT);
	    store(ENDPAT);
	    spp = pp;				/* Save pattern end */
	    while (--pp > lp)			/* Move pattern	down */
		*pp = pp[-1];			/* one byte */
	    *pp = (char)(((char)(c) == '*') ? STAR :
		(((char)(c) == '-') ? MINUS : PLUS));
	    pp = spp;				/* Restore pattern end */
	    continue;
	    }
	lp = pp;			/* All the others, remember start */
	switch (c)
	    {
#if 0
	    case '^':	// The Microsoft shell escape character
		store(BOL);
		break;

	    case '$':
		store(EOL);
		break;
#endif
	    case '.':
		store(ANY);
		break;

	    case '[':
		s = cclass(source, s);
		break;

	    case ':':
		if (*s == 0)
		    badpat(":x type missing", source, s);
		else
		    {
		    c = *s++;
		    switch (tolower(c))
			{
			case 'a':
			    store(ALPHA);
			    break;

			case 'c':
			    store(CTRL);
			    break;

			case 'd':
			    store(DIGIT);
			    break;

			case 'n':
			    store(ALPHANUM);
			    break;

			case 'p':
			    store(PUNCT);
			    break;

			case 's':
			    store(SPACE);
			    break;

			case 'x':
			    store(HEX);
			    break;

			case '(':
			    store(BOL);
			    break;

			case ')':
			    store(EOL);
			    break;

			default:
			    badpat(":x type unknown", source, s);
			}
		    break;
		    }

	    case '\\':
		if (*s)
		    c = *s++;

	    default:
		store(CHAR);
		store(adjust_case(c));
	    }
	}
    store(ENDPAT);
    store(ENDARRAY);				/* Terminate the array */

    if (debug)
	{
	for (lp = pbuf; lp < pp;)
	    {
	    if ((c = (*lp++ & 0xFF)) < ' ')
		printf("%s ", patname(c));
	    else
		printf("%c ", c);
	    }
	printf("\n");
	fflush(stdout);
	}
    }

/* ----------------------------------------------------------------------- */
    char *
cclass (source, src)		/* Compile a class (within []) */
    char  *source;		/* Pattern start (for error message) */
    char  *src;			/* Class start */

    {
    char  *s;			/* Source pointer */
    char  *cp;			/* Pattern start */
    int    c;			/* Current character */
    int    o;			/* Temp */

    s = src;
    o = CLASS;
    if (*s == '!')
	{
	++s;
	o = NCLASS;
	}
    store(o);
    cp = pp;
    store(0);					/* Byte count */
    while ((c = *s++)  &&  (c != ']'))
	{
	if (c == '\\')				/* Store quoted char */
	    {
	    if ((c = *s++) == '\0')		/* Gotta get something */
		badpat("pattern class terminates badly", source, s);
	    else
		store(adjust_case(c));
	    }

	else if ((c == '-')  &&  ((pp - cp) > 1)  &&  (*s != ']')  &&  (*s != '\0'))
	    {
	    c = pp[-1];				/* Range start */
	    pp[-1] = RANGE;			/* Range signal */
	    store(c);				/* Re-store start */
	    c = *s++;				/* Get end char... */
	    store(adjust_case(c));		/* ...and store it */
	    }
	else
	    store(adjust_case(c));		/* Store normal char */
	}

    if (c != ']')
	badpat("pattern class unterminated", source, s);
    if ((c = (pp - cp)) >= 256)
	badpat("pattern class too large", source, s);
    if (c == 0)
	badpat("pattern class empty", source, s);
    *cp = (char)(c);
    return  (s);
    }

/* ----------------------------------------------------------------------- */
    void
badpat (message, source, stop)
    char  *message;		/* Error message */
    char  *source;		/* Pattern start */
    char  *stop;		/* Pattern end */

    {
    printf("Pattern error (%s), pattern is\"%s\"\n", message, source);
    printf("  stopped at character %d: '%c'\n", stop-source, stop[-1]);
    exit(1);
    }

/* ----------------------------------------------------------------------- */
    void
store (op)
    int  op;

    {
    if (pp >= &pbuf[PMAX-1])	/* msm */
	{
	printf("Grep pattern too complex");
	exit(1);
	}
    *pp++ = (char)(op);
    }

/* ----------------------------------------------------------------------- */
    void
process (fp, fnp)		/* Scan the file for the pattern in pbuf[] */
    FILE  *fp;			/* File to process */
    char  *fnp;			/* Path/file name (for -f option) */

    {
    int    lno;
    int    count;
    int    m;
    int    Column;


    lno = 0;
    count = 0;
    while (fgetss(lbuf, sizeof lbuf, fp))
	{
	++lno;
	m = match();
	if ((m && !rflag)  ||  ((!m) && rflag))
	    {
	    ++count;
	    if (!cflag)
		{
		if (fflag && fnp)
		    {
		    file(fnp);
		    fnp = NULL;
		    }
		if ( ! fonly)
		    {
		    if (eflag)
			{
			if (MatchItemCount > 0)
			    {
			    char *p = MatchItems[0].Begin;
			    *(MatchItems[0].End) = '\0';
			    trim(p);
			    printf("%s\n", p);
			    }
			}
		    else /* ( ! eflag) */
			{
			if (nflag)
			    printf("%7d:", lno);

			trim(lbuf);
			if ( ! tty_flag)
			    puts(lbuf);

			else /* (tty_flag) */
			    {
			    char        ch;
			    int         Highlight   = FALSE;
			    int         Highlighted = FALSE;
			    char       *p           = lbuf;
			    MATCHITEM  *pMatchItem  = &MatchItems[0];
			    MATCHITEM  *pSentinel   = &MatchItems[MatchItemCount];

			    Column = 0;
			    while ((ch = *p) != '\0')
				{
				if (pMatchItem < pSentinel)
				    {
				    if (p == pMatchItem->Begin)
					Highlight = TRUE;

				    else if (p == pMatchItem->End)
					{
					Highlight = FALSE;
					while (++pMatchItem < pSentinel)
					    {
					    if (p < pMatchItem->End)
						{
						if (p >= pMatchItem->Begin)
						    Highlight = TRUE;
						break;
						}
					    }
					}

				    if (Highlighted != Highlight)
					{
					Highlighted  = Highlight;
					SetHighlight(Highlight);
					}
				    }
				if (ch != '\t')
				    {
				    putchar(ch);
				    ++Column;
				    }
				else
				    {
				    putchar(' ');
				    while ((++Column % tab_width) != 0)
					putchar(' ');
				    }
				++p;
				}

			    if (Highlighted)
				SetHighlight(FALSE);
			    putchar('\n');
			    }
			}
		    }
		}
	    }
	}

    if (cflag)
	{
	if (fflag && fnp)
	    file(fnp);
	printf("%d\n", count);
	}
    }

/* ----------------------------------------------------------------------- */
    void
file (fnp)			/* Print the file identification */
    char  *fnp;			/* Path/file name (for -f option) */

    {
    if (fonly)
	printf("%s\n", fnp);
    else
	printf("\nFile %s:\n\n", fnp);
    }

/* ----------------------------------------------------------------------- */
    void
trim (s)			/* Trim the output line to 80 columns */
    char  *s;			/* Pointer to the string */

    {
    int  col = 0;

    if (width == 0)
	return;

    if (nflag)				/* Allow for line numbers */
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

/* ----------------------------------------------------------------------- */
    int				/* Return nonzero if any matches, else 0 */
match ()			/* Match the current line in lbuf[] */

    {
    char  *l;			/* Line pointer */
    char  *efinish;		/* The patmatch() result */
    MATCHITEM  *pMatchItem	/* Ptr into the match item array */
	= &MatchItems[0];


    if (debug > 1)
	{
	printf("LINE(\"%s\")\n", lbuf);
	}

    MatchItemCount = 0;
    for (l = lbuf; *l; l++)
	{
	if (efinish = patmatch(l, pbuf))
	    {
	    if (MatchItemCount < MatchItemLimit)
		{
		pMatchItem->Begin = l;
		pMatchItem->End   = efinish;
		++pMatchItem;
		++MatchItemCount;
		}
	    }
	}

    if (debug > 1)
	{
	int         i;
	MATCHITEM  *p = &MatchItems[0];
	for (i = 0; (i < MatchItemCount); ++i)
	    {
	    printf("GREPMATCH(%d, %d, %d)\n", i, (p->Begin - lbuf), (p->End - p->Begin));
	    ++p;
	    }
	}

    return  (MatchItemCount);
    }

/* ----------------------------------------------------------------------- *\
|  Attempt to match the pattern
\* ----------------------------------------------------------------------- */
    char *			/* Returns ptr to next char, or NULL */
patmatch (line, pattern)
    char  *line;		/* (partial) line to match */
    char  *pattern;		/* (partial) pattern to match */

    {
    char  *l;			/* Current line pointer */
    char  *p;			/* Current pattern pointer */
    char   c;			/* Current character */
    char  *e;			/* End for STAR and PLUS match */
    int    op;			/* Pattern operation */
    int    n;			/* Class counter */
#ifdef SHORTESTMATCH
    char  *pNextPat;		/* Ptr to next pattern */
#else
    char  *are;			/* Start of STAR match */
#endif


    if (debug > 2)
	{
	printf("patmatch(\"%s\")\n", line);
	}

    l = line;
    p = pattern;
    while ((op = *p++) != ENDPAT)
	{
	if (debug > 3)
	    {
	    printf("byte[%d] = 0x%02X, '%c', op = %s\n", l - line, *l, *l, patname(op));
	    }

	switch (op)
	    {
	    case CHAR:
		c = *l++;
		if (adjust_case(c) != *p++)
		    return  (NULL);
		break;

	    case BOL:
		if (l != lbuf)
		    return  (NULL);
		break;

	    case EOL:
		if (*l != '\0')
		    return  (NULL);
		break;

	    case ANY:
		if (*l++ == '\0')
		    return  (NULL);
		break;

	    case DIGIT:
		c = *l++;
		if ( ! isdigit(c))
		    return  (NULL);
		break;

	    case ALPHA:
		c = *l++;
		if ( ! isalpha(c))
		    return  (NULL);
		break;

	    case ALPHANUM:
		c = *l++;
		if ( ! isalnum(c))
		    return  (NULL);
		break;

	    case PUNCT:
		c = *l++;
		if ( ! ispunct(c))
		    return  (NULL);
		break;

	    case SPACE:
		c = *l++;
		if ( ! isspace(c))
		    return  (NULL);
		break;

	    case CTRL:
		c = *l++;
		if ( ! iscntrl(c))
		    return  (NULL);
		break;

	    case HEX:
		c = *l++;
		if ( ! isxdigit(c))
		    return  (NULL);
		break;

	    case CLASS:
	    case NCLASS:
		c = *l++;
		c = adjust_case(c);
		n = *p++ & 0xFF;
		do  {
		    if (*p == RANGE)
			{
			p += 3;
			n -= 2;
			if ((c >= p[-2])  &&  (c <= p[-1]))
			    break;
			}
		    else if (c == *p++)
			break;
		    } while (--n > 1);

		if ((op == CLASS) == (n <= 1))
		    return  (NULL);
		if (op == CLASS)
		    p += n - 2;
		break;

	    case MINUS:
		e = patmatch(l, p);		/* Look for a match	*/
		while (*p++ != ENDPAT);		/* Skip over pattern	*/
		    if (e)			/* Got a match?		*/
			l = e;			/* Yes, update string	*/
		break;				/* Always succeeds	*/

	    case PLUS:				/* One or more ...	*/
		if ((l = patmatch(l, p)) == NULL)
		    return  (NULL);		/* Gotta have a match	*/
	    // NOTE: Intentional fallthrough to the STAR subcase

	    case STAR:				/* Zero or more ...	*/
		{
#ifdef SHORTESTMATCH // New way, looking for shortest match
		for (pNextPat = p; (*pNextPat++ != ENDPAT); )
		    ;				/* Skip over the STAR pattern */
		do  {				/* Try to match rest    */
		    if (e = patmatch(l, pNextPat))
			return  (e);		/* Success              */
		    }  while (*l  &&  (l = patmatch(l, p))); /* Advance STAR by one  */
#else // Old way, looking for longest match
		are = l;			/* Remember line start	*/
		while (*l  &&  (e = patmatch(l, p)))
		    l = e;			/* Get longest match	*/
		while (*p++ != ENDPAT)		/* Skip over pattern	*/
		    ;
		while (l >= are)		/* Try to match rest	*/
		    {
		    if (e = patmatch(l, p))
			return  (e);
		    --l;			/* Nope, try earlier	*/
		    }
#endif
		return  (NULL);			/* Nothing worked	*/
		}

	    default:
		printf("Bad grep pattern opcode (program error) %d\n", op);
		exit(1);
	    }
	}

    return  (l);
    }

/* ----------------------------------------------------------------------- */
    char *
fgetss (buf, len, fp)
    char  *buf;
    int    len;
    FILE  *fp;

    {
    char *p;
    char *q;

    if (p = fgets(buf, len, fp))
	{
	q = buf + strlen(buf) - 1;	/* Remove final newline, if any */
	if (*q == '\n')
	    *q = '\0';
	}
    return (p);
    }

/* ----------------------------------------------------------------------- */
    char *
patname (
    int    type)

    {
    char  *p;

    switch (type)
	{
	    case ENDARRAY:	p = "ENDARRAY";		break;
	    case CHAR:		p = "CHAR";		break;
	    case BOL:		p = "BOL";		break;
	    case EOL:		p = "EOL";		break;
	    case ANY:		p = "ANY";		break;
	    case CLASS:		p = "CLASS";		break;
	    case NCLASS:	p = "NCLASS";		break;
	    case STAR:		p = "STAR";		break;
	    case PLUS:		p = "PLUS";		break;
	    case MINUS:		p = "MINUS";		break;
	    case ALPHA:		p = "ALPHA";		break;
	    case DIGIT:		p = "DIGIT";		break;
	    case ALPHANUM:	p = "ALPHANUM";		break;
	    case PUNCT:		p = "PUNCT";		break;
	    case SPACE:		p = "SPACE";		break;
	    case CTRL:		p = "CTRL";		break;
	    case HEX:		p = "HEX";		break;
	    case RANGE:		p = "RANGE";		break;
	    case ENDPAT:	p = "ENDPAT";		break;
	    default:		p = "<unknown>";	break;
	}

    return (p);
    }

/* ----------------------------------------------------------------------- *\
|  Highlight subsystem - 32 bit
\* ----------------------------------------------------------------------- */

static	HANDLE  hConsole;		/* The stdout console handle */
static	WORD	HiLite;			/* The highlighted color value */
static	WORD	LoLite;			/* The unhighlighted color value */

/* ----------------------------------------------------------------------- */
    void
InitializeColors (void)

    {
    CONSOLE_SCREEN_BUFFER_INFO  ScreenInfo;	    /* Current screen buffer info */

    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    GetConsoleScreenBufferInfo(hConsole, &ScreenInfo);
    LoLite = ScreenInfo.wAttributes;

//  if (debug)
//	printf("Screen Colors: %04X\n", LoLite);

    if (HiLite == 0)
	HiLite = (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);

#if 0 // FOR SOME REASON THE BACKGROUND COLOR ISN'T AVAILABLE HERE
    if (LoLite & (BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE))
	HiLite = (FOREGROUND_RED | FOREGROUND_INTENSITY);
    else
//	HiLite = 0xAC;
//	HiLite = (BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE);
//	HiLite = (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY | BACKGROUND_BLUE);
	HiLite = (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
//	HiLite = (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
#endif
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
    if (hflag)
	SetConsoleTextAttribute(hConsole, (WORD)(Highlight ? HiLite : LoLite));
    }

/* ----------------------------------------------------------------------- */
