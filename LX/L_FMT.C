/* ----------------------------------------------------------------------- *\
|
|							   L Formatter Module
|
|								 Brian W Johnson
|					Copyright (c) 1985, 1990, All rights reserved
|								   19-Jun-90
|								   12-Jul-92
|								   22-Feb-93
|								   17-Aug-97
|								   22-Mar-98  QuoteFlag added
|								   25-Sep-01  Support for huge directories
|								   27-Sep-07  Support for 64 bit file sizes
|
\* ----------------------------------------------------------------------- */

#include  "stdio.h"
#include  "conio.h"
#include  "ctype.h"
#include  "stdlib.h"
#include  "string.h"
#include  "time.h"
#include  "fwild.h"
#include  "ptypes.h"

/* ----------------------------------------------------------------------- *\
|  Private definitions
\* ----------------------------------------------------------------------- */

#define	 LINEMAX	(1025)				/* The size of the line buffer */
#define	 LFSIZE		   (7)				/* The length field default size */
#define	 FIELDSIZE	  (16)				/* The wide mode field size */
#define	 FIELDGAP	   (2)				/* The inter-field gap */
#define	 FMTMAX		   (4)				/* The format buffer size */

#define	 SHOW_PATH	   (1)				/* Show the pathname field */
#define	 SHOW_NAME	   (2)				/* Show the name (only) field */
#define	 SHOW_SIZE	   (3)				/* Show the size field */
#define	 SHOW_TIME	   (4)				/* Show the date field */
#define	 SHOW_ATTR	   (5)				/* Show the attribute field */

/* ----------------------------------------------------------------------- *\
|  External variables
\* ----------------------------------------------------------------------- */

extern	void   *hp;						/* Pointer to the wild name data block */

extern	int		h_flag;					/* List in hexadecimal flag */
extern	int		l_flag;					/* Lower case flag */
extern	int		m_flag;					/* List in "more" mode flag */
extern	int		p_flag;					/* List parameters flag */
extern	int		u_flag;					/* Upper case flag */
extern	int		w_flag;					/* Wide listing mode flag */
extern	int		QuoteFlag;				/* Quote filenames with spaces */

extern	int		namesize;				/* The name field length */
extern	int		colsize;				/* The number of display columns */
extern	int		rowsize;				/* The number of display rows */

/* ----------------------------------------------------------------------- *\
|  Private variables
\* ----------------------------------------------------------------------- */

static	int		nameonly  = FALSE;		/* Strip off the path */
static	int		available = 0;			/* The available field space */
static	int		delta	  = 0;			/* Number of inter-field spaces needed */
static	int		spaces	  = 0;			/* Number of leading spaces required */
static	int		row		  = 0;			/* The current row counter */
static	int		rowlimit  = 0;			/* The maximum row counter */
static	int		column	  = 0;			/* The current column counter */
static	int		collimit  = 0;			/* The maximum column counter */
static	int		lmin_size = LFSIZE;		/* The minimum length of the size field */
static	int		lineopen  = FALSE;		/* The end of line flag */

static	char   *pline	  = NULL;		/* Pointer into the line buffer */
static	char	line [LINEMAX];			/* The line buffer */

static	char	lineformat [FMTMAX + 1] =  /* The P-on line format array */
					{ SHOW_PATH, SHOW_SIZE, SHOW_TIME, SHOW_ATTR, 0 };

static	char	nameformat [2] =		   /* The P-off line format array */
					{ SHOW_PATH, 0 };

/* ----------------------------------------------------------------------- *\
|  Private and external function prototypes
\* ----------------------------------------------------------------------- */

static	void	fdpr_normal		(char *fnp, int attr, UINT64 size);
static	void	fdpr_wide		(char *fnp, int attr);

static	void	put_name		(char *fnp);
static	void	put_size		(int attr, UINT64 size);
static	void	put_time		(void);
static	void	put_attr		(int attr);
static	void	interleave		(char *p2);

extern	void	delay			(void);

/* ----------------------------------------------------------------------- *\
|  Initialize to print the directory
\* ----------------------------------------------------------------------- */
	void
fdpr_init ()			/* Initialize for printing file/directory lines */

	{
	spaces	 = 0;
	row		 = 0;
	column	 = 0;
	lineopen = FALSE;
	collimit = (colsize * FIELDSIZE);
	rowlimit = (rowsize - 1);

	if (lineformat[0] == SHOW_SIZE)		/* Set the length field size */
		{
		if (h_flag)
			lmin_size = 8;
		else
			lmin_size = 11;
		}
	}

/* ----------------------------------------------------------------------- *\
|  Print one file/directory name
\* ----------------------------------------------------------------------- */
	void
