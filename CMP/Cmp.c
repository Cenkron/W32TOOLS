/* ----------------------------------------------------------------------- *\
|
|				      CMP
|
|	    Copyright (c) 1985, 1990, 2000, all rights reserved
|				Brian W Johnson
|				   18-Aug-90
|				   26-Dec-92
|				   31-Jan-93
|				   29-Jul-93
|				   29-Sep-96
|				    8-Mar-00 OS time incompatibility fix
|				   26-Mar-00 Quoteflag
|				    3-Apr-00 OS time fix moved to fgetfdt()
|				   18-Apr-00 Much faster operation
|				   26-Mar-01 Quiet option
|				    4-Nov-03 /p and /m options (for NTFS problems)
|				   29-Sep-07 for 64 bit file sizes
|
\* ----------------------------------------------------------------------- */

#include  <windows.h>

#include  <stdio.h>
#include  <stdlib.h>
#include  <ctype.h>
#include  <fcntl.h>
#include  <io.h>
#include  <string.h>

#include  "fwild.h"

#ifndef TRUE
#define FALSE	0
#define TRUE	1
#endif

char  *usagedoc [] =
{
"Usage:  cmp  [%c?abdehlLnoqQrstvXyz]  path1  [path2]  [>output_file]",
"",
"cmp compares files.  Various attributes and/or the data in one",
"or more pairs of files are compared.  Comparison differences",
"are noted to stdout.",
"",
"Path1 may be the name of a file, the name of a directory, or may",
"contain wildcards (\"?\" \"*\" \"**\").  In each case, a list of files to",
"be compared will be constructed, and all files specified by path1",
"will be compared with corresponding files specified by path2.  If",
"path2 is the name of a file, it will be used for all comparisons.",
"If path2 is the name of a directory, then path2 filenames will be",
"constructed from the path1 filenames.  If path1 is full recursive",
"(\"**\"), path2 filenames will correspond.  Path2 may not contain",
"any wildcards.  If path2 is not specified, it will default to \".\".",
"",
"    %ca      alarm beeps on error",
"    %cb      checks files bidirectionally (implies %ce)",
"    %cd      reports different data in the file pairs",
"    %ce      reports non-existence of the path2 file    (default)",
"    %ch      also checks hidden and system files",
"    %cs      reports different sizes of the file pairs  (default)",
"    %ct      reports different ages of the file pairs   (default)",
"    %cT nnn  Offset timestamp 1 by (+/-)nnn hours       (default 0)",
"    %cy      reports only \"younger\" files",
"    %co      reports only \"older\" files",
"    %cl      lists all file names as they are compared",
"    %cL      lists the path2 names as they are compared",
"    %cr      lists only file names of successful compares",
"    %cq      quiet mode (default off)",
"    %cQ      quotes file names with imbedded spaces (default on)",
"    %cn      lists all file names, but does not compare them",
"    %cu      use UNIX mode (text file translation, also uses -d)",
"    %cv      lists verbose information",
"    %cX <pathspec> e/X/cludes (possibly wild) files matching pathspec",
"    %cX @<xfile>   e/X/clude files that match pathspec(s) in xfile",
"    %cX-       Disable default file exclusion(s)",
"    %cX+       Show exclusion path(s)",
"    %cX=       Show excluded path(s)",
"    %cz      returns a zero completion code even if errors",
"",
"Copyright (c) 1988, 1993 by J & M Software, Dallas TX - All Rights Reserved",
NULL
};

/* ----------------------------------------------------------------------- */

int	defflg	= TRUE;			/* Use defaults flag */
int	a_flag	= FALSE;		/* Beep on error flag */
int	bi_flag	= FALSE;		/* Bidirectional flag */
int	c_flag	= TRUE;			/* Compare flag */
int	d_flag	= FALSE;		/* Data flag */
int	e_flag	= FALSE;		/* Exist flag */
int	o_flag	= FALSE;		/* Report only older flag */
int	s_flag	= FALSE;		/* Size flag */
int	t_flag	= FALSE;		/* Timedate flag */
int	b_flag	= TRUE;			/* List bad file names flag */
int	g_flag	= FALSE;		/* List good file names flag */
int	L_flag	= FALSE;		/* Use Long form when listing files */
int	q_flag	= FALSE;		/* quiet flag */
int	Q_flag	= TRUE;			/* quote names with spaces flag */
int	u_flag	= FALSE;		/* UNIX mode (text translation) */
int	v_flag	= 0;			/* Verbose information flag */
int	y_flag	= FALSE;		/* Report only younger flag */
int	z_flag	= FALSE;		/* Return zero even if error flag */

