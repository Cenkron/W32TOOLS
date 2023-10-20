/*********************************************************************\
                                XCOPY
 ---------------------------------------------------------------------
                        Extended COPY utility
 ---------------------------------------------------------------------
   Copyright (c) 1986-2018 Miller Micro Systems - All Rights Reserved
                    Written by Michael S. Miller

|	 4-Nov-03 /Tnnn option (for NTFS problems)
\*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>

#include <dtypes.h>
#include <fWild.h>

#include "xcopy.h"
#include "more_lib.h"

/**********************************************************************/

#define COPYRIGHT "V8.0 Copyright (c) 1986-2018 J & M Software, Dallas TX - All Rights Reserved"

#pragma comment (user, "XC ")
#pragma comment (user, COPYRIGHT " ")
#pragma comment (user, "Edited " __TIMESTAMP__ " ")
#pragma comment (user, "Compiled " __DATE__ " " __TIME__ " ")

#define VERSION "000410.092321"

/**********************************************************************/
	static char 
helpline [] =
	"Usage:\n  xc [-options?] [-Odt] [-Ydt] [-X...] src1 [[src2...] destpath]";

	static char 
optstring [] =
	"aAbcdDefhklLmnoO:pPqQrRsSuUvVwxX:yY:z?";

/* options used: aAb c dDe f   h     k lLm n oOpPqQrRsS  uUvVw xXyYz  */
/* options left:    B C   E FgG HiIjJ K   M N          tT     W     Z */

	char * 
usagedoc [] = {
	helpline,
	"",
	"e/X/tended /C/opy program.",
	"",
	"XC conditionally copies a set of files to another drive or directory.",
	"",
	"The source filespec[s] may consist of an optional drive designator, an ",
	"optional path name, and a possibly wild file name.  If no file name is",
	"specified, then '*' is assumed.  Wildcards '?', '*', and '**' are ",
	"allowed in the filespec.  Appearance of '**' in the filespec",
	"will cause the corresponding tree to be built on the destination",
	"unless -f is used.",
	"",
	"The destination path consists of either a drive designator or a path",
	"name, or both.  No renaming capability is provided.  If exactly one",
	"source filespec is specified, and the destination is omitted, then",
	"'.' is assumed for the destination.",
	"",
	"If -q or -Q is selected, a prompt will be issued before each copy.",
	"Enter Y to copy, N to not copy, R to copy remaining (cancels -q),",
	"C or Q to cancel/quit the copy. -q accesses stdin, -Q the raw keyboard.",
	"",
	"If -A is selected, the file is copied in ASCII mode (one line at a time).",
	"New-lines (LF) are converted to DOS convention (CR-LF).",
	"If -U is selected, -A is assumed, and furthermore, new-lines are not",
	"converted. Use -A when copying text files from a Unix-like system,",
	"and -U when copying text files to a Unix-like system.",
	"",
	"Date-time parameters (dt) for -O and -Y are virtually free-format.",
	"",
	"-a        clear /a/rchive attribute bit after copy",
	"-A        /A/SCII text copy mode",
	"-b        /b/rief output listing",
	"-c        pause for destination diskette /c/hange when full",
	"-d        copy only if destination exists and is /d/ifferent time",
	"-D        copy only if destination exists and is /D/ifferent size",
	"-e        copy only if destination /e/xists (overwrite it)",
	"-f        do not expand directory tree on destination (/f/lat)",
	"-h        include /h/idden files",
	"-k        /k/ill the old file after copy",
	"-l        no /l/ist:  do not output progress to screen",
	"-L        /L/ist long form of the file names when copying",
	"-m        copy only if destination is /m/issing (does not exist)",
	"-n        /n/o execute - just report what would be done",
	"-o        copy only if source is /o/lder than destination",
	"-Odt      copy only if source is /O/lder than 'dt'",
	"-p        /p/rotect destination: rename, copy, erase original if copy good",
	"-P        /P/redelete the destination file",
	"-q        /q/uery stdin for each source file before copying",
	"-Q        /Q/uery kbd for each source file before copying",
	"-r        even if destination is /r/ead-only",
	"-R        ASCII text copy mode with carriage /R/eturn at end of line (OS-9)",
	"-s        include /s/ystem files",
	"-S        don't check file /S/ize for destination disk fit",
	"-Tnnn     Offset timestamp 1 by (+/-)nnn hours, default 0",
	"-u        only if /u/narchived (archive attribute bit is set)",
	"-U        ASCII text copy mode with /U/nix output convention",
	"-v        /v/erify data after copy",
	"-V        /V/erbose output",
	"-w        don't report source file(s) not found",
	"-x        e/x/it on first error",
	"-X <pathspec> e/X/clude (possibly wild) paths that match pathspec",
	"-X @<xfile>   e/X/clude paths that match pathspec(s) in xfile",
	"-X-       Don't exclude the default system file exclusion(s)",
	"-X+       Show exclusion path(s) being used",
	"-X=       Show excluded paths",
	"-y        copy only if source is /y/ounger than destination",
	"-Ydt      copy only if source is /Y/ounger than 'dt'",
	"-z        always return exit code of /z/ero",
	"",
	COPYRIGHT,
	NULL};

