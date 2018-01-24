/* ----------------------------------------------------------------------- *\
|
|			Metaware Section Modifier Program
|
|		  Copyright (c) 1988, 1995  all rights reserved
|				Brian W Johnson
|				   17-Jul-96
|				   17-Aug-97
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <stdlib.h>
#include  <ctype.h>
#include  <string.h>
#include  <fcntl.h>
#include  <io.h>

#include  <ptypes.h>

/* ----------------------------------------------------------------------- *\
|  Definitions
\* ----------------------------------------------------------------------- */

#define  SWCH '-'		/* The switch character */

#ifndef FALSE
#define FALSE	  0
#define TRUE	  1
#endif

#ifdef  __HIGHC__
typedef  unsigned char   BYTE, *PBYTE;
typedef  unsigned short  UINT16;
#endif

#define match(p)  (memcmp((p), &Search[0], (SearchLength + 1)) == 0)

/* ----------------------------------------------------------------------- *\
|  Function prototypes
\* ----------------------------------------------------------------------- */

void	usagei        (int);
void	dprint        (char **);
void	FileOpen      (void);
void	ProcessSymbols(char *pSearch, char *pReplace);
void	ProcessFile   (void);

/* ----------------------------------------------------------------------- *\
|  Private variables
\* ----------------------------------------------------------------------- */

int	v_flag = 0;		// The verbose flag

int	infile;			// The input file handle
int	outfile;		// The output file handle
char   *pInfile;		// Pointer to the input filename
char   *pOutfile;		// Pointer to the output filename

BYTE	Search [64];		// The search symbol buffer
BYTE	Replace [64];		// The replace symbol buffer
int	SearchLength;		// Length of the search string
int	ReplaceLength;		// Length of the replace string
int	Delta;			// The length delta
int	Records;		// Record count
int	Replacements;		// Replacement count
BYTE	Correction;		// The record checksum correction

char	copyright [] =
"Copyright (c) 1988, 1996 by Brian W. Johnson, Dallas TX - All Rights Reserved";

/* ----------------------------------------------------------------------- *\
|  main () - The main program
\* ----------------------------------------------------------------------- */
    void
main (
    int    argc,
    char  *argv [])

    {
    char   ch;			// Parser temporary
    char  *s;			// Parser temporary
//  char  *s1;			// Parser temporary


    while (--argc > 0 && (*++argv)[0] == SWCH)
	for (s = argv[0] + 1; ((ch = *(s++)) != 0); )
	    switch (tolower(ch))
		{
#if 0
		case 'i':
		    s1 = s;
		    while (isgraph(*s))
			{
			*(pSearchEntry++) = s;
			while (isgraph(*s))
			    {
			    if (*s == ';')
				{
				*(s++) = '\0';
				break;
				}
			    ++s;
			    }
			}
		    if (s == s1)
			{
			fprintf(stderr, "Missing search path\n");
			usagei(1);
			}
		    break;

		case 'l':
		    ++l_flag;
		    break;

		case 's':
		    ++s_flag;
		    break;
#endif
		case 'v':
		    v_flag = 1;
		    break;

		default:
		    if (ch != '?')
			fprintf(stderr, "Invalid switch '%c'\n", ch);
		    usagei(ch != '?');
		    break;
		}

    if (argc != 4)
	usagei(1);

    ProcessSymbols(argv[0], argv[1]);

    pInfile  = argv[2];
    pOutfile = argv[3];
    FileOpen();

    ProcessFile();

    close(infile);
    close(outfile);

    if (v_flag > 0)
	{
	printf("Search  (length %d)  \"%s\"\n", Search[0],  &Search[1]);
	printf("Replace (length %d)  \"%s\"\n", Replace[0], &Replace[1]);
	printf("Delta           %d\n",         Delta);
	printf("Records         %d\n",         Records);
	printf("Replacements    %d\n",         Replacements);
	}

    exit(0);
    }

/* ----------------------------------------------------------------------- *\
|  usagei () - Display usage information and exit
\* ----------------------------------------------------------------------- */
    void
usagei (int value)		// Display help documentation

    {
    static char  *hdoc [] =
	{
	"usage:  section  [-v] search replace infile outfile",
	"",
	"  section processes an Intel object file \"infile\",",
	"  replacing the symbol \"search\" with the symbol \"replace\",",
	"  writing the new file to \"outfile\".",
	"",
	"    -v   verbose output",
//	"",
//	copyright,
	NULL
	};

    dprint(hdoc);
    exit(value);
    }

/* ----------------------------------------------------------------------- *\
|  dprint () - Display usage data
\* ----------------------------------------------------------------------- */
    void