int	ex_code	= 0;			/* Exit code */

int	filetypes = FW_FILE;		/* File types checked */

long    timedelta = 0L;			/* Timestamp compare correction */
#define  OneHour (60L * 60L)		/* The correction value */

char	swch = '-';				/* The switch character */

char   *buff1;					/* Disk data buffer pointers */
char   *buff2;

char	buffer [BUFSIZ];		/* Buffer for stdout */

#define	DATASIZE	8192		/* Size of the disk data buffers */

#define	OPENMODE	((u_flag) ? (O_RDONLY) : (O_RDONLY | O_RAW))

#define	DC_EQUAL	 (0)		/* datacomp() bitmap values */
#define	DC_DIFFERENT (1)
#define	DC_SMALLER	 (2)
#define	DC_LARGER	 (4)


void	filepair1 (char *, char *);
void	filepair2 (char *, char *);
void	process   (char *, char *);
void	cantfind  (char *);
void	f_err     (char *);
int		putdiff   (int, char *);
int		datacomp  (int fd1, int fd2);
int		unixcomp  (int fd1, int fd2);
char   *qname     (char *);

/* ----------------------------------------------------------------------- */
	void
main (
	int    argc,
	char  *argv [])

	{
	int    option;			/* Option character */
	int    nargs;			/* Number of path arguments */
	char  *fnp1 = NULL;		/* Input file name pointer */
	char  *fnp2 = NULL;		/* Input file name pointer */

static	char   *optstring = "?aAbBdDeEhHlLnNoOqQrRsStT:TuUvVX:yYzZ";


	setbuf(stdout, buffer);
	swch   = egetswch();
	optenv = getenv("CMP");

	while ((option = getopt(argc, argv, optstring)) != EOF)
		{
		switch (tolower(option))
			{
			case 'a':
				a_flag = !a_flag;
				break;

			case 'b':
				bi_flag = !bi_flag;
				break;

			case 'd':
				d_flag = !d_flag;
				defflg = FALSE;
				break;

			case 'e':
				e_flag = !e_flag;
				defflg = FALSE;
				break;

			case 'h':
				filetypes |= (FW_HIDDEN | FW_SYSTEM);
				break;

			case 'l':
				if (option == 'l')
					{
					++b_flag;
					++g_flag;
					}
				else
					L_flag = !L_flag;
				break;

			case 'n':
				++b_flag;
				++g_flag;
				c_flag = FALSE;
				defflg = FALSE;
				break;

			case 'o':
				++o_flag;
				defflg = FALSE;
				break;

			case 'r':
				b_flag = FALSE;
				++g_flag;
				break;

			case 's':
				s_flag = !s_flag;
				defflg = FALSE;
				break;

			case 't':
				if (option == 't')
					{
					t_flag = !t_flag;
					defflg = FALSE;
					}
				else if (optarg != NULL) /* (option == 'T') */
					timedelta = OneHour * strtol(optarg, NULL, 10);
				break;

			case 'q':
				if (option == 'q')
					q_flag = !q_flag;
				else
					Q_flag = !Q_flag;
				break;

			case 'u':
				++u_flag;
				++d_flag;
				break;

			case 'v':
				++v_flag;
				break;

			case 'x':
				if (option == 'x')
					usage();

					if      (optarg[0] == '-')
						fexcludeDefEnable(FALSE);		/* Disable default file exclusion(s) */
					else if (optarg[0] == '+')
						fexcludeShowConf(TRUE);			/* Enable stdout of exclusion(s) */
					else if (optarg[0] == '=')
						fexcludeShowExcl(TRUE);			/* Enable stdout of excluded path(s) */
					else if (fexclude(optarg))
						{
						printf("\7Exclusion string fault: \"%s\"\n", optarg);
						usage();
						}
				break;

			case 'y':
				++y_flag;
				defflg = FALSE;
				break;

			case 'z':
				z_flag = !z_flag;
				break;

			case '?':
				help();

			default:
				usage();
			}
		}

	if (defflg)
		{
		++e_flag;
		++s_flag;
		++t_flag;
		}

	if (d_flag)
		{
		buff1 = fmalloc(DATASIZE);	/* Get data buffers */
		buff2 = fmalloc(DATASIZE);
		}

	nargs = argc - optind;
	if (nargs < 1)
		f_err("At least one pathname is required");

	if (nargs > 2)
		f_err("Only two pathnames are allowed");

	if (v_flag >= 1)
		{
		printf("v_flag = %d\n", v_flag);
		fflush(stdout);
		}

	fnp1 = argv[optind++];

	if ((nargs != 2)  ||  ((fnp2 = argv[optind]) == NULL))
		fnp2 = ".";

	if (v_flag >= 1)
		{
		printf("Comparing \"%s\" and \"%s\"%s\n",
			fnp1, fnp2, (bi_flag ? " (bidirectionally)" : ""));
		fflush(stdout);
		}

	if (fnchkfil(fnp2))		// If two files specified,
		{
		if (fnchkfil(fnp1))
			process(fnp1, fnp2);	// Compare the two files.
		else
			f_err("If pathname2 is a file, pathname1 must also be a file");
		}
	else // fnp2 is not a file; it is either a directory or is wild
		{
		if (fnchkfil(fnp1))
			bi_flag = FALSE;		// No need to compare single file both ways

		filepair1(fnp1, fnp2);

		if (bi_flag)
			filepair2(fnp1, fnp2);
		}

	exit(ex_code);
	}

