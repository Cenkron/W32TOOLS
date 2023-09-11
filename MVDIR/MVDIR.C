/* ----------------------------------------------------------------------- *\
|
|				     MVDIR
|
|		    Copyright (c) 1985, 1990, all rights reserved
|				Brian W Johnson
|				   30-May-92
|				   18-Feb-93
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <direct.h>
#include  <ctype.h>

#include  "fwild.h"
#include  "mvd.h"

#ifndef TRUE
#define FALSE	  0
#define TRUE	  1
#endif

#ifndef MATCH
#define MATCH	  0
#endif

char	copyright [] =
"Copyright (c) 1993 by J & M Software, Dallas TX - All Rights Reserved";

#define  wild(s)		(strpbrk(s, "?*") != NULL)

/* ----------------------------------------------------------------------- */

int	c_flag = FALSE;			/* Create path flag */
int	e_flag = FALSE;			/* Exists OK flag */
int	l_flag = FALSE;			/* List file names flag */
int	m_flag = FALSE;			/* Use old create/copy/delete mode */
int	p_flag = FALSE;			/* List path names flag */

int	f_exist = FALSE;		/* Final_path exists flag */

char	swch = '-';			/* The switch character */

static  void    checkout (char *, char *);
		void    bldfinal (char *);

/* ----------------------------------------------------------------------- */

char  *usagedoc [] =
{
"Usage:  mvdir  [%c?celmp]  initial_path  final_path",
"",
"mvdir moves a single directory and all of its contents from one",
"place in the directory tree to another.",
"",
"    %cc  creates any part of the final_path which does not",
"        already exist, otherwise if an intermediate directory ",
"        is missing, an error will result",
"    %ce  allows the move even if the final_path directory",
"        already exists, otherwise if the directory already",
"        exists, an error will result",
"    %cl  lists the file and directory names as they are moved",
"    %cm  renames in place; rather than old create/copy/delete mode",
"    %cp  lists the resolved path names",
"",
"The initial_path name must be the non-wild name of a single",
"existing directory which is to be moved, along with all of",
"its contents, to the directory named by the final_path.",
"",
"The final_path name must be the non-wild name of a single",
"directory which is to receive the contents of the directory",
"specified by the initial path.  If the named directory already",
"exists, the %ce flag must be used, or an error will result.",
"If some intermediate part of the path does not exist, the",
"%cc flag must be used, or an error will result.",
"",
"A drive may be specified in the initial_path.  If so, then the",
"final_path must specify the same drive, or an error will result.",
"",
copyright,
NULL
};

/* ----------------------------------------------------------------------- */
	void
main (
	int    argc,
	char  *argv [])

	{
	int    option;		/* Option character */
	int    nargs;		/* Number of arguments */
	char  *ap;                  /* POinter to the initial path */

static	char   *optstring = "?cCeElLmMpP";


	swch   = egetswch();
	optenv = getenv("MVDIR");

	while ((option = getopt(argc, argv, optstring)) != EOF)
		{
		switch (tolower(option))
			{
			case 'c':
				++c_flag;
				break;

			case 'e':
				++e_flag;
				break;

			case 'l':
				++l_flag;
				break;

			case 'm':
				m_flag = TRUE;
				break;

			case 'p':
				++p_flag;
				break;

			case '?':
				help();

			default:
				usage();
			}
		}

	nargs = argc - optind;
	if (nargs < 2)
		fatalerr("Path name(s) missing");

	if (nargs > 2)
		fatalerr("Too many path names");

	ap = argv[optind++];		/* Point the initial path */

	checkout(ap, argv[optind++]);
	exit(0);
	}

/* ----------------------------------------------------------------------- */
	static void
checkout (				/* Process one input file */
	char  *s1,			/* Pointer to the initial_path name */ 
	char  *s2)			/* Pointer to the final_path name */ 

	{
	char  *p1;			/* Pointer to absolute initial_path name */
	char  *p2;			/* Pointer to absolute final_path name */
	char  *p3;			/* Pointer to absolute current directory */


	if (wild(s1) || wild(s2))
		fatalerr("Path names can not be wild");

	if ((p1 = fnabspth(s1)) < 0)
		fatalerr("src fnabspth error");

	if ((p2 = fnabspth(s2)) < 0)
		fatalerr("dst fnabspth error");

	if ((p3 = fnabspth(".")) < 0)
		fatalerr("fnabspth error");

	if (p_flag)
		{
		printf("Current_path: %s\n", p3);
		printf("Initial_path: %s\n", p1);
		printf("Final_path:   %s\n", p2);
		}

	if (p1[0] != p2[0])
		fatalerr("Both paths must be on the same drive");

	if (strlen(p1) == 3)
		fatalerr("Can't move the root directory");

	if (stricmp(p1, p2) == MATCH)
		fatalerr("Can't move a directory to itself");

	if (fnsubpth(p1, p2))
		fatalerr("Can't move between overlapping trees");

	if (fnsubpth(p1, p3))
		fatalerr("Can't move the current directory");

	if ( ! fnchkdir(p1))	/* Verify the existence of initial_path */
		fatalerr("Initial_path not found");

	bldfinal(p2);		/* Verify the existence of final_path */

	mvdir(p1, p2, l_flag, m_flag);	/* Perform the recursive move */
	}

/* ----------------------------------------------------------------------- */
	void
bldfinal (			/* Verify or build the final_path directory */
	char  *s)		/* Pointer to the directory path string */

	{
	char   temp [80];	/* Temporary file name string */
	char  *p;		/* Pointer into the temporary string */


	if (fnchkdir(s))
		{
		if (e_flag)
			{
			f_exist = TRUE;	/* The final_path directory already exists */
			return;
			}
		else
			fatalerr("Final_path exists, use the exists switch");
		}

	p = &temp[2];		/* Check or build the final_path */
	do	{
		strcpy(&temp[0], s);
		if (p = strpbrk((p + 1), "/\\"))
			{
			*p = '\0';
			if ( ! fnchkdir(&temp[0]))
				{
				if (c_flag)
					{
					if (mkdir(&temp[0]) == -1)
					fatalerr("Can't create the final_path");
					}
				else
					fatalerr("Final_path not found, use the create switch");
				}
			}
		} while (p);
	}

/* ----------------------------------------------------------------------- */
