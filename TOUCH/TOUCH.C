/* ----------------------------------------------------------------------- *\
|
|				    TOUCH
|
|		    Copyright (c) 1985, 2012, all rights reserved
|				Brian W Johnson
|				   14-Jan-91
|				   31-Jan-93
|				   17-Aug-97
|				    6-Sep-98
|				   14-Aug-00 Long filename support
|				   22-Jul-12 Touch from a file's timestamp
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <ctype.h>
#include  <fcntl.h>
#include  <sys\types.h>
#include  <sys\stat.h>
#include  <time.h>
#include  <io.h>

#include  "fwild.h"

/* ----------------------------------------------------------------------- */

char  *usagedoc [] =
{
"Usage:  touch  [%c?cfhiloqrstXyz]  file_list  [>output_file]",
"",
"touch updates the modification date/time of each file in the",
"file_list to the current (or zero) date/time.",
"",
"    %cc       creates the file 0 length (if nonexistent)",
"    %ch       touches hidden files also",
"    %ci       ignores errors, returns zero exit code",
"    %cl       lists file names as they are touched/created",
"    %co <td>  touches only if file is older than time specified by <td>",
"    %cq       queries before touching",
"    %cr       touches readonly files also",
"    %cs       touches system files also",
"    %ct <td>  touches to time specified by <td>",
"    %cf <pathname> touches to the timestamp of the file <pathname>",
"    %cX <pathspec> e/X/clude (possibly wild) files matching pathspec",
"    %cX @<xfile>   e/X/clude files that match pathspec(s) in xfile",
"    %cX-      disable default file exclusion(s)",
"    %cX+      show exclusion path(s)",
"    %cy <td>  touches only if file is younger than time specified by <td>",
"    %cz       backdates the files to 1-Jan-1980",
"",
"Copyright (c) 1985-1998 by J & M Software, Dallas TX - All Rights Reserved",
NULL
};

/* ----------------------------------------------------------------------- */

__time32_t	touchtime   = 0L;	/* The time to touch to */
__time32_t	oldertime   = 0L;	/* The older time bound */
__time32_t	youngertime = 0L;	/* The younger time bound */

char	swch = '-';		/* The switch character */

int	c_flag = FALSE;		/* Create if non-existent flag */
int	f_flag = FALSE;		/* touch time file used for input */
int	i_flag = FALSE;		/* Ignore errors in exit code flag */
int	l_flag = FALSE;		/* List file names flag */
int	q_flag = FALSE;		/* Query flag */
int	r_flag = FALSE;		/* Read-only flag */

int	exitcode = 0;


static	void	trycreate (char *);
static	void	process   (char *, int);
static	void	touch     (char *);

/* ----------------------------------------------------------------------- */
	void