/* ----------------------------------------------------------------------- */
	void
filepair1 (					/* Process the pathnames forward */
	char  *s1,				/* Pointer to the pathname1 string (Search) */
	char  *s2)				/* Pointer to the pathname2 string (Target) */

	{
	int    CatIndex1;		/* Concatenation index of the path */
	int    TermIndex1;		/* Termination index of the path (before concatenating) */
	void  *hp;				/* Pointer to wild file data block */
	char  *fnp1;			/* Pointer to the path1 pathname */
	char  *fnp2;			/* Pointer to the path2 pathname */
	char s2save[1024];
	strcpy(s2save, s2);

	if (iswild(s2))			/* Ensure non-wild path2 */
		f_err("Path2 cannot be wild");

	fnParse(s1, &CatIndex1, &TermIndex1);
// remove
	if (v_flag >= 3)
		{
		printf("Pat F1 ('%s')  F2 ('%s')\n", s1, s2);
		fflush(stdout);
		}

	fnreduce(s2);

	if (v_flag >= 3)
		{
		printf("Pat F1 ('%s')  F2 ('%s')\n", s1, s2);
		fflush(stdout);
		}

// printf("1 pattern: \"%s\"\n", s1);
// printf("1 s2save:  \"%s\"\n", s2save);
// printf("1 s2:      \"%s\"\n", s2);

	hp = fwinit(s1, filetypes);		/* Find the first path1 file */
	fwExclEnable(hp, TRUE);			/* Enable file exclusion */
	if ((fnp1 = fwild(hp)) == NULL)
		{
		hp = NULL;
		if (!bi_flag)
			cantfind(s1);
		}

	else do							/* Process all path1 files */
		{
		fnp2 = fncatpth(s2, (fnp1 + CatIndex1));

// printf("  fnp1:  %s\n", fnp1);
// printf("  fnp2:  %s\n", fnp2);
// fflush(stdout);

		if (v_flag >= 4)
			{
			printf("FNP F1 ('%s')  F2 ('%s')\n", fnp1, fnp2);
			fflush(stdout);
			}

		process(fnp1, fnp2);

		free(fnp2);
		} while ((fnp1 = fwild(hp)));
	hp = NULL;
	}

