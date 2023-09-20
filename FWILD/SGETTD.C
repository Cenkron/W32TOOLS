/* ----------------------------------------------------------------------- *\
|
|	Time string parse library subsystem
|
| - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
|
|	time = sgettd (s);		Return the parsed UNIX time
|
|	    char  *s;			Pointer to the string to be parsed
|
|	    long  time;			The returned UNIX time
|					(or -1L for failure)
| - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
|
|	s = serrtd (void);		Return the error message text
|
|	    char  *s;			Pointer to the returned text string
|
| - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
|
|	Note:  The TIMEDATE library is not reentrant.
|	Note:  serrtd() must be called following the return from sgettd().
|
| - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
|
|	10-Jun-90	Copyright (c) 1990, Brian W. Johnson,
|			All rights reserved
|
|	25-Dec-92	Correction to am/pm manipulations
|	15-Feb-93	Correction to am/pm manipulation day rollover
|	30-Jul-93	Correction to DST handling
|	17-Aug-97	Modified for NT
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <limits.h>
#include  <ctype.h>
#include  <time.h>
#include  <ptypes.h>
#include  <fwild.h>

// #define  TESTMODE	0

#ifdef  TESTMODE
#if  TESTMODE
#define  DBG(s,v)	printf("%s: %04X\n", s, v)
#else
#define  DBG(s,v)
#endif
#else
#define  DBG(s,v)
#endif

/* ----------------------------------------------------------------------- *\
|  Internal error codes
\* ----------------------------------------------------------------------- */

#define  DTR_SUCCESS	(0)		/* Successful conversion */
#define  DTR_NOSTRING	(1)		/* No string provided */
#define  DTR_INVDELIM	(2)		/* Invalid phrase delimiter */
#define  DTR_TOOMANYF	(3)		/* Too many fields */
#define  DTR_NOFIELD	(4)		/* No fields */
#define  DTR_INVSYMB	(5)		/* Invalid symbol */
#define  DTR_OVERFLOW	(6)		/* Field value overflow */
#define  DTR_INVFLD		(7)		/* Invalid field value */
#define  DTR_NOPHRCLASS	(8)		/* No possible phrase class type */
#define  DTR_NOFLDITEM	(9)		/* No possible field item type */
#define  DTR_AMBIGUOUS	(10)	/* Ambiguous expression */
#define  DTR_SYNTAX		(11)	/* Unparsable syntax */
#define  DTR_SYSTIME	(12)	/* System time unreasonably set */
#define  DTR_TMODE		(13)	/* Time mode (am pm 24hr) conflict */
#define  DTR_NS			(14)	/* Unsupported functionality */

/* ----------------------------------------------------------------------- *\
|  Field attribute values (bitmapped)
\* ----------------------------------------------------------------------- */

/* Attributes describing the field possibilities: */

#define	A_SEC		0x0001		/* May be a    SECONDS field */
#define	A_MIN		0x0002		/* May be a    MINUTES field */
#define	A_HOUR		0x0004		/* May be an   HOURS   field */
#define	A_DAY		0x0008		/* May be a    DAY     field */
#define	A_MON		0x0010		/* May be a    MONTH   field */
#define	A_YEAR		0x0020		/* May be a    YEAR    field */

#define	CL_TIME		0x0040		/* May be a time class field */
#define	CL_DATE		0x0080		/* May be a date class field */

/* Attributes describing the additional phrase/field requirements: */

#define	S_AM		0x0100		/* Is an AM time phrase */
#define	S_PM		0x0200		/* Is a  PM time phrase */
#define	S_24HR		0x0400		/* Is a 24hr time phrase */

#define	INVALID		0x8000		/* The field is an invalid field */

/* Attributes describing the attribute tests: */

#define A_TIME		(A_SEC  | A_MIN   | A_HOUR)
#define A_DATE		(A_DAY  | A_MON   | A_YEAR)

#define S_TIME		(S_AM   | S_PM    | S_24HR)

#define	REQTIME  	(A_TIME | CL_TIME | S_TIME)
#define	REQDATE  	(A_DATE | CL_DATE)

/* ----------------------------------------------------------------------- *\
|  Date field pattern processing structures
|
|	The patterns in this table are searched in linear order.
|	The first match is accepted, even if there is another entry that
|	would match.  This algorithm silently resolves some ambiguities.
|
\* ----------------------------------------------------------------------- */

#define  MAXFLD		(3)		/* Maximum number of fields */

typedef						/* DATTBL parse table definition */
	struct
	{
	UINT16	m [MAXFLD];		/* Acceptable field attributes */
	}  DATTBL;


#define	DAT_BND	9

static	DATTBL	dat_table [DAT_BND] = {
	{	A_MON,		A_DAY,		A_YEAR	},
	{	A_DAY,		A_MON,		A_YEAR	},
	{	A_YEAR,		A_MON,		A_DAY	},
	{	A_MON,		A_DAY,		0		},
	{	A_DAY,		A_MON,		0		},
	{	A_MON,		A_YEAR,		0		},
	{	A_YEAR,		0,			0		},
	{	A_MON,		0,			0		},
	{	A_DAY,		0,			0		} };

/* ----------------------------------------------------------------------- *\
|  PHRASE and FIELD structure definitions
\* ----------------------------------------------------------------------- */

#define  A_SIZE		20		/* Alpha symbol buffer size */

typedef struct tm	TSTR, *PTSTR;

typedef						/* FIELD definition */
	struct
	{
	UINT16	 possattr;		/* Possible attributes (bits) of the field */
	UINT16	 reqattr;		/* Required attributes (bits) of the field */
	time_t 	 value;			/* Decoded value of the field */
	}  FIELD;

