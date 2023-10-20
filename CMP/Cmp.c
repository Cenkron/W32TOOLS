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
|				   10-Sep-23 New fnreduce(), fnabspth, pathCopy, etc.
|
\* ----------------------------------------------------------------------- */

#include  <windows.h>

#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <ctype.h>
#include  <fcntl.h>
#include  <io.h>
#include  "time.h"	

#include  "fWild.h"
#include  "ptypes.h"

/* ----------------------------------------------------------------------- */
//	#define DEBUG

#ifndef TRUE
#define FALSE	0
#define TRUE	1
#endif

#define	PATHCH	('\\')

/* ----------------------------------------------------------------------- */

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
"    %ca      Reports attribute mismatch (not archive)",
"    %cA      Reports attribute mismatch (also archive)",
"    %cb      checks files bidirectionally (implies %ce)",
"    %cB      checks files only in the backward direction",
"    %cd      reports different data in the file pairs",
"    %cD      also tests directories, including file/dir mismatch",
"    %ce      reports non-existence of the path2 file    (default)",
"    %ch      also checks hidden and system files",
"    %cl      lists all file names as they are compared",
"    %cL      also lists the path2 names",
"    %cn      lists all file names, but does not compare them",
"    %cN      in error reports list Only the file/dir name",
"    %co <td> compares only files older than <td>",
"    %cO      reports only \"older\" files",
"    %cq      suppress error basic message (not file mismatches) (default off)",
"    %cQ      quotes file names with imbedded spaces (default on)",
"    %cr      lists only file names of successful compares",
"    %cR      reports raw times of unsuccessful compares",
"    %cs      reports different sizes of the file pairs  (default)",
"    %ct      reports different ages of the file pairs   (default)",
"    %cT nnn  Offset timestamp 1 by (+/-)nnn hours       (default 0)",
"    %cu      chack files in text mode (slower), else binary (faster))",
"    %cv      lists verbose information",
"    %cw      warning beeps on error",
"    %cX <pathspec> e/X/cludes (possibly wild) files matching pathspec",
"    %cX @<xfile>   e/X/clude files that match pathspec(s) in xfile",
"    %cX-       Disable default file exclusion(s)",
"    %cX+       Show exclusion path(s)",
"    %cX=       Show excluded path(s)",
"    %cy <td> compares only files younger than <td>",
"    %cY      reports only \"younger\" files",
"    %cz      returns a zero completion code even if errors",
"",
"Copyright (c) 1988, 1993 by J & M Software, Dallas TX - All Rights Reserved",
NULL
};

/* ----------------------------------------------------------------------- */

int	defflg			= TRUE;			/* Report attribute mismatch flag */
int	a_flag			= FALSE;		/* Report attribute mismatch (excluding archive) */
int	A_flag			= FALSE;		/* Report attribute mismatch (including archive) */
int	c_flag			= TRUE;			/* Compare flag (master flag) */
int	d_flag			= FALSE;		/* Report data mismatch */
int	e_flag			= FALSE;		/* Report existence mismatch */
int l_flag			= FALSE;		/* List all files */
int	L_flag			= FALSE;		/* Use Long form when listing files */
int	N_flag			= FALSE;		/* List only the fault filename(s) */
int	o_flag			= FALSE;		/* Compare only if older than <td> */
int	q_flag			= FALSE;		/* quiet flag */
int	Q_flag			= TRUE;			/* quote names with spaces flag */
int	R_flag			= FALSE;		/* show raw times of unsuccessful compares */
int	s_flag			= FALSE;		/* Report size mismatch */
int	t_flag			= FALSE;		/* Report Timedate mismatch */
int	u_flag			= FALSE;		/* UNIX mode (text translation) */
int	v_count			= 0;			/* Verbose information flag */
int	w_flag			= FALSE;		/* Beep on error flag */
int	y_flag			= FALSE;		/* Compare only if younger than <td> */
int	z_flag			= FALSE;		/* Return zero even if error flag */

