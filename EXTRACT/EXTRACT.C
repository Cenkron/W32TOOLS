/* ----------------------------------------------------------------------- *\
|
|				    EXTRACT
|
|		    Copyright (C) 1993; All rights reserved
|				Brian W. Johnson
|				    3-Mar-93
|				   30-Sep-07 for 64 bit file sizes
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <stdlib.h>
#include  <fcntl.h>
#include  <ctype.h>
#include  <sys\types.h>
#include  <sys\stat.h>
#include  <io.h>
#include  <errno.h>
#include  <limits.h>

#include  "fWild.h"

#ifndef TRUE
#define FALSE	  0
#define TRUE	  1
#endif

char	copyright [] =
"Copyright (c) 1993 by J & M Software, Dallas TX - All Rights Reserved";

#define  BUFFERSIZE  16384	/* The size of a file block */

#define READMODE    (O_RDONLY | O_BINARY)
#define WRITEMODE   (O_RDWR | O_CREAT | O_TRUNC | O_BINARY)
#define WRITEPERM   (S_IREAD | S_IWRITE)

#define S_MODE  (1)		/* Offset parameter bit flags */
#define D_MODE  (2)
#define E_MODE  (4)

typedef unsigned long  UINT32;

long	s_value;		/* Start specification */
long	d_value;		/* Length specification */
long	e_value;		/* End specification */

UINT64  in_size;		/* The size of the input file */
UINT64  out_size;		/* The size of the output file */
UINT64  start_offset;		/* The starting file offset */
UINT64  end_offset;		/* The ending file offset */
UINT64  current_offset;		/* The current file offset (fale lseek64) */

int	o_flags  = 0;		/* Offset specified bit flags */
int	v_flag   = FALSE;	/* Verbose flag */

char	swch     = '-';		/* The switch character */

static	void	process		(char *, char *);
static	void	parameters	(void);

/* ----------------------------------------------------------------------- */

char   *usagedoc [] =
{
"Usage: extract [%c?v] [%csNNN] [%cdNNN] [%ceNNN] input_file [output_file]",
"       extract %c?  for help",
"",
"extract copys a portion of the binary input file to the binary output",
"file, discarding bytes from the front, back, or both of the input file,",
"depending on the parameters.  The default output file is \"extract.out\".",
"Up to two of the three following offset specifications can be requested:",
"",
"    %cs NNN  starts output at file offset NNN",
"            0NNN is interpreted as octal",
"            0xNNN is interpreted as hexadecimal",
"            Negative NNN is offset from the end of the file",
"            (default is the beginning of the file)",
"    %cd NNN  sets the copy distance to be NNN",
"            0NNN is interpreted as octal",
"            0xNNN is interpreted as hexadecimal",
"            Negative NNN is length from the starting or ending point",
"            (default is the end of the file)",
"    %ce NNN  ends output at file offset NNN",
"            0NNN is interpreted as octal",
"            0xNNN is interpreted as hexadecimal",
"            Negative NNN is offset from the end of the file",
"            (default is the end of the file)",
"",
"    %cv      lists verbose information",
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
	char  *fnpIn;		/* Input file name pointer */
	char  *fnpOut;		/* Output file name pointer */

static	char   *optstring = "?e:E:d:D:s:S:v";


	swch   = egetswch();
	optenv = getenv("EXTRACT");

	while ((option = getopt(argc, argv, optstring)) != EOF)
		{
		switch (tolower(option))
			{
			case 'e':
				if (optvalue(optarg, &e_value, LONG_MIN, LONG_MAX))
					{
					printf("End parm error - %s\n", optvalerror());
					usage();
					}
				o_flags |= E_MODE;
				break;

			case 'd':
				if (optvalue(optarg, &d_value, LONG_MIN, LONG_MAX))
					{
					printf("Distance parm error - %s\n", optvalerror());
					usage();
					}
				o_flags |= D_MODE;
				break;

			case 's':
				if (optvalue(optarg, &s_value, LONG_MIN, LONG_MAX))
					{
					printf("Start parm error - %s\n", optvalerror());
					usage();
					}
				o_flags |= S_MODE;
				break;

			case 'v':
				v_flag = ! v_flag;
				break;

			case '?':
				help();

			default:
				usage();
			}
		}

	nargs = argc - optind;
	if ((nargs <= 0)  ||  (nargs > 2))
		usage();

	fnpIn = argv[optind++];
	if (fnchkdir(fnpIn))
		{
		sprintf(" \"%s\" is a directory\n", fnpIn);
		exit(1);
		}		

	if (! fnchkfil(fnpIn))
		{
		sprintf(" \"%s\" not found\n", fnpIn);
		exit(1);
		}		


	if (nargs == 2)
		fnpOut = argv[optind];
	else
		fnpOut = "extract.out";

	process(fnpIn, fnpOut);

	exit(0);
	}

/* ----------------------------------------------------------------------- */
	static void
parameters (void)		/* Determine the copy parameters */

	{
	switch (o_flags)
		{
		case (0):
			start_offset = 0L;
			end_offset   = in_size;
			break;

		case (S_MODE):
			if (s_value >= 0L)
				start_offset = s_value;
			else
				start_offset = in_size + s_value;
			end_offset = in_size;
			break;

		case (S_MODE | D_MODE):
			if (s_value >= 0L)
				start_offset = s_value;
			else
				start_offset = in_size + s_value;

			if (d_value >= 0L)
				end_offset = start_offset + d_value;
			else
				{
				end_offset   = start_offset;
				start_offset = end_offset + d_value;
				}
			break;

		case (S_MODE | E_MODE):
			if (s_value >= 0L)
				start_offset = s_value;
			else
				start_offset = in_size + s_value;

			if (e_value >= 0L)
				end_offset = e_value;
			else
				end_offset = in_size + e_value;
			break;

		case (D_MODE):
			start_offset = 0L;
			if (d_value >= 0L)
				end_offset = d_value;
			else
				end_offset = in_size + d_value;
			break;

		case (D_MODE | E_MODE):
			if (e_value >= 0L)
				end_offset = e_value;
			else
				end_offset = in_size + e_value;

			if (d_value >= 0L)
				start_offset = end_offset - d_value;
			else
				{
				start_offset = end_offset;
				end_offset   = start_offset - d_value;
				}
			break;

		case (E_MODE):
			start_offset = 0L;
			if (e_value >= 0L)
				end_offset = e_value;
			else
				end_offset = in_size + e_value;
			break;

		default:
			printf("Only two offset parameters allowed\n");
			usage();
		}

	if ((start_offset < 0L)  ||  (start_offset > in_size))
		{
		printf("Starting offset is invalid: %I64u (0x%04I64X); minimum 0, maximum %I64u (0x%04I64X)\n",
			start_offset, start_offset, in_size, in_size);
		usage();
		}

	if ((end_offset < 0L)  ||  (end_offset > in_size))
		{
		printf("Ending offset is invalid: %I64u (0x%04I64X); minimum 0, maximum %I64u (0x%04I64X)\n",
			end_offset, end_offset, in_size, in_size);
		usage();
		}

	if (end_offset < start_offset)
		{
		printf("The resulting file size would be negative: %I64d (0x%04I64X)\n",
			out_size, out_size);
		usage();
		}
	}

/* ----------------------------------------------------------------------- */
	static void
process (			/* Process the input / output file */
	char  *fnpIn,		/* Input file name */ 
	char  *fnpOut)		/* Output file name */ 

	{
	int     fdin;		/* The input file handle */
	int     fdout;		/* The output file handle */
	UINT64  remaining;		/* The remaining size of the output file */
	int     request;		/* The requested read/write size */
static char  buffer [BUFFERSIZE];


	if ((fdin = open(fnpIn, READMODE)) < 0)
		{
		printf("Unable to open the input file \"%s\"\n", fnpIn);
		usage();
		}

	if (fgetsize(fnpIn, &in_size) != 0)
		{
		printf("Cannot size the input file: \"%s\"\n", fnpIn);
		usage();
		}

	parameters();
	out_size = end_offset - start_offset;

	if ((fdout = open(fnpOut, WRITEMODE, WRITEPERM)) < 0)
		{
		printf("Unable to open the output file \"%s\"\n", fnpOut);
		usage();
		}

	if (v_flag)
		{
		printf("Input  file size: %8I64u (0x%02I64X); \"%s\"\n",
			in_size, in_size, fnpIn);
		printf("Output file size: %8I64u (0x%02I64X); \"%s\"\n",
			out_size, out_size, fnpOut);
		printf("Starting offset:  %8I64u (0x%02I64X)\n",
			start_offset, start_offset);
		printf("Ending   offset:  %8I64u (0x%02I64X)\n",
			end_offset, end_offset);
		fflush(stdout);
		}

	current_offset = 0;
	while (current_offset < start_offset)
		{
		long  dist = (long)(min((start_offset - current_offset), 0x7fffe00));

		lseek(fdin, dist, 1);
		current_offset += (UINT64)(dist);
		}

	for (remaining = out_size; (remaining > 0L); remaining -= (UINT32)(request))
		{
		request = (int)(min(remaining, BUFFERSIZE));
		if (read(fdin, (char *)(buffer), request) != request)
			{
			printf("Error reading the input file (%d)\n", errno);
			usage();
			}

		if (write(fdout, (char *)(buffer), request) != request)
			{
			printf("Error writing the output file (%d)\n", errno);
			usage();
			}
		}

	close(fdin);
	close(fdout);
	}

/* ----------------------------------------------------------------------- */
