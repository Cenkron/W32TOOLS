/*********************************************************************\
                CHMOD - change file attributes
 ---------------------------------------------------------------------
 Copyright (c) 1985-2018 Miller Micro Systems - All Rights Reserved
\*********************************************************************/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dos.h>
#include <fwild.h>
#include <getoptns.h>

/*--------------------------------------------------------------------*/
#define VERSION "930730.113707"
/*--------------------------------------------------------------------*/

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
usageline [] =
#if defined(DIRECTORIES)
	{"Usage:  chmod  [-aArRhHsS] [+arhs] [=arhs0] [-?cdqv] [-x[@]xfile] file_list"};
#else
	{"Usage:  chmod  [-aArRhHsS] [+arhs] [=arhs0] [-?cqv] [-x[@]xfile] file_list"};
#endif

/**********************************************************************/
	static char 
copyright [] =
	{"Version 4.3 Copyright (c) 1998 J & M Software, Dallas TX - All Rights Reserved"};

/**********************************************************************/
	char *
usagedoc [] ={
	usageline,
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
	"-x xspec  exclude:   exclude files which match xspec",
	"-x@xlist  exclude:   exclude files which match xspec(s) in xlist",
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

void    process (char *fnp);
int     query (char *s);
char ** getoptn (int *argc, char **argv);
void    cantopen (char *fnp);
void    procloop (char *arg, int smode);

/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
	void
process (
	char *fnp)             /* Process one input file */

	{
	int attrib = fgetattr(fnp);

	if (attrib < 0)
		printf("Unable to get attributes: %s\n", fnp);
	else if (plus_flags | minus_flags | equal_flags | do_anyway)
		{
		if (!c_flag)
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
				printf("Unable to change attributes: %s\n", fnp);
			attrib = fgetattr(fnp);
			}
		else  /* c_flag */
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
	int
query (
	char *s)

	{
	if (q_flag)
		{
		printf("change %-40s? [Y/N/R/QC] : ",s);

		const char key = get_key(FALSE, TRUE);
		if (key == 'y')
			return TRUE;
		if ((key == 'q')  ||  (key == 'c'))
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

/*--------------------------------------------------------------------*/
	char **
getoptn (
	int *argc,
	char **argv)

	{
	char *      s;

	plus_flags  = 0;
	minus_flags = 0;
	equal_flags = 0;
	do_anyway   = 0;
	c_flag      = 0;
	q_flag      = 0;

	while (--(*argc) > 0 )
		{
		++argv;
    
		if ( ((*argv)[0] == '/') )
			{
			for (s = argv[0]+1; *s != '\0'; s++)
				{
				switch (tolower(*s))
					{
					case 'c':   ++c_flag;               break;
#if defined(DIRECTORIES)
					case 'd':   ++d_flag;               break;
#endif
					case 'q':   ++q_flag;               break;
					case 'v':   ++v_flag;               break;

					case '?':   help();

					default:    
						fprintf(stderr, "invalid option '%c'\n", optchar);
						usage();
					}
				}
			}
    
		else if ( ((*argv)[0] == '-') )
			{
			for (s = argv[0]+1; *s != '\0'; s++)
				{
				switch (*s)
					{
					case 'a':   minus_flags |= _A_ARCH;         break;
					case 'h':   minus_flags |= _A_HIDDEN;       break;
					case 'r':   minus_flags |= _A_RDONLY;       break;
					case 's':   minus_flags |= _A_SYSTEM;       break;

					case 'A':   plus_flags  |= _A_ARCH;         break;
					case 'H':   plus_flags  |= _A_HIDDEN;       break;
					case 'R':   plus_flags  |= _A_RDONLY;       break;
					case 'S':   plus_flags  |= _A_SYSTEM;       break;

					case 'C':
					case 'c':   ++c_flag;               break;
#if defined(DIRECTORIES)
					case 'D':
					case 'd':   ++d_flag;               break;
#endif
					case 'Q':
					case 'q':   ++q_flag;               break;

					case 'V':
					case 'v':   ++v_flag;               break;

					case '?':   help();

					default:    
						fprintf(stderr, "invalid option '%c'\n", optchar);
						usage();
					}
				}
			}
    
		else if ((*argv)[0] == '+')
			{
			for (s = argv[0]+1; *s != '\0'; s++)
				{
				switch (tolower(*s))
					{
					case 'a':   plus_flags |= _A_ARCH;          break;
					case 'h':   plus_flags |= _A_HIDDEN;        break;
					case 'r':   plus_flags |= _A_RDONLY;        break;
					case 's':   plus_flags |= _A_SYSTEM;        break;

					default:    
						fprintf(stderr, "invalid option '%c'\n", optchar);
						usage();
					}
				}
			}

		else if (((*argv)[0] == '#') || ((*argv)[0] == '='))
			{
			for (s = argv[0]+1; *s != '\0'; s++)
				{
				switch (tolower(*s))
					{
					case 'a':   equal_flags |= _A_ARCH;         break;
					case 'h':   equal_flags |= _A_HIDDEN;       break;
					case 'r':   equal_flags |= _A_RDONLY;       break;
					case 's':   equal_flags |= _A_SYSTEM;       break;
					case '0':   ++do_anyway;                    break;

					default:    
						fprintf(stderr, "invalid option '%c'\n", optchar);
						usage();
					}
				}
			}
		else
			{
			break;
			}
		}
    
	return (argv);
	}

/*--------------------------------------------------------------------*/

/* ------------------------------------------------------------------------ */
	void
cantopen (
	char *fnp)                    /* Inform user of input failure */

	{
	printf("\nFile not found:  %s\n", fnp);
	}

/* ------------------------------------------------------------------------ */
	void
procloop (
	char *arg,
	int smode)

	{
	char  *fnp;	    // = NULL;                  /* Input file name pointer */
	char *hp = fwinit(arg, smode);             /* Process the input list */

	if ((fnp = fwildexcl(hp)) == NULL)
		cantopen(arg);
	else
		{
		do  {                           /* Process one filespec */
			if (strcmp(fnp, ".") != 0
			&&  strcmp(fnp, "..") != 0)
				{
//				strupr(fnp);
				fnreduce(fnp);
				strsetp(fnp, path_char);
				process(fnp);
				}
			} 
		while ((fnp = fwildexcl(hp)));
		}
	}

/* ------------------------------------------------------------------------ */

/* ------------------------------------------------------------------------ */
	int
main (
	int argc,
	char *argv[])

	{
	const int smode = FW_FILE
					+ FW_HIDDEN
					+ FW_SYSTEM;		/* File search mode attributes */

	path_char = egetpath();

	argv = getoptn(&argc, argv);		/* get option flags */

#if defined(DIRECTORIES)
	if (d_flag)
		smode |= FW_SUBD;
#endif

	if (argc == 0)
		procloop("*.*", smode);
	else
		{
		do  {
			procloop(*argv, smode);
			} 
		while (*++argv);
		}

	if (q_flag || !v_flag)
		printf("\n");

	return (0);
	}

/* ------------------------------------------------------------------------ */
/*                              EOF                                         */
/* ------------------------------------------------------------------------ */
