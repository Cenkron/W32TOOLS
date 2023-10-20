/*--------------------------------------------------------------------*\
|  RM - Remove files (and subdirectories)
\*--------------------------------------------------------------------*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <io.h>
#include <direct.h>
#include <malloc.h>
#include <fcntl.h>

#include <fWild.h>
#include <getoptns.h>

/*--------------------------------------------------------------------*/
#define VERSION "980906.043000"
/*--------------------------------------------------------------------*/

#define COPYRIGHT "Version 3.7 Copyright (c) 1998 J & M Software, Dallas TX - All Rights Reserved"

#pragma comment (user, "rm ")
#pragma comment (user, COPYRIGHT " ")
#pragma comment (user, "Edited " __TIMESTAMP__ " ")
#pragma comment (user, "Compiled " __DATE__ " " __TIME__ " ")

/*--------------------------------------------------------------------*/

int		d_flag	= FALSE;
int		i_flag	= FALSE;		// Enable pipe input from stdin
int		l_flag	= FALSE;		// Remove if larger than l_value bytes
int		n_flag	= FALSE;		// No remove listing output
long	l_value	= 0;
int		q_flag	= FALSE;
int		r_flag	= FALSE;
int		t_flag	= FALSE;
int		z_flag	= FALSE;

int		o_flag	= FALSE;
time_t	o_time	= 0;
int		y_flag	= FALSE;
time_t	y_time	= 0;

int		verbosity = 0;
int		NumberRequests = 0;

PHP		hp = NULL;				// FWILD instance pointer
PEX		xp = NULL;				// FEX instance pointer

/*--------------------------------------------------------------------*/

#define OLDERDT(x)	((x)<(o_time))
#define NEWERDT(x)	((y_time)<(x))

#define		NULCH	('\0')

/*--------------------------------------------------------------------*/

static	int		destroy (char *filename);
static	void	keep_dir_name (char *s);
static	void	proc_file (char *s);
static	void	processLoop (char *s, int att);
static	void	remove_directories (void);
static	int		query (char *s);
static	char   *UseStdin (void);		// Parse pathnames from stdin

/*--------------------------------------------------------------------*/

typedef
	struct DN
	{
	struct DN *	link;
	char *		name;
	} DIRNODE;

DIRNODE *root	= NULL;

/*--------------------------------------------------------------------*/
	char * 
usagedoc [] = {
	"Usage:  rm [-hnqrstvz] [-X...] [-lN] [-odt] [-ydt] file_list",
	"",
	"Remove (erase, delete) a [list of] file[s]",
	"The file_list may contain wildcards '?', '*', and '**'.",
	"",
	"-d    don't report 'files not found')",
	"-h    include /H/idden files",
	"-lN   remove only if /L/arger than N bytes",
	"-n    /N/o screen output (default: report failures)",
	"-odt  remove only if /O/lder than dt (datetime)",
	"-q    /Q/uery before each removal",
	"-r    remove even if /R/ead-only",
	"-s    include /S/ystem files",
	"-t    remove the empty directories in the /T/ree also",
	"-v    /V/erbose (report files as removed)",
	"-X <pathspec> e/X/clude (possibly wild) files that match pathspec",
	"-X @<xfile>   e/X/clude paths that match pathspec(s) in xfile",
	"-X-   disable default file exclusion(s)",
	"-X+   show exclusion path(s)",
	"-X=   show excluded path(s)",
	"-ydt  remove only if /Y/ounger than dt (datetime)",
	"-z    /Z/ero the file (make it unrecoverable - use with care)",
	"",
	COPYRIGHT,
	"",
	NULL};

/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
	int
