/* ----------------------------------------------------------------------- *\
|
|				     Crypt
|
|		    Copyright (c) 2001, all rights reserved
|				Brian W Johnson
|				   11-Oct-01
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <stdlib.h>
#include  <ctype.h>
#include  <io.h>
#include  <fcntl.h>
#include  <string.h>
#include  <ptypes.h>

#include  <fwild.h>

#ifndef TRUE
#define FALSE	  0
#define TRUE	  1
#endif

typedef  UINT16  *PUINT16;

#define  RECSIZE  (512)		// Size of the record buffer


char	swch = '-';		// The switch character

extern	int	optind;		// argv[] index from getopt()
extern	char   *optarg;		// argument pointer from getopt()


static  int	e_flag = FALSE;	// Encrypt flag
static  int	d_flag = FALSE;	// Decrypt flag
static  int	l_flag = FALSE;	// List file names flag

static  char    Password [1024];// The password buffer
static  UINT16  crc;            // The current CRC (initialized to zero)
static  int     attr;           // The current file attributes


static	int	process     (int fd, char *fnp);
static  void    Crypt       (PUINT16 p, UINT16 Length);
static  void    ProcessName (char *fnp);

/* ----------------------------------------------------------------------- */
char	copyright [] =
"Copyright (c) 2001 by Brian W Johnson, Lucas TX - All Rights Reserved";

char  *usagedoc [] =
	{
	"Usage:  crypt  [%c?del]  [file_list]",
	"",
	"crypt encrypts/decrypts files",
	"",
	"    %cl  lists file names as they are processed",
	"    %ce  encrypts the designated files",
	"    %cd  decrypts the designated files",
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
	int    smode                // File search mode attributes
			= (FW_FILE | FW_HIDDEN);
	int    option;		// Option character
	char  *ap;			// Argument pointer
	void  *hp  = NULL;		// Pointer to wild file data block
    char  *fnp = NULL;		// Input file name pointer
    int    fd  = 0;		// Input file descriptor


//  setbuf(stdout, fmalloc(BUFSIZ));
//  swch = egetswch();

	while ((option = getopt(argc, argv, "?dDeElL")) != EOF)
		{
		switch (tolower(option))
			{
			case 'd':
				d_flag = !d_flag;
				break;

			case 'e':
				e_flag = !e_flag;
				break;

			case 'l':
				l_flag = !l_flag;
				break;

			case '?':
				help();

			default:
				usage();
			}
		}


	if (!d_flag  &&  !e_flag)
		{
		fprintf(stderr, "Requires -d or -e\n");
		usage();
		}

	else if (optind >= argc)
		{
		fprintf(stderr, "Requires a file_list\n");
		usage();
		}
	
	else
		{
		if (d_flag)
			{
			int count = 3;

			for (;;)
				{
				printf("Password: ");
				gets(Password);
				if (strlen(Password) == 0)
					exit(0);
				else if (strcmp(Password, "bwj1012") == 0)
					break;
				else if (--count == 0)
					usage();
				fprintf(stderr, "Invalid password\n");
				}
			}

		while (optind < argc)
			{
			int  processed;
			long fdt;

			ap = argv[optind++];
			hp = finit(ap, smode);		// Process the input list
			if ((fnp = fwild(hp)) == NULL)
				{
				hp = NULL;
				cantopen(ap);
				}
			else
				{
				do  {				// Process one filespec
					fdt       = fwgetfdt(hp);
					attr      = fwtype(hp);
					processed = FALSE;
					if (attr & ATT_RONLY)
						{
						fsetattr(fnp, (attr & ~ATT_RONLY));
						processed = TRUE;
						}
					if ((fd = open(fnp, (O_BINARY | O_RDWR))) >= 0)
						{
						processed |= process(fd, fnp);
						close(fd);
						}
					else
						cantopen(fnp);

					if (processed)
						{
						fsetfdt (fnp, fdt);
						fsetattr(fnp, attr);
						}
					} while ((fnp = fwild(hp)));
				hp = NULL;
				}
			}
		}
	}

