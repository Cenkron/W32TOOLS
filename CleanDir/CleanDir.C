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
//#define  VERBOSE

#include  <windows.h>
#include  <stdio.h>
#include  <conio.h>

#include  "fwild.h"

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
static Decision_t	Decision    = Keep;

char star [2] = {'*', 0};
char dot  [2] = {'.', 0};

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
	char CurrentItem [4096];		// The current item name
	char NewPath     [4096];		// The new base path


#ifdef VERBOSE
	printf("Processing \"%s\"\n", pPath);
#endif

	strcpy(NewPath, pPath);		// Make the starting search path
	PathCat(NewPath, star);

	if ((sh = FindFirstFile(NewPath, &wfd)) != INVALID_HANDLE_VALUE)
		{
		BOOL   NextResult;		// Result of the next search
		DWORD  Attr;			// Attributes of the current search
		do  {

			// Save the useful parameters of this search, and perform
			// the next search, in case the current item is deleted

			Attr = wfd.dwFileAttributes;
			strcpy(CurrentItem, wfd.cFileName);
			NextResult = FindNextFile(sh, &wfd);

			if ((Attr & FILE_ATTRIBUTE_DIRECTORY)
			&&  ( ! (Attr & FILE_ATTRIBUTE_HIDDEN))
			&&  ( ! (Attr & FILE_ATTRIBUTE_SYSTEM)))
				{
				if (Interesting(CurrentItem))
					{
					strcpy(NewPath, pPath);	// Make the search path
					PathCat(NewPath, CurrentItem);
#ifdef VERBOSE
					printf(" Dir  found, \"%s\"\n", CurrentItem);
#endif
					PathNotEmpty |= ProcessPath(NewPath);
					}
				}
		
			else /* (the found item is a file) */
				{
#ifdef VERBOSE
				printf(" File found, \"%s\"\n", CurrentItem);
#endif
				PathNotEmpty = TRUE;
				}

			}  while (NextResult  &&  Running);
		FindClose(sh);
		}

	// If this directory is empty, try to delete it
	// If the delete fails, the path is not empty

	if ( ! PathNotEmpty)
		PathNotEmpty |= PathDelete(pPath);

#ifdef VERBOSE
	printf("Returning path%s empty\n", (PathNotEmpty ? " not" : ""));
#endif
	return (PathNotEmpty);
	}

/* ----------------------------------------------------------------------- *\
|  Process () - Check and process one path
\* ----------------------------------------------------------------------- */
	void
Process (char *p)		// Ptr to the raw path string

	{
	char  *s = fnabspth(p);	// The processed path

#ifdef VERBOSE
	printf("Path: \"%s\"\n", s);
#endif

	if (fnchkdir(s))
		ProcessPath(s);
	else
		{
		printf("Specified path must be a root or directory:\n");
		printf("    \"%s\"\n", s);
		printf("Usage: CleanDir [path1] .. [pathN]\n");
		}

	free(s);
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
	"Using the - a switch performs the deletions unconditionally;",
	"otherwise CleanDir asks for delete permission for each empty directory found",
	"",
	"    -a  signifies deletions are to be performed unconditionally, and listed.",
	"    -l  signifies deletions are to be denied unconditionally, but listed.",
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
|  main () - main program
\* ----------------------------------------------------------------------- */
	int
main (
	int     argc,		// Argument count
	char  **argv)		// Argument list

	{
	char  *s;			// Parser temporary

	while (--argc > 0 && (*++argv)[0] == '-')
	for (s = argv[0] + 1; *s; s++)
	    switch (tolower(*s))
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

		case '?':		// Lists help, and terminates the program
		    help();

		default:		// Lists usage, and terminates the program
		    usage();
		}

#ifdef VERBOSE
    printf("Argc: %d\n", argc);
	printf("Path: \"%s\"\n", argv[0]);
#endif

	if (argc == 0)
		Process(dot);
	else
		{
		while (argc-- > 0)
			Process(*(argv++));
		}
	return (0);
	}

/* -------------------------------- END ---------------------------------- */
