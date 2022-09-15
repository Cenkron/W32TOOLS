/* ----------------------------------------------------------------------- *\
|
|				      DUMP
|
|		Copyright (C) 1985, 1990, 1993; All rights reserved
|				Brian W. Johnson
|				    4-Jun-90
|				   20-Feb-93
|				   26-Feb-93
|				   30-Sep-07 for 64 bit file sizes
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <conio.h>
#include  <stdlib.h>
#include  <math.h>
#include  <ctype.h>
#include  <fcntl.h>
#include  <io.h>
#include  <limits.h>

#include  "fwild.h"
#include  "ptypes.h"

#ifndef TRUE
#define FALSE	  0
#define TRUE	  1
#endif

char	copyright [] =
"Copyright (c) 1993 by J & M Software, Dallas TX - All Rights Reserved";

#define  LINLEN	  16     	/* The number of bytes per dump line */
#define  BLKSIZ	 512	        /* The size of a file block */

#define S_MODE  (1)		/* Offset parameter bit flags */
#define D_MODE  (2)
#define E_MODE  (4)

long	s_value;		/* Start specification */
long	d_value;		/* Length specification */
long	e_value;		/* End specification */

UINT64  in_size;		/* The size of the input file */
UINT64  out_size;		/* The size of the output file */
UINT64  start_offset;		/* The starting file offset */
UINT64  end_offset;		/* The ending file offset */
UINT64  current_offset;		/* The current file offset (fale lseek64) */

int	o_flags  = 0;		/* Offset specified bit flags */

int	b_flag = FALSE;		/* List each block flag */
int	l_flag = FALSE;		/* List file name flag */
int	m_flag = FALSE;		/* More mode flag */
int	y_flag = FALSE;		/* Byte Swap flag */

UINT32	offset;			/* Working file offset */

char	swch = '-';		/* The switch character */

static void dumpfile (int, char *);
void dumpblk (char *, int);

/* ----------------------------------------------------------------------- */

char  *usagedoc [] =
	{
	"Usage: dump [%c?bly] [%csNNN] [%cdNNN] [%ceNNN] input_file_list [>output_file]",
	"",
	"dump shows the contents of each file in the input_file_list",
	"in both hexadecimal and ASCII format",
	"",
	"    %cb      dumps the file in block mode",
	"    %cl      lists file names as they are dumped",
	"    %cy      byte swaps the file before dumping it",
	"    %cm      dumps in \"more\" mode",
	"",
	"Up to two of the three offset specifications can be requested.",
	"",
	"    %cs NNN  starts dumping at file offset NNN",
	"            (default is the beginning of the file)",
	"    %cd NNN  sets the dump range distance to be NNN",
	"            (default is the size of the file)",
	"    %ce NNN  ends dumping at file offset NNN",
	"            (default is the end of the file)",
	"            0NNN is interpreted as octal",
	"            0xNNN is interpreted as hexadecimal",
	"            Negative NNN is offset from the end of the file",
	"",
	copyright,
	NULL
	};

/* ----------------------------------------------------------------------- */
	void
main (
	int   argc,
	char *argv [])

	{
	int    smode = FW_FILE;	/* File search mode attributes */
	int    option;		/* Option character */
	int    fd;			/* Input file descriptor */
	char  *ap;  		/* Pattern pointer */
	void  *hp  = NULL;		/* Pointer to wild file data block */
	char  *fnp = NULL;		/* Input file name pointer */

static	char   *optstring = "?bBd:D:e:E:lLmMs:S:y";


	setbuf(stdout, fmalloc(BUFSIZ));
	optenv = getenv("DUMP");
	swch = egetswch();

	while ((option = getopt(argc, argv, optstring)) != EOF)
		{
		switch (tolower(option))
			{
			case 'b':
				++b_flag;
				l_flag = FALSE;
				break;

			case 'd':
				if (optvalue(optarg, &d_value, LONG_MIN, LONG_MAX))
					{
					printf("Distance parm error - %s\n", optvalerror());
					usage();
					}
				o_flags |= D_MODE;
				break;

			case 'e':
				if (optvalue(optarg, &e_value, LONG_MIN, LONG_MAX))
					{
					printf("End parm error - %s\n", optvalerror());
					usage();
					}
				o_flags |= E_MODE;
				break;

			case 'l':
				++l_flag;
				b_flag = FALSE;
				break;

			case 'm':
				++m_flag;
				break;

			case 's':
				if (optvalue(optarg, &s_value, LONG_MIN, LONG_MAX))
					{
					printf("Start parm error - %s\n", optvalerror());
					usage();
					}
				o_flags |= S_MODE;
				break;

			case 'y':
				++y_flag;
				break;

			case '?':
				help();

			default:
				usage();
			}
		}

	if (optind >= argc)
		usage();

	while (optind < argc)
		{
		ap = argv[optind++];
		hp = fwinit(ap, smode);         /* Process the input list */
		if ((fnp = fwild(hp)) == NULL)
			{
			hp = NULL;
			cantopen(ap);
			}
		else
			{
			do  {			/* Process one filespec */
				if (fd = open(fnp, (O_RDONLY | O_RAW)))
					{
					dumpfile(fd, fnp);
					close(fd);
					}
				else
					cantopen(fnp);
				} while ((fnp = fwild(hp)));
			hp = NULL;
			}
		}
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
		printf("The resulting file size would be negative: %I64u (0x%04I64X)\n",
			out_size, out_size);
		usage();
		}
	}

