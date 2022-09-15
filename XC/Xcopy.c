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
#include <fwild.h>

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
	"specified, then '*.*' is assumed.  If no filetype is specified,",
	"then '.*' is appended.  Wildcards '?', '*', and '**' are ",
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
	"-p        /p/rotect destination: rename, copy, erase original if good copy",
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

A_Z_FLAGS       AZ_Flags        = {0};
A_Z_FLAGS       azFlags         = {0};
unsigned int    bsize		= 0;
long            timedelta       = 0L;	/* Timestamp compare correction */
char *          buffer		= NULL;
int             cols            = 40;
INT64           filesize        = 0;
int             mode            = 0;
time_t          o_time          = 0L;
char            path_char	= 0;
char            temp_name [80]	= "";
time_t          y_time          = 0L;

/* ----------------------------------------------------------------------- */

/* ----------------------------------------------------------------------- */
	EXPORT int
main (
	int argc,
	char *argv[])

	{
	int         c;
	int         lastarg;
	char *      dst;
	char        src [1024];

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
			case 'V': AZ_Flags.v = !AZ_Flags.v; break;	/* Verbose output    */
			case 'w': azFlags.w  = !azFlags.w;  break;	/* Quiet if no files */
			case 'x': azFlags.x  = !azFlags.x;  break;	/* exit on first error */

			case 'X':									/* exclude ... */
				if      (optarg[0] == '-')
					fexcludeDefEnable(FALSE);		/* Disable default file exclusion(s) */
				else if (optarg[0] == '+')
					fexcludeShowConf(TRUE);			/* Enable stdout of exclusion(s) */
				else if (optarg[0] == '=')
					fexcludeShowExcl(TRUE);			/* Enable stdout of excluded path(s) */
				else if (fexclude(optarg))
					break;

			case 'y': azFlags.y  = !azFlags.y;  break;	/* if younger        */
			case 'z': azFlags.z  = !azFlags.z;  break;	/* return errorlevel 0 */

			case 'T':									/* Offset timestamp by (+/-)nnn hours, default 0" */
				timedelta = OneHour * strtol(optarg, NULL, 10);
				break;
    
			case 'O':									/* copy only if source is /O/lder than 'dt' */
				if ((o_time=sgettd(optarg)) < 0)
					{
					sprintf(src, "Date-time %s error %s", optarg, serrtd());
					fatal(src);
					}
				else
					++AZ_Flags.o;
				break;
    
			case 'Y':									/* copy only if source is /Y/ounger than 'dt' */
				if ((y_time=sgettd(optarg)) < 0)
					{
					sprintf(src, "Date-time %s error %s", optarg, serrtd());
					fatal(src);
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

	if (optind < argc)
		{
		if (optind == (argc-1))
			{
			dst = ".";
			lastarg = argc;
			}
		else
			{
			dst = argv[argc-1];
			lastarg = argc-1;
			}

		if (fwvalid(dst) != FWERR_NONE)
			fatal("Invalid destination path specification");
		else if (fnchkfil(dst))
			fatal("Destination cannot be a file");
		else if (iswild(dst))
			fatal("Cannot have wildcards in the destination");

		dst = fnabspth(dst);

		while (optind < lastarg)
			{
			strcpy(src, argv[optind]);

			if (fwvalid(src) != FWERR_NONE)
				fatal("Invalid source file specification");
			else if ((fnchkdir(src))			// if src is a directory,    assume *.*
				 ||  (strcmp(fntail(src), "**") == 0))	// if src is recurse wild,   assume *.*
				catpth(src,"*.*");
			else if (strchr(fntail(src), '.') == NULL)	// if src specifies no type, assume  .*     */
				strcat(src,".*");

			filepair(src, dst);

			++optind;
			}
		free(dst);
		}

	else if (optind == (argc-1))
		fatal("Must specify destination pathname");

	else
		usage();

	return(0);
	}

/* ----------------------------------------------------------------------- */

/* ----------------------------------------------------------------------- */
	void
filepair(          /* Process the pathnames */
	char* s1,                  /* Pointer to the pathname1 string */
	char* dstpath             /* Pointer to the pathname2 string */
	)

	{
	char* srcname;             /* Pointer to the path1 pathname */
	char* dstname;             /* Pointer to the path2 pathname */

	const int index = suffix(s1);       /* Set pointer to construct path2 */

	char* hp = fwinit(s1, mode);		/* Find the first path1 file */
	fwExclEnable(hp, TRUE);				/* Enable file exclusion */
	if ((srcname = fwild(hp)) == NULL)	/* Process files */
		{
		hp = NULL;
		notfound(s1);
		}
	
	else do  
		{				/* Process all path1 files */
		filesize = (__int64)(fwsize(hp));

// printf("src: \"%s\"\n", srcname);
// printf("dp:  \"%s\"\n", dstpath);
// printf("tail:\"%s\"\n", fntail(srcname));
// printf("ndx: \"%s\"\n", (srcname + index));

		if (azFlags.f)
			dstname = fncatpth(dstpath, fntail(srcname));
		else
			dstname = fncatpth(dstpath, (srcname + index));

		process(srcname, hp, dstname, dstpath);

		free(dstname);
		} while ((srcname = fwild(hp)) != NULL);
	hp = NULL;
	}

/**********************************************************************\
                                EOF
\**********************************************************************/