/**********************************************************************/

EXPORT  int     main (int, char**);

PRIVATE void    filepair (char *s1, char *s2);

/**********************************************************************/

A_Z_FLAGS       AZ_Flags             = {0};
A_Z_FLAGS       azFlags              = {0};
int				v_flag				 = 0;
unsigned int    bsize                = 0;
time_t          timedelta            = 0L;	/* Timestamp compare correction */
char *          buffer               = NULL;
int             cols                 = 40;
UINT64          filesize             = 0;
int             mode                 = 0;
time_t          o_time               = 0L;
char            path_char            = 0;
char            temp_name [MAX_PATH] = "";
time_t          y_time               = 0L;
int				copy_rename		     = 0;	// Enables direct copy and rename option

PHP		hp = NULL;				// FWILD instance pointer
PEX		xp = NULL;				// FEX instance pointer

static char			src  	[MAX_PATH];	// src file/pathspec to filepair()
static char			dst  	[MAX_PATH];	// dst file/pathspec to filepair()
static char			temp 	[MAX_PATH];	// for error messages
static char			dstName	[MAX_PATH];	// for building the destination filename
static char			effPath [MAX_PATH];	// for the dst ref path in process()

/* ----------------------------------------------------------------------- */

/* ----------------------------------------------------------------------- */
	EXPORT int