/* ----------------------------------------------------------------------- */
	static void
dumpfile (				/* Process one input file */
	int   fd,			/* Input file descriptor */
	char *fnp)			/* Input file name */ 

	{
	int     size;		/* The read size of the file */
	int     blk_num;		/* The block number */
	UINT16  block [BLKSIZ / 2];	/* The block buffer */


	if (l_flag)
		printf("\n%s\n", fnp);

	if (fgetsize(fnp, &in_size) != 0)
		{
		printf("Cannot size the input file: \"%s\"\n", fnp);
		usage();
		}

	parameters();
	out_size = end_offset - start_offset;


	if (b_flag)
		{
		start_offset &= (~(BLKSIZ - 1L));
		blk_num = (int)(start_offset / BLKSIZ);
		}
	else
		offset &= (~(LINLEN - 1L));


	offset = (UINT32)(start_offset);
	current_offset = 0;
	while (current_offset < start_offset)
		{
		long  dist = (long)(min((start_offset - current_offset), 0x7fffe00));

		lseek(fd, dist, 1);
		current_offset += (UINT64)(dist);
		}

	while ((size = read(fd, (char *) &block[0], BLKSIZ)) > 0)
		{
		if (offset >= end_offset)	/* Check if finished */
			break;
		if (b_flag)
			printf("\n%s    block %d    %d bytes\n", fnp, blk_num, size);
		if (y_flag)
			swab((char *)(&block[0]), (char *)(&block[0]), size);
		dumpblk((char *)&block[0], size);
		++blk_num;

		if (m_flag)
			{
			fputs("-More-", stdout);
			fflush(stdout);
			getch();
			printf("\r      \r");
			fflush(stdout);
			}
		}
	}

/* ----------------------------------------------------------------------- */
	void
dumpblk (				/* Dump n bytes of the file */
    char  *p,			/* Pointer to the byte buffer */
    int   n)			/* The number of bytes to dump */

	{
	int    i;			/* Byte counter */
	int    m;			/* Bytes in the current line */
	unsigned char  *q;		/* Pointer to the current byte */


	while (n > 0)
		{
		m = min(n, LINLEN);		/* Calculate the line length */
		n -= m;				/* Update the remaining length */

		if ((b_flag == FALSE)  &&  (offset >= end_offset))
			break;			/* Exit if finished */

		printf("%08lX ", offset);	/* Print the byte number */
		offset += LINLEN;		/* Update it */

		q = (unsigned char *)(p);	/* Dump in hexadecimal */
		for (i = 0; i < LINLEN; ++i)
			{
			if ((i & 0x0003) == 0)
				putchar(' ');
			if (i < m)
				printf("%02X ", *(q++));
			else
				printf("   ");
			}

		putchar(' ');			/* Dump in ASCII */
		for (i = 0; i < m; ++i)
			{
			if ((*p >= ' ')  &&  (*p <= '~'))
				putchar(*p);
			else
				putchar('.');
			++p;
			}
		putchar('\n');
		fflush(stdout);
		}
	}

/* ----------------------------------------------------------------------- */
