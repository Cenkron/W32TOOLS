/* ----------------------------------------------------------------------- *\
|
|						Program to delete empty directories
|
|					Copyright (c) 2003, all rights reserved
|					    Brian W Johnson
|						8-Feb-03
|						23-Dec-17 Added 'a' response for uncondional deletion mode
|						30-Aug-21 Added command line options -a and -l
|						30-Aug-21 Added command line usage and help
|
\* ----------------------------------------------------------------------- */

//#define  TESTMODE

#include  <windows.h>
#include  <stdio.h>
#include  <conio.h>

#include  <fWild.h>

#include  <getoptns.h>
#include  <getopt2.h>

char	copyright [] =
"Copyright (c) 1985, 2021 by J & M Software, Dallas TX - All Rights Reserved";

typedef enum decision
{
Delete = 0,
Keep = 1
} Decision_t;

// The state variables

static BOOL			Running     = TRUE;
static BOOL			Listing     = FALSE;
static BOOL			Interactive = TRUE;
static BOOL			Verbose     = FALSE;
static int			xmode       = 0;		// Dummy, not used in the CleanDir version of fWild
static Decision_t	Decision    = Keep;

PEX		xp = NULL;				// FEX instance pointer

char star [2] = {'*', 0};
char dot  [2] = {'.', 0};

/* ----------------------------------------------------------------------- *\
|  IsRootPath () - Return if the path is a root path
\* ----------------------------------------------------------------------- */
	static int
IsRootPath (char *pPath)

	{
	return (*(pPath + strlen(pPath) - 1) == '\\');
	}

/* ----------------------------------------------------------------------- *\
|  RootedX () - Return the path with a root prefix
\* ----------------------------------------------------------------------- */
	static char *
RootedX (char *pPath)

	{
static char pRooted [1024] = {'\\' };

	strcpy((pRooted + 1), pPath);
	return (pRooted);
	}

/* ----------------------------------------------------------------------- *\
|  GetKey () - Get the response key for QueryUser() (Interactive mode only)
\* ----------------------------------------------------------------------- */
	char		// Returns the validated keystroke in interactive mode
GetKey (void)

	{
	int  key;

	do  {
		key = _getch();
		} while ((key < 0x20) || (key > 0x7E));

	switch (key = tolower(key))
		{
		case 'a':				break;
		case 'y':				break;
		case 'q':				break;
		case 'c':				break;
		default:   key = 'n';	break;
		}

	printf("%c\n", key);
	return ((char)(key));
	}

/* ----------------------------------------------------------------------- *\
|  QueryUser () - Query whether to remove the directory (Interactive mode only)
\* ----------------------------------------------------------------------- */
	static void
QueryUser (char *s)		// The path of the candidate directory

	{
	printf("    ? [Y/N/A/QC]: ");	// Prompt for the keystroke

	switch (GetKey())				// Get and process the response
		{
		case 'a':
			Interactive	= FALSE;
			Decision	= Delete;	// Make Delete sticky
			break;

		case 'y':
			Decision	= Delete;
			break;

		case 'q':
		case 'c':
			Running		= FALSE;
			Interactive	= FALSE;
			Decision	= Keep;		// Make Keep sticky (until termination)
			break;

		case 'n':
			Decision	= Keep;
			break;
		}

	return;
	}

/* ----------------------------------------------------------------------- *\
|  PathDelete () - Determine whether to delete the path, and try to do so
\* ----------------------------------------------------------------------- */
	BOOL				// Returns true if the path is not deleted, else false
PathDelete(
	char  *s)			// The pointer to the item name

	{
	BOOL  PathNotEmpty = TRUE;	// The returned path not empty result (defaulted for Keep)

	if (Running)
		{
		if (Interactive)
			{
			printf("Delete  \"%s\"", s);
			QueryUser(s);	// Ascertain the user Decision
			}
		else if (Decision == Delete) // (and not interactive)
			printf("Deleting  \"%s\"\n", s);
		else // (Decision = Keep)
			printf("Would delete  \"%s\"\n", s);

		if (Decision == Delete)
			{
// RemoveDirectoryA() returns nonzero iff delete successful

#ifdef TESTMODE
			PathNotEmpty = (Decision == Keep);
#else
			PathNotEmpty = (RemoveDirectoryA(s) == 0);
#endif
			}
		}

	return (PathNotEmpty);
	}