typedef						/* PHRASE definition */
	struct
	{
	UINT16	 possattr;		/* Possible attributes (bits) of the phrase */
	UINT16	 reqattr;		/* Required attributes (bits) of the phrase */
	DATTBL  *dp;			/* Pointer into the date mask table */
	int      fcount;		/* Number of parsed fields */
	FIELD	*fp;			/* Phrase field pointer */
	FIELD	 f [MAXFLD];	/* Phrase field descriptors */
	}  PHRASE;

typedef						/* PHRASE definition */
	struct
	{
	int		 pcount;		/* Number of phrases successfully parsed */
	int      tderror;		/* The error returned to the caller */
	time_t	 ct;			/* The current time */
	time_t	 mt;			/* The modified time */
	TSTR     ctstr;			/* The current time structure */
	TSTR     mtstr;			/* The modified time structure */
	PHRASE  *timephrase;	/* The PHRASE which describes the time */
	PHRASE  *datephrase;	/* The PHRASE which describes the date */
	PHRASE	 phr [2];		/* The two phrase structures */
	}  GETTD;

static	GETTD	x;			/* The parser data structure instance */

/* ----------------------------------------------------------------------- *\
|  Static function prototypes
\* ----------------------------------------------------------------------- */

static	int		str_parse   (char *s);
static	void	phr_init    (PHRASE *pp);
static	int		phr_parse   (PHRASE *pp, char *s, char **sret);
static	int		fld_parse   (PHRASE *pp, char *s, char **sret);
static	int		alpha_parse (PHRASE *pp, char *s, char **sret);
static	int		alpha_eval  (PHRASE *pp, char *s);
static	int		num_parse   (PHRASE *pp, char *s, char **sret);
static	int		zulu_eval   (PHRASE *pp, char *s, char **sret);
static	int		num_eval    (PHRASE *pp, UINT16 n);
static	int		fld_entry   (PHRASE *pp, UINT16 pattr, UINT16 rattr, time_t n);			/* The field value */

static	int		phr_reduce  (PHRASE *pp);
static	int		pair_reduce (void);
static	int		pair_check  (void);
static	int		datematch   (PHRASE *pp);
static	int		timematch   (PHRASE *pp);
static	time_t	finalparse  (void);
static	FIELD  *fld_find    (UINT16 attr);

#ifdef  TESTMODE
extern	void	prn_tstr    (TSTR *t);
#endif

/* ----------------------------------------------------------------------- *\
|  sgettd () - Parse a string to a UNIX time
\* ----------------------------------------------------------------------- */
	time_t				/* Return the UNIX timedate, or -1L if err */
sgettd (
	char *s)			/* Pointer to the time/date string */

	{
	time_t timedate;	/* The returned UNIX time/date */
	PTSTR  pTS;			/* Pointer to the TSTR */


	x.ct = time(NULL);	/* Initialize data structures */
	if ((pTS = localtime(&x.ct)) != NULL)
		{
		memcpy(&x.ctstr, pTS, sizeof(TSTR));
		x.pcount     = 0;
		x.timephrase = NULL;
		x.datephrase = NULL;
		phr_init(&x.phr[0]);
		phr_init(&x.phr[1]);
		if ((x.tderror = str_parse(s)) == DTR_SUCCESS)
			timedate = finalparse();
		else
			timedate = -1L;
		}
	else
		{
		x.tderror = DTR_SYSTIME;
		timedate  = -1L;
		}

	return  (timedate);				/* Return the specified timedate */
	}

/* ----------------------------------------------------------------------- *\
|  str_parse () - Parse the string to a pair of PHRASE structures
\* ----------------------------------------------------------------------- */
	static int			/* Return the parse validity result */
str_parse (
	char *s)			/* Pointer to the string */

	{
	int      result;	/* The returned result */
	PHRASE  *pp;		/* Pointer to a PHRASE */


	if (s == NULL)
		{
		result = DTR_NOSTRING;	/* No string provided */
		goto exit;
		}

	strlwr(s);
	if ((result = phr_parse(&x.phr[0], s, &s)) != DTR_SUCCESS)
		goto exit;		/* Invalid first phrase */

	s = stpblk(s);		/* Skip intermediate white space */
	if (*s)
		{
		if (((result = phr_parse(&x.phr[1], ++s, &s)) != DTR_SUCCESS)
		||  ((result = pair_reduce()) != DTR_SUCCESS))
			goto exit;
		}

	if ((result = pair_check()) != DTR_SUCCESS)
		goto exit;

	for (pp = &x.phr[0]; pp != &x.phr[x.pcount]; ++pp)
		{
		if (pp->possattr & CL_TIME)
			{
			x.timephrase = pp;
			if ((result = timematch(pp)) != DTR_SUCCESS)
				goto exit;
			}
		if (pp->possattr & CL_DATE)
			{
			x.datephrase = pp;
			if ((result = datematch(pp)) != DTR_SUCCESS)
				goto exit;
			}
		}

exit:
DBG("str_parse()", result);
	return  (result);
	}

/* ----------------------------------------------------------------------- *\
|  phr_init () - Initialize a PHRASE structure
\* ----------------------------------------------------------------------- */
    static void
phr_init (
	PHRASE *pp)		/* Pointer to the PHRASE structure */

	{
	memset(pp, 0, sizeof(PHRASE));
	}

/* ----------------------------------------------------------------------- *\
|  phr_parse () - Parse phrase values and attributes
\* ----------------------------------------------------------------------- */
	static int			/* Return the parse validity result */