/* ----------------------------------------------------------------------- */
	void
filepair2 (					/* Process the pathnames backward */
	char  *s1,				/* Pointer to the pathname1 string (target) */
	char  *s2)				/* Pointer to the pathname2 string (search) */

	{
	int    CatIndex1;		/* Concatenation index of path 1 */
	int    TermIndex1;		/* Termination index of path 1 (before concatenating) */
	int    CatIndex2;		/* Concatenation index of path 2 */
	int    TermIndex2;		/* Termination index of path 2 (before concatenating) */
	void  *hp;				/* Pointer to wild file data block */
	char  *fnp1;			/* Pointer to the path1 pathname */
	char  *fnp2;			/* Pointer to the path2 pathname */
	char  *fnppat2;			/* Pointer to the path2 pattern */


	if (iswild(s2))			/* Ensure non-wild path2 */
		f_err("Path2 cannot be wild");

	if (v_flag >= 3)
		{
		printf("Pat F1 ('%s')  F2 ('%s')\n", s1, s2);
		fflush(stdout);
		}

	e_flag  = TRUE;			/* Do    check the existence */
	d_flag  = FALSE;		/* Don't check the data */
	s_flag  = FALSE;		/* Don't check the size */
	t_flag  = FALSE;		/* Don't check the timedate */

	fnParse(s1, &CatIndex1, &TermIndex1);	/* Determine the path1 offsets */
	fnParse(s2, &CatIndex2, &TermIndex2);	/* Determine the path2 offsets */

	fnppat2 = fncatpth(s2, (s1 + CatIndex1));	/* Build the search pattern */

	*(s1 + TermIndex1) = '\0';		// (does not delete a possible root separator

// printf("2 s1:      \"%s\"\n", s1);
// printf("2 Index 1:  %d\n",    CatIndex1);
// printf("2 Term  1:  %d\n",    TermIndex1);
// printf("2 s2:      \"%s\"\n", s2);
// printf("2 Index 2:  %d\n",    CatIndex2);
// printf("2 Term  2:  %d\n",    TermIndex1);
// printf("2 Pattern: \"%s\"\n", fnppat2);
//   fflush(stdout);

	if (strlen(fnppat2) == 0)		// Handle cat -b .. . case
		fnppat2 = ".";
	hp = fwinit(fnppat2, filetypes);	/* Process all path2 files */
	fwExclEnable(hp, TRUE);				/* Enable file exclusion */
	while ((fnp2 = fwild(hp)) != NULL)
		{
		fnp1 = fncatpth(s1, (fnp2 + CatIndex2));

// printf("3 Pattern: \"%s\"\n", fnppat2);
// printf("3 File B:    \"%s\"\n", fnp2);
// printf("3 File A:    \"%s\"\n", fnp1);
//fflush(stdout);

		if (v_flag >= 4)
			{
			printf("FNP F1 ('%s')  F2 ('%s')\n", fnp1, fnp2);
			fflush(stdout);
			}

		process(fnp1, fnp2);
		free(fnp1);
//exit(1);
		}
	hp = NULL;
	free(fnppat2);
	}

/* ----------------------------------------------------------------------- */
	void
