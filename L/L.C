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
|
\* ----------------------------------------------------------------------- */

#include  "windows.h"

#include  "stdio.h"
#include  "stdlib.h"
#include  "string.h"
#include  "ctype.h"
#include  <io.h>
#include  "time.h"

#include  "fwild.h"
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
"    %cc        List (comprehensively) all files and directories",
"    %cC N      Use N columns for wide listings (default 5 columns)",
"    %cd        List directories (default)",
"    %ce        Check only for existence of the files",
"    %cf        List files (default)",
"    %cF pnstaw Determine the list format (default \"psta\")",
"    %ch        List hidden files",
"    %cH        List file lengths in hexadecimal",
"    %ci        Take pathnames from stdin",
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
"    %cs        List system files",
"    %ct        List file and byte count totals",
"    %cu        List in upper case",
"    %cv[v]     List the volume identification [no legend]",
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

#define	  FW_NORM	   (FW_FILE | FW_SUBD)
#define	  DEFNAMESIZE  (40)				/* The default name field size */

/* ----------------------------------------------------------------------- */

int		defflg = TRUE;					/* Use the defaults flag */
int		c_flag = FALSE;					/* List all files/directories flag */
int		d_flag = FALSE;					/* List directories flag */
int		e_flag = TRUE;					/* Existence check flag */
int		f_flag = FALSE;					/* List files flag */
int		h_flag = FALSE;					/* List lengths in hexadecimal flag */
int		i_flag = FALSE;					/* Take pathnames from stdin */
int		l_flag = FALSE;					/* Lower case flag */
int		m_flag = FALSE;					/* List in "more" mode */
int		o_flag = FALSE;					/* List older than <td> flag */
int		p_flag = FALSE;					/* List parameters flag */
int		r_flag = FALSE;					/* List at requested rate flag */
int		q_flag = FALSE;					/* Quiet errors flag */
int		t_flag = FALSE;					/* File and byte total flag */
int		u_flag = FALSE;					/* Upper case flag */
int		v_flag = 0;						/* Volume name flag */
int		w_flag = FALSE;					/* Wide listing mode flag */
int		x_flag = 0;						/* Extended datetime */
int		y_flag = FALSE;					/* List younger than <td> flag */
int		z_flag = FALSE;					/* Return zero even if failure */

void   *hp	   = NULL;					/* Pointer to wild name data block */

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

__time32_t	oldertime = 0L;				/* The older-than time */
__time32_t	youngertime = 0L;			/* The younger-than time */

/* ----------------------------------------------------------------------- */

#define	 SETEXIT(n)	  {if (exitcode < (n)) exitcode = (n);}

static	void	process (char *);
static	void	volprnt (char *);
static	void	finish	(void);
static	char   *stdpath (void);
static	void	delay_init (void);
static	int		test_attr (int attr);
static	void	set_attr (char *s);
static	void	set_attr_default (void);

extern	void	fdpr_init		(void);
extern	int		fdpr_format		(char *);
extern	void	fdpr			(char *fnp, int type, UINT64 size);
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
	char  *fnp = NULL;					/* Input file name pointer */