main (
	int   argc,
	char *argv[])

	{
	int		c;
	int		attrib = (FW_FILE);
	char   *pathName;


	if ((hp = fwOpen()) == NULL)
		exit(1);
	if ((xp = fExcludeOpen()) == NULL)
		exit(1);

	optenv = getenv("RM");

	while ( (c=getopt(argc,argv,"dDhHl:L:nNo:O:qQrRsStTvVX:y:Y:zZ?")) != EOF )
		{
		switch (tolower(c))
			{
			case 'x' :
				if (c == 'x')
					usage();

				if      (optarg[0] == '-')
					fExcludeDefEnable(xp, FALSE);	// Disable default file exclusion(s)
				else if (optarg[0] == '+')
					fExcludeShowConf(xp, TRUE);		// Enable stdout of exclusion(s)
				else if (optarg[0] == '=')
					fExcludeShowExcl(xp, TRUE);		// Enable stdout of excluded path(s)
				else if (fExclude(xp, optarg))
					{
					printf("\7Exclusion string fault: \"%s\"\n", optarg);
					usage();
					}
				break;

			case 'd' :
				++d_flag;
				verbosity = 1;
				break;

			case 'h' :
				attrib |= FW_HIDDEN;
				break;

			case 'l' :
				l_value = atol(optarg);
				if (l_value > 0)
					l_flag = TRUE;
				break;

			case 'n' :
				++n_flag;
				break;

			case 'o':
				if ((o_time=sgettd(optarg)) == 0)
					fatalerr("Bad date/time value");
				else
					++o_flag;
				break;

			case 'q' :
				++q_flag;
				break;

			case 'r' :
				++r_flag;
				break;

			case 's' :
				attrib |= FW_SYSTEM;
				break;

			case 't' :
				attrib |= FW_DIR;
				++t_flag;
				break;

			case 'v' :
				++verbosity;
				break;

			case 'y':
				if ((y_time=sgettd(optarg)) == 0)
					fatalerr("Bad date/time value");
				else
					++y_flag;
				break;

			case 'z' :
				++z_flag;
				break;

			case '?':
				help();

			default:
				fprintf(stderr, "invalid option '%c'\n", optchar);
				usage();
			}
		}

	i_flag = ( ! isatty(fileno(stdin)));

	if (i_flag)
		{
		while ((pathName = UseStdin()) != NULL)		// Process the stdin list
			{
			if (verbosity > 1)
				printf("PathName (stdin): \"%s\"\n", pathName);
			++NumberRequests;

			processLoop(pathName, attrib);

			if (t_flag)
				remove_directories();
			}
		}

	else // (i_flag == FALSE)
		{
		while (optind < argc)						// Process the command line list
			{
			pathName = argv[optind++];
			if (verbosity > 1)
				printf("PathName (cmd): \"%s\"\n", pathName);
			++NumberRequests;
			processLoop(pathName, attrib);

			if (t_flag)
				remove_directories();
			}
		}

	if ((NumberRequests == 0) && ! d_flag)
		{
		fprintf(stderr, "No file(s) specified\n");
		usage();
		return(0);
		}

	xp = fExcludeClose(xp);			// Close the Exclusion instance
	hp = fwClose(hp);				// Close the fWild instance
	return(0);
	}

/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
	static void
processLoop (
	char *s,	// filename
	int   attr)	// attributes

	{
	char *fn;	// found filename
	char *t;
	BOOL  flag = FALSE;


	if (fwInit(hp, s, attr) != FWERR_NONE)	// Process the pattern
		fwInitError(s);
	if ((t_flag)
	&&  (s[strlen(s)-1] != '.'))
		{
		t = s + strlen(s) - 1;
		while (t>s && (*t=='*' || *t=='.' || *t=='\\'))
			{
			(*(t--)) = '\0';
			flag = TRUE;
			}
		if (flag && fnchkdir(s))
			{
			keep_dir_name(s);
			}
		}

	fExcludeConnect(xp, hp);		// Connect the exclusion instance
	if ((fn = fWild(hp)) == NULL)
		{
		if (! d_flag)
			printf("%s not found\n", s);
		return;
		}
    else
		{
		do	{
			proc_file(fn);
			} while (fn = fWild(hp));
		}
	}

/*--------------------------------------------------------------------*/
	static void
proc_file (
	char *s)	// filename

	{
	int		attrib	= fwtype(hp);
	int		result;
	time_t	fdt = 0;

	if ((attrib != (-1)) 
	&&  (attrib & ATT_DIR))
		{
		if ((t_flag)
		&&  (s[strlen(s)-1] != '.'))
			{
			keep_dir_name(s);
			}
		}

	else if ((!l_flag || (fwsize(hp) >= l_value))
		 &&  (!o_flag || OLDERDT(fwgetfdt(hp)))
		 &&  (!y_flag || NEWERDT(fwgetfdt(hp))))
		{
		if ( (attrib & ATT_RONLY) && !r_flag)
			{
			printf("%s is read/only\n", s);
			}
		else
			{
			if (query(s))
				{
				BOOL  OutDone = FALSE;
		
				if (verbosity > 0)
					{
					printf("%s", s);
					OutDone = TRUE;
					}
    
				if ((attrib & ATT_RONLY)
				&& (fsetattr(s, _A_NORMAL) < 0))
					{
					printf(" attribute change failed");
					OutDone = TRUE;
					}

				if (z_flag)
					result = destroy(s);
				else
					result = unlink(s);

				if (result)
					{
					printf(" remove failed");
					OutDone = TRUE;
					}
				else
					{
					if (verbosity > 1)
						{
						printf(" removed");
						OutDone = TRUE;
						}
					}
				if (OutDone)
					printf("\n");
				}
			}
		}
    }

/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
	static int
query (
	char *s)

	{
	int         i;
	int		retval		= TRUE;
	char        key;

	if (q_flag)
		{
		i = printf("Remove  %s ", s);

		for (; i<32; ++i)
			putchar(' ');

		printf("? [Y/N/R/QC] : ");

		key = get_key(FALSE, TRUE);
	
		switch (tolower(key))
			{
			case 'y':
				retval = TRUE;
				break;

			case 'r':
				q_flag = FALSE;
				retval = TRUE;
				break;

			case 'q':
			case 'c':
				printf("Remove terminated\n");
				exit(0);

			default:
				retval = FALSE;
				break;
			}
		}

	return (retval);
	}

/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
	static int
destroy (
	char * filename)

	{
	int		fh;
	long	fsize;
	int		result	= (-1);
	char	buffer  [1024];
	char	szDrive [1024];
	char	szDir   [1024];
	char	szName  [1024];
	char	szExt   [1024];

	if ((fh=open(filename, O_RDWR|O_BINARY)) >= 0)
		{
		memset(buffer, 0, sizeof(buffer));
		fsize = filelength(fh);
		lseek(fh, 0L, SEEK_SET);

		/* Convert fsize to a count of buffers */
		for (fsize /= sizeof(buffer), ++fsize;  fsize;  --fsize)
			(void)write(fh, buffer, sizeof(buffer));
		close(fh);

		_splitpath(filename, szDrive, szDir, szName, szExt);
		_makepath(buffer, szDrive, szDir, "_", NULL);

		unlink(buffer);
		result  = rename(filename, buffer);
		result |= unlink(buffer);
		}

	return (result);
	}

/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
	static void
remove_directories (void)

	{
	char *	s;

	for (; root; root = root->link)
		{
		s = root->name;

		if (query(s))
			{
			if (verbosity > 0)
				printf("%s <DIR>", s);
			if (rmdir(s) == 0)
				{
				if (verbosity > 1)
					printf(" removed\n");
				}
			else
				{
				if (verbosity > 0)
					printf("%s <DIR>", s);
				if (verbosity > 1)
					printf(" remove error\n");
				}
			}
		}
    }

/*--------------------------------------------------------------------*/
	static void
keep_dir_name (
	char *s)

	{
	DIRNODE *dirnode;

	// Don't keep names of form 'd:'

	if ((isalpha(*s))
	&&  (*(s+1) == ':')
	&&  (strlen(s) == 2))
		return;

	if ((dirnode=malloc(sizeof(DIRNODE))) != NULL)
		{
		dirnode->name = strdup(s);
		dirnode->link = root;
		root = dirnode;
		}
	}

/* ----------------------------------------------------------------------- */
	static char *
UseStdin ()						/* Parse pathnames from stdin */

	{
static	char  line [MAX_PATH];

	if (fgets(line, MAX_COPY, stdin) == NULL)
		return (NULL);

	// Truncate the line ending

	char *pStr;
	char  ch;

	pStr = line;
	while ((ch = *pStr) != NULCH)
		{
		if ((ch == '\r') || (ch == '\n'))
			{
			*pStr = NULCH;
			break;
			}
		++pStr;
		}
	
	return (line);
	}

/*--------------------------------------------------------------------*/
/*-------------------------- EOF -------------------------------------*/
/*--------------------------------------------------------------------*/
