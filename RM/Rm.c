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

#include <fwild.h>
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

int		l_flag	= FALSE;
long	l_value	= 0;
int		o_flag	= FALSE;
time_t	o_time	= 0;
int		q_flag	= FALSE;
int		r_flag	= FALSE;
int		t_flag	= FALSE;
int		y_flag	= FALSE;
time_t	y_time	= 0;
int		z_flag	= FALSE;

int		verbosity = 1;

/*--------------------------------------------------------------------*/

#define OLDERDT(x)	((x)<(o_time))
#define NEWERDT(x)	((y_time)<(x))

/*--------------------------------------------------------------------*/

int		destroy (char * filename);
void	fatalerr (char *s);
void	keep_dir_name (char *s);
void	proc_file (char *s, char *dta);
void	process (char *s, int att);
void	remove_directories (void);
int		query (char *s);

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
	int		attrib	= FW_FILE | FW_SUBD;

	optenv = getenv("RM");

	while ( (c=getopt(argc,argv,"hHl:L:nNo:O:qQrRsStTvVX:y:Y:zZ?")) != EOF )
	switch (tolower(c))
		{
		case 'x' :
			if (c == 'x')
				usage();

			if      (optarg[0] == '-')
				fexcludeDefEnable(FALSE);		/* Disable default file exclusion(s) */
			else if (optarg[0] == '+')
				fexcludeShowConf(TRUE);			/* Enable stdout of exclusion(s) */
			else if (optarg[0] == '=')
				fexcludeShowExcl(TRUE);			/* Enable stdout of excluded paths(s) */
			else if (fexclude(optarg))
				{
				printf("\7Exclusion string fault: \"%s\"\n", optarg);
				usage();
				}
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
			verbosity = max(0, (verbosity - 1));
			break;

		case 'o':
			if ((o_time=sgettd(optarg)) < 0)
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
			++t_flag;
			break;

		case 'v' :
			verbosity = min(2, (verbosity + 1));
			break;

		case 'y':
			if ((y_time=sgettd(optarg)) < 0)
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

	if (optind == argc)
		{
		fprintf(stderr, "No file(s) specified.\n");
		usage();
		return(0);
		}

	while (optind < argc)
		process(argv[optind++], attrib);

	if (t_flag)
		remove_directories();

	return(0);
	}

/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
	void
process (
	char *s,
	int   att)

	{
	char *dta;
	char *fn;
	char *t;
	BOOL  flag = FALSE;

//	strupr(s);

	if (! (dta=fwinit(s,att)))
		{
		printf("rm %s\n", s);
		fatalerr("fwinit() failed");
		}

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

	fwExclEnable(dta, TRUE);			/* Enable file exclusion */
	if ((fn = fwild(dta)) == NULL)
		{
		dta = NULL;
		if (verbosity > 0)
			printf("%s not found.\n", s);
		return;
		}
    else
		{
		do	{
			proc_file(fn, dta);
			} while (fn = fwild(dta));
		dta = NULL;
		}
	}

/*--------------------------------------------------------------------*/
	void
proc_file (
	char *s,
	char *dta)

	{
	int		attrib	= fwtype(dta);
	int		result;

	if ((attrib != (-1)) 
	&&  (attrib & ATT_SUBD))
		{
		if ((t_flag)
		&&  (s[strlen(s)-1] != '.'))
			{
			keep_dir_name(s);
			}
		}

	else if ((!l_flag || (fwsize(dta) >= l_value))
		 &&  (!o_flag || OLDERDT(fwgetfdt(dta)))
		 &&  (!y_flag || NEWERDT(fwgetfdt(dta))))
		{
		if ( (attrib & ATT_RONLY) && !r_flag)
			{
			if (verbosity > 0)
				printf("%s is read/only.\n", s);
			}
		else
			{
			if (query(s))
				{
				BOOL  OutDone = FALSE;
		
				if (verbosity > 1)
					{
					printf("%s",s);
					OutDone = TRUE;
					}
    
				if ((attrib & ATT_RONLY)
				&& (fsetattr(s, _A_NORMAL) < 0)
				&& (verbosity > 0))
					{
					printf(" attribute change failed.");
					OutDone = TRUE;
					}

				if (z_flag)
					result = destroy(s);
				else
					result = unlink(s);

				if (result)
					{
					if (verbosity == 1)
						{
						printf("%s",s);
						OutDone = TRUE;
						}

					if (verbosity > 0)
						{
						printf(" remove failed.");
						OutDone = TRUE;
						}
					}
				else
					{
					if (verbosity > 1)
						{
						printf(" removed.");
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
	int
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
	int
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
	void
remove_directories (void)

	{
	char *	s;

	for (; root; root = root->link)
		{
		s = root->name;

		if (query(s))
			{
			if (verbosity > 1)
				printf("%s <DIR>",s);
			if (rmdir(s) == 0)
				{
				if (verbosity > 1)
					printf(" removed.\n");
				}
			else
				{
				if (verbosity == 1)
					printf("%s <DIR>",s);
				if (verbosity > 0)
					printf(" remove error.\n");
				}
			}
		}
    }

/*--------------------------------------------------------------------*/
	void
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

/*--------------------------------------------------------------------*/
/*-------------------------- EOF -------------------------------------*/
/*--------------------------------------------------------------------*/