phr_parse (
	PHRASE  *pp,		/* Pointer to the PHRASE structure */
	char    *s,			/* Pointer to the string */
	char   **sret)		/* Pointer to the returned string pointer */

	{
	int     result;		/* Returned result */
	int     ch;			/* Delimiter character */


	s = stpblk(s);		/* Skip leading white space */

	pp->fp = &pp->f[0];
	for (;;)
		{
		if ((result = fld_parse(pp, s, &s)) != 0)
			goto exit;

		switch (ch = *s)
			{
			case '-':			/* Date field delimiter */
				pp->reqattr |= CL_DATE;
				++s;
				break;			/* Parse next field */

			case '/':			/* Date field delimiter */
				pp->reqattr |= CL_DATE;
				++s;
				break;			/* Parse next field */

			case ':':			/* Time field delimiter */
				pp->reqattr |= CL_TIME;
				++s;
				break;			/* Parse next field */

			case '\0':			/* Inter-phrase delimiters */
			case '\n':
			case '\r':
			case '\t':
			case '.':
			case ',':
			case ';':
			case ' ':
				goto process;

			default:
				if ( ! isalnum(ch))			/* Inter-phrase delimiters */
					{
					result = DTR_INVDELIM;	/* Invalid delimiter */
					goto exit;
					}
				break;
			}

		}
process:
	if (pp->fcount > 0)
		result = phr_reduce(pp);		/* Reduce the phrase */
	else
		result = DTR_NOFIELD;			/* No fields */

exit:
	if (result == 0)
		++x.pcount;
	*sret = s;
DBG("phr_parse()", result);
	return (result);
	}

/* ----------------------------------------------------------------------- *\
|  fld_parse () - Parse field values and attributes
\* ----------------------------------------------------------------------- */
	static int			/* Return the parse validity result */
fld_parse (
	PHRASE  *pp,		/* Pointer to the PHRASE structure */
	char    *s,			/* Pointer to the string */
	char   **sret)		/* Pointer to the returned string pointer */

	{
	int     result;		/* Returned result */
	int     ch;			/* Delimiter character */


	ch = *s;

	if (isalpha(ch))				/* Process FIELD strings */
		result = alpha_parse(pp, s, &s);
	else if (isdigit(ch))
		result = num_parse(pp, s, &s);
	else
		result = DTR_INVSYMB;		/* Invalid symbol */

	*sret = s;
DBG("fld_parse()", result);
	return (result);
	}

/* ----------------------------------------------------------------------- *\
|  alpha_parse () - Parse alpha field values and attributes
\* ----------------------------------------------------------------------- */
	static int				/* Return the parse validity result */
alpha_parse (
	PHRASE  *pp,			/* Pointer to the PHRASE structure */
	char    *s,				/* Pointer to the string */
	char   **sret)			/* Pointer to the returned string pointer */

	{
	int     result;				/* Returned result */
	char    ch;						/* Temporary character */
	char   *bp;						/* Parse buffer pointer */
	char    buffer [A_SIZE + 1];	/* Parse buffer */


	bp = &buffer[0];
	for (;;)
		{
		ch = *s;

		if ( ! isalpha(ch))		/* Process FIELD strings */
			{
			*bp = '\0';
			break;
			}

		if (bp != &buffer[A_SIZE])
			*(bp++) = ch;		/* Copy the character */

		++s;
		}

	result = alpha_eval(pp, &buffer[0]);

	*sret = s;
DBG("alpha_parse()", result);
	return (result);
	}

/* ----------------------------------------------------------------------- *\
|	Alphabetic string field processing
\* ----------------------------------------------------------------------- */

typedef			/* STRINGS alpha identifier table definition */
	struct
	{
	char   *s;				/* Pointer to the match string */
	int     class;			/* Parse class */
	time_t  value;			/* Value (or selector code) */
	}  STRINGS;

#define  X_MON1		(0)		/* Class definitions */
#define  X_MON2		(1)
#define  X_MON3		(2)
#define  X_WDAY		(3)
#define  X_RDAY		(4)
#define  X_DAY3		(5)
#define  X_TIME		(6)
#define  X_AMPM		(7)

#define	 NUM_STR	(32)		/* Number of matchable strings */

static	STRINGS	str_table [NUM_STR] =
	{
	{ "jan",	X_MON1,		 1	},	/* January */
	{ "feb",	X_MON1,		 2	},	/* February */
	{ "mar",	X_MON1,		 3	},	/* March */
	{ "apr",	X_MON1,		 4	},	/* April */
	{ "may",	X_MON1,		 5	},	/* May */
	{ "jun",	X_MON1,		 6	},	/* June */
	{ "jul",	X_MON1,		 7	},	/* July */
	{ "aug",	X_MON1,		 8	},	/* August */
	{ "sep",	X_MON1,		 9	},	/* September */
	{ "oct",	X_MON1,		10	},	/* October */
	{ "nov",	X_MON1,		11	},	/* November */
	{ "dec",	X_MON1,		12	},	/* December */

	{ "thisy",	X_MON3,		 1	},	/* this year */

	{ "newy",	X_MON2,		 1	},	/* New Years */

	{ "sun",	X_WDAY,		 0	},	/* Sunday */
	{ "mon",	X_WDAY,		 1	},	/* Monday */
	{ "tue",	X_WDAY,		 2	},	/* Tuesday */
	{ "wed",	X_WDAY,		 3	},	/* Wednesday */
	{ "thu",	X_WDAY,		 4	},	/* Thurday */
	{ "fri",	X_WDAY,		 5	},	/* Friday */
	{ "sat",	X_WDAY,		 6	},	/* Saturday */

	{ "lastw",	X_WDAY,		 7	},	/* last week */
	{ "thisw",	X_WDAY,		 8	},	/* this week */
	{ "nextw",	X_WDAY,		 9	},	/* next week */

	{ "yes",	X_RDAY,		-1	},	/* yesterday */
	{ "tod",	X_RDAY,		 0	},	/* today */
	{ "tom",	X_RDAY,		 1	},	/* tomorrow */

	{ "thism",	X_DAY3,		 1	},	/* this month */

	{ "mid",	X_TIME,		 0	},	/* midnight */
	{ "noo",	X_TIME,		12	},	/* noon */

	{ "a",		X_AMPM,		 0	},	/* am time */
	{ "p",		X_AMPM,		12	},	/* pm time */
	};

static	STRINGS *str_find   (char *s);

/* ----------------------------------------------------------------------- *\
|  alpha_eval () - Evaluate an alpha field
\* ----------------------------------------------------------------------- */
	static int			/* Return the evaluation validity result */
alpha_eval (
	PHRASE  *pp,		/* Pointer to the PHRASE structure */
	char    *s)			/* Pointer to the alpha string */

	{
	int		  result = DTR_SUCCESS;	/* Returned result */
	time_t	  delta;				/* Relative day offset */
	time_t	  value;				/* Local copy of p->value */
	TSTR     *tstrp;				/* Pointer to the TSTR */
	STRINGS  *p;					/* Pointer into the string table */


	if ((p = str_find(s)) == NULL)
		{
		result = DTR_INVFLD;			/* Invalid symbol */
		goto exit;
		}

	switch (p->class)
		{
		case X_MON1:
			pp->reqattr |= CL_DATE;
			if (p->value > (x.ctstr.tm_mon + 1))
				x.ctstr.tm_year -= 1;
			result = fld_entry(pp, A_MON, A_MON, p->value);
			break;

		case X_MON2:
			pp->reqattr |= CL_DATE;
			fld_entry(pp, A_DAY, A_DAY, 1);
			result = fld_entry(pp, A_MON, A_MON, p->value);
			break;

		case X_MON3:
			pp->reqattr |= CL_DATE;
			fld_entry(pp, A_DAY,  A_DAY,   1);
			fld_entry(pp, A_MON,  A_MON,   p->value);
			result = fld_entry(pp, A_YEAR, A_YEAR, (x.ctstr.tm_year + 1900));
			break;

		case X_WDAY:
			pp->reqattr |= CL_DATE;
			if (p->value <= 6)
				{
				value = p->value;
				delta = 0;
				}
			else
				{
				value = 0;
				delta = 7 * (p->value - 8);
				}
			delta += ((7 + value - x.ctstr.tm_wday) % 7) - 7;
			x.mt = x.ct + (delta * 86400L);
			if (tstrp = localtime(&x.mt))
				memcpy(&x.mtstr, tstrp, sizeof(TSTR));
			fld_entry(pp, A_DAY,  A_DAY,   x.mtstr.tm_mday);
			fld_entry(pp, A_MON,  A_MON,  (x.mtstr.tm_mon + 1));
			result = fld_entry(pp, A_YEAR, A_YEAR, (x.mtstr.tm_year + 1900));
			break;

		case X_RDAY:
			pp->reqattr |= CL_DATE;
			x.mt = x.ct + (p->value * 86400L);
			if (tstrp = localtime(&x.mt))
				memcpy(&x.mtstr, tstrp, sizeof(TSTR));
			fld_entry(pp, A_DAY,  A_DAY,   x.mtstr.tm_mday);
			fld_entry(pp, A_MON,  A_MON,  (x.mtstr.tm_mon + 1));
			result = fld_entry(pp, A_YEAR, A_YEAR, (x.mtstr.tm_year + 1900));
			break;

		case X_DAY3:
			pp->reqattr |= CL_DATE;
			fld_entry(pp, A_DAY,  A_DAY,   1);
			fld_entry(pp, A_MON,  A_MON,  (x.ctstr.tm_mon + 1));
			result = fld_entry(pp, A_YEAR, A_YEAR, (x.ctstr.tm_year + 1900));
			break;

		case X_TIME:
			pp->reqattr |= (CL_TIME | S_24HR);
			fld_entry(pp, A_HOUR, A_HOUR, p->value);
			fld_entry(pp, A_MIN,  A_MIN,  0);
			result = fld_entry(pp, A_SEC,  A_SEC,  0);
			break;

		case X_AMPM:
			pp->reqattr |= (CL_TIME | ((p->value) ? (S_PM) : (S_AM)));
			break;
		}

exit:
DBG("alpha_eval()", result);
	return (result);
	}

/* ----------------------------------------------------------------------- *\
|  str_find () - Find the string in the STRING table
\* ----------------------------------------------------------------------- */
	static STRINGS *	/* Return a pointer into the string table */
str_find (
	char *s)			/* Pointer to the search string */

	{
	STRINGS  *p;		/* Pointer into the string table */
	STRINGS  *pret;		/* Returned pointer into the string table */


	pret = NULL;
	for (p = &str_table[0]; p != &str_table[NUM_STR]; ++p)
		{
		if (strncmp(s, p->s, strlen(p->s)) == MATCH)
			{
			pret = p;
			break;
			}
		}

	return (pret);
	}

/* ----------------------------------------------------------------------- *\
|  num_parse () - Parse numeric field values and attributes
\* ----------------------------------------------------------------------- */
	static int			/* Return the parse validity result */
