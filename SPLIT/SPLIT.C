/* ----------------------------------------------------------------------- */
// SPLIT
//
// Split a (large) file into small(er) pieces
//
/* ----------------------------------------------------------------------- */
//#define VERSION "951208.213517"
#define VERSION "990430.000000"
/* ----------------------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <io.h>
#include <fcntl.h>
#include <sys\stat.h>

#include <dtypes.h>
#include <fwild.h>
#include <getoptns.h>

/* ----------------------------------------------------------------------- */

#define INPUT_MODE	(O_BINARY|O_RDONLY)
#define OUTPUT_MODE	(O_BINARY|O_WRONLY|O_CREAT),(S_IREAD|S_IWRITE)

/* ----------------------------------------------------------------------- */
	char
optstring [] = 
	{"aAbBl:L:s:S:?"};

	char
usageline [] = 
	"Usage:  split  [-ab?] [-sn[K|M]] [-ln]  [path]filespec[.ext]";

	char
copyright [] = 
	"Version 3.2 Copyright (c) 1995 J & M Software, Dallas TX - All Rights Reserved";

/* ----------------------------------------------------------------------- */
	char *
usagedoc [] = {
	usageline,
	"",
	"Split a (large) file into a set of small(er) file(s).",
	"",
	"-a    /a/scii mode",
	"-b    /b/inary mode (default)",
	"-sN   maximum file /s/ize (n bytes, nK kilobytes, nM megabytes)",
	"-lN   number of text /l/ines per file (ascii mode only)",
	"",
	"The filespec may contain wildcards '?', '*', and '**'.",
	"",
	"Output files are named the same as input files, with the",
	"extensions changed to .000 .001 .002 etc.",
	"",
	copyright,
	NULL};

/* ----------------------------------------------------------------------- */

static char	buffer [16*1024];

/* ----------------------------------------------------------------------- */

#define	bytes	optdata.lVal[GETOPT_S]
#define	lines	optdata.lVal[GETOPT_L]
#define	a_flag	optdata.flags.a
#define	b_flag	optdata.flags.b
#define	l_flag	optdata.flags.l

/* ----------------------------------------------------------------------- */

char   *MakeExtentName	(char *fn, int extent);
void	process		(FILE *fp, char *fnp);
void	SplitAscii	(FILE *fp, char *fnp);
void	SplitBinary	(FILE *fp, char *fnp);

/* ----------------------------------------------------------------------- */

/* ----------------------------------------------------------------------- */
	int
special_options (
	int c,
	char *arg)			 /*lint -esym(715,arg)  */

	{
	switch (tolower(c))
		{
		case 'a':
			b_flag = FALSE;
			break;

		case 'b':
			if (lines > 0)
				fatalerr("incompatible options -b -l");
			a_flag = FALSE;
			b_flag = TRUE;
			break;

		case 'l':
			if (b_flag)
				fatalerr("incompatible options -b -l");
			a_flag = TRUE;
			break;

		default:;
		}

	return (0);
	}

/* ----------------------------------------------------------------------- */
    int
main (
	int   argc,
	char *argv[])

	{
	int		iResult;
	const int	smode	= FW_FILE;	/* File search mode attributes */
	char *	fnp;	// = NULL;		/* Input file name pointer */
	FILE *	fp;	// = NULL;		/* Input file descriptor */


	optdata.pProc[GETOPT_A] = special_options;
	optdata.pProc[GETOPT_B] = special_options;
	optdata.pProc[GETOPT_L] = special_options;
	b_flag = TRUE;
	a_flag = FALSE;
	bytes = 1450000L;

	if ((iResult=GetOptions("SPLIT", argc, argv, optstring)) != 0)
		return (iResult);

	if ((bytes == 0) && (b_flag || (lines == 0)))
		fatalerr("How do you want the files split? (Type split -? for help)");

	if (optind == argc)
		process(stdin, "<stdin>");
	else
		{
		do  {
			char *	hp = fwinit(argv[optind], smode);	/* Process the input list */
			if ((fnp = fwild(hp)) == NULL)
				{
				hp = NULL;
				cantopen(argv[optind]);
				}
			else
				{
				do  {				/* Process one filespec */
					if ((fp = fopen(fnp, "r")) != NULL)
						{
						process(fp, fnp);
						fclose(fp);
						}
					else
						cantopen(fnp);
					} while ((fnp = fwild(hp)));
				hp = NULL;
				}
			} while (++optind < argc);
		}

	return (0);
	}

/* ----------------------------------------------------------------------- */

/* ----------------------------------------------------------------------- */
	void
process(				/* Process one file */
	FILE  *fp,			/* File descriptor */
	char  *fnp)			/* File name */ 

	{
	if (b_flag)
		SplitBinary(fp, fnp);
	else
		SplitAscii(fp, fnp);
	}

/* ----------------------------------------------------------------------- */
	char *
MakeExtentName (
	char *fnp,
	int extent)

	{
static char	ofname [1024];
	char	drive  [1024];
	char	dir    [1024];
	char	fname  [1024];
	char	ext    [1024];

	_splitpath(fnp, drive, dir, fname, ext);
	sprintf(ext, "%03d", extent);
	_makepath(ofname, drive, dir, fname, ext);

	return (ofname);
	}

/* ----------------------------------------------------------------------- */

/* ----------------------------------------------------------------------- */
	void
SplitAscii (			/* Process one input file */
	FILE  *fp,			/* Input file descriptor */
	char  *fnp)			/* Input file name */ 

	{
	long	b	= 0L;
	int		extent	= 0;
	long	l	= 0L;
	FILE *	ofp	= NULL;			/* Output file descriptor */

	while (fgets(buffer, sizeof(buffer), fp))
		{
		const size_t reclen = strlen(buffer) + 1;

		if (bytes  &&  ((long)(b+reclen) > bytes))
			{
			fclose(ofp);
			ofp = NULL;
			}

		if (ofp == NULL)
			{
			char *	ofname = MakeExtentName(fnp, extent++);
			if ((ofp = fopen(ofname, "w")) == NULL)
				cantopen(ofname);
			b = l = 0L;
			}

		fputs(buffer, ofp);
		fflush(ofp);
		b += reclen;
		l += 1;
    
		if (lines && (l > lines))
			{
			fclose(ofp);
			ofp = NULL;
			}
		}

	fprintf(stderr, "Input File '%s' split into %d file(s)\n", fnp, extent);
	}

/* ----------------------------------------------------------------------- */

/* ----------------------------------------------------------------------- */
	void
SplitBinary (			/* Process one input file */
	FILE  *fp,			/* Input file descriptor */
	char  *fnp)			/* Input file name */ 

	{
	long	b	= 0L;
	int		extent	= 0;
	int		ofh	= -1;			/* Output file handle */
	size_t	reclen;

	const size_t buflen = (min(sizeof(buffer), (size_t)bytes));

	fclose(fp);
	const int ifh = open(fnp, INPUT_MODE);

	while ((reclen=read(ifh, buffer, buflen)) > 0)
		{
		if ((long)(b+reclen)  >  bytes)
			{
			close(ofh);
			ofh = (-1);
			}

		if (ofh < 0)
			{
			char *	ofname = MakeExtentName(fnp, extent++);
			if ((ofh = open(ofname, OUTPUT_MODE)) < 0)
				cantopen(ofname);
			b = 0L;
			}

		write(ofh, buffer, reclen);
		b += reclen;
		}

	fprintf(stderr, "Input File '%s' split into %d file(s)\n", fnp, extent);
	}

/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */
