/* ----------------------------------------------------------------------- *\
|
|									 L / LS
|
|								 Brian W Johnson
|			Copyright (c) 1985, 1990, 1993, 1998; All rights reserved
|								   19-Jun-90
|								   15-May-91
|								   12-Jul-92
|								   22-Feb-93
|								   17-Aug-97
|								   22-Mar-98  QuoteFlag added
|								   25-Sep-01  Support for huge directories
|								   27-Sep-07  Support for 64 bit file sizes
|								   16-Jun-22  Added exclusion path mechanism
|								   22-Oct-23  Major cleanup
|
\* ----------------------------------------------------------------------- */

#include  <windows.h>

#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <ctype.h>
#include  <io.h>
#include  <time.h>

#include  "fWild.h"
#include  "ptypes.h"

/* ----------------------------------------------------------------------- */

char  *usagedoc [] =
{
"Usage:  l  [%c?<flags>]  [file_list]  [>output_file]",
"",
"l lists the file directory of all pathnames in the file_list",
"",
"l returns an exit code as follows:",
"",
"     2  Command syntax error",
"     1  No file/directory/volume name was found (and no %cq option)",
"     0  At least one file/directory/volume name was found (or %cq option)",
"",
"    %ca        List only un-archived entries",
"    %cA A      List by Boolean [arhs] attributes: (e.g., %cA HS+ar )",
"    %cb        List file lengths in hexadecimal",
"    %cc        List (comprehensively) all files and directories",
"    %cC N      Use N columns for wide listings (default 5 columns)",
"    %cd        List only directories, not files (default both)",
"    %cD        Enable listing \"Access Denied\" warnings",
"    %ce        Check only for existence of the files",
"    %cE        Don't do automatic directory expansion",
"    %cf        List only files, not directories (default all)",
"    %cF pnstaw Determine the list format (default \"psta\")",
"    %ch        List hidden files and/or directories",
"    %cH        List only hidden files and/or directories",
"    %cl        List in lower case",
"    %cL N      Use N lines for \"more\" listings (default 25 lines)",
"    %cm        List in \"more\" mode, using the default",
"    %cM N      List in \"more\" mode, using N lines",
"    %cn        Don't list files or directories",
"    %cN N      Use N columns for the name field",
"    %co <td>   List only if older than <td>",
"    %cp        List file or directory parameters (default)",
"    %cP pnstaw List with a specified format (no default)",
"    %cq        Don't report failures",
"    %cQ        Quote filenames with whitespace [on]",
"    %cr        List at the requested output rate",
"    %cR N      Use N as the requested output rate, (default 25 lps)",
"    %cs        List system files and/or directories",
"    %cS        List only system files and/or directories",
"    %ct        List file and byte count totals",
"    %cT        List file time as raw UNIX time",
"    %cu        List in upper case",
"    %cv        Verbose output (usually used for debugging",
"    %cV[v]     List (only) the volume identification [no legend]",
"    %cw        List in wide mode, using the default",
"    %cW N      List in wide mode, using N columns",
"    %cx        List extended datetime information",
"    %cX <pathspec> e/X/clude (possibly wild) paths matching pathspec",
"    %cX @<xfile>   e/X/clude files that match pathspec(s) in xfile",
"    %cX-       Disable default file exclusion(s)",
"    %cX+       Show exclusion path(s)",
"    %cX=       Show excluded path(s)",
"    %cy <td>   List only if younger than <td>",
"    %cz        Exit with zero return code even if not found",
"",
"Copyright (c) 1985, 1993 by J & M Software, Dallas TX - All Rights Reserved",
NULL
};

/* ----------------------------------------------------------------------- */

#define		FW_NORM	   (FW_FILE | FW_DIR)	// Default search type
#define		DEFNAMESIZE  (40)				// The default name field size

#define		NULCH	('\0')

/* ----------------------------------------------------------------------- */