static	char   *fargv [] = { "" };		/* Fake argv array */
static	char   *optstring = "?aA:cC:dDeEfF:hHiIlL:mM:nN:o:O:pP:qQrR:sStTuUvwW:VxX:y:Y:zZ";


	setbuf(stdout, fmalloc(BUFSIZ));
	optenv = getenv("L");

	while ((option = getopt(argc, argv, optstring)) != EOF)
		{
		switch (tolower(option))
			{
			case 'a':
				if (option == 'A')
					set_attr(optarg);
				else
					{
					set_attr("A");
					defflg = FALSE;
					}
				f_flag = TRUE;
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
				else
					{
					defflg = FALSE;
					c_flag = TRUE;
					d_flag = TRUE;
					f_flag = TRUE;
					p_flag = TRUE;
					t_flag = TRUE;
					v_flag = 1;
					set_attr("H+S+A+R");
					}
				break;

			case 'd':
				defflg = FALSE;
				d_flag = TRUE;
				break;

			case 'e':
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
				else
					{
					defflg = FALSE;
					f_flag = TRUE;
					}
				break;

			case 'h':
				if (option == 'H')
					h_flag = !h_flag;
				else
					set_attr("H+*");
				break;

			case 'i':
				i_flag = TRUE;
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
				else
					{
					l_flag = ! l_flag;
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
				else
					defflg = FALSE;
				break;

			case 'o':
				if ((oldertime = fwsgettd(optarg)) < 0L)
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
				set_attr("S+*");
				break;

			case 't':
				defflg = FALSE;
				t_flag = TRUE;
				break;

			case 'u':
				l_flag = FALSE;
				u_flag = ! u_flag;
				break;

			case 'v':
				defflg = FALSE;
				++v_flag;
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
				w_flag = ! w_flag;
				break;

			case 'x':
				if (option == 'x')					// (lower case only)
					{
//printf("lower X\n");
					++x_flag;						// Show datetime info
					}
				else // (option == 'X')				// (upper case only)
					{
//printf("upper X\n");
//printf("optarg %s\n", optarg);

					if (optarg[0] == '-')			// (Upper case)
						fexcludeDefEnable(FALSE);		/* Disable default file exclusion(s) */
					else if (optarg[0] == '+')
						fexcludeShowConf(TRUE);			/* Enable stdout of exclusion(s) */
					else if (optarg[0] == '=')
						fexcludeShowExcl(TRUE);			/* Enable stdout of excluded path(s) */
					else if (fexclude(optarg))
						printf("Exclusion string fault: \"%s\"\n", optarg);
					}
				break;

			case 'y':
				if ((youngertime = fwsgettd(optarg)) < 0L)
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

	if (( ! isatty(fileno(stdin)))
	||	( ! isatty(fileno(stdout))))
		m_flag = FALSE;

	set_attr_default();
	if (defflg)
		{
		d_flag = TRUE;
		f_flag = TRUE;
		p_flag = TRUE;
		}

	fdpr_init();
	delay_init();
	if (i_flag)
		{
		while (ap = stdpath())						/* Process the stdin list */
			{
			itemcode = 1;
			if (v_flag > 0)							/* Process the volume name */
				volprnt(ap);
			if ((hp = fwinit(ap, smode)) == NULL)	/* Process the input list */
				fwinitError(ap);
			fwExclEnable(hp, TRUE);					/* Enable file exclusion */
			while (fnp = fwild(hp))					/* Process each filespec */
				process(fnp);
			hp = NULL;

			if ((itemcode != 0)
			&& ((f_flag | d_flag | t_flag)	&&	( ! q_flag)))
				cantfind(ap);						/* Couldn't find any */

			SETEXIT(itemcode);
			}
		}

	else											/* Process the command line */
		{
		if (optind >= argc)				// if no filespec provided, use default "*"
			{
			optind = 0;
			argc   = 1;
			argv   = fargv;
			}

		while (optind < argc)
			{
			ap = argv[optind++];
			itemcode = 1;
			if (v_flag > 0)							/* Process the volume name */
				volprnt(ap);

//printf("L Pattern: \"%s\"\n", ap);
//fflush(stdout);

			if ((hp = fwinit(ap, smode)) == NULL)	/* Process the input list */
				fwinitError(ap);
			fwExclEnable(hp, TRUE);					/* Enable file exclusion */
			while (fnp = fwild(hp))					/* Process each filespec */
				process(fnp);
			hp = NULL;

			if ((itemcode != 0)
			&& ((f_flag | d_flag | t_flag)	&&	( ! q_flag)))
				cantfind(ap);						/* Couldn't find any */

			SETEXIT(itemcode);
			}
		}

	fdpr_complete();
	finish();
	if (x_flag > 0)
		printf("Exit code: %d\n", exitcode);
	exit((z_flag) ? (0) : (exitcode));
	}

/* ----------------------------------------------------------------------- */
	static int
timebound (void)

	{
	__time32_t  t;

	if (o_flag	||	y_flag	||	(x_flag > 0))
		t = fwgetfdt(hp);

	if (x_flag > 0)
		{
		printf("Datetime: %s", asctime(_localtime32(&t)));
		if (x_flag > 1)
			printf("Datetime: %ld\n", t);
		if (z_flag)
			{
			if (youngertime != 0L)
				printf("Younger:  %s", asctime(_localtime32(&youngertime)));
			if (oldertime != 0L)
				printf("Older:    %s", asctime(_localtime32(&oldertime)));
			}
		}

	return (((y_flag == FALSE)	||	(t >= youngertime))
		&&	((o_flag == FALSE)	||	(t <= oldertime)));
	}

/* ----------------------------------------------------------------------- */
	static void
process (fnp)					/* Process one filename */
	char  *fnp;					/* Pointer to the file name */ 

	{
	int		type;
	UINT64	size;


//printf("L: Found: \"%s\"\n", fnp);
//fflush(stdout);

	type = fwtype(hp);
	size = fwsize(hp);

	if (type & ATT_SUBD) 
		{
		if (c_flag || ( ! fndot(fnp))  &&  test_attr(type)  &&  timebound())
			{
			++dtotal;
			itemcode = 0;
			if (d_flag	&&	e_flag)
				fdpr(fnp, type, size);
			}
		}

	else /* a file */
		{
		if (test_attr(type)	 &&	 timebound())
			{
			++ftotal;
			btotal += size;
			itemcode = 0;
			if (f_flag	&&	e_flag)
				fdpr(fnp, type, size);
			}
		}
	}

/* ----------------------------------------------------------------------- */
	static void
volprnt (s)						/* Get and process the volume name */
	char  *s;					/* Pointer to the argument */ 

	{
	char  *vnp;
	char  *stpchr();

// vnp = vol_name(s);
// printf("Source string: %s\n", s);
// printf("vnp:           %04X\n", vnp);
// printf("vnp[0]:        %02X\n", *vnp);

	if (v_flag == 1)
		printf("Volume name: ");

	if (vnp = vol_name(s))
		printf("%s\n", vnp);
	else
		{
		SETEXIT(1);						/* Exit code - show failure */
		printf("<None>\n");
		}
	fflush(stdout);
	}

/* ----------------------------------------------------------------------- */
	static void
finish ()

	{
	if (t_flag)
		{
		printf("\nTotal of %I64u bytes in %lu files", btotal, ftotal);
		if (d_flag || ( ! f_flag))
			printf(" and %lu directories", dtotal);
		printf("\n");
		fflush(stdout);
		}
	}

/* ----------------------------------------------------------------------- */
	static char *
stdpath ()						/* Parse pathnames from stdin */

	{
	int	 ch;
	char  *p;
static	int	  eofflag = FALSE;
static	char  line [81];

	line[0] = '\0';
	if ( ! eofflag)
		{
		for (;;)
			{
			ch = getchar();
			if (ch == EOF)
				{
				eofflag = TRUE;
				break;
				}
			if (isgraph(ch))
				break;
			}
		}

	if ( ! eofflag)
		{
		for (p = &line[0]; ; )
			{
			if (p >= &line[80])
				{
				line[0] = '\0';
				break;
				}
			*(p++) = (char)(ch);
			*p = '\0';
			ch = getchar();
			if ( ! isgraph(ch))
				{
				if (ch == EOF)
					eofflag = TRUE;
				break;
				}
			}
		}

	p = (line[0] != '\0') ? (&line[0]) : (NULL);
	return (p);
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