main (
	int argc,
	char *argv[])

	{
	int         c;
	int         nargs;


	if ((hp = fwOpen()) == NULL)
		exit(1);
	if ((xp = fExcludeOpen()) == NULL)
 		exit(1);

	optenv = getenv("XC");

	mode = FW_FILE;

	while ((c=getopt(argc, argv, optstring)) != EOF)
		switch (c)
			{
			case 'a': azFlags.a  = !azFlags.a;  break;	/* clear archive bit */
			case 'A': AZ_Flags.a = !AZ_Flags.a; break;	/* ASCII mode        */
			case 'b': azFlags.b  = !azFlags.b;  break;	/* brief output      */
			case 'c': azFlags.c  = !azFlags.c;  break;	/* change disk       */
			case 'd': azFlags.d  = !azFlags.d;  break;	/* if different time */
			case 'D': AZ_Flags.d = !AZ_Flags.d; break;	/* if different size */
			case 'e': azFlags.e  = !azFlags.e;  break;	/* if exist          */
			case 'f': azFlags.f  = !azFlags.f;  break;	/* flat              */
//			case 'g': azFlags.g  = !azFlags.g;  break;	/*   -- unused --    */
			case 'h': azFlags.h  = !azFlags.h;  break;	/* even hidden files */
//			case 'i': azFlags.i  = !azFlags.i;  break;	/*   -- unused --    */
//			case 'j': azFlags.j  = !azFlags.j;  break;	/*   -- unused --    */
			case 'k': azFlags.k  = !azFlags.k;  break;	/* kill old file     */
			case 'l': azFlags.l  = !azFlags.l;  break;	/* list              */
			case 'L': AZ_Flags.l = !AZ_Flags.l; break;	/* list simple form  */
			case 'm': azFlags.m  = !azFlags.m;  break;	/* if missing        */
			case 'n': azFlags.n  = !azFlags.n;  break;	/* no execute        */
			case 'o': azFlags.o  = !azFlags.o;  break;	/* if older          */
			case 'p': azFlags.p  = !azFlags.p;  break;	/* protect dest      */
			case 'P': AZ_Flags.p = !AZ_Flags.p; break;	/* predelete dest    */
			case 'q': azFlags.q  = !azFlags.q;  break;	/* query first       */
			case 'Q': AZ_Flags.q = !AZ_Flags.q; break;	/* query first       */
			case 'r': azFlags.r  = !azFlags.r;  break;	/* even if read_only */
			case 'R': AZ_Flags.r = !AZ_Flags.r; break;	/* ASCII-CR          */
			case 's': azFlags.s  = !azFlags.s;  break;	/* even system files */
			case 'S': AZ_Flags.s = !AZ_Flags.s; break;	/* check file size   */
//			case 't': azFlags.t  = !azFlags.t;  break;	/*   -- unused --    */
			case 'u': azFlags.u  = !azFlags.u;  break;	/* if archive bit    */
			case 'U': AZ_Flags.u = !AZ_Flags.u; break;	/* Unix output mode  */
			case 'v': azFlags.v  = !azFlags.v;  break;	/* verify            */
			case 'w': azFlags.w  = !azFlags.w;  break;	/* Quiet if no files */
			case 'x': azFlags.x  = !azFlags.x;  break;	/* exit on first error */

			// Options that are treated differently

			case 'V':	++v_flag;				break;	// Verbose output

			case 'X':									/* exclude ... */
				if      (optarg[0] == '-')
					fExcludeDefEnable(xp, FALSE);		/* Disable default file exclusion(s) */
				else if (optarg[0] == '+')
					fExcludeShowConf(xp, TRUE);			/* Enable stdout of exclusion(s) */
				else if (optarg[0] == '=')
					fExcludeShowExcl(xp, TRUE);			/* Enable stdout of excluded path(s) */
				else if (fExclude(xp, optarg))
					break;

			case 'y': azFlags.y  = !azFlags.y;  break;	/* if younger        */
			case 'z': azFlags.z  = !azFlags.z;  break;	/* return errorlevel 0 */

			case 'T':									/* Offset timestamp by (+/-)nnn hours, default 0" */
				timedelta = OneHour * strtol(optarg, NULL, 10);
				break;
    
			case 'O':									/* copy only if source is /O/lder than 'dt' */
				if ((o_time=sgettd(optarg)) == 0)
					{
					sprintf(temp, "Date-time %s error %s", optarg, serrtd());
					fatal(temp);
					}
				else
					++AZ_Flags.o;
				break;
    
			case 'Y':									/* copy only if source is /Y/ounger than 'dt' */
				if ((y_time=sgettd(optarg)) == 0)
					{
					sprintf(temp, "Date-time %s error %s", optarg, serrtd());
					fatal(temp);
					}
				else
					++AZ_Flags.y;
				break;
    
			case '?':									/* Show detailed help */
				mhelp();
				/*unreachable */
    
			default:
				fprintf(stderr, "invalid option '%c'\n", optchar);
				usage();
			}

	if (v_flag)
		printf("Verbose is %d\n", v_flag);

	if (azFlags.s)
		mode |= FW_SYSTEM;              /* even system files */
	if (azFlags.h)
		mode |= FW_HIDDEN;              /* even hidden files */

	if (AZ_Flags.q)                     /* one query deserves another */
		azFlags.q = TRUE;

	if (AZ_Flags.u || AZ_Flags.r)       /* if Unix or OS-9 then ascii */
		AZ_Flags.a = TRUE;

	/* adjust screen output for actual screen size */
	if (isatty(fileno(stdout))
	&&  ((c=getcols()) > 80))
		cols = c / 2;

	nargs = argc - optind;
	if (nargs < 1)
		fatal("At least one pathname is required");
	if (nargs > 2)
		fatal("Only two pathnames are allowed");

	pathCopy(src, argv[optind++], MAX_COPY);	// Load the filename buffers
	if (nargs == 2)
		pathCopy(dst, argv[optind], MAX_COPY);
	else // (nargs == 1)
		strcpy(dst, "");
//BWJ
//	if (strcmp(src, ".") == 0)			// Change "." to "*"
//		*src = '*';

//BWJ - no longer needed  (see shcopy.c)
//	if ((_fnabspth(dst, dst)) != 0)
//		fatal("dst pathspec error");

	if ((! isPhysical(src))
	||  (fwValid(src) != FWERR_NONE))
		fatal("Invalid source file/path specification");

	if ((! isPhysical(dst))
	||  (fwValid(dst) != FWERR_NONE))
		fatal("Invalid destination file/path specification");

	if (isWild(dst))
		fatal("Destination cannot be wild");
	if (pnOverlap(src, dst))
		fatal("Source, destination overlap");

	// if src is an existing file, and dst could be a valid filename after copying
	// then treat it as a file copy with rename of the dst name

	if (v_flag)
		{
		printf("XC Src path:  \"%s\"\n", src);
		printf("XC Dst path:  \"%s\"\n", dst);
		}

	// Handle the single file copy and rename case

	if ((fnchkfil(src))
	&&  (! fnchkdir(dst))
	&&  (fnValidName(dst)))
		{
		if (v_flag)
			printf("Doing a copy-rename to:  \"%s\"\n", dst);
		copy_rename	= TRUE;
		}

	else if (fnchkfil(dst) != FWERR_NONE)
		fatal("Destination cannot be an existing file");

	if (v_flag)
		printf("Doing a normal copy to:  \"%s\"\n", dst);

	filepair(src, dst);

	xp = fExcludeClose(xp);					// Close the Exclusion instance
	hp = fwClose(hp);
	return (0);
	}

/* ----------------------------------------------------------------------- */

/* ----------------------------------------------------------------------- */
// filepair ()
/* ----------------------------------------------------------------------- */
	void
filepair(     				/* Process the pathname pairs */
	char *pSrcPath,			/* Pointer to the pathname1 string */
	char *pDstPath)			/* Pointer to the pathname2 string */

	{
	int   CatIndex;			/* Concatenation index of the path */
	int   TermIndex;		/* Termination index of the path (not used here) */
	char *pSrcName;         /* Pointer to the path1 filename */
	char *pDstName;         /* Pointer to the path2 filename */
	char *pDstEffPath;		/* Pointer to the path2 pathname */

	if (v_flag > 3)
		{	
printf("FP srcp: \"%s\"\n", pSrcPath);
printf("FP dstp: \"%s\"\n", pDstPath);
		}
	if (fnParse(pSrcPath, &CatIndex, &TermIndex) < 0)	/* Set pointer to construct path2 */
		fatal("src pathspec error");

	fnreduce(pDstPath);

	if (fwInit(hp, pSrcPath, mode) != FWERR_NONE)	// Process the pattern
		fwInitError(pSrcPath);
	fExcludeConnect(xp, hp);						// Connect the exclusion instance
	if ((pSrcName = fWild(hp)) == NULL)				// Process files
		{
		notfound(pSrcPath);
		}
	else
		{
		do	{			/* Process all srcPath files */
			filesize = (__int64)(fwsize(hp));

			if (copy_rename)	// For this case, pDstPath is actually the dst filename
				{
				pDstName = pDstPath;					// Use the dst path as the filename
				int result = fnGetPath(effPath, pDstPath);		// Manufacture the dst path
				if (! result)
					fatal("dst pathspec error 1");
				pDstEffPath = effPath;
				}

			else if (azFlags.f)	// For the remaining two cases, pDstPath is a dst directory
				{
				pDstName = dstName;
				if ((_fncatpth(pDstName, pDstPath, fntail(pSrcName))) != 0)
					fatal("dst pathspec error 2");
				pDstEffPath = pDstPath;
				}
			else
				{
				pDstName = dstName;
				if (v_flag > 3)
					{
printf("-pDstName  \"%s\"\n", pDstName);
printf("-pSrcPath  \"%s\"\n", pDstPath);
printf("-pSrcName  \"%s\"\n", pSrcName);
printf("-[%d]       \"%s\"\n", CatIndex, (pSrcName + CatIndex));
					}
				if ((_fncatpth(pDstName, pDstPath, (pSrcName + CatIndex))) != 0)
					fatal("dst pathspec error 3");
				pDstEffPath = pDstPath;
				}

			if (v_flag > 3)
				{	
printf("FP1 srcn: \"%s\"\n", pSrcName);
printf("FP1 dstn: \"%s\"\n", pDstName);
printf("FP1 EP:   \"%s\"\n", pDstEffPath);
fflush(stdout);
				}

			process(hp, pSrcName, pDstName, pDstEffPath);

			} while ((pSrcName = fWild(hp)) != NULL);

		}

	if (v_flag)
		{
printf("done\n");
fflush(stdout);
		}
	}

/**********************************************************************\
                                EOF
\**********************************************************************/