int		defflg = TRUE;					/* Use the defaults flag */
int		b_flag = FALSE;					/* List lengths in hexadecimal flag */
int		c_flag = FALSE;					/* List all files/directories flag */
int		e_flag = TRUE;					/* Existence check flag */
int		E_flag = FALSE;					/* Don't do automatic directory expansion */
int		H_flag = FALSE;					/* List only hidden files and/or directories */
int		i_flag = FALSE;					/* Take pathnames from stdin (automatic) */
int		l_flag = FALSE;					/* Lower case flag */
int		m_flag = FALSE;					/* List in "more" mode */
int		o_flag = FALSE;					/* List older than <td> flag */
int		p_flag = FALSE;					/* List parameters flag */
int		r_flag = FALSE;					/* List at requested rate flag */
int		q_flag = FALSE;					/* Quiet errors flag */
int		S_flag = FALSE;					/* List only hidden files and/or directories */
int		t_flag = FALSE;					/* File and byte total flag */
int		u_flag = FALSE;					/* Upper case flag */
int		V_flag = 0;						/* Show volume name */
int		w_flag = FALSE;					/* Wide listing mode flag */
int		x_flag = 0;						/* Extended datetime */
int		y_flag = FALSE;					/* List younger than <td> flag */
int		z_flag = FALSE;					/* Return zero even if failure */

int		verbose		= 0;				/* Show verbose output */
int		rawTimeFlag	= FALSE;			/* Show time/date as raw time */
int		LoopEnb		= TRUE;				/* Run the process loop */
int		DispEnb		= TRUE;				/* General display control */

PHP		hp		 = NULL;				/* Pointer to the fWild instance */
PEX		xp		 = NULL;				/* Pointer to the exclusion instance */
int		smode	 = FW_NORM;				/* File search mode attributes */

int		namesize = DEFNAMESIZE;			/* Length of the name field */
int		colsize	 = 5;					/* The number of display columns */
int		rowsize	 = 25;					/* The number of display rows */

int		outrate	 = 25;					/* Requested output rate */
UINT64	btotal = 0;						/* Total byte count */
UINT32	ftotal = 0;						/* Total file count */
UINT32	dtotal = 0;						/* Total directory count */

int		exitcode = 0;					/* Exit code - assume success */
int		itemcode = 0;					/* Exit code for each item */

int		QuoteFlag = 1;					/* Quote filenames with spaces */

time_t	oldertime = 0L;					/* The older-than time */
time_t	youngertime = 0L;				/* The younger-than time */

static	char	pSrch[MAX_PATH];		// Buffer for the search path

/* ----------------------------------------------------------------------- */

#define	 SETEXIT(n)	  {if (exitcode < (n)) exitcode = (n);}

static	void	ProcessLoop (const char *);
static	void	Process (char *);
static	int		timebound (void);
static	void	VolPrint (const char *);
static	void	listTotal	(void);
static	char   *UseStdin (void);
static	void	delay_init (void);
static	int		test_attr (int attr);
static	void	set_attr (char *s);
static	void	set_attr_default (void);

extern	void	fdpr_init		(void);
extern	int		fdpr_format		(char *);
extern	void	fdpr			(viod);
extern	void	fdpr_complete	(void);

/* ----------------------------------------------------------------------- */
	void