num_parse (
	PHRASE  *pp,		/* Pointer to the PHRASE structure */
	char    *s,			/* Pointer to the numeric string */
	char   **sret)		/* Pointer to the returned string pointer */

	{
	int      result;	/* Returned result */
	char     ch;		/* Temporary character */
	char    *ss = s;	/* Saved string pointer */
	UINT32   n;			/* The evaluated number */


	n  = 0L;
	while (isdigit(ch = *s))		/* Terminate scanning on non-digit */
		{
		n *= 10L;
		n += (ch - '0');

		if (n > (UINT32)(UINT_MAX))
			{
			result = DTR_OVERFLOW;	/* Field value overflow */
			goto exit;
			}
		++s;
		}

	if ((*s == 'z')
	&&  (s == (ss + 4))
	&&  (pp->fcount == 0)
	&&  ((result = zulu_eval(pp, ss, &s)) == DTR_SUCCESS))
		goto exit;

	result = num_eval(pp, (UINT16)(n));

exit:
	*sret = s;
DBG("num_parse()", result);
	return (result);
	}

/* ----------------------------------------------------------------------- *\
|  zulu_eval () - Attempt to evaluate the string as ZULU time
\* ----------------------------------------------------------------------- */
	static int
zulu_eval (
	PHRASE  *pp,		/* Pointer to the PHRASE structure */
	char    *s,			/* Pointer to the digit string */
	char   **sret)		/* Pointer to the returned string pointer */

	{
	int      result;	/* Returned result */
	int      hour;		/* Decoded hour */
	int      minute;	/* Decoded minute */

	hour    = (*(s++) - '0') * 10;
	hour   +=  *(s++) - '0';
	minute  = (*(s++) - '0') * 10;
	minute +=  *(s++) - '0';

	if ((hour < 24)  &&  (minute < 60))
		{
		pp->reqattr |= (CL_TIME | S_24HR);
		fld_entry(pp, A_HOUR, A_HOUR, hour);
		fld_entry(pp, A_MIN,  A_MIN,  minute);
		fld_entry(pp, A_SEC,  A_SEC,  0);

		*sret  = ++s;			/* Skip over the 'z' */
		result = DTR_SUCCESS;
		}
	else
		result = DTR_INVFLD;		/* Invalid field value */

	return (result);
	}

/* ----------------------------------------------------------------------- *\
|
|	Numeric field evaluation and classification
|
|	Valid field value ranges for various types of time quantities
|
|	Lowest	Highest	Field type
|	0	59	Second
|	0	59	Minute
|	0	23	Hour
|	1	31	Day
|	1	12	Month
|	0	99	Year	(0 - 79 => 20xx; 80 - 99 => 19xx)
|	1980	2099	Year
|	all other	Invalid
|
\* ----------------------------------------------------------------------- */

typedef				/* BOUND numeric LUB table definition */
	struct
	{
	UINT16  lub;		/* Inclusive upper bound of a value range */
	UINT16  possattr;	/* Possible attributes for the range */
	UINT16  reqattr;	/* Required attributes for the range */
	}  BOUND;

#define	 NUM_BND	9

static	BOUND	lub_table [NUM_BND] =
	{
	{     0,	A_YEAR | A_TIME,				0			},
	{    12,	A_DATE | A_TIME,				0			},
	{    23,	A_YEAR | A_DAY | A_TIME,		0			},
	{    31,	A_YEAR | A_DAY | A_MIN | A_SEC,	0			},
	{    59,	A_YEAR | A_MIN | A_SEC,			0			},
	{    99,	A_YEAR,							A_YEAR		},
	{  1979,	INVALID,						INVALID		},
	{  2099,	A_YEAR,							A_YEAR		},
	{ 65535,	INVALID,						INVALID		}
	};

/* ----------------------------------------------------------------------- *\
|  num_eval () - Evaluate a numeric field
\* ----------------------------------------------------------------------- */
	static int			/* Return the evaluation validity result */
num_eval (
	PHRASE  *pp,		/* Pointer to the PHRASE structure */
	UINT16   n)			/* The numeric field value */

	{
	int     result = DTR_INVFLD;/* The returned result */
	BOUND  *bp;			/* Pointer to a BOUND entry */


	for (bp = &lub_table[0]; bp != &lub_table[NUM_BND]; ++bp)
		{
		if (n <= bp->lub)
			{
			result = fld_entry(pp, bp->possattr, bp->reqattr, n);
			break;
			}
		}

DBG("num_eval()", result);
	return (result);
	}

/* ----------------------------------------------------------------------- *\
|  fld_entry () - Enter field values into a field (check for overflow)
\* ----------------------------------------------------------------------- */
	static int
fld_entry (
	PHRASE  *pp,		/* Pointer to the PHRASE structure */
	UINT16   possattr,	/* The possible attribute */
	UINT16   reqattr,	/* The required attribute */
	time_t   n)			/* The field value */

	{
	int     result;		/* The returned result */

	if (pp->fcount < MAXFLD)
		{
		pp->fp->possattr = possattr;
		pp->fp->reqattr  = reqattr;
		pp->fp->value    = n;
		++pp->fcount;
		++pp->fp;
		result = DTR_SUCCESS;
		}
	else
		result = DTR_TOOMANYF;		/* Too many fields */

	return (result);
	}

