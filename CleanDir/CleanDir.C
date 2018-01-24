/* ----------------------------------------------------------------------- *\
|
|			Program to delete empty directories
|
|		      Copyright (c) 2003, all rights reserved
|				    Brian W Johnson
|				        8-Feb-03
|				       23-Dec-17 Added 'a' key
|
\* ----------------------------------------------------------------------- */

//#define  TESTMODE
//#define  VERBOSE

#include  <windows.h>
#include  <stdio.h>
#include  <conio.h>

#include  "fwild.h"

static BOOL  Running = TRUE;
static BOOL  All = FALSE;

char star [2] = {'*', 0};
char dot  [2] = {'.', 0};

/* ----------------------------------------------------------------------- *\
|  GetKey () - Get the response key for Query()
\* ----------------------------------------------------------------------- */
	char		// Returns the validated keystroke
GetKey (void)

	{
	char  c;


	for (;;)
		{
		while (((c = (char)(_getch())) == 0)  ||  (c == (char)(0xE0)))
			_getch();

		c = tolower(c);

		if ((c == 'a')
		||  (c == 'y')
		||  (c == 'n')
		||  (c == 'q')
		||  (c == 'c'))
			break;
		}

	putchar(c);
	putchar('\n');
	return (c);
	}

/* ----------------------------------------------------------------------- *\
|  Query () - Query whether to remove the directory
\* ----------------------------------------------------------------------- */
	static BOOL		// Returns TRUE to remove the directory
Query (char *s)		// The path of the directory

	{
	int   col;		// Number of columns
	char  key;		// The current key value
	int   retval;	// The returned value


	if (All)
		retval = TRUE;
	else
		{
		col = printf("Remove  %s ", s);

		for (; (col < 32); ++col)
		putchar(' ');

		printf("? [Y/N/A/QC]: ");

		key = GetKey();
		switch (tolower(key))
			{
			case 'a':
				All     = TRUE;
				retval  = TRUE;
				break;

			case 'y':
				retval  = TRUE;
				break;

			case 'q':
			case 'c':
				Running = FALSE;	// and fall through to the default

			default:
				retval  = FALSE;
				break;
			}
		}

	return (retval);
	}

/* ----------------------------------------------------------------------- *\
|  PathDelete () - Determine whether to delete the path, and try to do so
\* ----------------------------------------------------------------------- */
	static int			// Returns 0 if delete succeeded, else 1
PathDelete(
	char  *s)			// The pointer to the item name

	{
	long  retval = 1;		// The returned result


	if (Running  &&  Query(s))
		{
printf("  Deleting \"%s\"\n", s);
#ifdef TESTMODE
		retval = 0;
#else
		retval = (RemoveDirectory(s) ? 0 : 1);
#endif
		}
	return (retval);
	}

/* ----------------------------------------------------------------------- *\
|  PathCat () - Concatenate a suffix (directory) to a path
\* ----------------------------------------------------------------------- */
	static void
PathCat(
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
	static int			// Returns 1 if the path is not empty
ProcessPath (
	char  *pPath)			// Ptr to the path string

	{
	int              Nonempty = 0;	// The path nonempty flag
	HANDLE           sh;		// The find search handle
	WIN32_FIND_DATA  wfd;		// The find information
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
					Nonempty |= ProcessPath(NewPath);
					}
				}
		
			else /* (the found item is a file) */
				{
#ifdef VERBOSE
				printf(" File found, \"%s\"\n", CurrentItem);
#endif
				Nonempty = 1;
				}

			}  while (NextResult  &&  Running);
		FindClose(sh);
		}

	// If this directory is empty, try to delete it

	if (Nonempty == 0)
		Nonempty = PathDelete(pPath);

#ifdef VERBOSE
	printf("Returning %d\n", Nonempty);
#endif
	return (Nonempty);
	}

/* ----------------------------------------------------------------------- *\
|  Process () - Check and process one path
\* ----------------------------------------------------------------------- */
	void
Process (char *p)		// Ptr to the raw path string

	{
	char  *s = fnabspth(p);	// The processed path


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
|  main () - main program
\* ----------------------------------------------------------------------- */
	int
main (
	int     argc,		// Argument count
	char  **argv)		// Argument list

	{
	if (argc == 1)
		Process(dot);
	else
		{
		while (--argc > 0)
			Process(*(++argv));
		}
	return (0);
	}

/* -------------------------------- END ---------------------------------- */