process (				/* Compare one pair one of input files */
	char  *fnp1,		/* Input file name 1 */ 
	char  *fnp2)		/* Input file name 2 */ 

	{
	int   exist1    = FALSE;	/* TRUE if file 1 exists */
	int   exist2    = FALSE;	/* TRUE if file 2 exists */
	int   younger   = FALSE;	/* TRUE if file 1 is younger than file 2 */
	int   older     = FALSE;	/* TRUE if file 1 is older than file 2 */
	int   larger    = FALSE;	/* TRUE if file 1 is larger than file 2 */
	int   smaller   = FALSE;	/* TRUE if file 1 is smaller than file 2 */
	int   different = FALSE;	/* TRUE if a data difference */
	int   dc;			/* The result from datacomp() */
	int   diff;			/* OR of the above flags */
	int   error;		/* OR of the above flags */
	int   cflag     = FALSE;	/* TRUE if comma needed in message */
	int   fd1       = -1;	/* Input file descriptor 1 */
	int   fd2       = -1;	/* Input file descriptor 2 */
	int   dir1;			/* TRUE if file 1 is a directory */
	int   dir2;			/* TRUE if file 2 is a directory */
	unsigned long  td1;		/* File 1 time/date */
	unsigned long  td2;		/* File 2 time/date */
	UINT64         size1;	/* File 1 size */
	UINT64         size2;	/* File 2 size */


	if (c_flag)
		{
		int  retval;

	if ((dir1 = fnchkdir(fnp1)) == FALSE)
		{
		td1    = fgetfdt(fnp1);
		retval = fgetsize(fnp1, &size1);
		exist1 = ((td1 != -1)  ||  (retval == 0));
		}
	else	// Treat a directory as non-existent (mismatches a file)
		{
		td1    = -1;
		size1  = 0;
		exist1 = FALSE;
		}

	if ((dir2 = fnchkdir(fnp2)) == FALSE)
		{
		td2    = fgetfdt(fnp2);
		retval = fgetsize(fnp2, &size2);
		exist2 = ((td2 != -1)  ||  (retval == 0));
		}
	else	// Treat a directory as non-existent (mismatches a file)
		{
		td2    = -1;
		size2  = 0;
		exist2 = FALSE;
		}

	if (exist1  &&  exist2)
		{
		td1 += timedelta;
		if ((t_flag  ||  y_flag)  &&  (td1 > td2))
			++younger;

		if ((t_flag  ||  o_flag)  &&  (td1 < td2))
			++older;

		if (     ( ! u_flag)  &&  s_flag  &&  (size1 > size2))
			++larger;
		else if (( ! u_flag)  &&  s_flag  &&  (size1 < size2))
			++smaller;
		else if (d_flag)
			{
			if (((fd1 = open(fnp1, OPENMODE)) >= 0)
			&&  ((fd2 = open(fnp2, OPENMODE)) >= 0))
				{
				if (u_flag)
					dc = unixcomp(fd1, fd2);
				else
					dc = datacomp(fd1, fd2);
				if (dc & DC_DIFFERENT)
					++different;
				if (s_flag  &&  (dc & DC_SMALLER))
					++smaller;
				if (s_flag  &&  (dc & DC_LARGER))
					++larger;
				}
			if (fd1 >= 0)
				close(fd1);
			if (fd2 >= 0)
				close(fd2);
			}
		}
	}

	diff  =  younger  ||  older  ||  larger  ||  smaller  ||  different;
	error =  diff     ||  (e_flag  &&  ((!exist1)  ||  (!exist2)));

	if ((b_flag  &&  error)  ||  (g_flag  &&  ! error))
		{
		if (error  &&  a_flag)
			putchar('\7');

		if (L_flag)
			printf("%-22s  and  %-22s", qname(fnp1), qname(fnp2));
		else
			printf("%-40s", qname(fnp1));

		if (! exist1)
			{
			if (e_flag)
				{
				if (dir1)
					fputs("  File 1 is a directory", stdout);
				else
					{
					fputs("  File 1 is missing", stdout);
//					printf("\nFile1 \"%s\"\n", fnp1);
//					printf("File2 \"%s\"\n", fnp2);
					}
				}
			}
		else if (! exist2)
			{
			if (e_flag)
				{
				if (dir2)
					fputs("  File 2 is a directory", stdout);
				else
					fputs("  File 2 is missing", stdout);
				}
			}
		else if (diff)
			{
			fputs("  File 1 is", stdout);
			if (younger)
				cflag = putdiff(cflag, "younger");
			else if (older)
				cflag = putdiff(cflag, "older");
			if (larger)
				cflag = putdiff(cflag, "larger");
			else if (smaller)
				cflag = putdiff(cflag, "smaller");
			if (different)
				cflag = putdiff(cflag, "different");
			}
		putchar('\n');
		fflush(stdout);
		}

	if (error  &&  (z_flag == FALSE))
		ex_code = 1;
	}

