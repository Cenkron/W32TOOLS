/* ----------------------------------------------------------------------- */
// SPLICE
//
// Splice together what SPLIT split
//
/* ----------------------------------------------------------------------- */
#define VERSION "951208.213519"
/* ----------------------------------------------------------------------- */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
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
    {"aAbB?"};

    char
usageline [] = 
    "Usage:  splice  [-ab?]  [path]filespec.ext";

    char
copyright [] = 
    "Version 1.2 Copyright (c) 1995 J & M Software, Dallas TX - All Rights Reserved";

/* ----------------------------------------------------------------------- */
    char *
usagedoc [] = {
    usageline,
    "",
    "Splice a set of small(er) file(s) into a (large) file.",
    "Splice is intended to reassemble a set of files produced by Split.",
    "",
    "-a    /a/scii mode",
    "-b    /b/inary mode (default)",
    "",
    "Input files are named the same as output files, with the",
    "extensions changed to .000 .001 .002 etc.",
    "",
    copyright,
    NULL};

/* ----------------------------------------------------------------------- */

static char	buffer [16*1024];

/* ----------------------------------------------------------------------- */

#define	a_flag	optdata.flags.a
#define	b_flag	optdata.flags.b

/* ----------------------------------------------------------------------- */

char *	MakeExtentName	(char *fn, int extent);
void	process		(FILE *fp, char *fnp);
void	SpliceAscii	(FILE *fp, char *fnp);
void	SpliceBinary	(FILE *fp, char *fnp);

/* ----------------------------------------------------------------------- */

/* ----------------------------------------------------------------------- */
    int
special_options (int c, char *arg) /*lint -esym(715,arg)  */
    {

    switch (tolower(c))
	{
	case 'a':
	    b_flag = FALSE;
	    break;

	case 'b':
	    a_flag = FALSE;
	    b_flag = TRUE;
	    break;

	default:;
	}

    return (0);
    }

/* ----------------------------------------------------------------------- */
    int
main (int argc, char *argv[])
    {
    int		iResult;
    FILE *	fp;	// = NULL;		/* Input file descriptor */

    optdata.pProc[GETOPT_A] = special_options;
    optdata.pProc[GETOPT_B] = special_options;
    b_flag = TRUE;
    a_flag = FALSE;

    if ((iResult=GetOptions("SPLICE", argc, argv, optstring)) != 0)
	return (iResult);

    if (optind == argc)
	process(stdin, "<stdin>");
    else
	{
	do  {
	    char *fnp = argv[optind];
	    if ((fp=fopen(fnp, "w")) == NULL)
		cantopen(fnp);
	    else
		process(fp, fnp);
	    } while (++optind < argc);
	}

    return (0);
    }

/* ----------------------------------------------------------------------- */

/* ----------------------------------------------------------------------- */
    void
process(			/* Process one file */
    FILE  *fp,			/* File pointer */ 
    char  *fnp			/* File name */ 
    )
    {

    if (b_flag)
	SpliceBinary(fp, fnp);
    else
	SpliceAscii(fp, fnp);
    }

/* ----------------------------------------------------------------------- */
    char *
MakeExtentName (char *fnp, int extent)
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
SpliceAscii (				/* Process one output file */
    FILE  *fp,				/* Output file descriptor */
    char  *fnp				/* Output file name */ 
    )
    {
    int		extent	= 0;
    FILE *	ifp;			/* Output file descriptor */

    while ((ifp = fopen(MakeExtentName(fnp, extent++), "r")) != NULL)
	{
	while (fgets(buffer, sizeof(buffer), ifp))
	    {
	    fputs(buffer, fp);
	    fflush(fp);
	    }

	fclose(ifp);
	}

    fprintf(stderr, "%d Input file(s) Spliced into '%s'\n", --extent, fnp);
    }

/* ----------------------------------------------------------------------- */
    void
SpliceBinary (				/* Process one output file */
    FILE  *fp,				/* Output file descriptor */
    char  *fnp				/* Output file name */ 
    )
    {
    int		extent	= 0;
    int		ifh;		// = (-1);		/* Input file handle */
    size_t	reclen;


    fclose(fp);
    const int ofh = open(fnp, OUTPUT_MODE);

    while ((ifh = open(MakeExtentName(fnp, extent++), INPUT_MODE)) >= 0)
	{
	while ((reclen=read(ifh, buffer, sizeof(buffer))) > 0)
	    write(ofh, buffer, reclen);
    
	close(ifh);
	}

    fprintf(stderr, "%d Input file(s) Spliced into '%s'\n", --extent, fnp);
    }

/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */
