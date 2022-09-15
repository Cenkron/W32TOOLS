/*--------------------------------------------------------------------*\
|  SPATH - Search DOS PATH for an executable file
\*--------------------------------------------------------------------*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fwild.h>

/*--------------------------------------------------------------------*/
#define VERSION "950412.152651"
/*--------------------------------------------------------------------*/

#define COPYRIGHT "Version 0.1 Copyright (c) 1995 J & M Software, Dallas TX - All Rights Reserved"

#pragma comment (user, "spath ")
#pragma comment (user, COPYRIGHT " ")
#pragma comment (user, "Edited " __TIMESTAMP__ " ")
#pragma comment (user, "Compiled " __DATE__ " " __TIME__ " ")

/*--------------------------------------------------------------------*/

int	verbosity	= 1;
int	attrib		= FW_FILE;
char *szPath		= NULL;
char *aszPaths [32]	= {NULL};
char	szTempPath [2048];

char *aszExts []	= {".BAT", ".COM", ".EXE", ".DLL", NULL};

/*--------------------------------------------------------------------*/
	char * 
usagedoc [] = {
	"Usage:  spath [-?hnsv] [-X...] file_list",
	"",
	"Search the DOS PATH for a [list of] file[s]",
	"The file_list may contain wildcards '?' and '*'.",
	"",
	"-h           include /H/idden files",
	"-s           include /S/ystem files",
	"-n           /N/o screen output",
	"-v           /V/erbose output",
	"-X <pathspec> e/X/clude (possibly wild) paths that match pathspec",
	"-X @(xfile>   e/X/clude paths that match pathspec(s) in xfile",
	"-X-          disable default file exclusion(s)",
	"-X+          show exclusion path(s)",
	"",
	COPYRIGHT,
	"",
	NULL};

/*--------------------------------------------------------------------*/

BOOL	GetPaths (void);
void	proc_name (char *szDir, char *szName, char *szExt);
void	proc_dir (char *s);
void	proc_file (char *s, char *dta);
void	process (char *s);

/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
	int
main (int argc, char *argv[])
	{
	int		c;

	optenv = getenv("SPATH");

	while ( (c=getopt(argc,argv,"hHnNsSvVX:?")) != EOF )
		{
		switch (tolower(c))
			{
			case 'x' :
				if (c == 'x')
					usage();

				if      (optarg[0] == '-')
					fexcludeDefEnable(FALSE);		/* Disable default file exclusion(s) */
				else if (optarg[0] == '+')
					fexcludeShowConf(TRUE);			/* Enable stdout of exclusion(s) */
				else if (fexclude(optarg))
					{
					fprintf(stderr, "Error excluding %s\n", optarg);
					usage();
					}
				break;

			case 'h' :
				attrib |= FW_HIDDEN;
				break;

			case 's' :
				attrib |= FW_SYSTEM;
				break;

			case 'n' :
				verbosity = 0;
				break;

			case 'v' :
				++verbosity;
				break;

			case '?':
				help();

			default:
				fprintf(stderr, "invalid option '%c'\n", optchar);
				usage();
			}
		}

	if (optind == argc)
		{
		fprintf(stderr, "No file(s) specified.\n");
		usage();
		return(0);
		}

	if (!GetPaths())
		return (1);

	while (optind < argc)
		process(argv[optind++]);

	return(0);
	}

/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
	void
process (
	char *s)

	{
	int		i;
	int		j;
	char	szDrive [1024];
	char	szDir   [1024];
	char	szName  [1024];
	char	szExt   [1024];

	strupr(s);

	_splitpath(s, szDrive, szDir, szName, szExt);
	if (*szDrive || *szDir)
		{
		proc_dir(s);
		return;
		}

	for (i=0; (i < 32) && aszPaths[i]; ++i)
		{
		if (*szExt)
			proc_name(aszPaths[i], szName, szExt);
		else
			{
			for (j=0; aszExts[j]; ++j)
				proc_name(aszPaths[i], szName, aszExts[j]);
			}
		}
	}

/*--------------------------------------------------------------------*/
	void
proc_name (
	char *szDir,
	char *szName,
	char *szExt)

	{
	char	szPath [1024];

	_makepath(szPath, NULL, szDir, szName, szExt);
	if (verbosity > 1)
		fprintf(stderr, "Processing '%s'\n", szPath);
	proc_dir(szPath);
	}

/*--------------------------------------------------------------------*/
	void
proc_dir (
	char *s)

	{
	char *	dta;
	char *	fn;

	if (! (dta=fwinit(s, attrib)))
		fatalerr("fwinit() failed");
	fwExclEnable(dta, TRUE);				/* Enable file exclusion */
	if ((fn = fwild(dta)) == NULL)
		{
		dta = NULL;
		if (verbosity > 1)
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
	int		n;
	int		attr	= fwtype(dta);

	if (verbosity > 0)
		{
		n = printf("%s ", s);
		while (n++ < 41)
			putchar(' ');
		printf("%8lld  %9s  %6s  %c%c%c%c\n", 
			fwsize(dta), fwdate(dta), fwtime(dta),
			(attr & ATT_ARCH)   ? 'a' : '-',
			(attr & ATT_RONLY)  ? 'r' : '-',
			(attr & ATT_HIDDEN) ? 'h' : '-',
			(attr & ATT_SYSTEM) ? 's' : '-' );
		}
	else
		printf("Full name is '%s'\n", s);
	}

/*--------------------------------------------------------------------*/
	BOOL
GetPaths (void)
    
	{
	BOOL	bResult = FALSE;
	int		i;
	char *	s;


	szPath = getenv("PATH");
	if (verbosity > 2)
		fprintf(stderr, "Original path: '%s'\n", szPath);

	aszPaths[0] = ".";
	if (verbosity > 2)
		fprintf(stderr, "Default comp:  '%s'\n", aszPaths[0]);

	if (szPath)
		{
		strncpy(szTempPath, szPath, sizeof(szTempPath)-1);

		s = strtok(szTempPath, ";");
		if (s)
			{
			for (i=1; s && (i < 32); ++i, s=strtok(NULL, ";"))
				{
				aszPaths[i] = s;
				if (verbosity > 2)
					fprintf(stderr, "Component:     '%s'\n", s);
				}
			}
		bResult = TRUE;
		}

	return (bResult);
	}

/*--------------------------------------------------------------------*/
/*-------------------------- EOF -------------------------------------*/
/*--------------------------------------------------------------------*/
