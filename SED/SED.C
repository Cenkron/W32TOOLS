/* ----------------------------------------------------------------------- *\
|  SED
|  -----------------------------------------------------------------------
|  16-Mar-93  msm  original
\* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <io.h>
#include  <stdlib.h>
#include  <string.h>
#include  <ctype.h>

#include  "fwild.h"
#include  "sr_exter.h"
#include  "greplib.h"

/* ----------------------------------------------------------------------- */
    char *
usagedoc [] =
    {
    "Usage:  sed  [-?CfglmnrsvwX]  [@]search-expr  replacement  [input_file_list]",
    "",
    "sed (stream editor) replaces a given text pattern in one or more files.",
    "",
    "-f      Don't print file names",
    "-g      Not global - replace first occurrence in each line",
    "-k      Keep backup (input.bak) file",
    "-m      Inverts the usage of the metacharacter escape",
    "-s      Find shortest match (default: find longest)",
    "-vv     Print verbose internal information",
	"-X <pathspec> e/X/clude (possibly wild) files matching pathspec",
	"-X @<xfile>   e/X/clude files that match pathspec(s) in xfile",
	"-X-       Disable default file exclusion(s)",
	"-X+       Show exclusion path(s)",
    "",
    "Input is from the file list.  Output replaces input.",
    "The input file list path names accept the wildcards '?', '*', and '**'.",
    "",
    "If the search-expression is preceeded by the character @, then the",
    "expression is taken to be the name of a file containing a regular",
    "expression.",
    "",
    "x       any character except metacharacters (listed below) match themselves",
    ".       matches any single character",
    "[abc]   matches any one of the characters inside the []",
    "[^abc]  matches any character except one of the characters inside the []",
    "\\x      escapes a metacharacter (matches a metacharacter)",
    "RE*     matches zero or more occurrences of the RE",
    "RE+     matches one or more occurrences of the RE",
    "RE{m}   matches exactly m occurrences of the RE",
    "RE{m,n} matches at least m, at most n occurrences of the RE",
    "        m < n  match the shortest sequence",
    "        m > n  match the longest sequence",
    "a|b     matches a or b",
    "(RE)    groups an RE",
    "<m>     matches null string m positions from beginning of string",
    "<m-n>   matches any null string from position m thru n",
    "<m,n,p> matches any null string at position m, or n, or p",
    "<~m>    matches null string m positions from end of string",
    "RE$a    assigns the RE to a pseudovariable",
    "<a>     matches RE previously assigned to pseudovariable",
    ":a      matches any alphabetic character",
    ":b      matches any binary digit",
    ":c      matches any c identifier character",
    ":d      matches any decimal digit",
    ":f      matches any float point digit, or -+.E",
    ":l      matches any lower case alphabetic",
    ":n      matches any alphanumeric",
    ":o      matches any octal digit",
    ":p      matches any punctuation",
    ":s      matches any white space",
    ":u      matches any upper case alphabetic",
    ":x      matches any hex digit",
    "",
    "The concatenation of regular expressions is a regular expression.",
    "",
    "Version 1.2 Copyright (c) 1996 J & M Software, Dallas TX - All Rights Reserved",
    NULL
    };

/* ----------------------------------------------------------------------- */

int             case_flag       = 1;    /* TRUE to preserve case */
int             debug           = 0;    /* Increment for debug output */
int             fflag           = 1;
int             gflag           = 1;
int             kflag           = 0;
char            lbuf [LMAX];            /* Line buffer for input  */
char            llbuf [LMAX];           /* Lower-case line buffer */
char *          pszReplacement;         /* Replacement string */
SR_CPAT *       psr_cpat        = NULL;

/* ----------------------------------------------------------------------- */

void    process         (char *);
void    sed             (FILE *, char *, FILE *);
char *  MakeBakName     (char *fnp);

/* ----------------------------------------------------------------------- */

/* ----------------------------------------------------------------------- *\
|  Program main
\* ----------------------------------------------------------------------- */
    void