/* ----------------------------------------------------------------------- *\
|  PathCat () - Concatenate a suffix (directory) to a path
\* ----------------------------------------------------------------------- */
	static void
PathCat (
	char  *pBase,		// The pointer to the base name
	char  *pItem)		// The pointer to the suffix name

	{
	if (pBase[strlen(pBase)-1] != '\\')
		strcat(pBase, "\\");
	strcat(pBase, pItem);
	}

/* ----------------------------------------------------------------------- *\
|  Interesting () - Determine whether the found item is to be examined
\* ----------------------------------------------------------------------- */
	static BOOL			// Returns TRUE if the item is interesting
Interesting (
	char  *s)			// The pointer to the item name

	{
	return ((strcmp(s, ".") != 0)  &&  (strcmp(s, "..") != 0));
	}

/* ----------------------------------------------------------------------- *\
|  ProcessPath () - Process one recursive level of path processing
\* ----------------------------------------------------------------------- */
	static BOOL			// Returns true if the path is not empty, else false
ProcessPath (
	char  *pPath)		// Ptr to the path string

	{
	BOOL             PathNotEmpty = FALSE;	// The path is not empty flag, starting as empty
	HANDLE           sh;			// The find search handle
	WIN32_FIND_DATA  wfd;			// The find information
	char CurrentFile [1024];		// The current item name, assuming a file
	char CurrentDir  [1024];		// The current directory name
	char NewPath     [1024];		// The new base path


	if (Verbose)
		printf("Processing path \"%s\"\n", pPath);

	strcpy(NewPath, pPath);		// Make the starting search path
	PathCat(NewPath, star);

	// The following code is a simplified abstraction from the fWild library

	if ((sh = FindFirstFile(NewPath, &wfd)) != INVALID_HANDLE_VALUE)
		{
		BOOL   NextResult;		// Result of the next search
		DWORD  Attr;			// Attributes of the current search

		do  {

			// Save the useful parameters of this search, and perform
			// the next search, in case the current item is deleted

			Attr = wfd.dwFileAttributes;
			strcpy(CurrentFile, wfd.cFileName);
			NextResult = FindNextFile(sh, &wfd);

			if (Interesting(CurrentFile))
				{
				if (Attr & FILE_ATTRIBUTE_DIRECTORY)
					{
					strcpy(CurrentDir, CurrentFile);	// Current file is a directory
					strcpy(NewPath, pPath);				// Make the child search path
					PathCat(NewPath, CurrentDir);

					// Check if the file/path is excluded

					if (fExcludePathCheck(xp, NewPath))
						{
						if (Verbose)
//BWJ fix xmode system
							printf(" Dir  found, \"%s\" (EXCLUDED)\n", CurrentDir);

						PathNotEmpty = TRUE;
						}

					else if (Attr & FILE_ATTRIBUTE_HIDDEN)
						{
						if (Verbose)
							printf(" Dir  found, \"%s\" (HIDDEN)\n", CurrentDir);

						PathNotEmpty = TRUE;
						}

					else if (Attr & FILE_ATTRIBUTE_SYSTEM)
						{
						if (Verbose)
							printf(" Dir  found, \"%s\" (SYSTEM)\n", CurrentDir);

						PathNotEmpty = TRUE;
						}

					else
						{
						if (Verbose)
							printf(" Dir  found, \"%s\"\n", CurrentDir);

						PathNotEmpty |= ProcessPath(NewPath);	// Search it recursively
						}
					}

				else /* (the current item is a file), and is therefore automatically excluding its parent directory */
					{
					if (Verbose > 1)
						printf(" File found, \"%s\"\n", CurrentFile);

					PathNotEmpty = TRUE;
					}
				}

			}  while (NextResult  &&  Running);
			FindClose(sh);
		}

	// If this directory is empty, try to delete it
	// If the delete fails, the path is not empty

	if ( ! PathNotEmpty)
		PathNotEmpty |= PathDelete(pPath);

	if (Verbose)
		printf("Returning \"%s\"%s empty\n", pPath, (PathNotEmpty ? " not" : ""));

	return (PathNotEmpty);
	}

/* ----------------------------------------------------------------------- *\
|  ProcessRootPath () - Process the first level of root level recursive path processing
\* ----------------------------------------------------------------------- */
	void