fdpr (
	char   *fnp,		/* Pointer to the filename */
	int		attr,		/* File attributes */
	UINT64	size)		/* Size of the file */

	{
	if (l_flag)
		strlwr(fnp);			/* Convert path/filename to lower case */
	else if (u_flag)
		strupr(fnp);			/* Convert path/filename to upper case */

	if (m_flag	&&	(row >= rowlimit))
		{
		fputs("-More-", stdout);
		fflush(stdout);
		getch();
		printf("\r      \r");
		fflush(stdout);
		row = 0;
		}

	delay();
	if (w_flag)
		fdpr_wide(fnp, attr);
	else
		fdpr_normal(fnp, attr, size);
	}

/* ----------------------------------------------------------------------- *\
|  Complete the directory listing (used mostly for wide mode)
\* ----------------------------------------------------------------------- */
	void
fdpr_complete ()		/* Complete printing file/directory lines */

	{
	if (lineopen)
		putchar('\n');
	fflush(stdout);
	spaces	 = 0;
	++row;
	column	 = 0;
	lineopen = FALSE;
	}

/* ----------------------------------------------------------------------- *\
|  Set the parameter-on listing format
\* ----------------------------------------------------------------------- */
	int
fdpr_format (
	char  *fmtstr)

	{
	char   ch;			/* The current format character */
	char  *pfmt;		/* Pointer to the format buffer */


	nameonly = FALSE;	 
	pfmt = lineformat;
	while (ch = *(fmtstr++))
		{
		if (pfmt == (lineformat + FMTMAX))
			return (-1);
		switch (tolower(ch))
			{
			case 'n':	*pfmt = SHOW_NAME;
						 nameonly = TRUE;
						 namesize = 12;							break;

			case 'p':	*pfmt = SHOW_PATH;						break;
			case 's':	*pfmt = SHOW_SIZE;						break;
			case 't':	*pfmt = SHOW_TIME;						break;
			case 'a':	*pfmt = SHOW_ATTR;						break;
			case 'w':	w_flag = !w_flag;						continue;
			default:	return (-1);
			}
		*(++pfmt) = 0;
		}
	return (0);
	}

/* ----------------------------------------------------------------------- *\
|  List one entry in normal (one line per file) mode
\* ----------------------------------------------------------------------- */
	static void
fdpr_normal (			/* Print a file/directory name in normal mode */
	char   *fnp,		/* Pointer to the filename */
	int		attr,		/* File attributes */
	UINT64	size)		/* Size of the file */

	{
	char   fmt;			/* The current line format item */
	char  *pfmt;		/* Pointer into the line format array */
	char  *ptrim;		/* Pointer into the line while trimming */


	delta	  = 0;
	available = 0;
	pline	  = line;
	pfmt	  = (p_flag) ? (lineformat) : (nameformat);
	while (fmt = *pfmt)					/* Write the line */
		{
		while (delta)
			{
			*(pline++) = ' ';
			--delta;
			}
		switch (fmt)
			{
			case SHOW_PATH:		put_name(fnp);			break;
			case SHOW_NAME:		put_name(fntail(fnp));	break;
			case SHOW_SIZE:		put_size(attr, size);	break;
			case SHOW_TIME:		put_time();				break;
			case SHOW_ATTR:		put_attr(attr);			break;
			}
		delta += FIELDGAP;
		++pfmt;
		}

	while (pline != line)				/* Trim the line */
		{
		if ( ! isspace(*(ptrim = pline - 1)))
			break;
		pline = ptrim;
		}

	*(pline++) = '\n';					/* Terminate and output the line */
	*pline	   = '\0';
	fputs(line, stdout);
	fflush(stdout);
	++row;								/* Update the row count */
	}

/* ----------------------------------------------------------------------- *\
|  List one filename entry in normal (one line per file) mode
\* ----------------------------------------------------------------------- */
	static void
put_name (				/* Print a file/directory name in normal mode */
	char  *fnp)			/* Pointer to the filename */

	{
	int	 length;		/* Length of the line */
	int	 Quotes = 0;	/* Allowance required for quoted filename */


	length	  = (int)(strlen(fnp));
	if (QuoteFlag)				// Account for quoting, if necessary
		{
		char  *p = fnp;

		for (p = fnp; (*p != '\0'); ++p)
			{
			if (isspace(*p))
				{
				Quotes = 2;		// Quotes are needed
				break;
				}
			}
		}

	if (Quotes)
		*(pline++) = '\"';
	strcpy(pline, fnp);
	pline += length;
	if (Quotes)
		*(pline++) = '\"';
	available = (namesize - (length + Quotes));
	delta	  = max(0, available);
	}

