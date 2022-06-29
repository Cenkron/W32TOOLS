/*********************************************************************\
                CHMOD - change file attributes
\*********************************************************************/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dos.h>
#include <fwild.h>
#include <getoptns.h>
#include <getopt2.h>

/*--------------------------------------------------------------------*/
#define VERSION "930730.113707"
/*--------------------------------------------------------------------*/

#define DEBUG_LEVEL 0

/*--------------------------------------------------------------------*/
//#define DIRECTORIES
/*--------------------------------------------------------------------*/

#define MASK  (ATT_VOLU | ATT_SUBD)

int     plus_flags;
int     minus_flags;
int     equal_flags;
int     do_anyway;
int     c_flag;
#if defined(DIRECTORIES)
int     d_flag;
#endif
int     q_flag;
int     v_flag;
char    path_char;

/**********************************************************************/
	static char 
copyright [] =
	{"Version 4.3+ Copyright (c) 1998, 2022 J & M Software, Dallas TX - All Rights Reserved"};

/**********************************************************************/
	char *
usagedoc [] =
{
#if defined(DIRECTORIES)
"Usage:  chmod  [-aArRhHsS] [+arhs] [=arhs0] [-?cdqv] [-X...] [file_list]",
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
"-d        directory: change attributes on directories also",
#endif
"-v        verbose:   do not output filenames as they are changed",
"-q        query:     ask before changing each file",
"-X <pathspec> e/X/clude (possibly wild) matching pathspec",
"-X @<xfile>   e/X/clude files that match pathspec(s) in xfile",
"-X-       disable default file exclusion(s)",
"-X+       show exclusion path(s)",
"-c        compare:   do not change attributes, but",
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
	int attrib = fgetattr(fnp);

	if (attrib < 0)
		printf("\7Unable to get attributes: %s\n", fnp);
	else if (plus_flags | minus_flags | equal_flags | do_anyway)
		{
		if (!c_flag)	// Change flags
			{
			if (equal_flags | do_anyway)
				{
				attrib &= MASK;
				attrib |= equal_flags;
				}
			else
				{
				attrib &= (~minus_flags);
				attrib |= plus_flags;
				}

			if (query(fnp)  &&  (fsetattr(fnp, attrib) < 0))
				printf("\7Unable to change attributes: %s\n", fnp);
			attrib = fgetattr(fnp);
			}
		else  /* c_flag, only compare flags */
			{
			if (equal_flags || do_anyway)
				{
				if ((attrib&MASK) == equal_flags)
					exit(1);
				else
					exit(0);
				}
			else 
				{
				if ((plus_flags&attrib) != plus_flags)
					exit(0);
				else if ((minus_flags&attrib) != 0)
					exit(0);
				else
					exit(1);
				}
			}
		}

	if (q_flag || !v_flag)
		{
		printf("%c%c%c%c%c   %s\n",
			((attrib & _A_ARCH)   ? 'a' : '-'),
			((attrib & _A_RDONLY) ? 'r' : '-'),
			((attrib & _A_HIDDEN) ? 'h' : '-'),
			((attrib & _A_SYSTEM) ? 's' : '-'),
			((attrib & _A_SUBDIR) ? 'd' : ' '),
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
		case 'c':   ++c_flag;	break;
		case 'C':   ++c_flag;	break;
#if defined(DIRECTORIES)
		case 'd':   ++d_flag;	break;
		case 'D':   ++d_flag;	break;
#endif
		case 'q':   ++q_flag;	break;
		case 'Q':   ++q_flag;	break;

		case 'v':   ++v_flag;	break;
		case 'V':   ++v_flag;	break;

		case '?':   help();

		case 'X':
			if      (optarg[0] == '-')			// (Upper case)
				fexcludeDefEnable(FALSE);		/* Disable default file exclusion(s) */
			else if (optarg[0] == '+')
				fexcludeShowExcl(TRUE);			/* Enable stdout of exclusion(s) */
			else if (fexclude(optarg))
				printf("Exclusion string fault: \"%s\"\n", optarg);
			break;


		// Attribute settings

		case 'a':   minus_flags |= _A_ARCH;		break;
		case 'A':   minus_flags |= _A_ARCH;		break;
		case 'h':   minus_flags |= _A_HIDDEN;	break;
		case 'H':   minus_flags |= _A_HIDDEN;	break;
		case 'r':   minus_flags |= _A_RDONLY;	break;
		case 'R':   minus_flags |= _A_RDONLY;	break;
		case 's':   minus_flags |= _A_SYSTEM;	break;
		case 'S':   minus_flags |= _A_SYSTEM;	break;

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
		case 'a':   plus_flags  |= _A_ARCH;		break;
		case 'h':   plus_flags  |= _A_HIDDEN;	break;
		case 'r':   plus_flags  |= _A_RDONLY;	break;
		case 's':   plus_flags  |= _A_SYSTEM;	break;

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

#if DEBUG_LEVEL > 0
	printf("ChmodConfig (=, %c)\n", optchar);
#endif

	switch (optchar)
		{
		case 'a':   equal_flags &= ~_A_ARCH;	break;
		case 'h':   equal_flags &= ~_A_HIDDEN;	break;
		case 'r':   equal_flags &= ~_A_RDONLY;	break;
		case 's':   equal_flags &= ~_A_SYSTEM;	break;

		case 'A':   equal_flags |= _A_ARCH;		break;
		case 'H':   equal_flags |= _A_HIDDEN;	break;
		case 'R':   equal_flags |= _A_RDONLY;	break;
		case 'S':   equal_flags |= _A_SYSTEM;	break;
		case '0':   ++do_anyway;				break;
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

OPTINIT	minusOptions = { NULL, '-', "aAhHrRsS?cqvVX:", configMinusOptions };
OPTINIT	plusOptions  = { NULL, '+', "aAhHrRsS",        configPlusOptions  };
OPTINIT	equalOptions = { NULL, '=', "0aAhHrRsS",       configEqualOptions };

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
	int		smode	= FW_FILE;	/* File search mode attributes */
	void   *hp		= NULL;		/* Pointer to wild file data block */
	char   *ap		= NULL;		/* Argument pointer */
	char   *fnp		= NULL;		/* Target file name pointer */
	char   *envPtr	= NULL;		/* Target file name pointer */

//BWJ	setbuf(stdout, fmalloc(BUFSIZ));
	envPtr = getenv("CHMOD");

	// Set up the switch table state machine

	getoptInit(&minusOptions);
	getoptInit(&plusOptions);
	getoptInit(&equalOptions);

	errorCode = getopt2(argc, argv, envPtr, &argIndex);
	if (errorCode != OPTERR_NONE)
		{
		printf("\7Error (%d), %s\n", errorCode, getoptErrorStr());
		exit(1);
		}

#if defined(DIRECTORIES)
	if (d_flag)
		smode |= FW_SUBD;
#endif

#if DEBUG_LEVEL > 0
	printf("Files: argIndex: %d, argc: %d\n", argIndex, argc);
	printf("getopt2() returned (%d) \"%s\"\n", errorCode, getoptErrorStr());
	diagnostics();
#endif

	if (argIndex >= argc)	// Default to *.* if no files specified
		{
		argIndex = 0;
		argc     = 1;
		argv[0]  = "*.*";
		}

	while (argIndex < argc)
		{
		ap = argv[argIndex++];
		hp = fwinit(ap, smode);		/* Process the input list */
		if ((fnp = fwildexcl(hp)) != NULL)
			{
			do  {			/* Process one filespec */
				process(fnp);
				} while (fnp = fwildexcl(hp));
			}
		else
			{
			exitcode = 1;
			cantfind(ap);
			}
		}

	exit (exitcode);
	}

/*--------------------------------------------------------------------*/
/*                              EOF                                   */
/*--------------------------------------------------------------------*/
