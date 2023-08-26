/*********************************************************************\
				MV
 ---------------------------------------------------------------------
               Move (a) file(s) to another directory
 ---------------------------------------------------------------------
   Copyright (c) 1986-2018 Miller Micro Systems - All Rights Reserved
                    Written by Michael S. Miller
\*********************************************************************/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <process.h>

#include "dtypes.h"
#include "fwild.h"
#include "getoptns.h"

/**********************************************************************/
#define VERSION "980906.043000"
//#define DEBUG
/**********************************************************************/

#define COPYRIGHT "Version 5.5 Copyright (c) 2018 J & M Software, Dallas TX - All Rights Reserved"

#pragma comment (user, "mv ")
#pragma comment (user, COPYRIGHT " ")
#pragma comment (user, "Edited " __TIMESTAMP__ " ")
#pragma comment (user, "Compiled " __DATE__ " " __TIME__ " ")

/**********************************************************************/
    static char
optstring [] =
    "cCfFhHlLnNoOqQrRsSx:X:zZ?";

    char
helpline [] =
    {"Usage:  mv  [-?cfhlnoqrsxz]  [-X...]  [path\\]file_list  new_path"};

    char *
usagedoc [] = {
    helpline,
    "",
    "Move the files in the file_list to the subdirectory new_path",
    "",
    "    -c    use x/c/ to copy & delete the files if to different drive",
    "    -f    do not expand directory tree on destination (/f/lat)",
    "    -h    include /h/idden files",
    "    -l    no /l/ist: do not display progress",
    "    -n    /n/o execute: do not actually execute the command(s)",
    "    -o    /o/verwrite destination file if it exists",
    "    -q    /q/uery before each move",
    "    -r    even if destination is /r/ead-only",
    "    -s    include /s/ystem files",
    "    -x    e/x/it on first error",
    "    -X<pathspec> e/X/clude (possibly wild) paths that match pathspec",
    "    -X@<xfile>   e/X/clude files that match pathspec(s) in xfile",
	"    -X-   disable default file exclusion(s)",
	"    -X+   show exclusion path(s)",
	"    -X=   show excluded path(s)",
    "    -z    always return exit code of /z/ero",
    "",
    "The file_list may contain the wildcard characters '**', '*', and/or '?'.",
    "",
    COPYRIGHT,
    NULL};

/**********************************************************************/

int	main         (int, char**);
void	catpth       (char *s, char *t);
void	error        (char *filename, char *message);
void	fatal        (char *filename, char *message);
void	filepair     (char *s1, char *s2);
void	move         (char *src, char *dst);
void	notfound     (char *fn);
int	query        (char *src, char *dst);
int     SameDisk     (char *src, char *dst);
void	MVprocess    (char *src, char *dst, char *path);
void	XCprocess    (char *src, char *dst);

int	is_readonly  (char *file);
int	clr_readonly (char *file);
int	set_readonly (char *file);
void	InterSpace   (char *src, char *dst);

/**********************************************************************/

A_Z_FLAGS	azFlags		= {0};
char		path_char;
int             cols            = 40;
int		attrib		= FW_FILE;

/* ----------------------------------------------------------------------- */
#ifdef DEBUG
#define debug(x) printf x
#else
#define debug
#endif
/* ----------------------------------------------------------------------- */

/* ----------------------------------------------------------------------- */
	int
special_options (
	int c,
	char *arg) /*lint -esym(715,arg)  */

	{
	switch (c)
		{
		case 'x':
			optdata.flags.x = !optdata.flags.x;	// Lower case:
			break;

		case 'X':
			if      (optarg[0] == '-')			// (Upper case)
				fexcludeDefEnable(FALSE);		/* Disable default file exclusion(s) */
			else if (optarg[0] == '+')
				fexcludeShowConf(TRUE);			/* Enable stdout of exclusion(s) */
			else if (optarg[0] == '=')
				fexcludeShowExcl(TRUE);			/* Enable stdout of excluded path(s) */
			else if (fexclude(optarg))
				{
				fprintf(stderr, "Error excluding '%s'\n", arg);
				exit(1);
				}
			break;

		default:;
		}

	return (0);
	}

/* ----------------------------------------------------------------------- */

/* ----------------------------------------------------------------------- */
	int
main (
	int   argc,
	char *argv[])

	{
	int		iResult;	// = 0;
	int		lastarg;
	int		c;
	char   *dst;
	char	src [1024];

	optdata.pProc[GETOPT_X] = special_options;

	if ((iResult=GetOptions("MV", argc, argv, optstring)) != 0)
		return (iResult);

	if (optdata.flags.s)	attrib |= FW_SYSTEM;
	if (optdata.flags.h)	attrib |= FW_HIDDEN;

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
			fatal(dst, "Invalid destination path specification");
		else if (fnchkfil(dst))
//		else if ( ! fnchkdir(dst))
			fatal(dst, "Destination is not a directory");
		else if (iswild(dst))
			fatal(dst, "Cannot have wildcards in the destination");

		dst = fnabspth(dst);

		while (optind < lastarg)
			{
			strcpy(src, argv[optind]);

			if (fwvalid(src) != FWERR_NONE)
				fatal(src, "Invalid source file specification");
			else if ((fnchkdir(src))			// if src is a directory,    assume *.*
				 ||  (strcmp(fntail(src), "**") == 0))	// if src is recurse wild,   assume *.*
				catpth(src, "*.*");
			else if (strchr(fntail(src), '.') == NULL)	// if src specifies no type, assume  .*     */
				strcat(src, ".*");

//debug(("before filepair('%s', '%s')\n", src, dst));

			if (SameDisk(src, dst))
				filepair(src, dst);
			else
				XCprocess(src, dst);

			++optind;
			}
		if (dst)
			free(dst);
		}

	else if (optind == (argc-1))
		fatal("?", "Must specify destination pathname");

	else
		usage();

	return(0);
	}

/* ----------------------------------------------------------------------- */
	void
filepair (					/* Process the pathnames */
	char  *s1,				/* Pointer to the pathname1 string */
	char  *dstpath)			/* Pointer to the pathname2 string */

	{
	int    CatIndex;		/* Concatenation index of the path */
	int    TermIndex;		/* Termination index of the path (not used here) */
	char  *srcname;			/* Pointer to the path1 pathname */
	char  *dstname;			/* Pointer to the path2 pathname */

	fnParse(s1, &CatIndex, &TermIndex);	/* Set pointer to construct path2 */

	char *hp = fwinit(s1, attrib);		/* Find the first path1 file */
	fwExclEnable(hp, TRUE);				/* Enable file exclusion */
	if ((srcname = fwild(hp)) == NULL)
		{
		hp = NULL;
		notfound(s1);
		}

	else do
		{				/* Process all path1 files */
		if (optdata.flags.f)
			dstname = fncatpth(dstpath, fntail(srcname));
		else
			dstname = fncatpth(dstpath, (srcname + CatIndex));

//debug(("before process('%s', '%s', '%s')\n", srcname, dstname, dstpath));

		MVprocess(srcname, dstname, dstpath);

		if (dstname)
			free(dstname);
		}  while ((srcname = fwild(hp)) != NULL);
	hp = NULL;
	}

/* ----------------------------------------------------------------------- *\
|  Quote a path
\* ----------------------------------------------------------------------- */
	static void
quote_path (		// Print a file/directory name in normal mode
	char  *unquoted,	// Pointer to the unquoted path string
	char  *quoted)	// Pointer to the quoted path string

	{
	int  quotes = 0;	// Number of quotes to be added


	for (char *p = unquoted; (*p != '\0'); ++p)
		{
		if (isspace(*p))
			{
			quotes = 2;	// Quotes are needed
			break;
			}
		}

	if (quotes != 0)
		*(quoted++) = '\"';
	strcpy(quoted, unquoted);
	quoted += strlen(unquoted);
	if (quotes != 0)
		{
		*(quoted++) = '\"';
		*quoted     = '\0';
		}
	}