main (
	int    argc,			/* Argument count */
	char  *argv [])			/* Argument list pointer */

	{
	int    smode = FW_FILE;		/* File search mode attributes */
	long   t;				/* DATETIME of the file */
	int    option;			/* Option character */
	void  *hp   = NULL;			/* Pointer to wild file data block */
	char  *ap;				/* Argument pointer */
	char  *fnp  = NULL;			/* Input file name pointer */

static	char   *optstring = "?cCf:F:hHiIlLo:O:qQrRsSt:T:X:y:Y:zZ";


	swch   = egetswch();
	optenv = getenv("TOUCH");

	while ((option = getopt(argc, argv, optstring)) != EOF)
		{
		switch (tolower(option))
			{
			case 'c':
				c_flag = TRUE;
				break;

			case 'h':
				smode |= FW_HIDDEN;
				break;

			case 'i':
				++i_flag;
				break;

			case 'l':
				++l_flag;
				break;

			case 'f':
				++f_flag;
				touchtime = fgetfdt(optarg);
				if (touchtime == -1L)
					{
					exitcode = 1;
					printf("\7Time file not found - %s\n", fwserrtd());
					usage();
					}
				break;

			case 'o':
				if ((oldertime = fwsgettd(optarg)) < 0L)
					{
					exitcode = 1;
					printf("\7Older time - %s\n", fwserrtd());
					usage();
					}
				break;

			case 'q':
				++q_flag;
				break;

			case 'r':
				r_flag = TRUE;
				break;

			case 's':
				smode |= FW_SYSTEM;
				break;

			case 't':
				if ((touchtime = fwsgettd(optarg)) < 0L)
					{
					exitcode = 1;
					printf("\7Touch time - %s\n", fwserrtd());
					usage();
					}
				break;

			case 'x':
printf("option = %c\n", option);

				if (option == 'x')
					usage();

				if      (optarg[0] == '-')
					fexcludeDefEnable(FALSE);		/* Disable default file exclusion(s) */
				else if (optarg[0] == '+')
					fexcludeShowExcl(TRUE);			/* Enable stdout of exclusion(s) */
				else if (fexclude(optarg))
					{
					exitcode = 1;
					printf("\7Exclusion string fault: \"%s\"\n", optarg);
					usage();
					}
				break;

			case 'y':
				if ((youngertime = fwsgettd(optarg)) < 0L)
					{
					exitcode = 1;
					printf("\7Younger time - %s\n", fwserrtd());
					usage();
					}
				break;

			case 'z':
				touchtime = zerotime();
				break;

			case '?':
				help();

			default:
				usage();
			}
		}

	if (optind >= argc)
		usage();

	if (touchtime == 0L)		/* Use current time as the default */
		_time32(&touchtime);

	while (optind < argc)
		{
		ap = argv[optind++];
		hp = fwinit(ap, smode);		/* Process the input list */
		if ((fnp = fwildexcl(hp)) != NULL)
			{
			do  {			/* Process one filespec */
				t = fwgetfdt(hp);
				if (((youngertime == 0L)  ||  (t >= youngertime))
				&&  ((oldertime   == 0L)  ||  (t <= oldertime)))
					process(fnp, FALSE);
				} while (fnp = fwildexcl(hp));
			}
		else if (c_flag)
			trycreate(ap);
		else
			{
			if (i_flag == FALSE)
				exitcode = 1;
			cantfind(ap);
			}
		}

	exit (exitcode);
	}

/* ----------------------------------------------------------------------- */
	static void
trycreate (				/* Attempt to create the file */
	char  *fnp)			/* Input file name */

	{
	int  fh;


	strupr(fnp);
	if (iswild(fnp))
		{
		exitcode = 1;
		printf("\7Unable to create wild filename: %s\n", fnp);
		}
	else if ((fh = popen(fnp, (O_CREAT | O_EXCL), (S_IREAD | S_IWRITE))) > 0)
		{
		close(fh);
		process(fnp, TRUE);
		}
	else
		{
		if (i_flag == FALSE)
		exitcode = 1;
		printf("\7Unable to find or create file: %s\n", fnp);
		}
	}

/* ----------------------------------------------------------------------- */
	static void
process (			/* Process one input file */
	char  *fnp,			/* Input file name */ 
	int    cflag)		/* TRUE for a fresh creation */

	{
	char    inp [1024];


	if (q_flag)
		{
		printf("%-16s  [y/n] ? ", fnp);
		gets(&inp[0]);
		if (tolower(inp[0]) != 'y')
			goto exit;
		}
	else if (l_flag)
		printf("%s%s\n", fnp, ((cflag) ? (" (created)") : ("")));

	touch(fnp);

exit:;
	}

/* ----------------------------------------------------------------------- *\
|  Touch one input file
\* ----------------------------------------------------------------------- */
	static void
touch (
	char *fnp)			/* Input file name */ 

	{
	int      attr;
	int      Error = FALSE;


	if (r_flag)
		{
		if (((attr = fgetattr(fnp)) == -1)
			||  (((attr & ATT_RONLY) != 0)  &&  (fsetattr(fnp, (attr & ~ATT_RONLY)) == -1)))
				Error = TRUE;
		}

	else /* ( ! r_flag) */
		{
		if (((attr = fgetattr(fnp)) < 0)
		||  ((attr & ATT_RONLY) != 0)
		||  (fsetattr(fnp, attr) < 0))
			Error = TRUE;
		}

	if (Error)
		{
		if (i_flag == FALSE)
			exitcode = 1;
		printf("\7Cannot touch %s\n", fnp);
		goto exit;
		}


	if (fsetfdt(fnp, touchtime) != 0)
		{
		if (i_flag == FALSE)
			exitcode = 1;
		printf("\7Cannot touch %s\n", fnp);
		}


	if (r_flag  &&  ((attr & ATT_RONLY) != 0))
		{
		if (fsetattr(fnp, attr) < 0)
			{
			printf("\7Cannot touch %s\n", fnp);
			goto exit;
			}
		}

	exit:;
	}

/* ----------------------------------------------------------------------- */
