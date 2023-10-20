/*********************************************************************\
                CHMOD - change file attributes
\*********************************************************************/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dos.h>
#include <fWild.h>
#include <getoptns.h>
#include <getopt2.h>

/*--------------------------------------------------------------------*/
#define VERSION "930730.113707"
/*--------------------------------------------------------------------*/

#define DEBUG_LEVEL 0

/*--------------------------------------------------------------------*/
//#define DIRECTORIES
/*--------------------------------------------------------------------*/

#define DIRECTORIES
#define MASK		(ATT_VOLU | ATT_DIR)
#define HS_MODES	(FW_HIDDEN | FW_SYSTEM)

int		smode		= FW_FILE;	// File search mode attributes

int		plus_flags  = 0;		// Attribute changes added
int		minus_flags = 0;		// Attribute changes removed

int		c_flag      = 0;		// Compare attributes with proposed change, only 
int		f_flag      = 0;		// Force updates, even if no change
int		l_flag      = 0;		// List all files, not just those changed
int		q_flag      = 0;		// Query flag
int		Q_flag      = 0;		// Quiet output
int		change_req  = 0;		// Nonzero if any change requests entered

#if defined(DIRECTORIES)
int		d_flag      = 0;		// include directory options
#endif

static	char   *fargv [] = { "" };		/* Fake argv array */

PHP		hp			= NULL;		// FWILD instance pointer
PEX		xp			= NULL;		// FEX instance pointer

/**********************************************************************/
	static char 
copyright [] =
	{"Version 4.3+ Copyright (c) 1998, 2022 J & M Software, Dallas TX - All Rights Reserved"};

/**********************************************************************/
	char *
usagedoc [] =
{
#if defined(DIRECTORIES)
"Usage:  chmod  [-aAdDrRhHsS] [+arhs] [=arhs0] [-?cdqv] [-X...] [file_list]",
#else
"Usage:  chmod  [-aArRhHsS] [+arhs] [=arhs0] [-?cqv] [-X...] [file_list]",
#endif
"",
"Change the attributes of files in the file_list to match the given parameters.",
"",
"+         add attribute",
"-         remove attribute (lower case), add attribute (upper case)",
"=         set file to these attributes",
"",
"a         unarchived",
"r         read-only",
"h         hidden",
"s         system",
"0         no attributes",
"",
#if defined(DIRECTORIES)
"-d        directory: change attributes only on directories",
#endif
"-e        even if hidden or system; permit attribute changes",
"-F        force:     force change even if no change needed",
"-l        list:      list all files processed, not only those that are changed",
"-q        query:     ask before changing each file",
"-Q        Quiet:     do not output anything, except errors",
"-X <pathspec> e/X/clude (possibly wild) matching pathspec",
"-X @<xfile>   e/X/clude files that match pathspec(s) in xfile",
"-X-       disable default file exclusion(s)",
"-X+       show exclusion path(s)",
"-X=       show excluded path(s)",
"-c        compare:   do not change attributes, but check them",
"          return errorlevel 1 if file matches specified attributes",
"                            0 if it does not",
"",
"The file_list may contain the wild-card characters '**' '*' and/or '?'.",
"",
copyright,
NULL
};

/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
	int
query(
	char* s)

	{
	if (q_flag)
		{
		printf("change %-40s? [Y/N/R/QC] : ", s);

		const char key = get_key(FALSE, TRUE);
		if (key == 'y')
			return TRUE;
		if ((key == 'q') || (key == 'c'))
			{
			printf("Chmod terminated\n");
			exit(0);
			}
		if (key == 'r')
			{
			q_flag = FALSE;
			return TRUE;
			}

		return FALSE;
		}

	return TRUE;
	}

/*--------------------------------------------------------------------*/
	void
process (
	char *fnp)             /* Process one input file */

	{
	int attrib_old = fgetattr(fnp);
	int attrib_new = attrib_old;
	int changed    = FALSE;

	if (attrib_old < 0)
		fprintf(stderr, "Unable to get attributes: %s\n", fnp);
	else if (plus_flags | minus_flags)
		{
		attrib_new &= (~minus_flags);	// Determine the requested flags
		attrib_new |= plus_flags;

		if (!c_flag)	// Change flags, not just compare them
			{
			if (f_flag || (attrib_new != attrib_old))
				{
				if (query(fnp)  &&  (fsetattr(fnp, attrib_new) < 0))
					fprintf(stderr, "Unable to change attributes: %s\n", fnp);
				else
					{
					++changed;
#if 0
					printf("Changed attributes: %s\n", fnp);
#endif
					}
				}
			attrib_old = fgetattr(fnp);	// Refresh the flags
			}

		else  /* c_flag, only compare flags */
			{
			if (attrib_new != attrib_old)
				exit(0);
			else
				exit(1);
			}
		}

	if ( !Q_flag && (l_flag || !change_req || changed))
		{
		printf("%c%c%c%c%c   %s\n",
			((attrib_old & _A_ARCH)   ? 'a' : '-'),
			((attrib_old & _A_RDONLY) ? 'r' : '-'),
			((attrib_old & _A_HIDDEN) ? 'h' : '-'),
			((attrib_old & _A_SYSTEM) ? 's' : '-'),
			((attrib_old & _A_SUBDIR) ? 'd' : ' '),
			fnp);
		}
	}

/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
	int
configMinusOptions (
	char	optchar,
	char   *optarg)

	{
	int result = 0;

#if DEBUG_LEVEL > 0
	printf("ChmodConfig (-, %c, %s)\n", optchar, ((optarg) ? optarg : "(NULL)"));
#endif

	switch (optchar)
		{
		case 'c':
		case 'C':   ++c_flag;	break;

#if defined(DIRECTORIES)
		case 'd':
		case 'D':
			{
			++d_flag;
			smode |= FW_DIR;	// Change only directory attributes
			smode &= ~FW_FILE;
			break;
			}
#endif

		case 'e':
		case 'E':   smode |= HS_MODES;	break;

		case 'f':
		case 'F':   ++f_flag;	break;

		case 'l':
		case 'L':   ++l_flag;	break;

		case 'q':   ++q_flag;	break;
		case 'Q':   ++Q_flag;	break;

		case '?':   help();

		case 'x':
		case 'X':
			if      (optarg[0] == '-')			// (Upper case)
				fExcludeDefEnable(xp, FALSE);	// Disable default file exclusion(s)
			else if (optarg[0] == '+')
				fExcludeShowConf(xp, TRUE);		// Enable stdout of exclusion(s)
			else if (optarg[0] == '=')
				fExcludeShowExcl(xp, TRUE);		// Enable stdout of excluded path(s)
			else if (fExclude(xp, optarg))
				printf("Exclusion string fault: \"%s\"\n", optarg);
			break;


		// Attribute setting requests

		case 'a':   minus_flags |= _A_ARCH;		++change_req;	break;
		case 'A':   minus_flags |= _A_ARCH;		++change_req;	break;
		case 'h':   minus_flags |= _A_HIDDEN;	++change_req;	break;
		case 'H':   minus_flags |= _A_HIDDEN;	++change_req;	break;
		case 'r':   minus_flags |= _A_RDONLY;	++change_req;	break;
		case 'R':   minus_flags |= _A_RDONLY;	++change_req;	break;
		case 's':   minus_flags |= _A_SYSTEM;	++change_req;	break;
		case 'S':   minus_flags |= _A_SYSTEM;	++change_req;	break;

		default:    
			fprintf(stdout, "invalid \'-\' option \'%c\'\n", optchar);
			result = -1;
		}

	return (result);
	}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	int
configPlusOptions (
	char	optchar,
	char   *optarg)

	{
	int result = 0;

#if DEBUG_LEVEL > 0
	printf("ChmodConfig (+, %c)\n", optchar);
#endif

	switch (tolower(optchar))
		{
		case 'a':   plus_flags  |= _A_ARCH;		++change_req;	break;
		case 'h':   plus_flags  |= _A_HIDDEN;	++change_req;	break;
		case 'r':   plus_flags  |= _A_RDONLY;	++change_req;	break;
		case 's':   plus_flags  |= _A_SYSTEM;	++change_req;	break;

		default:    
			fprintf(stdout, "invalid \'+\' option '%c'\n", optchar);
			result = -1;
		}

	return (result);
	}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	int
configEqualOptions(
	char	optchar,
	char* optarg)

	{
	int result = 0;
static int onceonly = 1;

#if DEBUG_LEVEL > 0
	printf("ChmodConfig (=, %c)\n", optchar);
#endif

	if (onceonly)
		{
		onceonly    = 0;
		plus_flags  = ( 0 );
		minus_flags = (_A_ARCH | _A_HIDDEN | _A_RDONLY | _A_SYSTEM);
		}

	switch (tolower(optchar))
		{
		case '0': break;
		case 'a': plus_flags |= (_A_ARCH);   minus_flags &= ~(_A_ARCH);   ++change_req;	break;
		case 'h': plus_flags |= (_A_HIDDEN); minus_flags &= ~(_A_HIDDEN); ++change_req;	break;
		case 'r': plus_flags |= (_A_RDONLY); minus_flags &= ~(_A_RDONLY); ++change_req;	break;
		case 's': plus_flags |= (_A_SYSTEM); minus_flags &= ~(_A_SYSTEM); ++change_req;	break;
		default:
			fprintf(stdout, "invalid \'=\' option '%c'\n", optchar);
			result = -1;
		}

	return (result);
	}
		
/*--------------------------------------------------------------------*/

// diagnostics ()
/*--------------------------------------------------------------------*/
#if DEBUG_LEVEL > 0

	void
diagnostics (void)

	{
	printf("Plus_flags:  ");
	printf("%c%c%c%c\n",
		((plus_flags & _A_ARCH)   ? 'A' : 'a'),
		((plus_flags & _A_RDONLY) ? 'R' : 'r'),
		((plus_flags & _A_HIDDEN) ? 'H' : 'h'),
		((plus_flags & _A_SYSTEM) ? 'S' : 's'));
//	printf("\n");

	printf("Minus_flags: ");
	printf("%c%c%c%c\n",
		((minus_flags & _A_ARCH)   ? 'A' : 'a'),
		((minus_flags & _A_RDONLY) ? 'R' : 'r'),
		((minus_flags & _A_HIDDEN) ? 'H' : 'h'),
		((minus_flags & _A_SYSTEM) ? 'S' : 's'));
//	printf("\n");

	printf("Equal_flags: ");
	printf("%c%c%c%c\n",
		((equal_flags & _A_ARCH)   ? 'A' : 'a'),
		((equal_flags & _A_RDONLY) ? 'R' : 'r'),
		((equal_flags & _A_HIDDEN) ? 'H' : 'h'),
		((equal_flags & _A_SYSTEM) ? 'S' : 's'));

	printf("  do_anyway: %s\n", ((do_anyway > 0) ? "TRUE" : "FALSE"));
	}
#endif

/*--------------------------------------------------------------------*/
// getopt initialization data
/*--------------------------------------------------------------------*/

#if defined(DIRECTORIES)
OPTINIT	minusOptions = { NULL, '-', "aAdDeEfFhHlLrRsS?cqQX:", configMinusOptions };
#else
OPTINIT	minusOptions = { NULL, '-', "aAeEfFhHlLrRsS?cqQX:",	configMinusOptions };
#endif
OPTINIT	plusOptions  = { NULL, '+', "aAhHrRsS", 			configPlusOptions  };
OPTINIT	equalOptions = { NULL, '=', "0aAhHrRsS",    		configEqualOptions };

/*--------------------------------------------------------------------*/
// main()
/*--------------------------------------------------------------------*/
	void
main (
	int    argc,			/* Argument count */
	char  *argv [])			/* Argument list pointer */

	{
	int		exitcode = 0;		// The program exit code
	int		argIndex;
	int		errorCode;			// The getopt2 completion code
	char   *ap		= NULL;		/* Argument pointer */
	char   *fnp		= NULL;		/* Target file name pointer */
	char   *envPtr	= NULL;		/* Target file name pointer */


	if ((hp = fwOpen()) == NULL)
		exit(1);
	if ((xp = fExcludeOpen()) == NULL)
		exit(1);

//BWJ	setbuf(stdout, fmalloc(BUFSIZ));
	envPtr = getenv("CHMOD");

	// Set up the switch table state machine

	getoptInit(&minusOptions);
	getoptInit(&plusOptions);
	getoptInit(&equalOptions);

	errorCode = getopt2(argc, argv, envPtr, &argIndex);
	if (errorCode != OPTERR_NONE)
		{
		fprintf(stderr, "Error (%d), %s\n", errorCode, getoptErrorStr());
		exit(1);
		}

#if defined(DIRECTORIES)
	if (d_flag)
		smode |= FW_DIR;
#endif

#if DEBUG_LEVEL > 0
	printf("Files: argIndex: %d, argc: %d\n", argIndex, argc);
	printf("getopt2() returned (%d) \"%s\"\n", errorCode, getoptErrorStr());
	diagnostics();
#endif

	if (argIndex >= argc)				// if no filespec provided, use default
		{
		optind = 0;
		argc   = 1;
		argv   = fargv;
		}

	while (argIndex < argc)
		{
		ap = argv[argIndex++];
		
		if (fwInit(hp, ap, smode) != FWERR_NONE)	// Process the pattern
			fwInitError(ap);
		fExcludeConnect(xp, hp);					// Connect the exclusion instance
		if ((fnp = fWild(hp)) != NULL)
			{
			do  {			/* Process one filespec */
				process(fnp);
				} while (fnp = fWild(hp));
			}
		else
			{
			exitcode = 1;
			cantfind(ap);
			}
		}

	xp = fExcludeClose(xp);					// Close the Exclusion instance
	hp = fwClose(hp);						// Close the fWild instance
	exit (exitcode);
	}

/*--------------------------------------------------------------------*/
/*                              EOF                                   */
/*--------------------------------------------------------------------*/
