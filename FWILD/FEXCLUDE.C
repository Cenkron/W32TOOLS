/* ----------------------------------------------------------------------- *\
|
|				   FEXCLUDE
|		     Wild Card File Name Exclusion Server
|
|		   Copyright (c) 1993, all rights reserved
|				Brian W Johnson
|				   17-Feb-93
|				   17-Aug-97
|
\* ----------------------------------------------------------------------- */

#include  <windows.h>
#include  <stdio.h>
#include  <stdlib.h>
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

/* ----------------------------------------------------------------------- */
	char *				/* Return the pointer to the file name */
fwildexcl (				/* Get the first/next wild filename (ex mode) */
	DHV  *pdhv)			/* Pointer to the wild file structure */

	{
	char  *result;

	do	{
		result = fwild(pdhv);
		} while ((result != NULL)  &&  (exclude(result)));

	return (result);
	}

/* ----------------------------------------------------------------------- */
	static int			/* Return TRUE to exclude the pathname string */
exclude (				/* Test the string for exclusion */
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
	int					/* Return non-zero if an error */
fexclude (				/* Exclude a path from the fwild search */
	char  *pattern)		/* Pointer to the pattern string */

	{
	if (pattern == NULL)
		{
		clear_list();
		return (0);
		}
	else if (*pattern == '@')
		return (do_file(pattern + 1));

//	printf("Excluding %s\n", pattern);

	return (add_list(pattern));
	}

/* ----------------------------------------------------------------------- */
	static int			/* Return non-zero if an error */
do_file (				/* Process an exclusion file */
	char  *pattern)		/* Pointer to the pattern string */

	{
	int    result = 0;	/* The returned result */
	int    length;		/* Length of each line */
	FILE  *fp;			/* The open exclusion file */
	char   line [LINEBUFFER];	/* The line buffer */

	if (((pattern = fwfirst(pattern)) == NULL)
	||  ((fp = fopen(pattern, "rt")) == NULL))
		result = -1;

	else
		{
		while (fgets(line, LINEBUFFER, fp) != NULL)
			{
			if ((length = strlen(line)) <= 1)
				break;

			line[length - 1] = '\0';
			if (result = add_list(strtok(line, " \t")))
				break;
			}

		fclose(fp);
		}

	return (result);
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