ProcessRootPath (
	char  *pPath)		// Ptr to the path string

	{
	HANDLE           sh;			// The find search handle
	WIN32_FIND_DATA  wfd;			// The find information
	char CurrentFile [1024];		// The current item name, assuming a file
	char CurrentDir  [1024];		// The current directory name
	char NewPath     [1024];		// The new base path


	if (Verbose)
		printf("Processing root \"%s\"\n", pPath);

	strcpy(NewPath, pPath);		// Make the starting search path
	PathCat(NewPath, star);

	// The following code is a simplified abstraction from the fWild library

	if ((sh = FindFirstFile(NewPath, &wfd)) != INVALID_HANDLE_VALUE)
		{
		BOOL   NextResult;		// Result of the next search
		DWORD  Attr;			// Attributes of the current search
		do  {

			// Save the useful parameters of this search, and perform
			// the next search, in case the current item is deleted

			Attr = wfd.dwFileAttributes;
			strcpy(CurrentFile, wfd.cFileName);
			NextResult = FindNextFile(sh, &wfd);

			if ((Interesting(CurrentFile))
			&&  (Attr & FILE_ATTRIBUTE_DIRECTORY))
				{
				strcpy(CurrentDir, CurrentFile);	// Current file is a directory
				strcpy(NewPath, pPath);				// Make the child search path
				PathCat(NewPath, CurrentDir);

				// Check if the file/path is excluded

				if (fExcludePathCheck(xp, NewPath))
					{
					if (Verbose)
//BWJ fix xmode system
						printf(" Dir  found, \"%s\" (EXCLUDED)\n", CurrentDir);
					}

				else if (Attr & FILE_ATTRIBUTE_HIDDEN)
					{
					if (Verbose)
						printf(" Dir  found, \"%s\" (HIDDEN)\n", CurrentDir);
					}

				else if (Attr & FILE_ATTRIBUTE_SYSTEM)
					{
					if (Verbose)
						printf(" Dir  found, \"%s\" (SYSTEM)\n", CurrentDir);
					}

				else
					{
					if (Verbose)
						printf(" Dir  found, \"%s\"\n", CurrentDir);

					ProcessPath(NewPath);			// Begin a recursive search of this directory
					}
				}
			}  while (NextResult  &&  Running);
		FindClose(sh);
		}

	return;
	}

/* ----------------------------------------------------------------------- *\
|  Process () - Check and process the base path
\* ----------------------------------------------------------------------- */
	int
Process (char *p)		// Ptr to the raw path string

	{
	int    exitcode = 0;
	char *pPath;	// The path to process

    if ((pPath = fnabspth(p)) == NULL)
		cantfind(p);
 
	if (Verbose)
		printf("BasePath: \"%s\"\n", pPath);

	if ( ! fnchkdir(pPath))
		{
		printf("Specified path must be a root or directory:\n");
		printf("    \"%s\"\n",pPath);
		printf("Usage: CleanDir [path1] .. [pathN]\n");
		exitcode = 1;
		}
	else if (IsRootPath(pPath))
		ProcessRootPath(pPath);
	else
		ProcessPath(pPath);

	free(pPath);

	return (exitcode);
	}

/* ----------------------------------------------------------------------- *\
|  dprint () - Print the help or usage strings
\* ----------------------------------------------------------------------- */
    void
dprint (dp)			/* Print documentation text */
    char  **dp;		/* Document array pointer */

    {
    while (*dp)
	{
	printf(*(dp++), '-');
	putchar('\n');
	}
    }

/* ----------------------------------------------------------------------- *\
|  usage () - Print the usage message and terminate
\* ----------------------------------------------------------------------- */
    void
usage ()			/* Display usage documentation */

    {
    static char  *udoc [] =
	{
	"Usage:  cleandir  [-?al]  [input_directory_list]  [>output_file]",
	"        cleandir  -?  for help",
	NULL
	};

    dprint(udoc);
    exit(1);
    }

/* ----------------------------------------------------------------------- *\
|  help () - Print the help message and terminate
\* ----------------------------------------------------------------------- */
    void