/* ----------------------------------------------------------------------- *\
|  phr_reduce () - Reduce the parsed phrase
|
|   Incoming condition:
|	Field item possibilities (from field parse).
|	Field item requirements (from field parse).
|	Field class requirements (from special and AM/PM field parse).
|	Phrase class requirements (from phrase parse).
|
|   Outgoing condition:
|	Field class possibilities are set.
|	Field item possibilities guide the pattern match.
|	Phrase class possibilities guide the phrase selection match.
|
|	Field class and item requirements are unchanged.
|	Phrase item possibilities are cleared.
|	Phrase class and item requirements are unchanged.
|
|   Algorithm:
|	Build required field classes from required items.
|
|	Delete all field item possibilities which:
|	    Contradict the phrase class requirements.
|	    Contradict all field class requirements.
|	    Contradict the same field item requirements.
|	    Contradict the other field item requirements.
|
|	Build the field class possibilities.
|
|	Set the phrase class possibilities from:
|	    The AND of the field class possibilities, with CLASS.
|
|	If no phrase class possibilities exist, error.
|
|	If any field item possibilities are zero, error.
|
\* ----------------------------------------------------------------------- */
	static int			/* Return the parse validity result */
phr_reduce (
	PHRASE *pp)			/* Pointer to the PHRASE structure */

	{
	int     result = 0;		/* Returned result, assume failure */
	int     n;				/* The number of fields ( >= 1 ) */
	int     i;				/* The source field index */
	int     j;				/* The dest field index */
	UINT16  attr;			/* Temporary attribute */
	UINT16  pattr;			/* Temporary phrase attribute */
	UINT16  fattr [MAXFLD];	/* Temporary field attributes */

#define  TIMEDATE	(A_TIME  | A_DATE)
#define  CLASS		(CL_TIME | CL_DATE)


	n = pp->fcount;			/* Get the field count */

	for (i = 0; i < n; ++i)
		{
		attr = pp->f[i].reqattr;	/* Build field class requirements */
		if (attr & A_TIME)
			pp->f[i].reqattr |= CL_TIME;
		if (attr & A_DATE)
			pp->f[i].reqattr |= CL_DATE;
		}

	pattr = CLASS;					/* Build the phrase prohibition */
	attr  = pp->reqattr;
	if (attr & CL_TIME)
		pattr |= A_DATE;
	if (attr & CL_DATE)
		pattr |= A_TIME;

	for (i = 0; i < n; ++i)
		{
		fattr[i] = pattr;				/* Phrase class requirement */

		attr = pp->f[i].reqattr;		/* Same field item requirement */
		if (attr & A_SEC)
			fattr[i] |= (A_MIN | A_HOUR | A_DATE);
		if (attr & A_MIN)
			fattr[i] |= (A_SEC | A_HOUR | A_DATE);
		if (attr & A_HOUR)
			fattr[i] |= (A_SEC | A_MIN | A_DATE);
		if (attr & A_DAY)
			fattr[i] |= (A_TIME | A_MON | A_YEAR);
		if (attr & A_MON)
			fattr[i] |= (A_TIME | A_DAY | A_YEAR);
		if (attr & A_YEAR)
			fattr[i] |= (A_TIME | A_DAY | A_MON);

		for (j = 0; j < n; ++j)
			{
			attr = pp->f[j].reqattr;	/* All field class requirement */
			if (attr & CL_TIME)
				fattr[i] |= A_DATE;
			if (attr & CL_DATE)
				fattr[i] |= A_TIME;
			if (i != j)					/* Other field item requirement */
				fattr[i] |= (attr & TIMEDATE);
			}
		}

	pattr = CLASS;
	for (i = 0; i < n; ++i)				/* Perform the actual bit clear */
		{
		attr = (pp->f[i].possattr &= ~fattr[i]);
		if (attr & A_TIME)				/* Build the field class */
			pp->f[i].possattr |= CL_TIME;
		if (attr & A_DATE)
			pp->f[i].possattr |= CL_DATE;

		pattr &= (pp->f[i].possattr & CLASS);	/* Build the phrase class */
		}
	pp->possattr = pattr;				/* Install the phrase possible class */


	if ((pp->possattr & CLASS) == 0)
		{
		result = DTR_NOPHRCLASS;		/* No possible phrase class type */
		goto exit;
		}


	for (i = 0; i < n; ++i)
		{
		if ((pp->f[i].possattr & TIMEDATE) == 0)
			{
			result = DTR_NOFLDITEM;		/* No possible field item type */
			goto exit;
			}
		}

	if (((attr = (pp->reqattr & S_TIME)) != 0)
	&&  (attr != S_AM)
	&&  (attr != S_PM)
	&&  (attr != S_24HR))
		result = DTR_TMODE;				/* Time mode conflict */

exit:
DBG("phr_reduce()", result);
	return (result);
	}

/* ----------------------------------------------------------------------- *\
|  pair_reduce () - Reduce the parsed phrase pair
|
|   Incoming condition:
|	Phrase class possibilities (from two phrases).
|
|   Outgoing condition:
|	Phrase class possibilities (updated) guide the phrase selection match.
|
|	Field class and item possibilities are unchanged.
|
|   Algorithm:
|	If either phrase can only be one class,
|	    delete that class from the other phrase.
|
|	If no phrase class possibilities exist, error.
|
\* ----------------------------------------------------------------------- */
	static int				/* Return the parse validity result */
pair_reduce (void)

	{
	int     result = 0;				/* Returned result, assume success */
	UINT16  attr0;					/* Temporary phrase attribute */
	UINT16  attr1;					/* Temporary phrase attribute */

	attr0 = x.phr[0].possattr & CLASS;	/* Build the phrase classes */
	attr1 = x.phr[1].possattr & CLASS;

	if (attr0 != CLASS)				/* Delete opposing singular classes */
		x.phr[1].possattr &= ~attr0;

	if (attr1 != CLASS)
		x.phr[0].possattr &= ~attr1;

	if (((x.phr[0].possattr & CLASS) == 0)
	||  ((x.phr[1].possattr & CLASS) == 0))
		result = DTR_NOPHRCLASS;	/* No possible phrase class type */

DBG("pair_reduce()", result);
	return (result);
	}

/* ----------------------------------------------------------------------- *\
|  pair_check () - Check all phrases for ambiguity
\* ----------------------------------------------------------------------- */
	static int				/* Return the check validity result */
pair_check (void)

	{
	int     result = 0;		/* Returned result, assume success */
	int     i;				/* Phrase counter */
	UINT16  attr;			/* Temporary attribute */

	for (i = 0; i < x.pcount; ++i)
		{
		attr = x.phr[i].possattr & CLASS;
		if (attr == CLASS)
			{
			result = DTR_AMBIGUOUS;
			break;
			}
		}

DBG("pair_check()", result);
	return (result);
	}

/* ----------------------------------------------------------------------- */
	static int
datematch (
	PHRASE *pp)				/* Pointer to the PHRASE structure */

	{
	int      result;		/* Returned result */
	int      matched;		/* Match flag */
	DATTBL  *p;				/* Pointer into the DATTBL */

	matched = FALSE;
	for (p = &dat_table[0]; p != &dat_table[DAT_BND]; ++p)
		{
		if ((pp->f[0].possattr & p->m[0])
		&&  (((pp->fcount < 2)  &&  (p->m[1] == 0))
			||  (pp->f[1].possattr & p->m[1]))
		&&  (((pp->fcount < 3)  &&  (p->m[2] == 0))
			||  (pp->f[2].possattr & p->m[2])))
			{
			matched = TRUE;
			pp->dp = p;

			/* Cancel the unmatched possibilities */

			pp->f[0].possattr &= ~((A_DATE & ~(p->m[0])) | A_TIME | CL_TIME);
			pp->f[1].possattr &= ~((A_DATE & ~(p->m[1])) | A_TIME | CL_TIME);
			pp->f[2].possattr &= ~((A_DATE & ~(p->m[2])) | A_TIME | CL_TIME);
			break;
			}
		}

	result = (matched) ? (DTR_SUCCESS) : (DTR_SYNTAX);
DBG("datematch()", result);
	return (result);
	}

/* ----------------------------------------------------------------------- */
	static int
timematch (
	PHRASE *pp)			/* Pointer to the PHRASE structure */

	{
	int  result;		/* Returned result */


	if ((pp->fcount >= 1)  &&  pp->f[0].possattr & A_HOUR
	&& ((pp->fcount <  2)  ||  pp->f[1].possattr & A_MIN)
	&& ((pp->fcount <  3)  ||  pp->f[2].possattr & A_SEC))
		{
		result = DTR_SUCCESS;

		/* Cancel the unmatched possibilities */

		pp->f[0].possattr &= ~(A_MIN  | A_SEC | A_DATE | CL_DATE);
		pp->f[1].possattr &= ~(A_HOUR | A_SEC | A_DATE | CL_DATE);
		pp->f[2].possattr &= ~(A_HOUR | A_MIN | A_DATE | CL_DATE);

		/* Perform AM/PM validations / corrections */

		if ((pp->reqattr & S_TIME) == S_AM)
			{
			if ((pp->f[0].value < 1)  ||  (pp->f[0].value > 12))
				result = DTR_INVFLD;
			else if (pp->f[0].value == 12)
				pp->f[0].value -= 12;
			}

		else if ((pp->reqattr & S_TIME) == S_PM)
			{
			if ((pp->f[0].value < 1)  ||  (pp->f[0].value > 12))
				result = DTR_INVFLD;
			else if (pp->f[0].value != 12)
				pp->f[0].value += 12;
			}
		}
	else
		result = DTR_SYNTAX;

DBG("timematch()", result);
	return (result);
	}

/* ----------------------------------------------------------------------- *\
|  finalparse () - Perform the final parse to build the TSTR
\* ----------------------------------------------------------------------- */
	static time_t
finalparse (void)

	{
	int     flag = FALSE;	/* Field set flag */
	time_t  temp;			/* Temporary value */
	FIELD  *fp;				/* Pointer to the FIELD structure */
	time_t  tresult;		/* The returned time result */


	/* Default midnight */

	memcpy(&x.mtstr, &x.ctstr, sizeof(TSTR));
	x.mtstr.tm_sec = x.mtstr.tm_min = x.mtstr.tm_hour = 0;

	if ((fp = fld_find(A_YEAR)) != NULL)
		{
		temp = fp->value;
		if (temp <= 79)
			temp += 2000;
		else if (temp <= 99)
			temp += 1900;
		x.mtstr.tm_year = (int)(temp - 1900);	/* UNIX correction */
		flag = TRUE;
		}

	if ((fp = fld_find(A_MON)) != NULL)
		{
		x.mtstr.tm_mon = (int)(fp->value - 1);	/* UNIX correction */
		flag = TRUE;
		}
	else if (flag)
		x.mtstr.tm_mon = 0;		/* January */

	if ((fp = fld_find(A_DAY)) != NULL)
		x.mtstr.tm_mday = (int)(fp->value);
	else  if (flag)
		x.mtstr.tm_mday = 1;	/* 1st of month */

    if (x.timephrase)
		{
		if (x.timephrase->f[0].possattr & A_HOUR)
			x.mtstr.tm_hour = (int)(x.timephrase->f[0].value);
		if (x.timephrase->f[1].possattr & A_MIN)
			x.mtstr.tm_min  = (int)(x.timephrase->f[1].value);
		if (x.timephrase->f[2].possattr & A_SEC)
			x.mtstr.tm_sec  = (int)(x.timephrase->f[2].value);
		}

	/* Set tm_isdst FALSE; if it changes, then there */
	/* is a one hour error which needs to be corrected */

	x.mtstr.tm_isdst = FALSE;
	tresult = mktime(&x.mtstr);
	if (x.mtstr.tm_isdst)
		tresult -= (60L * 60L);

	return (tresult);
	}

/* ----------------------------------------------------------------------- *\
|  fld_find () - Find a date field having a specified attribute possibility
\* ----------------------------------------------------------------------- */
	static FIELD *			/* Return pointer to the FIELD structure */
fld_find (
	UINT16 attr)			/* The specified attribute */

	{
	FIELD  *fp;				/* Returned pointer to the FIELD structure */


	if (x.datephrase)
		{
		for (fp = &x.datephrase->f[0]; fp != &x.datephrase->f[MAXFLD]; ++fp) 
			{
			if (fp->possattr & attr)
			goto exit;
			}
		}

	fp = NULL;			/* Attribute not found */

exit:
	return (fp);
	}

/* ----------------------------------------------------------------------- *\
|  serrtd () - Return the text string explaining the last parse completion
\* ----------------------------------------------------------------------- */
	char *
serrtd (void)

	{
	char  *p;			/* Pointer to the returned string */

	switch (x.tderror)
		{
		case DTR_SUCCESS:		p = "Successful conversion";			break;
		case DTR_NOSTRING:		p = "No string provided";				break;
		case DTR_INVDELIM:		p = "Invalid phrase delimiter";			break;
		case DTR_TOOMANYF:		p = "Too many fields";					break;
		case DTR_NOFIELD:		p = "No fields";						break;
		case DTR_INVSYMB:		p = "Invalid symbol";					break;
		case DTR_OVERFLOW:		p = "Field value overflow";				break;
		case DTR_INVFLD:		p = "Invalid field value";				break;
		case DTR_NOPHRCLASS:	p = "No possible phrase class type";	break;
		case DTR_NOFLDITEM:		p = "No possible field item type";		break;
		case DTR_AMBIGUOUS:		p = "Ambiguous expression";				break;
		case DTR_SYNTAX:		p = "Unparsable syntax";				break;
		case DTR_SYSTIME:		p = "System time unreasonably set";		break;
		case DTR_TMODE:			p = "Time mode (am/pm/24hr) conflict";	break;
		case DTR_NS:			p = "Unsupported functionality";		break;
		default:				p = "???";								break;
		}

	return (p);
	}

/* ----------------------------------------------------------------------- */
#ifdef  TESTMODE
	static void
prn_attr (
	UINT16 attr)

	{
	printf("%04X  ", attr);
	if (attr & A_SEC)	printf("A_SEC ");
	if (attr & A_MIN)	printf("A_MIN ");
	if (attr & A_HOUR)	printf("A_HOUR ");
	if (attr & A_DAY)	printf("A_DAY ");
	if (attr & A_MON)	printf("A_MON ");
	if (attr & A_YEAR)	printf("A_YEAR ");
	if (attr & CL_TIME)	printf("CL_TIME ");
	if (attr & CL_DATE)	printf("CL_DATE ");
	if (attr & S_AM)	printf("S_AM ");
	if (attr & S_PM)	printf("S_PM ");
	if (attr & S_24HR)	printf("S_24HR ");
	if (attr & INVALID)	printf("INVALID ");
	printf("\n");
	}

/* ----------------------------------------------------------------------- */
	static void
prn_fld (
	FIELD *fp,
	int i)

	{
	printf("Field    %d: %lld\n", i, fp->value);
	printf("  Possible: ");
	prn_attr(fp->possattr);
	printf("  Required: ");
	prn_attr(fp->reqattr);
	}

/* ----------------------------------------------------------------------- */
	static void
prn_phr (
	PHRASE *pp,
	int j)

	{
	int  i;
	int  n = pp->fcount;

	printf("Phrase %d\n", j);
	printf("  Possible: ");
	prn_attr(pp->possattr);
	printf("  Required: ");
	prn_attr(pp->reqattr);

	if (n == 0)
		printf("No fields\n");
	else
		{
		for (i = 0; i < n; ++i)
			prn_fld(&pp->f[i], i);
		}
	}

/* ----------------------------------------------------------------------- */
	static void
prn_both (void)

	{
	int  i;

	if (x.pcount == 0)
		printf("No phrases\n");
	else
		{
		for (i = 0; i < x.pcount; ++i)
			prn_phr(&x.phr[i], i);
		}
	}

/* ----------------------------------------------------------------------- */
	static void
prn_dt (
	time_t t)

	{
	printf("Time: %s", ctime(&t));
	}

/* ----------------------------------------------------------------------- */
	void
prn_tstr (
	TSTR *t)

	{
	printf("tm_year %d\n", t->tm_year);
	printf("tm_mon  %d\n", t->tm_mon);
	printf("tm_mday %d\n", t->tm_mday);
	printf("tm_hour %d\n", t->tm_hour);
	printf("tm_min  %d\n", t->tm_min);
	printf("tm_sec  %d\n", t->tm_sec);
	printf("dst     %d\n", t->tm_isdst);
	}

/* ----------------------------------------------------------------------- */
	void
main (
	int   argc,
	char *argv [])

	{
	time_t  retval;

	++argv;
	retval = sgettd(*argv);
	printf("Parse retval: %08lX", retval);
	printf(": %s\n", serrtd());
	prn_both();
	prn_tstr(&x.mtstr);
	printf("String: %s\n", *argv);
	if (retval > 0L)
		prn_dt(retval);
	}
#endif
/* ----------------------------------------------------------------------- */
