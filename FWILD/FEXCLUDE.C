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

static	int	 add_list   (char *str);
static	int	 do_file    (char *str);
static	int	 exclude    (char *str);
static	void clear_list (void);

static int	showExcl	= FALSE;		/* Show   excluded paths, default no */
static int	enbExcl		= TRUE;			/* Enable excluded paths, default yes */
static int	initNeeded	= TRUE;			/* Permit initialization only once */

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

// Exclusion files can contain comments, lines beginning with '~'.

/* ----------------------------------------------------------------------- */
/* Private methods
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
		strcpy(p->name, pattern);	/* Allocate and init a new EXC element */
		p->link = root;		// List link the new entry
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
	EXC  *p;
	
	while (root != NULL)
		{
		p = root->link;	// Point the second element
		free(root);		// Free the top element
		root = p;		// Update the root
		} 
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
	static char *
GetInitPath (void)	// Get the pathname of the default exclusion file

	{
    DWORD	length;
	char  *pDest;
static char Dest [MAX_PATH];

	if ((length = GetModuleFileName(NULL, Dest, sizeof(Dest))) == 0)
		return (NULL);

	fnreduce(Dest);
	Dest[strlen(Dest) - strlen(fntail(Dest))] = '\0';
	pDest = fncatpth(Dest, "exclude.ini");

	return ((strlen(pDest)  > 0) ? pDest : NULL);
	}

/* ----------------------------------------------------------------------- */
	static int					/* Return non-zero if a file error */
fexcludeFromFile (
	char* pfn)			// Exclusion File pathname

	{
	FILE* fp;			// Handle of the opened exclusion file
	char  line[4096];	// Init file line buffer

	if (showExcl)				// Display the comment, if enabled
		printf("ExFile:    %s\n", pfn);

	if ((pfn == NULL)
	|| ((fp = fopen(pfn, "r")) == NULL))
		return (-1);		// File read failed, fall back to internal defaults

	while (fgets(line, sizeof line, fp))
		{
		if (strlen(line) > 0)
			fexclude(line);
		}

	fclose(fp);
	return (0);			// File read suceeded
	}

/* ----------------------------------------------------------------------- */
/* Global methods
/* ----------------------------------------------------------------------- */
	char *				/* Performs fwild() but skips over excluded items */
fwildexcl (
	DHV  *pdhv)			/* Pointer to the returned wild file structure */

	{
	char  *result;

	do	{
		result = fwild(pdhv);
		} while ((result != NULL)  &&  (excluded(result)));

	return (result);
	}

/* ----------------------------------------------------------------------- */
	void
fexcludeDefEnable (		/* Enable/disable default exclusion paths */
	int flag)					// True to enable exclusions

	{
	enbExcl = flag;
	}

/* ----------------------------------------------------------------------- */
	void
fexcludeShowExcl (		/* Enable/disable showing exclusion paths */
	int flag)					// True to show exclusions

	{
	showExcl = flag;
	}

/* ----------------------------------------------------------------------- */
	void				/* Return non-zero if an error */
fexcludeClean (void)

	{
	clear_list();
	}

/* ----------------------------------------------------------------------- */
	int					/* Return non-zero if an error */
fexclude (				/* Exclude a path or path file from the fwild search */
	char  *pattern)		/* Pointer to the pattern string */

	{
	if (pattern == NULL)			// Ignore NULL string pointers
		return (0);

	else if (pattern[0] == '@')	// '@' denotes pattern is the name of an exclusion file
		{
		return (fexcludeFromFile(pattern + 1));
		}

	// Remove trailing newlines, white space, etc.

	char  *p = (pattern + strlen(pattern));
	while ((p > pattern)  &&  (isspace(*(--p))))
		*p = '\0';					// NUL terminate the pattern;

		if ((pattern[0] == '\0')	// Comment empty line
	||  (!isgraph(pattern[0]))		// Comment blank or empty line
	||  (isspace(pattern[0]))		// Comment line
	||  (pattern[0] == '~'))		// Explicit comment line
		{
		if (showExcl)				// Display the comment, if enabled
			printf("ExComment: %s\n", pattern);
		return (0);					// Ignore this request
		}

	return (add_list(pattern));		// Otherwise pattern IS a pattern
	}

/* ----------------------------------------------------------------------- */
	int							/* Return non-zero if an error */
fexcludeInit (void)

	{
	if (initNeeded)
		{
		initNeeded = FALSE;		// Prevent multiple initializations

		if (showExcl)			// Display the comment, if enabled
			{
			if (enbExcl)
				printf("ExInit:\n");
			else
				printf("ExInit:    Default exclusions disabled\n");
			}

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
		}

	return (0);
	}

/* ----------------------------------------------------------------------- */