main (
	int	   argc,						/* Argument count */
	char  *argv [])						/* Argument list pointer */

	{
	int	   option;						/* Option character */
	long   ltemp;						/* Used for optvalue() */
	char  *ap;							/* Argument pointer */

static	char   *optstring = "?aA:bcC:dDeEfF:hHlL:mM:nN:o:O:pP:qQrR:sStTuUvwW:VxX:y:Y:zZ";


	if ((hp = fwOpen()) == NULL)
		exit(1);
	if ((xp = fExcludeOpen()) == NULL)
		exit(1);


//	setbuf(stdout, fmalloc(BUFSIZ));
	optenv = getenv("L");

	while ((option = getopt(argc, argv, optstring)) != EOF)
		{
		switch (tolower(option))
			{
			case 'a':
				if (option == 'A')
					set_attr(optarg);
				else // (option == 'a')
					{
					set_attr("A");
					}
				break;

			case 'b':
				b_flag = TRUE;
				break;

			case 'c':
				if (option == 'C')
					{
					if (optvalue(optarg, &ltemp, 1, 256))
						{
						printf("Column size parm error - %s\n", optvalerror());
						usage();
						}
					colsize = (int)(ltemp);
					break;
					}
				else // (option == 'c')
					{
//					defflg  = FALSE;
					LoopEnb = TRUE;
					DispEnb = TRUE;
					c_flag  = TRUE;
					p_flag  = TRUE;
					t_flag  = TRUE;
					V_flag  = 1;
					set_attr("H+S+A+R");
					}
				break;

			case 'd':
				if (option == 'D')
					{
					smode |=  FW_AD;
					}
				else // (option == 'd')
					{
					smode |=  FW_DIR;
					smode &= ~FW_FILE;
					}
				break;

			case 'e':
				if (option == 'E')
					E_flag = TRUE;
				else
					e_flag = FALSE;
				break;

			case 'f':
				if (option == 'F')
					{
					if (fdpr_format(optarg))
						{
						printf("Format parm error\n");
						usage();
						}
					}
				else // (option == 'f')
					{
					smode |=  FW_FILE;					}
					smode &= ~FW_DIR;
				break;

			case 'h':
				if (option == 'H')
					H_flag = TRUE;
				set_attr("H+*");
				break;

			case 'l':
				if (option == 'L')
					{
					if (optvalue(optarg, &ltemp, 1, 256))
						{
						printf("Row size parm error - %s\n", optvalerror());
						usage();
						}
					rowsize = (int)(ltemp);
					if (rowsize == 1)
						rowsize = 2;
					}
				else // (option == 'l')
					{
					l_flag = TRUE;
					u_flag = FALSE;
					}
				break;

			case 'm':
				if (option == 'M')
					{
					if (optvalue(optarg, &ltemp, 1, 256))
						{
						printf("Row size parm error - %s\n", optvalerror());
						usage();
						}
					rowsize = (int)(ltemp);
					if (rowsize == 1)
						rowsize = 2;
					}
				m_flag = TRUE;
				break;

			case 'n':
				if (option == 'N')
					{
					if (optvalue(optarg, &ltemp, 1, 256))
						{
						printf("Name size parm error - %s\n", optvalerror());
						usage();
						}
					namesize = (int)(ltemp);
					}
				else // (option == 'n')
					{
					DispEnb = FALSE;
					defflg  = FALSE;
					}
				break;

			case 'o':
				if ((oldertime = fwsgettd(optarg)) == 0)
					{
					printf("Older time - %s\n", fwserrtd());
					usage();
					}
				o_flag = TRUE;
				break;

			case 'p':
				if ((option == 'P')
				&&	(fdpr_format(optarg)))
					{
					printf("Format parm error\n");
					usage();
					}
				p_flag = TRUE;
				break;

			case 'q':
				if (option == 'Q')
					QuoteFlag =	 1 - QuoteFlag;
				else
					q_flag = 1 - q_flag;
				break;

			case 'r':
				if (option == 'R')
					{
					if (optvalue(optarg, &ltemp, 1, 256))
						{
						printf("Rate size parm error - %s\n", optvalerror());
						usage();
						}
					outrate = (int)(ltemp);
					}
				else
					r_flag = TRUE;
				break;

			case 's':
				if (option == 'S')
					S_flag = TRUE;
				set_attr("S+*");
				break;

			case 't':
				if (option == 'T')
					{
					rawTimeFlag = TRUE;
					}
				else
					{
					defflg  = FALSE;
					LoopEnb = TRUE;
					DispEnb = FALSE;
					t_flag  = TRUE;
					}
				break;

			case 'u':
				l_flag = FALSE;
				u_flag = TRUE;
				break;

			case 'v':
				if (option == 'V')
					{
					defflg  = FALSE;
					LoopEnb = FALSE;
					DispEnb = FALSE;
					V_flag  = TRUE;
					}
				else
					{
					++verbose;
					}
				break;

			case 'w':
				if (option == 'W')
					{
					if (optvalue(optarg, &ltemp, 1, 256))
						{
						printf("Column size parm error - %s\n", optvalerror());
						usage();
						}
					colsize = (int)(ltemp);
					}
				w_flag = TRUE;
				break;

			case 'x':
				if (option == 'x')					// (lower case only)
					{
//printf("lower X\n");
					x_flag = TRUE;					// Show datetime info
					}
				else // (option == 'X')				// (upper case only)
					{
//printf("upper X\n");
//printf("optarg %s\n", optarg);

					if (optarg[0] == '-')			// (Upper case)
						fExcludeDefEnable(xp, FALSE);	// Disable default file exclusion(s)
					else if (optarg[0] == '+')
						fExcludeShowConf(xp, TRUE);		// Enable stdout of exclusion(s)
					else if (optarg[0] == '=')
						fExcludeShowExcl(xp, TRUE);		// Enable stdout of excluded path(s)
					else if (fExclude(xp, optarg))
						printf("Exclusion string fault: \"%s\"\n", optarg);
					}
				break;

			case 'y':
				if ((youngertime = fwsgettd(optarg)) == 0)
					{
					printf("Younger time - %s\n", fwserrtd());
					usage();
					}
				y_flag = TRUE;
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

	i_flag = ( ! isatty(fileno(stdin)));

	if	( ! isatty(fileno(stdout)))
		m_flag = FALSE;
	
	if (defflg)
		{
		p_flag = TRUE;
		}

	fdpr_init();
	delay_init();

	if (i_flag)
		{
		while ((ap = UseStdin()) != NULL)				// Process the stdin list
			{
			if (V_flag > 0)					// Process the volume name
				VolPrint(ap);
			if (LoopEnb)
				{
				if (verbose > 2)
					printf("(STDIN) Pattern: \"%s\"\n", ap);
				ProcessLoop(ap);
				}
			}
		}

	else if (optind >= argc)				// if no filespec provided, use default ""
		{
		ProcessLoop("");
		}

	else		 							
		{
		while (optind < argc)				// Process the command line list
			{
			ap = argv[optind++];
			if (V_flag > 0)					// Process the volume name
				VolPrint(ap);
			if (LoopEnb)
				{
				if (verbose > 2)
					printf("(CMD) Pattern: \"%s\"\n", ap);
				ProcessLoop(ap);
				}
			}
		}

	listTotal();
	xp = fExcludeClose(xp);					// Close the Exclusion instance
	hp = fwClose(hp);						// Close the fWild instance
	}

/* ----------------------------------------------------------------------- */
	static void
ProcessLoop (
	const char *pPattern)		// Search pathspec

	{
	char  *fnp;					// Found file name pointer

	pathCopy(pSrch, pPattern, MAX_COPY);		// Copy the pathspec

	if (verbose > 1)
		printf("L1 Pattern: \"%s\"\n", pSrch);

// BWJ E_flag may be worthless
//	if ((*pSrch != '\0')  &&  fnchkdir(pSrch)  &&  (! E_flag))
//		pathCat(pSrch, "*", MAX_COPY);

	if (verbose > 0)
printf("L2 Pattern: \"%s\"\n", pSrch);

	if (fwInit(hp, pSrch, smode) != FWERR_NONE)	// Process the pattern
		fwInitError(pSrch);
	fExcludeConnect(xp, hp);					// Connect the exclusion instance
	while (fnp = fWild(hp))						// Process each filespec
		Process(fnp);

	if ((itemcode != 0)
	&& (( ! t_flag)  &&	 ( ! q_flag)))
		cantfind(pSrch);						// Couldn't find any

	SETEXIT(itemcode);
	}
	
/* ----------------------------------------------------------------------- */
	static void
Process (fnp)					/* Process one filename */
	char  *fnp;					/* Pointer to the file name */ 

	{
	int		type = fwtype(hp);
	UINT64	size = fwsize(hp);

	if (verbose > 1)
		{
printf("L: Found: \"%s\"\n", fnp);
printf("L: Attr:  \"%s\"\n", showFileAttr(type));
		}

	if ((H_flag && ! (type & ATT_HIDDEN))	// Special cases
	||  (S_flag && ! (type & ATT_SYSTEM)))
		return;

	if (type & ATT_DIR) 
		{
		if (c_flag || ( ! fndot(fnp))  &&  test_attr(type)  &&  timebound())
			{
			++dtotal;
			itemcode = 0;
			if (e_flag && DispEnb)	// If file/directory listing is enabled
				fdpr();
			}
		}

	else /* a file */
		{
		if (test_attr(type)	 &&	 timebound())
			{
			++ftotal;
			btotal += size;
			itemcode = 0;
			if (e_flag && DispEnb)	// If file/directory listing is enabled
				fdpr();
			}
		}
	}

/* ----------------------------------------------------------------------- */
	static int
timebound (void)

	{
	time_t  t;

	if (o_flag	||	y_flag	||	(x_flag > 0))
		t = fwgetfdt(hp);

	if (x_flag > 0)
		{
		printf("Datetime: %s", asctime(localtime(&t)));
		if (x_flag > 1)
			printf("Datetime: %lld\n", t);
		if (z_flag)
			{
			if (youngertime != 0L)
				printf("Younger:  %s", asctime(localtime(&youngertime)));
			if (oldertime != 0L)
				printf("Older:    %s", asctime(localtime(&oldertime)));
			}
		}

	return (((y_flag == FALSE)	||	(t >= youngertime))
		&&	((o_flag == FALSE)	||	(t <= oldertime)));
	}

/* ----------------------------------------------------------------------- */
	static void
VolPrint (						// Get and print the volume name
	const char  *pPath)			// Pointer to the pathspec

	{
	char  *vnp;

	if (verbose)
printf("VolPrint path; \"%s\"\n", pPath);

	printf("Volume name: ");

	if ((vnp = volName(pPath)) != NULL)
		printf("%s\n", vnp);
	else
		{
		SETEXIT(1);						/* Exit code - show failure */
		printf("<invalid>\n");
		}

	if (verbose)
printf("VolPrint exit\n");
	fflush(stdout);
	}

/* ----------------------------------------------------------------------- */
	static void
listTotal ()

	{
	if (t_flag)
		{
		printf("\nTotal of %I64u bytes in %lu files", btotal, ftotal);
		printf(" and %lu directories", dtotal);
		printf("\n");
		fflush(stdout);
		}
	}

/* ----------------------------------------------------------------------- */
	static char *
UseStdin ()						/* Parse pathnames from stdin */

	{
static	char  line [MAX_PATH];

	if (fgets(line, MAX_COPY, stdin) == NULL)
		return (NULL);

	// Truncate the line ending

	char *pStr;
	char  ch;

	pStr = line;
	while ((ch = *pStr) != NULCH)
		{
		if ((ch == '\r') || (ch == '\n'))
			{
			*pStr = NULCH;
			break;
			}
		++pStr;
		}
	
	return (line);
	}

/* ----------------------------------------------------------------------- *\
|  Rate-controlled output section
\* ----------------------------------------------------------------------- */

#define	 LOWRATE			5					// Minimum rate value
#define	 STOPLIMIT		(1000L / (LOWRATE + 1)) // Threshold for detecting XOFF
#define	 THRESHOLD		   25L					// Threshold for requesting delay

static	long	delta;
static	long	told	  = 0L;
static	long	tnew;
static	long	sumold	  = 0L;
static	long	sum		  = 0L;
static	long	increment = 0L;
static	long	dummy	  = 0L;
static	long	factor	  = 40L;
static	long	accum	  = 0L;
//static		int		starting  = TRUE;

static	void	timekill (void);

/* ----------------------------------------------------------------------- */
	static void
delay_init (void)

	{
	if (r_flag)
		{
		if (outrate < LOWRATE)
			outrate = LOWRATE;
		if (outrate < 1000)
			{
			increment = (1000L / (long)(outrate));
			sum = 0L;
			}
		else
			{
			r_flag	= FALSE;
			outrate = 0;
			}
		}
	}

/* ----------------------------------------------------------------------- */
	void
delay (void)

	{
	if (r_flag)
		{
		tnew = clock();
		delta = tnew - told;
		if (delta > STOPLIMIT)
			sum = 0L;
		else
			sum += (increment - delta);
		accum += sum + sum - sumold;
		timekill();
//		printf("%ld\n", accum);
		told   = tnew;
		sumold = sum;
		}
	}

/* ----------------------------------------------------------------------- */
	static void
timekill (void)

	{
	long  count;

	for (count = (factor * accum); count > 0L; --count)
		dummy += (count + factor) + (dummy);
	}

/* ----------------------------------------------------------------------- *\
|  Attribute testing mechanism definitions
\* ----------------------------------------------------------------------- */

#define	 ALIST_MAX	(32)

typedef
	struct
		{
		int	 mask;
		int	 value;
		}  ATTR_LIST;

static	ATTR_LIST	attr_list [ALIST_MAX] = {0};
static	ATTR_LIST  *attr_limit			  = &attr_list[0];

/* ----------------------------------------------------------------------- */
	static int			/* Returns TRUE if accepted */
test_attr (
	int	 attr)			/* The attribute of the file */

	{
	int			result = TRUE;
	ATTR_LIST  *p	   = &attr_list[0];


	for (p = &attr_list[0]; p != attr_limit; ++p)
		{
		result = FALSE;
		if ((attr & p->mask) == p->value)
			{
			result = TRUE;
			break;
			}
		}

	return (result);
	}

/* ----------------------------------------------------------------------- */
	static void
set_attr_default (void)

	{
	if (attr_limit == &attr_list[0])
		set_attr("hs");
	}

/* ----------------------------------------------------------------------- */
	static void
set_attr (
	char *str)			/* Pointer to the attribute expression string */

	{
	char		ch;
	int			result = 0;
	int			flag   = FALSE;
	char	   *s	   = str;
	ATTR_LIST  *p	   = attr_limit;


	while (ch = *(s++))
		{
		if ((ch == '+')	 ||	 (ch == '|'))
			{
			if (flag)
				{
				if (p++ == &attr_list[ALIST_MAX - 1])
					{
					printf("Attribute option table overflow: \"%s\"\n", str);
					usage();
					}
				flag = FALSE;
				}
			}

		else
			{
			switch (ch)
				{
				case '*':
					p->value &= ~(ATT_HIDDEN | ATT_SYSTEM);
					p->mask	 |=	 (ATT_HIDDEN | ATT_SYSTEM);
					break;

				case 'A':
					p->value |= ATT_ARCH;
				case 'a':
					p->mask	 |= ATT_ARCH;
					break;

				case 'H':
					p->value |= ATT_HIDDEN;
					smode    |=  FW_HIDDEN;
				case 'h':
					p->mask	 |= ATT_HIDDEN;
					break;

				case 'R':
					p->value |= ATT_RONLY;
				case 'r':
					p->mask	 |= ATT_RONLY;
					break;

				case 'S':
					p->value |= ATT_SYSTEM;
					smode    |=  FW_SYSTEM;
				case 's':
					p->mask	 |= ATT_SYSTEM;
					break;

				default:
					printf("Attribute option syntax error: \"%s\"\n", str);
					usage();
				}

			if ( ! flag)
				{
				attr_limit = p + 1;
				flag = TRUE;
				}
			}
		}
	}

/* ----------------------------------------------------------------------- */