/* ----------------------------------------------------------------------- *\
|  List one size entry in normal (one line per file) mode
\* ----------------------------------------------------------------------- */
	static void
put_size (				/* Print a file/directory size in normal mode */
	int		attr,		/* File attributes */
	UINT64	size)		/* Size of the file */

	{
	char  *p;
	char   buffer [33];


	if (attr & ATT_SUBD)
		p = "<DIR> ";

	else if (h_flag)
		sprintf(p = buffer, "%I64X", size);

	else /* Decimal mode */
		sprintf(p = buffer, "%I64u", size);

	interleave(p);
	strcpy(pline, p);
	pline += strlen(pline);
	available = 0;
	}

/* ----------------------------------------------------------------------- *\
|  List one time entry in normal (one line per file) mode
\* ----------------------------------------------------------------------- */
	static void
put_time (void)			/* Print a file/directory date/time in normal mode */

	{
	sprintf(pline, "%9s  %6s", fwdate(hp), fwtime(hp));
	pline += 17;
	available = 0;
	}

/* ----------------------------------------------------------------------- *\
|  List one attribute entry in normal (one line per file) mode
\* ----------------------------------------------------------------------- */
	static void
put_attr (
	int	 attr)					/* The file attributes */

	{
	*(pline++) = (attr & ATT_ARCH)	 ? ('a') : ('-');
	*(pline++) = (attr & ATT_RONLY)	 ? ('r') : ('-');
	*(pline++) = (attr & ATT_HIDDEN) ? ('h') : ('-');
	*(pline++) = (attr & ATT_SYSTEM) ? ('s') : ('-');
	available = 0;
	}

/* ----------------------------------------------------------------------- *\
|  Determine the interleave between this field and the previous field
|  (Only useful for placing a right-justified field)
\* ----------------------------------------------------------------------- */
	static void
interleave (
	char  *pf2)			/* Pointer to the right-justified field */

	{
	int		pad;		/* Number of pad spaces needed */


	pad = lmin_size - (int)(strlen(pf2));	/* Determine the length discrepancy */

	/* If the field is shorter than the minimum (pad > 0): */
	/* If the previous field overran (available < 0), */
	/* then reduce the pad by the magnitude of the overrun */

	if (pad > 0)
		{
		if (available < 0)
			pad += available;
		while (pad-- > 0)
			*(pline++) = ' ';
		}

	/* If the field is longer than the minimum (pad < 0): */
	/* Look for reusable space in the preceding field. */
	/* If there is reusable space (available > 0), and -pad is the */
	/* amount of space reuse needed.  Find the lesser of these, then */
	/* adjust the line pointer by that amount.	No pad is required. */

	else if ((pad < 0)	&&	(available > 0))
		{
		if (-pad <= available)
			pline += pad;
		else
			pline -= available;
		}
	}

/* ----------------------------------------------------------------------- *\
|  List one entry in wide mode
\* ----------------------------------------------------------------------- */
	static void
fdpr_wide (				/* Print a file/directory name in wide mode */
	char  *fnp,			/* Pointer to the filename */
	int	   attr)		/* File attributes */

	{
	int		length;		/* Length of the current filename */
	int		extra;		/* Allowance for a subdirectory */
	int		Quotes = 0;	/* Allowance required for quoted filename */


	if (nameonly)				/* Strip the path, if requested */
		fnp = fntail(fnp);

	if (QuoteFlag)				// Account for quoting, if necessary
		{
		char  *p = fnp;

		for (p = fnp; (*p != '\0'); ++p)
			{
			if (isspace(*p))
				{
				Quotes = 2;		// Quotes are needed
				break;
				}
			}
		}

	extra	= (attr & ATT_SUBD) ? (2) : (0);
	length	= (int)(strlen(fnp)) + extra + Quotes;
	column += (spaces + length);

	if (column >= collimit)		/* Prevent oversize lines */
		{
		fdpr_complete();
		column = length;
		}

	while (spaces-- > 0)		/* Skip to the next field position */
		putchar(' ');

	if (extra)
		{
		putchar('(');
		if (Quotes)
			fputc('\"', stdout);
		fputs(fnp, stdout);		/* directoryname output */
		if (Quotes)
			fputc('\"', stdout);
		putchar(')');
		}
	else
		{
		if (Quotes)
			fputc('\"', stdout);
		fputs(fnp, stdout);		/* filename output */
		if (Quotes)
			fputc('\"', stdout);
		}

	lineopen = TRUE;

	spaces = FIELDSIZE - length;
	while (spaces <= 0)
		spaces += FIELDSIZE;
	}

/* ----------------------------------------------------------------------- */