/* ----------------------------------------------------------------------- */
	static int                  // Returns TRUE if the file is processed
process (
	int     fd,			// Input file descriptor
	char   *fnp)		// Input file name

	{
	int     Processed = FALSE;  // TRUE if the file is processed
	int     Len;                // Length of the current buffer
	long    Pos = 0L;           // Current file offset
	char    Record [RECSIZE];   // The record buffer


	ProcessName(fnp);
	if (e_flag)
		{
		if ((attr & ATT_HIDDEN) == 0)
			{
			if (l_flag)
				printf("Processing file: %s\n", fnp);

			while ((Len = read(fd, Record, RECSIZE)) > 0)
				{
				Crypt((PUINT16)(Record), (UINT16)(((Len + 1) / 2)));

				lseek(fd, Pos, SEEK_SET);
				write(fd, Record, Len);
				lseek(fd, (Pos += Len), SEEK_SET);
				}

			attr |= ATT_HIDDEN;
			Processed = TRUE;
			}

		else /* (hidden, therefore already encrypted) */
			printf("File: %s not processed (hidden)\n", fnp);
		}

	else /* (d_flag) */
		{
		if ((attr & ATT_HIDDEN) != 0)
			{
			if (l_flag)
				printf("Processing file: %s\n", fnp);

			while ((Len = read(fd, Record, RECSIZE)) > 0)
				{
				Crypt((PUINT16)(Record), (UINT16)(((Len + 1) / 2)));

				lseek(fd, Pos, SEEK_SET);
				write(fd, Record, Len);
				lseek(fd, (Pos += Len), SEEK_SET);
				}

			attr &= ~ATT_HIDDEN;
			Processed = TRUE;
			}

		else /* (not hidden, therefore not encrypted) */
			printf("File: %s not processed (plain)\n", fnp);
		}

	return (Processed);
	}

/* ----------------------------------------------------------------------- */
	static void
Crypt (                     // XOR the UINT16 CRC
	PUINT16  p,				// Pointer to the UINT16 array
	UINT16   Len)			// Length (U16 words) of the array

	{
static	UINT16	crc_table [16] =	// CRC-16 lookup table
	{
	0x0000, 0xCC01, 0xD801, 0x1400, 0xF001, 0x3C00, 0x2800, 0xE401,
    0xA001, 0x6C00, 0x7800, 0xB401, 0x5000, 0x9C01, 0x8801, 0x4400
    };


	while (Len != 0)
		{
		crc ^= Len & 0x00FF;
		crc  = (crc >> 4) ^ crc_table[crc & 0x000F];
		crc  = (crc >> 4) ^ crc_table[crc & 0x000F];

		*(p++) ^= crc;
		--Len;
		}
	}

/* ----------------------------------------------------------------------- */
	static void
NameCrypt (				// Calculate the initial CRC
	char  *p,				// Pointer to the byte array
	int     Len)			// Length of the array
    
	{
static	UINT16	crc_table [16] =	// CRC-16 lookup table
	{
	0x0000, 0xCC01, 0xD801, 0x1400, 0xF001, 0x3C00, 0x2800, 0xE401,
	0xA001, 0x6C00, 0x7800, 0xB401, 0x5000, 0x9C01, 0x8801, 0x4400
	};


	while (Len-- > 0)
		{
		crc ^= (UINT16)(*(p)++) & 0x00FF;
		crc  = (crc >> 4) ^ crc_table[crc & 0x000F];
		crc  = (crc >> 4) ^ crc_table[crc & 0x000F];
		}
	}

/* ----------------------------------------------------------------------- */
	static void
ProcessName (                   // Process the filename
	char  *fnp)			// Pointer to the filename

	{
	char    Drive  [1024];
	char    Dir    [1024];
	char    Fname  [1024];
	char    Ext    [1024];


	_splitpath(fnp, Drive, Dir, Fname, Ext);

	crc = 0;
	NameCrypt(Fname, 1);
	NameCrypt(Fname, strlen(Fname));
	}

/* ----------------------------------------------------------------------- */