/* ----------------------------------------------------------------------- */
	char *				/* Returned name pointer */
qname (
	char  *NameIn)		/* The message string */

	{
static char NameOut [1024];

	if (Q_flag  &&  (strchr(NameIn, ' ') != NULL))
		{
		strcpy(NameOut, "\"");
		strcat(NameOut, NameIn);
		strcat(NameOut, "\"");
		return  (NameOut);
		}

	return  (NameIn);
	}

/* ----------------------------------------------------------------------- */
	int
putdiff (
	int    flag,		/* The comma flag */
	char  *msg)			/* The message string */

	{
	if (flag)
	putchar(',');
	putchar(' ');
	flag = TRUE;
	fputs(msg, stdout);

	return (flag);
	}

/* ----------------------------------------------------------------------- */
	int					/* Return TRUE if they are different */
unixcomp (				/* Compare the data of two open files */
	int  fd1,
	int  fd2)

	{
	FILE  *fp1;			/* FILE ptr 1 */
	FILE  *fp2;			/* FILE ptr 2 */
	int    len1;		/* Length of data read 1 */
	int    len2;		/* Length of data read 2 */
	int    result = DC_EQUAL;	/* The returned result */

	fp1 = fdopen(fd1, "rt");
	fp2 = fdopen(fd2, "rt");

	for (;;)
		{
		len1 = fread(buff1, sizeof(char), DATASIZE, fp1);
		len2 = fread(buff2, sizeof(char), DATASIZE, fp2);
		if (v_flag >= 2)
			printf("len1: %d  len2: %d\n", len1, len2);

		if (len1 > len2)			/* (len1 > len2), done */
			{
			if (v_flag >= 2)
				printf("larger\n");
			result |= DC_LARGER;
			if ((len2 > 0)  &&  memcmp(buff1, buff2, len2))
				{
				if (v_flag >= 2)
					printf("different (l)\n");
				result |= DC_DIFFERENT;
				}
			break;
			}

		if (len1 < len2)			/* (len1 < len2), done */
			{
			if (v_flag >= 2)
				printf("smaller\n");
			result |= DC_SMALLER;
			if ((len1 > 0)  &&  memcmp(buff1, buff2, len1))
				{
				if (v_flag >= 2)
					printf("different (s)\n");
				result |= DC_DIFFERENT;
				}
			break;
			}

		if (len1 == 0)				/* (len1 == len2 == 0), done */
			{
			if (v_flag >= 2)
				printf("zero\n");
			break;
			}

		if (memcmp(buff1, buff2, len1))		/* (len1 == len2 != 0), cont */
			{
			if (v_flag >= 2)
				printf("different ()\n");
			result |= DC_DIFFERENT;
			}

		if (v_flag >= 2)
			printf("equal\n");
		}

	fclose(fp1);
	fclose(fp2);
	return (result);
	}

/* ----------------------------------------------------------------------- */
	int					/* Return TRUE if they are different */
datacomp (				/* Compare the data of two open files */
	int  fd1,
	int  fd2)

	{
	int  len1;			/* Length of data read 1 */
	int  len2;			/* Length of data read 2 */


	while ((len1 = read(fd1, buff1, DATASIZE)) > 0)
		{
		if (((len2 = read(fd2, buff2, DATASIZE)) != len1)
		||  (memcmp(buff1, buff2, len1)))
			return (DC_DIFFERENT);
		}
	return (DC_EQUAL);
	}

/* ----------------------------------------------------------------------- */
	void
cantfind (				/* Inform user of input failure */
	char  *fnp)			/* Input file name */
    
	{
	if (!q_flag)
		fprintf(stderr, "Unable to find file: %s\n", fnp);
	if (z_flag)
		exit(0);
	else if (!q_flag)
		usage();
	}

/* ----------------------------------------------------------------------- */
	void
f_err (					/* Report a fatal error */
	char  *s)			/* Pointer to the message string */

	{
	fprintf(stderr, "%s\n", s);
	if (z_flag)
		exit(0);
	else
		usage();
	}

/* ----------------------------------------------------------------------- */