dprint (dp)			// Print documentation text
    char  **dp;			// Document array pointer

    {
    while (*dp)
	{
	printf(*(dp++));
	putchar('\n');
	}
    }

/* ----------------------------------------------------------------------- *\
|  FileOpen () - Attempt to open the files
\* ----------------------------------------------------------------------- */
    void
FileOpen (void)

    {
    if ((infile = open(pInfile, (O_BINARY | O_RDONLY))) < 0)
	{
	fprintf(stderr, "Unable to open input file \"%s\"\n", pInfile);
	exit(1);
	}

    if ((outfile = open(pOutfile, (O_BINARY | O_WRONLY | O_CREAT | O_TRUNC))) < 0)
	{
	fprintf(stderr, "Unable to open output file \"%s\"\n", pOutfile);
	exit(1);
	}
    }

/* ----------------------------------------------------------------------- *\
|  error () - Report file errors
\* ----------------------------------------------------------------------- */
    void
error (
    int  n)		// The error number

    {
    printf("File parse error %d\n", n);
    }

/* ----------------------------------------------------------------------- *\
|  ProcessSymbols () - Process the symbols
\* ----------------------------------------------------------------------- */
    void
ProcessSymbols (
    char  *pSearch,		// Pointer to the search string
    char  *pReplace)		// Pointer to the replace string

    {
    int  i;

    SearchLength  = strlen(pSearch);
    Search[0]     = (BYTE)(SearchLength);
    strcpy(&Search[1], pSearch);

    ReplaceLength = strlen(pReplace);
    Replace[0]    = (BYTE)(ReplaceLength);
    strcpy(&Replace[1], pReplace);

    Delta = ReplaceLength - SearchLength;

    Correction = 0;
    for (i = 0; (i <= ReplaceLength); ++i)
	Correction += Replace[i];
    for (i = 0; (i <= SearchLength);  ++i)
	Correction -= Search[i];
    }

/* ----------------------------------------------------------------------- *\
|  AdjustBuffer () - Adjust the data buffer
\* ----------------------------------------------------------------------- */
    void
AdjustBuffer (
    PBYTE  p1,		// Pointer to the first byte of the field
    PBYTE  p2)		// Pointer to the last byte of the record

    {
    if (Delta < 0)
	memcpy(p1, (p1 - Delta), (p2 - p1 + Delta));

    else if (Delta > 0)
	{
	do  {
	    --p2;
	    *(p2 + Delta) = *p2;
	    } while (p2 > p1);
	}
    }

/* ----------------------------------------------------------------------- *\
|  ProcessFile () - Process a file
\* ----------------------------------------------------------------------- */
    void
ProcessFile (void)

    {
    int    length;		// read/write returned length
    int    datalength;		// Length of a data field
    int    fieldlength;		// Length of a field
    PBYTE  p1;			// Ptr to current subrecord
    PBYTE  p2;			// Ptr to record end

static BYTE    RecType;		// The record type
static UINT16  RecLength;	// The record length
static BYTE    Data [32767];	// The data buffer
static BYTE    RecChecksum;	// The record checksum

    for (;;)
	{
	length = read(infile, &RecType, sizeof(RecType));
	if (length != sizeof(RecType))
	    break;

	++Records;
	length = read(infile, (char *)(&RecLength), sizeof(RecLength));
	if (length != sizeof(RecLength))
	    {
	    error(1);
	    break;
	    }
	datalength = RecLength - 1;	// Exclude the checksum

//printf("type     %02X\n", RecType);
//printf("length   %02X\n", RecLength);

	if (datalength > 0)
	    {
	    length = read(infile, &Data[0], datalength);
	    if (length != (int)(datalength))
		{
		error(2);
		break;
		}
	    }

	length = read(infile, &RecChecksum, sizeof(RecChecksum));
	if (length != sizeof(RecChecksum))
	    {
	    error(3);
	    break;
	    }

	if (RecType == 0x96)		// LNAMES record
	    {
	    p1 = &Data[0];
	    p2 = &Data[datalength];
	    while (p1 < p2)
		{
		fieldlength = *p1;
		if (match(p1))
		    {
		    ++Replacements;
		    AdjustBuffer(p1, p2);
		    memcpy(p1, &Replace[0], (ReplaceLength + 1));
		    RecChecksum += Correction;
		    RecLength   += Delta;
		    fieldlength += Delta;
		    datalength  += Delta;
		    }
		p1 += (fieldlength + 1);
		}
	    }

	write(outfile, &RecType,     sizeof(RecType));
	write(outfile, &RecLength,   sizeof(RecLength));
	if (datalength > 0)
	    write(outfile, &Data[0], datalength);
	write(outfile, &RecChecksum, sizeof(RecChecksum));
	}
    }

/* --------------------------------- EOF --------------------------------- */