help ()				/* Display help documentation */

    {
    static char  *hdoc [] =
	{
	"Usage:  cleandir  [-?al]  [input_directory_list]  [>output_file]",
	"",
	"cleandir cleans one or more directory tree(s) of all empty directories.",
	"Trees are specified as a parameter list (or from stdin).",
	"Using the -a switch performs the deletions unconditionally;",
	"otherwise CleanDir asks for delete permission for each empty directory found.",
	"",
	"    -a  signifies deletions are to be performed unconditionally, and listed.",
	"    -l  signifies deletions are to be denied unconditionally, but listed.",
	"    -v  shows verbose output (0..2).",
	"    -X <pathspec> e/X/clude (possibly wild) matching pathspec.",
	"    -X @<xfile>   e/X/clude files that match pathspec(s) in xfile.",
	"    -X+           enable showing the configuration progress.",
	"    -X-           disable default file exclusion(s).",
	"    -X=           enable showing excluded paths dynamically.",
	"",
	"    When running interactively, valid responses are:",
	"",
	"    y   permits the deletion,",
	"    n   denies the deletion, and skips over the item,",
	"    a   changes the mode to unconditional for the remainder of the scan,",
	"    q   terminates the scan,",
	"    c   terminates the scan,",
	"    *   any other response denies the deletion, and skips over the item,",
	"",
	copyright,
	NULL
	};

    dprint(hdoc);
    exit(0);
    }

/* ----------------------------------------------------------------------- *\
|  configOptions () - Process command line options
\* ----------------------------------------------------------------------- */
	int
configOptions (
	char	optchar,
	char   *optarg)

	{
		int exitcode = 0;		// The program exit code
		int result = 0;

	switch (optchar)
		{
		case 'a':		// Signifies blanket acceptance of deletions
		    Running		= TRUE;
		    Interactive	= FALSE;
			Listing		= FALSE;
		    Decision	= Delete;
		    break;

		case 'l':		// Signifies blanket denial of deletions, but lists the candidates
		    Running		= TRUE;
		    Interactive	= FALSE;
			Listing		= TRUE;
		    Decision	= Keep;
		    break;

		case 'v':		// Signifies run in verbose mode
		    ++Verbose;
		    break;

		case 'X':		// Signifies path exclusion entry
			if      (optarg[0] == '-')
				fExcludeDefEnable(xp, FALSE);	/* Disable default file exclusion(s) */
			else if (optarg[0] == '+')
				fExcludeShowConf(xp, TRUE);		/* Enable stdout of exclusion(s) */
			else if (optarg[0] == '=')
				fExcludeShowExcl(xp, TRUE);		/* Enable stdout of excluded path(s) */
			else if (fExclude(xp, optarg))
				printf("Exclusion string fault: \"%s\"\n", optarg);
			break;

		case '?':		// Lists help, and terminates the program
		    help();

		default:    
			fprintf(stdout, "invalid \'-\' option \'%c\'\n", optchar);
			exitcode = 1;
		}

	return (exitcode);
	}

/*--------------------------------------------------------------------*/

OPTINIT	options = { NULL, '-', "alvX:?", configOptions };

/*--------------------------------------------------------------------*/
// main()
/*--------------------------------------------------------------------*/
	void
main (
	int    argc,				// Argument count
	char  *argv [])				// Argument list pointer

	{
	int		exitcode = 0;		// The program exit code
	int		argIndex;
	int		errorCode;			// The getopt2 completion code
	char   *envPtr = NULL;		// Ptr to environment string


	if ((xp = fExcludeOpen()) == NULL)
		exit(1);

	envPtr = getenv("CleanDir");

	getoptInit(&options);		// Set up the switch table state machine

	errorCode = getopt2(argc, argv, envPtr, &argIndex);
	if (errorCode != OPTERR_NONE)
		usage();

	fExcludeConnect(xp, NULL);	// Init only the exclusion instance

	if (argIndex >= argc)
		{
		if (Verbose)
			printf("Arg: <empty>\n");

		exitcode = Process(dot);
		}

	else
		{
		while ((argIndex < argc)  &&  (exitcode == 0))
			{
			if (Verbose)
				printf("Arg: \"%s\"\n", argv[argIndex]);

			exitcode = Process(argv[argIndex++]);
			}
		}

	xp = fExcludeClose(xp);					// Close the Exclusion instance
	exit (exitcode);
	}

/*--------------------------------------------------------------------*/
/*                              EOF                                   */
/*--------------------------------------------------------------------*/