/* ----------------------------------------------------------------------- *\
|  Perform a same disk move
\* ----------------------------------------------------------------------- */
	void
MVprocess (
	char *	src,	/* source file name */
	char *	dst,	/* desination file name */
	char *	path)	/* destination path name */

	{
	fnreduce(src);
	fnreduce(dst);
	fnreduce(path);

//debug(("inside process(): src ='%s'\n", src));
//debug(("inside process(): dst ='%s'\n", dst));
//debug(("inside process(): path='%s'\n", path));

	if (query(src, path))
		move(src, dst);
	}

/* ----------------------------------------------------------------------- *\
|  Perform a different disk move using spawned XC
\* ----------------------------------------------------------------------- */
	void
XCprocess (
	char *	src,	/* source file name */
	char *	dst)	/* desination path name */

	{
	if (optdata.flags.c)	/* if src drive =/= dst drive understood */
		{
		char  xcflags [8] = "-vk";
		char  qsrc [1027];
		char  qdst [1027];

		fnreduce(src);
		fnreduce(dst);

		quote_path(src, qsrc);
		quote_path(dst, qdst);

//debug(("inside process(): src ='%s'\n", qsrc));
//debug(("inside process(): dst ='%s'\n", qdst));

		if (optdata.flags.f)
			strcat(xcflags, "f");
		if (optdata.flags.q)
			strcat(xcflags, "q");

		const int rc = (int)(spawnlp(P_WAIT, "xc", "xc", xcflags, qsrc, qdst, NULL));
		if (rc)
			fatal(src, "copy/delete process failed");
		}

	else
		fatal(src, "cannot move to a different drive:  use -c");
    }

/* ----------------------------------------------------------------------- *\
|  Determine whether the requested move is same disk or not
\* ----------------------------------------------------------------------- */
	int			// Returns TRUE if a same disk move is requested
SameDisk (
	char *	src,	/* source file name */
	char *	dst)	/* desination path name */

	{
	fnreduce(src);
	char *	xsrc = fnabspth(src);

	// Note: also returns FALSE if UNC paths are specified

	const int result = (tolower(xsrc[0]) == tolower(dst[0]))
		&& (xsrc[1] == ':')
		&& ( dst[1] == ':');

	if (xsrc)
		free(xsrc);

	return result;
	}

/* ----------------------------------------------------------------------- */

/* ----------------------------------------------------------------------- */
	void
move (char *src, char *dst)
	{
	    //debug(("inside move('%s', '%s')\n", src, dst));
    
	if (!optdata.flags.l && !optdata.flags.q)
		{
		printf("Moving     %s", src);
		InterSpace(src, dst);
		printf("  to  %s", dst /*path*/);
		}

	const long dstdate = fgetfdt(dst);

	if (dstdate != -1L)
		{
		if (!optdata.flags.o)
			{
			error(dst, "target file exists: use -o switch");
			goto err_exit;
			}

		if (is_readonly(dst))
			{
			if ( !optdata.flags.r )
				{
				error(dst, "target file exists and is read-only: use -ro switches");
				goto err_exit;
				}
			else
				{
				if (!optdata.flags.n)
					clr_readonly(dst);
				}
			}
    
		if ((!optdata.flags.n)
		&&  (unlink(dst) != 0))
			{
			error(dst, "Unable to delete existing file");
			goto err_exit;
			}
		}
    
	if ((!optdata.flags.n)
	&&  (prename(src, dst) != 0))
		error(src, "Unable to rename file");

err_exit:
	if (!optdata.flags.l && !optdata.flags.q)
		putchar('\n');
	}

/* ----------------------------------------------------------------------- */
/*--------------------------------------------------------------------*/
	void