main (
    int    argc,
    char  *argv [])
    {
static char *   optstring = "?fFgGkKmMsSvVX:";
    char *      ap;                     /* Argument pointer */
    char *      fnp   = NULL;           /* Input file name pointer */
    void *      hp    = NULL;           /* Pointer to wild file data block */
    int         option;                 /* Option character */
    char *      pszPattern;             /* Pointer to regular expression */
    int         smode = FW_FILE;        /* File search mode attributes */

    optenv = getenv("SED");

    while ((option = getopt(argc, argv, optstring)) != EOF)
        {
        switch (tolower(option))
            {
            case 'm':
                Sr_metaflag = !Sr_metaflag;
                break;

            case 's':
                Sr_shortmatch = !Sr_shortmatch;
                break;

            case 'f':           fflag = !fflag;         break;
            case 'g':           gflag = !gflag;         break;
            case 'k':           kflag = !kflag;         break;
            case 'v':           ++debug;                break;

            case 'x':
				if (option == 'x')
					usage();

				if (optarg[0] == '-')
					fexcludeDefEnable(FALSE);		/* Disable default file exclusion(s) */
				else if (optarg[0] == '+')
					fexcludeShowConf(TRUE);			/* Enable stdout of exclusion(s) */
				else if (fexclude(optarg))
                    {
                    printf("Exclusion string fault: \"%s\"\n", optarg);
                    usage();
                    }
                break;

            case '?':
                help();

            default:
                usage();
            }
        }

    if (optind >= argc)
        {
        printf("Search pattern missing\n");
        usage();
        }
    else
        {
        pszPattern = argv[optind++];
        if (!case_flag)
            strlwr(pszPattern);
        psr_cpat = sr_compile(precompile(pszPattern));
        
        if (debug >= 3)
            fprintf(stderr, "Raw Pattern: '%s'\n", pszPattern);
        if (debug >= 2)
            fprintf(stderr, "Use Pattern: '%s'\n", precompile(pszPattern));
        }

    if (optind >= argc)
        {
        printf("Replace pattern missing\n");
        usage();
        }
    else
        {
        pszReplacement = argv[optind++];
        }

    if (optind >= argc)
        {
        printf("File (list) missing\n");
        usage();
        }
    else
        {
        while (optind < argc)
            {
            ap = argv[optind++];
            hp = fwinit(ap, smode);             /* Process the input list */
			fwExclEnable(hp, TRUE);				/* Enable file exclusion */
            if ((fnp = fwild(hp)) == NULL)
				{
				hp = NULL;
                cantopen(ap);
				}
            else
                {
                do  {                           /* Process one filespec */
                    process(fnp);
                    } while ((fnp = fwild(hp)));
				hp = NULL;
                }
            }
        }

    exit(0);
    }

/* ----------------------------------------------------------------------- */

/* ----------------------------------------------------------------------- */
    void
process (char *fnp)
    {
    FILE *      fpIn    = NULL;
    FILE *      fpOut   = NULL;
    char *      fnpBak  = NULL;

    if ((fnpBak=MakeBakName(fnp)) == NULL)
        fatalerr("Can't make bak name\n");
    
    unlink(fnpBak);
    
    if (rename(fnp, fnpBak) != 0)
        fatalerr("Can't rename input to bak\n");
    
    if ((fpIn=fopen(fnpBak, "r")) == NULL)
        fatalerr("Can't open input (bak) file\n");
    
    if ((fpOut=fopen(fnp, "w")) == NULL)
        fatalerr("Can't open output file\n");

    sed(fpIn, fnp, fpOut);

    fclose(fpIn);
    fclose(fpOut);

    if ( ! kflag   &&   (unlink(fnpBak) != 0) )
        fatalerr("Can't delete input (bak) file\n");
    }

/* ----------------------------------------------------------------------- */

/* ----------------------------------------------------------------------- */
    void
sed (                           /* Scan the file for the pattern */
    FILE  *fpIn,                /* Input File to process */
    char  *fnp,                 /* Path/file name (for -f option) */
    FILE  *fpOut)               /* Output File to process */
    {
    int         m;

    while (fgetss(lbuf, sizeof(lbuf), fpIn))
        {
	if (debug >= 3)
	    printf("INPUT:  %s\n", lbuf);

        /* Do the search/replace on the line */
	if (gflag)
	    m = sr_csrg(lbuf, psr_cpat, pszReplacement);
	else
	    m = sr_csr(lbuf, psr_cpat, pszReplacement);

	if (debug >= 3)
	    printf("OUTPUT: %s\n", lbuf);

        /* Output the line */
        fprintf(fpOut, "%s\n", lbuf);

        /* Perhaps report the filename to the console */
        if (m && fflag && fnp)
            {
            printf("%s\n", fnp);
            fnp = NULL;
            }
        }
    }

/* ----------------------------------------------------------------------- */

/* ----------------------------------------------------------------------- */
    char *
MakeBakName (char *fnp)
    {
static char     ofname [1024];
    char        drive  [1024];
    char        dir    [1024];
    char        fname  [1024];
    char        ext    [1024];

    _splitpath(fnp, drive, dir, fname, ext);
    _makepath(ofname, drive, dir, fname, "BAK");

    return (ofname);
    }

/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */

