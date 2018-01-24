/* ----------------------------------------------------------------------- *\
|
|				Filter Template
|
|			      Using level 2 input
|
|		    Copyright (c) 1990, 1995, all rights reserved
|				Brian W Johnson
|				    6-Feb-95
|
\* ----------------------------------------------------------------------- */

#include  "stdio.h"
#include  "ctype.h"
#include  "fwild.h"
#include  "string.h"

/* ----------------------------------------------------------------------- */

char   *usagedoc [] =
{

"Usage:  scan  [%c?deil]  [input_file_list]  [>output_file]",
"",
"scan selectively copies the input stream to the output stream",
"based on control strings detected within the input stream.",
"",
"    %cd <string>  disables copying (to 0)",
"    %ce <string>  enables copying (to 1)",
"    %ci <0 | 1>   determines the initial state (default 0)",
"    %cl           lists file names as they are processed",
"",
"Copyright (c) 1995 by Brian W Johnson, Lucas TX - All Rights Reserved",
NULL
};

/* ----------------------------------------------------------------------- */

int	ArgFlag     = FALSE;			/* The active argument flag */
int	Enabled     = FALSE;			/* The enabled flag */
int	l_flag      = FALSE;			/* List file names flag */
char   *pEnableStr  = NULL;			/* Enabling string */
char   *pDisableStr = NULL;			/* Disabling string */
int	EnableSize  = 0;			/* Length of the enable string */
int	DisableSize = 0;			/* Length of the disable string */

static	void	process (FILE *, char *);

/* ----------------------------------------------------------------------- */
    void
main (
    int    argc,			/* Argument count */
    char  *argv [])			/* Argument list pointer */

    {
    int    smode = FW_FILE;		/* File search mode attributes */
    int    option;			/* Option character */
    char  *ap;				/* Argument pointer */
    void  *hp  = NULL;			/* Pointer to wild file data block */
    char  *fnp = NULL;			/* Input file name pointer */
    FILE  *fp  = NULL;			/* Input file descriptor */


    setbuf(stdout, fmalloc(BUFSIZ));

//  optenv = getenv("SCAN");

    while ((option = getopt(argc, argv, "?d:D:e:E:i:I:lL")) != EOF)
	{
	switch (tolower(option))
	    {
	    case 'd':
		++ArgFlag;
		pDisableStr = optarg;
		DisableSize = strlen(optarg);
		break;

	    case 'e':
		++ArgFlag;
		pEnableStr = optarg;
		EnableSize = strlen(optarg);
		break;

	    case 'i':
		Enabled = *optarg == '1';
		break;

	    case 'l':
		++l_flag;
		break;

	    case '?':
		help();

	    default:
		usage();
	    }
	}


    if ( ! ArgFlag)
	usage();

    if (optind >= argc)
	process(stdin, "<stdin>");

    else
	{
	while (optind < argc)
	    {
	    ap = argv[optind++];
	    hp = fwinit(ap, smode);		/* Process the input list */
	    if ((fnp = fwild(hp)) == NULL)
		cantopen(ap);
	    else
		{
		do  {				/* Process one filespec */
		    if (fp = fopen(fnp, "r"))
			{
			process(fp, fnp);
			fclose(fp);
			}
		    else
			cantopen(fnp);
		    } while ((fnp = fwild(hp)));
		}
	    }
	}
    }

/* ----------------------------------------------------------------------- */
    static void
process (fp, fnp)		/* Process one input file */
    FILE  *fp;			/* Input file descriptor */
    char  *fnp;			/* Input file name */

    {
    char  record [16384];

    if (l_flag)
	printf("Scanning file: %s\n", fnp);

    while (fgets(record, sizeof record, fp))
	{
	if (Enabled)
	    {
	    if ((DisableSize != 0)
	    &&  (strncmp(pDisableStr, record, DisableSize) == 0))
		Enabled = FALSE;
	    }
	else
	    {
	    if ((EnableSize != 0)
	    &&  (strncmp(pEnableStr, record, EnableSize) == 0))
		Enabled = TRUE;
	    }

	if (Enabled)
	    {
	    fputs(record, stdout);
	    fflush(stdout);
	    }
	}
    }

/* ----------------------------------------------------------------------- */