catpth (
	char *s,
	char *t)

	{
	char * p;

	if ((p=fncatpth(s,t)) != NULL)
		{
		strcpy(s,p);
		free(p);
		}
	}

/*--------------------------------------------------------------------*/
    int
query (
	char *src,
	char *dst)

    {
	int		retval		= TRUE;

	if (optdata.flags.q)
		{
		printf("Move    %s", src);
		InterSpace(src, dst);
		printf("  to  %s?  [Y/N/R/QC] : ", dst);

		const char key = get_key(FALSE, TRUE);
	
		switch (tolower(key))
			{
			case 'y':
				retval = TRUE;
				break;

			case 'r':
				optdata.flags.q = FALSE;
				retval = TRUE;
				break;

			case 'q':
			case 'c':
				printf("Move terminated\n");
				exit(0);

			default:
				retval = FALSE;
				break;
			}
		}

	return (retval);
	}

/*--------------------------------------------------------------------*/
	int
is_readonly (
	char *filename)

	{
    int	    attrib;	    // = -1;

	if ((attrib=fgetattr(filename)) < 0)
		return (0);

	return (attrib & ATT_RONLY);
	}

/*--------------------------------------------------------------------*/
	int
clr_readonly (
	char *filename)

	{
	int	    attrib;	    // = -1;

	if ((attrib=fgetattr(filename)) >= 0)
		{
		if (fsetattr(filename, attrib & ~ATT_RONLY) < 0)
			{
			if (!optdata.flags.z)
				printf("\n\aUnable to change attributes: %s\n",filename);
			}
		}
	return (0);
	}

/*--------------------------------------------------------------------*/
	int
set_readonly (
	char *filename)

	{
	int	    attrib;	    // = -1;

    if ((attrib=fgetattr(filename)) >= 0)
		{
		if (fsetattr(filename, attrib | ATT_RONLY) < 0)
			{
			if (!optdata.flags.z)
				printf("\n\aUnable to change attributes: %s\n",filename);
			}
		}
	return (0);
	}

/*--------------------------------------------------------------------*/
	void
notfound (
	char *fn)
    
	{
	if (!optdata.flags.z)
		printf("\n\aFile not found: %s\n",fn);

	if (optdata.flags.x)
		{
		if (optdata.flags.z)
			exit(0);
		else
			exit(1);
		}
    }

/*--------------------------------------------------------------------*/
	void
error (
	char *filename,
	char *message)

	{
	if (!optdata.flags.z)
		printf("\n\aFile error: %s - %s\n", filename, message);

	if (optdata.flags.x)
		exit(optdata.flags.z ? 0 : 1);
	}

/*--------------------------------------------------------------------*/
	void
fatal (
	char *file,
	char *text)

	{
	if (optdata.flags.z)
		exit(0);
	else
		{
		printf("\n\aFatal error: ('%s') %s\n", file, text);
		exit(1);
		}
	}

/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*\

	---left---   srclen -middle- dstlen --------right----------

	Moving_____    %s    __to__    %s   ?[Y/N/R/QC]_:_  %c<ret>

\*--------------------------------------------------------------------*/
	void
InterSpace (char *src, char *dst)
	{
	int		spacing;
	const int	left	= 11;
	const int	middle	= 6;
	int		right;

		 if (azFlags.q)	right = 18;
	else if (azFlags.v)	right = 4;
	else		right = 1;

	const int srclen = (int)(strlen(src));
	const int dstlen = (int)(strlen(dst));

// Note that 'cols' is half the screen width (40 on 80x25 screen)

	if ((srclen+dstlen+left+middle+right) > (2*cols))
		spacing = 0;
	else if ((dstlen+middle+right) > cols)
		spacing = 2*cols - srclen - dstlen - left - middle - right;
	else
		spacing = cols - srclen - left;

	while (spacing--  >  0)	// takes care of negative case too
		putchar(' ');
	}

/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
