/* ----------------------------------------------------------------------- *\
|
|				   FEXCLUDE
|		     Wild Card File Name Exclusion Server
|
|		   Copyright (c) 1993, all rights reserved
|				Brian W Johnson
|				   17-Feb-93
|				   17-Aug-97
|				   16-Jun-22	Added default exclusion path mechanism
|
\* ----------------------------------------------------------------------- */

#include  <windows.h>
#include  <stdio.h>
#include  <stdlib.h>
#include  <ctype.h>
#include  <string.h>

#define  FWILD_INTERNAL		/* Enable FWILD internal definitions */

#include  "fwild.h"

/* ----------------------------------------------------------------------- */

#define  LINEBUFFER	(1024)

typedef struct EXC
	{
	struct EXC  *link;		/* Link to the next EXC element */
	char        name [1];	/* Storage for the pattern string */
	} EXC;

static  EXC  *root = NULL;	/* Root of the EXC structure list */

static	int	add_list   (char *str);
static	int	do_file    (char *str);
static	int	exclude    (char *str);
static	void	clear_list (void);

static int	showExcl = 0;						/* Show   excluded paths, default no */
static int	enbExcl  = 1;						/* Enable excluded paths, default yes */

// Internal default file exclusion list for Windows 10
// This is used only if there is no external default file list file.

static	char	*DefExcludeFiles [] = 
					{
					"\\$RECYCLE.BIN\\**",
					"\\$WinREAgent\\**",
					"\\System Volume Information\\**",
					"\\pagefile.sys",
					"\\swapfile.sys",
					"\\hiberfile.sys",
					NULL
					};

/* ----------------------------------------------------------------------- */
	char *				/* Return the pointer to the file name */
fwildexcl (				/* Get the first/next wild filename (ex mode) */
	DHV  *pdhv)			/* Pointer to the wild file structure */

	{
	char  *result;

	do	{
		result = fwild(pdhv);
		} while ((result != NULL)  &&  (excluded(result)));

	return (result);
	}

/* ----------------------------------------------------------------------- */
	static int			/* Return TRUE to exclude the pathname string */
excluded (				/* Test the string for exclusion */
	char  *target)		/* Pointer to the target string */

	{
	EXC  *p;


	for (p = root; (p != NULL); p = p->link)
		{
		if (psubmatch(p->name, target, 0))
			return (TRUE);	/* Exclude if a match */
		} 

	return (FALSE);
	}

/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */
	void
fexcludeDefEnable (
	int flag)					// True to enable exclusions

	{
	enbExcl = flag;
	}

/* ----------------------------------------------------------------------- */
	void
fexcludeShowExcl (
	int flag)					// True to show exclusions

	{
	showExcl = flag;
	}

/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */
	static char *
GetInitPath (void)	// Get the pathname of the default exclusion file

	{
    DWORD	length;
	char  *pDest;
//	int		Insert;
static char Dest [MAX_PATH];

	if ((length = GetModuleFileName(NULL, Dest, sizeof(Dest))) == 0)
		return (NULL);

	fnreduce(Dest);
	Dest[strlen(Dest) - strlen(fntail(Dest))] = '\0';
	pDest = fncatpth(Dest, "exclude.ini");
//	printf("Path: %s\n", pDest);

	return ((strlen(pDest)  > 0) ? pDest : NULL);
	}

/* ----------------------------------------------------------------------- */
	static int					/* Return non-zero if a file error */
fexcludeFromFile (
	char* pfn)			// Exclusion File pathname

	{
	FILE* fp;			// Handle of the opened exclusion file
	char  line[4096];	// Init file line buffer

	if ((pfn == NULL)
	|| ((fp = fopen(pfn, "r")) == NULL))
		return (-1);		// File read failed, fall back to internal defaults

	while (fgets(line, sizeof line, fp))
		{
		if (strlen(line) > 0)
			{
			int  length = (strlen(line) - 1);

			if (length > 0)				// Remove the endline character
				line[length] = '\0';

			if (line[0] == '*')			// '*' denotes a comment line
				{
				if (showExcl)			// Display the comment, if enabled
					printf("Excluding: %s\n", line);
				}

			else	
				fexclude(line);
			}
		}

	fclose(fp);
	return (0);			// File read suceeded
	}

/* ----------------------------------------------------------------------- */
	static int			/* Return non-zero if an error */
add_list (				/* Add a string to the exclusion list */
	char  *pattern)		/* Pointer to the pattern string */

	{
	EXC   *p;			/* Pointer to the new EXC element */
	int    length;		/* Length of the pattern string */

	if (((length = strlen(pattern)) > 0)
	&&  ((p = (EXC *)(malloc(sizeof(EXC) + length))) != NULL))
		{
		strcpy(p->name, pattern);	/* Allocate a new EXC element */
		p->link = root;
		root    = p;

		if (showExcl)
			printf("Excluding: %s\n", pattern);

		return (0);
		}

	return (-1);		/* Failure */
	}

/* ----------------------------------------------------------------------- */
	static void
clear_list (void)		/* Clear out the exclusion list */

	{
	EXC  *p1;
	EXC  *p2;

	for (p1 = root; (p2 = p1); )
		{
		p1 = p1->link;	/* Point the next element */
		free(p1);		/* Free the current element */
		} 
	root = NULL;		/* Show the list empty */
	}

/* ----------------------------------------------------------------------- */
	int					/* Return non-zero if an error */
fexclude (				/* Exclude a path from the fwild search */
	char  *pattern)		/* Pointer to the pattern string */

	{
	if (pattern == NULL)
		{
		clear_list();
		return (0);
		}

	else if (*pattern == '@')	// '@' denotes pattern is the name of an exclusion file
		return (fexcludeFromFile(pattern + 1));

	return (add_list(pattern));	// Otherwise pattern IS a pattern
	}

/* ----------------------------------------------------------------------- */
	int							/* Return non-zero if an error */
fexcludeDefault (void)

	{
	if (enbExcl)

		// If default exclusions are enabled,
		// first try the default file,
		// but fail over to using the internal defaults

		{
		if (fexcludeFromFile(GetInitPath()) != 0)

			// The default file failed, so use the internal defaults instead

			{
			for (char **p = &DefExcludeFiles[0]; (*p != NULL); ++p)
				fexclude(*p);
			}
		}

	else // Defult exclusions are disabled
		{
		if (showExcl)
			printf("Default exclusions disabled\n");
		}

	return (0);
	}

/* ----------------------------------------------------------------------- */