int	good_flag		= FALSE;		/* List good file names flag */
int	bad_flag		= TRUE;			/* List bad file names flag */
int	fwd_enable		= TRUE;			/* Compare in the forward direction */
int	rev_enable		= FALSE;		/* Compare only in the backward direction */
int	Older_flag		= FALSE;		/* Report only older flag */
int	Younger_flag	= FALSE;		/* Report only younger flag */

int	exit_code	= 0;				/* Exit code */

int	filetypes = FW_FILE;		/* File types checked */

long    timedelta = 0L;			/* Timestamp compare correction */
#define  OneHour (60L * 60L)	/* The correction value */

char	swch = '-';				/* The switch character */

char   *buff1;					/* Disk data buffer pointers */
char   *buff2;

PHP		hp = NULL;				// FWILD instance pointer
PEX		xp = NULL;				// FEX instance pointer

//char	buffer [BUFSIZ];		/* Buffer for stdout */

#define	DATASIZE	1000000		/* Size of the disk data buffers */

#define	OPENMODE	((u_flag) ? (_O_RDONLY | _O_TEXT) : (_O_RDONLY | _O_BINARY))

#define	DC_EQUAL	 (0)		/* dataComp() bitmap values */
#define	DC_MISMATCH  (1)		// file 1, 2 content is different
#define	DC_SMALLER   (2)		// File 1 is shorter than file 2
#define	DC_LARGER    (4)		// File 1 is smaller than file 2


void	filepair1	(void);
void	filepair2	(void);
void	process		(char *, char *);
void	file1error	(char *);
void	file2error	(char *);
void	fat_err		(char *);
int		putdiff		(int, char *);
int		dataComp	(int fd1, int fd2);
int		sizeComp	(int fd1, int fd2);

char   *qname		(char *);

time_t	oldertime   = 0L;			/* The older-than time */
time_t	youngertime = 0L;			/* The younger-than time */

int		CopyIndex1;				// Copy index of path 1
int 	TermIndex1;				// Termination index of path1
int		CopyIndex2;				// Copy index of path 2

char	pPath1   [MAX_PATH];	// Primary Search path buffer
char	pPath2   [MAX_PATH];	// Secondary search path buffer
char	pSearch  [MAX_PATH];	// Search filespec
char	pCompare [MAX_PATH];	// Compare filespec

/* ----------------------------------------------------------------------- */
	static int
timebound (
	void *hp,				/* Pointer to wild file data block */
	char *fnp)

	{
	time_t  t;

	if (o_flag || y_flag)
		{
		t = fwgetfdt(hp);

		if (v_count >= 2)
			{
			printf("\n");
			printf("Datetime: %s", asctime(localtime(&t)));
			printf("Datetime: %lld\n", t);
			printf("y_flag: %d\n", y_flag);
			if (youngertime != 0L)
				printf("Younger:  %s", asctime(localtime(&youngertime)));
			printf("o_flag: %d\n", o_flag);
			if (oldertime != 0L)
				printf("Older:    %s", asctime(localtime(&oldertime)));
//			printf("\n");
			}

		if (v_count >= 1)
			{
//			printf("\n");
			if ((y_flag)  &&  (t >= youngertime))
				printf("Younger:  %s", fnp);

			if ((o_flag)  &&  (t <= oldertime))
				printf("Older:  %s", fnp);

			printf("\n");
			}
		}

	return (((y_flag == FALSE)	||	(t >= youngertime))
		&&	((o_flag == FALSE)	||	(t <= oldertime)));
	}

/* ----------------------------------------------------------------------- */
	void
main (
	int    argc,
	char  *argv [])

	{
	int    option;			/* Option character */
	int    nargs;			/* Number of path arguments */

static	char   *optstring = "?aAbBdDeEhHlLnNo:OqQrRsStT:uUvVwWX:y:YzZ";


	if ((hp = fwOpen()) == NULL)
		exit(1);
	if ((xp = fExcludeOpen()) == NULL)
		exit(1);

//	setbuf(stdout, buffer);
	swch   = egetswch();
	optenv = getenv("CMP");

	while ((option = getopt(argc, argv, optstring)) != EOF)
		{
//printf("Switching\n");
		switch (tolower(option))
			{
			case 'a':
				if (option == 'A')	// Alarm
					A_flag = TRUE;
				a_flag = TRUE;
				defflg = FALSE;
				break;

			case 'b':
				if (option == 'B')	// Backward only
					{
					fwd_enable = FALSE;
					rev_enable = TRUE;
					}
				else // bidirectional
					{
					fwd_enable = TRUE;
					rev_enable = TRUE;
					}
				break;

			case 'd':
				if (option == 'D')
					filetypes |= FW_DIR;	// Also check directories
				else
					{
					d_flag = !d_flag;
					defflg = FALSE;
					}
				break;

			case 'e':
				e_flag = TRUE;
				defflg = FALSE;
				break;

			case 'h':
				filetypes |= (FW_HIDDEN | FW_SYSTEM);
				break;

			case 'l':
				if (option == 'L')
					++L_flag;	// Use the long form when listing files
				else // (option == 'l')
					++l_flag;	// List all files
				break;

			case 'n':
				if (option == 'n')
					{
					l_flag = TRUE;
					c_flag = FALSE;
					defflg = FALSE;
					}
				else // (option == 'N')		// List only the faulted filename
					++N_flag;
				break;

			case 'o':
				if (option == 'O')
					{
					++Older_flag;
					defflg = FALSE;
					}
				else // (option == 'o')
					{
					if ((oldertime = fwsgettd(optarg)) == 0)
						{
						printf("Older time - %s\n", fwserrtd());
						usage();
						}
					o_flag = TRUE;
					}
				break;

			case 'q':
				if (option == 'Q')
					Q_flag = TRUE;
				else // (option == 'Q')
					q_flag = TRUE;
				break;

			case 'Q':
				break;

			case 'r':
				if (option == 'R')
					{
					R_flag = TRUE;
					}
				else // (option == 'r')
					{
					l_flag    = FALSE;
					bad_flag  = FALSE;
					good_flag = TRUE;
					}
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
				else // (option == 'T')
					{
					if (optarg != NULL)
						timedelta = OneHour * strtol(optarg, NULL, 10);
					}
				break;

			case 'u':
				++u_flag;
				break;

			case 'v':
				++v_count;
				break;

			case 'w':
				w_flag = TRUE;
				break;

			case 'x':	// (Includes 'X')
				if      (optarg[0] == '-')
					fExcludeDefEnable(xp, FALSE);	/* Disable default file exclusion(s) */
				else if (optarg[0] == '+')
					fExcludeShowConf(xp, TRUE);		/* Enable stdout of exclusion(s) */
				else if (optarg[0] == '=')
					fExcludeShowExcl(xp, TRUE);		/* Enable stdout of excluded path(s) */
				else if (fExclude(xp, optarg))
					{
					printf("\7Exclusion string fault: \"%s\"\n", optarg);
					usage();
					}
				break;

			case 'y':
				if (option == 'Y')
					{
					++Younger_flag;
					defflg = FALSE;
					}
				else // (option == 'y')
					{
					if ((youngertime = fwsgettd(optarg)) == 0)
						{
						printf("Younger time - %s\n", fwserrtd());
						usage();
						}
					y_flag = TRUE;
					}
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
		bad_flag = TRUE;
//		a_flag = TRUE;
//		A_flag = TRUE;
		e_flag = TRUE;
		s_flag = TRUE;
		t_flag = TRUE;
		}

	if (d_flag || u_flag)
		{
		buff1 = fmalloc(DATASIZE);	/* Get data buffers */
		buff2 = fmalloc(DATASIZE);
		}

	if (v_count >= 1)
		printf("v_count = %d\n", v_count);

	// Copy the path argument(s) to the internal buffers
	
	nargs = argc - optind;
	if (nargs < 1)
		fat_err("At least one pathname is required");

	if (nargs > 2)
		fat_err("Only two pathnames are allowed");

	pathCopy(pPath1, argv[optind++], MAX_COPY);	// Load the filename buffers
	if (nargs == 2)
		pathCopy(pPath2, argv[optind], MAX_COPY);
	else // (nargs == 1)	
		strcpy(pPath2, ".");

	// Standardize the path characters in the buffers

	strsetp(pPath1, PATHCH);
	strsetp(pPath2, PATHCH);
	
	if (isWild(pPath2))					// Ensure non-wild path2
		fat_err("Path2 cannot be wild");

	// Validate the path devices

	if (! isPhysical(pPath1))			// Protect against NAS down
		fat_err("Pathspec 1 unavailable");
	if (! isPhysical(pPath2))
		fat_err("Pathspec 2 unavailable");
	
	// Report the analysis plan

	if (v_count >= 1)
		{
		printf("Comparing \"%s\" and \"%s\"", pPath1, pPath2);
		if (! fwd_enable)		// (! fwd_enable && rev_enable)
			printf(" (in reverse only)\n");
		else if (rev_enable)	// (fwd_enable  && rev_enable)
			printf(" (bidirectionally)\n");
		else 				// (fwd_enable && ! rev_enable)
			printf("\n");
		}

	fnreduce(pPath1);
	fnreduce(pPath2);

	// Check and handle the two files comparison case

//BWJ this fails the mismatched type test

	if (fnchkfil(pPath2))				// If two files specified,
		{
		if (fnchkfil(pPath1))
			process(pPath1, pPath2);	// Compare the two files.
		else
			fat_err("If pathname2 is a file, pathname1 must also be a file");
		}

	else // pPath2 is not a file; it is likely a directory
		{
		if (fwd_enable)
			filepair1();

		if (rev_enable)
			filepair2();
		}

	xp = fExcludeClose(xp);					// Close the Exclusion instance
	hp = fwClose(hp);
	exit(exit_code);
	}

// ---------------------------------------------------------------------------
//	Build forward search path
// ---------------------------------------------------------------------------
		void inline
	BuildForwardPaths (void)

	{
//BWJ how about if directories are included in the compare

	pathCopy(pSearch,  pPath1, MAX_COPY);
	pathCopy(pCompare, pPath2, MAX_COPY);

// BWJ
//	if (fnchkdir(pSearch))		// Make a directory spec wild
//		pathCat(pSearch, "*", MAX_COPY);

	fnreduce(pSearch);

	fnParse(pSearch, &CopyIndex1, &TermIndex1);

	if (v_count >= 3)
		printf("F Search Pat F1 ('%s')  F2 ('%s')\n", pSearch, pPath2);
	}

// ---------------------------------------------------------------------------
//	Build forward compare path
// ---------------------------------------------------------------------------
		void inline
	BuildForwardCompareTarget (
		char *fnp)		// filespec to compare to

	{
	_fncatpth(pCompare, pPath2, (fnp + CopyIndex1));
	
//	fnreduce(pCompare);

	if (v_count >= 3)
		printf("F Compare Pat F1 ('%s')  F2 ('%s')\n", fnp, pCompare);
	}

// ---------------------------------------------------------------------------
//	Perform the forward direction compare
// ---------------------------------------------------------------------------
	void
filepair1 (void)		// Process the pathnames forward

	{
	char  *fnp;			// Pointer to the found path1 pathname

	BuildForwardPaths();

	if (v_count >= 1)
		printf("F Comparing Pat ('%s')\n", pPath2);

	if (fwInit(hp, pSearch, filetypes) != FWERR_NONE)	// Find the first path1 file
		fwInitError(pSearch);
	else
		{ // hp is valid
		fExcludeConnect(xp, hp);		// Connect the exclusion instance
		if ((fnp = fWild(hp)) == NULL)
			{
			if (! rev_enable)	// Don't require forward reports if also checking reverse reports
				{
				cantfind(pPath1);
				}
			}
		else do							/* Process all path1 files */
			{
			if (timebound(hp, fnp))
				{		
				BuildForwardCompareTarget(fnp);

				if (v_count >= 2)
					printf("F Comparing F/D ('%s')  target ('%s')\n", fnp, pCompare);

				process(fnp, pCompare);
				} // End if (timebound)
			} while (fnp = fWild(hp));
		}
	}

// ---------------------------------------------------------------------------
//	Build reverse search paths
// ---------------------------------------------------------------------------
		void inline
	BuildReversePaths (void)

	{
	fnParse(pPath1, &CopyIndex1, &TermIndex1);
	fnParse(pPath2, &CopyIndex2, NULL);

	// Copy the wild part of path1 to path2, then terminate path1
	
	_fncatpth(pSearch, pPath2, (pPath1 + CopyIndex1));
	pathCopy(pCompare, pPath1, MAX_PATH);
	*(pPath1 + TermIndex1) = '\0';

	if (v_count >= 3)
		printf("R Search Pat F1 ('%s')  F2 ('%s')\n", pSearch, pPath1);
	}

// ---------------------------------------------------------------------------
//	Build reverse compare path
// ---------------------------------------------------------------------------
		void inline
	BuildReverseCompareTarget (
		char *fnp)		// filespec to compare to

	{
	_fncatpth(pCompare, pPath1, (fnp + CopyIndex2));
	
//	fnreduce(pCompare);

	if (v_count >= 3)
		printf("R Compare Pat F1 ('%s')  F2 ('%s')\n", fnp, pCompare);
	}

// ---------------------------------------------------------------------------
//	Perform the reverse direction compare
// ---------------------------------------------------------------------------
	void
filepair2 (void)		// Process the pathnames backward

	{
	char  *fnp;			// Pointer to the found path1 pathname


	BuildReversePaths();

	if (v_count >= 1)
		printf("R Compare Pat ('%s')  target ('%s')\n", pSearch, pPath1);

	if (fwd_enable)	// Don't test these thing going both ways
		{
//BWJ  -h issue
//		e_flag  = TRUE;			/* Do    check the existence */
		a_flag  = FALSE;		/* Don't check the attributes */
		A_flag  = FALSE;		/* Don't check the attributes */
		d_flag  = FALSE;		/* Don't check the data */
		s_flag  = FALSE;		/* Don't check the size */
		u_flag  = FALSE;		/* Don't check the size */
		t_flag  = FALSE;		/* Don't check the timedate */
		}

	if (fwInit(hp, pSearch, filetypes) != FWERR_NONE)	// Find the first path1 file
		fwInitError(pSearch);
	else
		{ // hp is valid
		fExcludeConnect(xp, hp);		// Connect the exclusion instance
		while ((fnp = fWild(hp)) != NULL)
			{
			if (timebound(hp, fnp))
				{		
				BuildReverseCompareTarget(fnp);

				if (v_count >= 2)
					printf("R Comparing F/D ('%s')  target ('%s')\n", fnp, pCompare);

				process(pCompare, fnp);	// Filespecs switched for reverse direction
				} // End if (timebound)
			} while (fnp = fWild(hp));
		}
	}

/* ----------------------------------------------------------------------- */
//	Report differences between the two files (or directories, or ???)
/* ----------------------------------------------------------------------- */

static char DiffList [128] = {0};		// Buffer used to report errors
static int  diffCount;					// Treated as Boolean

#define ATT_TYPE	(ATT_FILE | ATT_DIR)
#define ATT_PROPS	(ATT_ARCH | ATT_RONLY | ATT_HIDDEN | ATT_SYSTEM)
#define ATT_ARMASK	(~(A_flag ? 0 : ATT_ARCH))

/* ----------------------------------------------------------------------- */
	static void
ListFiles (			// List files without the trailing newline
	char *fnp1,
	char *fnp2)

	{
	if (L_flag)
		printf("%-22s  and  %-22s", qname(fnp1), qname(fnp2));
	else
		printf("%-40s", qname(fnp1));
	}

/* ----------------------------------------------------------------------- */
	static void
putFile1 (void)			// Place the "File 1 is" prefix for the message
	{
	if (DiffList[0] == '\0')
		strcpy(DiffList, "File 1 is ");
	}
	
/* ----------------------------------------------------------------------- */
	static void
putDifference (				// Place the message body
	int   commaEnb,
	char *pMsg)
	
	{
	if (commaEnb && diffCount++)
		strcat(DiffList, ", ");
	strcat(DiffList, pMsg);
	}
	
/* ----------------------------------------------------------------------- */
	static void
putType (
	int  n,
	int	 type)

	{
	char buffer [20];

	sprintf(buffer, "File %d is %s  ",
		n,
		((type & ATT_FILE) ? "FILE" : ((type & ATT_DIR) ? "DIR" : "UNKNOWN")));
	putDifference(0, buffer);
	}

/* ----------------------------------------------------------------------- */
	static void
putAttr (
	int  n,
	int	 attr)

	{
	char buffer [20];

	sprintf(buffer, "File %d: %c%c%c%c   ",
		n,
		(attr & ATT_ARCH)	? ('a') : ('-'),
		(attr & ATT_RONLY)	? ('r') : ('-'),
		(attr & ATT_HIDDEN) ? ('h') : ('-'),
		(attr & ATT_SYSTEM) ? ('s') : ('-'));
	putDifference(0, buffer);
	}

/* ----------------------------------------------------------------------- */
	void
process (				/* Compare one pair one of input files */
	char  *fnp1,		/* Input file name 1 */ 
	char  *fnp2)		/* Input file name 2 */ 

	{
	int		exist1		= FALSE;		/* TRUE if file 1 exists */
	int		exist2		= FALSE;		/* TRUE if file 2 exists */

	int		typeDiff	= FALSE;		/* TRUE if file 1 type mismatches file 2 type */
	int		attrDiff	= FALSE;		/* TRUE if type 1 attr mismatches type 2 attr */
	int		existDiff	= FALSE;		/* TRUE if one of the two files is missing */
	int		youngerDiff	= FALSE;		/* TRUE if file 1 is younger than file 2 */
	int		olderDiff	= FALSE;		/* TRUE if file 1 is older than file 2 */
	int		largerDiff	= FALSE;		/* TRUE if file 1 is larger than file 2 */
	int		smallerDiff	= FALSE;		/* TRUE if file 1 is smaller than file 2 */
	int		contentDiff	= FALSE;		/* TRUE if file content is different */
	int		different	= FALSE;		// Gated OR of the above 8 *Diff flags

	int		dc			= DC_EQUAL;		/* The result from dataComp() and sizeComp() */
	int		fd1			= -1;			/* Input file descriptor 1 */
	int		fd2			= -1;			/* Input file descriptor 2 */
	int		attr1		= 0;			/* Path1 type */
	int		attr2		= 0;			/* Path2 type */
	int		type1		= 0;			/* Path1 type */
	int		type2		= 0;			/* Path2 type */
	time_t	fdt1		= 0;			/* File 1 time/date */
	time_t	fdt2		= 0;			/* File 2 time/date */
	time_t	mfdt1		= 0;			/* File 1 Microsoft time/date */
	time_t	mfdt2		= 0;			/* File 2 Microsoft time/date */
	UINT64	size1		= 0;			/* File 1 size */
	UINT64	size2		= 0;			/* File 2 size */
	
	diffCount	= 0;					// Keeps track of difference message comma necessity
	DiffList[0] = '\0';					// Clear the difference list

	// If requested, determine all differences
		
	if (l_flag && ! N_flag)		// Listing all files, not only those that are different
		ListFiles(fnp1, fnp2);

	if (c_flag)
		{
		PFI pFi;

		if ((! fExcludePathCheck (xp, fnp1))
		&&  ((pFi = FileInfoOpen(filetypes, fnp1)) != NULL))
			{
			exist1	= TRUE;
			type1	= FileInfoAttr(pFi) & ATT_TYPE;
			attr1	= FileInfoAttr(pFi) & ATT_PROPS;
			size1	= FileInfoSize(pFi);
			fdt1	= FileInfoFDT(pFi);
			mfdt1	= FileInfoMtime(pFi);
			}
		if (pFi)
			FileInfoClose(pFi);

		if ((! fExcludePathCheck (xp, fnp2))
		&&  ((pFi = FileInfoOpen(filetypes, fnp2)) != NULL))
			{
			exist2	= TRUE;
			type2	= FileInfoAttr(pFi) & ATT_TYPE;
			attr2	= FileInfoAttr(pFi) & ATT_PROPS;
			size2	= FileInfoSize(pFi);
			fdt2	= FileInfoFDT(pFi);
			mfdt2	= FileInfoMtime(pFi);
			}
		if (pFi)
			FileInfoClose(pFi);

		// Determine existential status (NOT gated by c_flag)
		// exist1 should never be FALSE, because fWild succeeded)

		existDiff = (! (exist1 && exist2));
		if (! existDiff)
			typeDiff  = (type1  != type2);

		// If both files exist, and are the same type, compare them

		if ((! existDiff) && (! typeDiff))
			{
			if (c_flag)
				{
				if ((a_flag) && ((attr1 ^ attr2) & ATT_ARMASK))
					++attrDiff;

				if (type1 == ATT_FILE) // (includes type2)
					{
					fdt1 += timedelta;
					if ((t_flag  ||  Younger_flag)  &&  (fdt1 > fdt2))
						++youngerDiff;

					if ((t_flag  ||  Older_flag)  &&  (fdt1 < fdt2))
						++olderDiff;

					if (d_flag || (s_flag && u_flag))
						{
						if ((fd1 = _open(fnp1, OPENMODE)) < 0)
							file1error(fnp1);
						if ((fd2 = _open(fnp2, OPENMODE)) < 0)
							file2error(fnp2);
						}

					if (s_flag && ! d_flag)
						{
						if (u_flag)
							{
							switch (sizeComp(fd1, fd2))
								{
								case DC_LARGER:  ++largerDiff;   break;
								case DC_SMALLER: ++smallerDiff;  break;
								default:  break;
								}
							}
						else // (! u_flag)
							{
							if 		(size1 > size2)
								++largerDiff;
							else if	(size1 < size2)
								++smallerDiff;
							}
						}

					if (d_flag)
						{
						dc = dataComp(fd1, fd2);

						if (dc & DC_MISMATCH)
							++contentDiff;
						if (s_flag  &&  (dc & DC_SMALLER))
							++smallerDiff;
						if (s_flag  &&  (dc & DC_LARGER))
							++largerDiff;
						}

					if (d_flag || (s_flag && u_flag))
						{
						if (fd1 >= 0)
							close(fd1);
						if (fd2 >= 0)
							close(fd2);
						}
					} // end if (type == ATT_FILE)
				} // end if (c_flag)
			} // end if existDiff)

		// Report requested differences
		
		if (e_flag)		// Report existence and type mismatch
			{
			if (existDiff)
				{
				if (! exist1)
					putDifference(1, "File 1 is missing");

				if (! exist2)
					putDifference(1, "File 2 is missing");
				different = TRUE;
				}

			else if (typeDiff)
				{
				putType(1, type1);
				putType(2, type2);
				different = TRUE;
				}
			}

		if (a_flag)		// Report type attributes mismatch
			{
			if (attrDiff)
				{
				putAttr(1, attr1);
				putAttr(2, attr2);
				different = TRUE;
				}
			}

		if (t_flag)		// Report timestamp mismatch
			{
			if (youngerDiff  &&  ! Older_flag)
				{
				putFile1();
				putDifference(1, "younger");
				different = TRUE;
				}
			else if (olderDiff &&  ! Younger_flag)
				{
				putFile1();
				putDifference(1, "older");
				different = TRUE;
				}
			}

		if (s_flag)		// Report size mismatch
			{
			if (largerDiff)
				{
				putFile1();
				putDifference(1, "larger");
				different = TRUE;
				}
			else if (smallerDiff)
				{
				putFile1();
				putDifference(1, "smaller");
				different = TRUE;
				}
			}

		if (d_flag)		// Report data content mismatch
			{
			if (contentDiff)
				{
				putFile1();
				putDifference(1, "content");
				different = TRUE;
				}
			}
		}

	if (different  &&  (! z_flag))		// Set the exit code
		exit_code = 1;

	// Report as directed

	if ((bad_flag && different)  ||  (good_flag && (! different)))
		{
		if ((! l_flag) && (! N_flag))	// Already listed earlier
			ListFiles(fnp1, fnp2);

		if (different  &&  w_flag  &&  ! N_flag)
			putchar('\7');

		if (N_flag)
			printf("\"%s\"\n", fnp1);	// List file 1 for pipeline use
		else if (DiffList[0] != '\0')
			printf("    %s\n", DiffList);
		}
	else if (l_flag && ! N_flag)
		printf("\n");	// Terminate up front no diff listing

	// If requested, also report the raw timestamp data for the two files

	if ((! N_flag) && R_flag && different && ((youngerDiff  &&  ! Older_flag) || (olderDiff &&  ! Younger_flag)))
		{
		printf("File1: %010llX\n", fdt1);
		printf("File2: %010llX\n", fdt2);
		}
	}

/* ----------------------------------------------------------------------- */
	int					// Returns DC_* to report UNIX size differences
sizeComp (				// Compare the data length of two open files
	int  fd1,
	int  fd2)

	{
	int  result = DC_EQUAL;
	int  len1;
	int  len2;
	int  size1 = 0;		/* Total size of data read file 1 */
	int  size2 = 0;		/* Total size of data read file 2 */

//printf("Comparing data length\n");
	while ((len1 = read(fd1, buff1, DATASIZE)) > 0)
		size1 += len1;
		
	while ((len2 = read(fd2, buff2, DATASIZE)) > 0)
		size2 += len2;

	if		(size1 > size2)
		result = DC_LARGER;
	else if	(size1 < size2)
		result = DC_SMALLER;

	return (result);
	}

/* ----------------------------------------------------------------------- */
	int					/* Return TRUE if they are different */
dataComp (				/* Compare the data of two open files */
	int  fd1,
	int  fd2)

	{
	int  len1;			/* Length of data read 1 */
	int  len2;			/* Length of data read 2 */

//("Comparing data content\n");
	while ((len1 = read(fd1, buff1, DATASIZE)) > 0)
		{
		if (((len2 = read(fd2, buff2, DATASIZE)) != len1)
			|| (memcmp(buff1, buff2, len1)))
			return (DC_MISMATCH
				| ((len1 > len2) ? DC_LARGER : 0)
				| ((len1 < len2) ? DC_SMALLER : 0));
		}

	return (DC_EQUAL);
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
//	int
//putdiff (
//	int    flag,		/* The comma flag */
//	char  *msg)			/* The message string */
//
//	{
//	if (flag)
//	putchar(',');
//	putchar(' ');
//	flag = TRUE;
//	fputs(msg, stdout);
//
//	return (flag);
//	}

/* ----------------------------------------------------------------------- */
	void
cantfind (				/* Inform user of input failure */
	const char  *fnp)	/* Input file name */
    
	{
	if (!q_flag)
		fprintf(stderr, "Unable to find file: %s\n", fnp);
	if (z_flag)
		exit(1);
	}

/* ----------------------------------------------------------------------- */
	void
file1error (			/* Inform user of input failure */
	char  *fnp)			/* Input file name */
    
	{
	if (!q_flag)
		fprintf(stderr, "File 1 is missing or invalid: %s\n", fnp);
	if (z_flag)
		exit(1);
	else if (!q_flag)
		usage();
	}

/* ----------------------------------------------------------------------- */
	void
file2error (			/* Inform user of input failure */
	char  *fnp)			/* Input file name */
    
	{
	if (!q_flag)
		fprintf(stderr, "File 2 is missing or invalid: %s\n", fnp);
	if (z_flag)
		exit(1);
	else if (!q_flag)
		usage();
	}

/* ----------------------------------------------------------------------- */
	void
fat_err (					/* Report a fatal error */
	char  *s)			/* Pointer to the message string */

	{
	fprintf(stderr, "%s\n", s);
	if (z_flag)
		exit(1);
	else
		usage();
	}

/* ----------------------------------------------------------------------- */
