/* ----------------------------------------------------------------------- *\
|
|		     Wild Card File Name Server Include File
|			  for Microsoft C on the IBM PC
|
|	Copyright (c) 1985, 1990, 1991, 1993, 1997, 1998, all rights reserved
|				Brian W Johnson
|				   17-Jun-90
|				   14-May-91
|				   28-Feb-93
|				   20-May-93
|				   17-Dec-94
|				   16-Aug-97 for NT
|				   26-Sep-97 for UNC
|				   17-Feb-98 for HexDump
|				   30-Sep-07 for 64 bit file sizes
|				   07-Oct-10 added pfopen()
|
\* ----------------------------------------------------------------------- */

#ifndef __FWILD_H__
#define __FWILD_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include <CRTDEFS.H>
#include <SYS\STAT.H>
#include <SYS\TIMEB.H>

typedef signed   __int64  INT64,  *PINT64;
typedef unsigned __int64  UINT64, *PUINT64;

/* ------------------------------------------------------------------------ *\
|  fwild() internal / external definitions
\* ------------------------------------------------------------------------ */

/* Mode request bit definitions */

#define  FW_FILE      1			/* Ordinary file request */
#define  FW_HIDDEN    2			/* Hidden file request */
#define  FW_SYSTEM    4			/* System file request */

#ifdef  FWILD_INTERNAL
#define  FW_VOLU      8			/* Volume name request */
#endif

#define  FW_SUBD     16			/* Subdirectory request */
#define  FW_DSTAR    32			/* Implied dot star request */

/* File attribute bit definitions */

#define  ATT_RONLY    1			/* Read only file */
#define  ATT_HIDDEN   2			/* Hidden file */
#define  ATT_SYSTEM   4			/* System file */
#define  ATT_VOLU     8			/* Volume identification */
#define  ATT_SUBD    16			/* Subdirectory file */
#define  ATT_ARCH    32			/* File not archived */

#ifdef  FWILD_INTERNAL

#define  f_type  link->dta.dta_type	/* File attribute, type */
#define  f_fdt   link->fdt		/* File attribute, timestamp */
#define  f_size  link->dta.dta_size	/* File attribute, size */

typedef				/* The DTA used by findf/finfn/findc */
	struct
		{
		char      dta_type;		/* File attribute, type */
		UINT64    dta_size;		/* File attribute, size */
		char      dta_name [_MAX_PATH];	/* The found filename */

		/* NT support */

		int       result;		/* The returned result */
		int       SearchAttr;	/* The original search attributes */
		HANDLE    sh;			/* The WIN32 search handle */
		WIN32_FIND_DATA	 wfd;	/* The WIN32 FIND_DATA structure */
		}  DTA_BLK;


typedef				/* Recursive DTA list element */
	struct dtaele
		{
		struct dtaele  *link;	/* Link to the next (inactive) DTA_ELE */
		int       wild;			/* Current wild search mode */
		int       state;		/* Current state */
		long      fdt;			/* The file timestamp */
		char     *proto;		/* Pointer to prototype string, first char */
		char     *last;			/* Pointer to prototype string, last delim */
		char     *next;			/* Pointer to prototype string, next char */
		char     *search;		/* Pointer to current search filename */
		char     *pattern;		/* Pointer to current pattern filename */
		char     *found;		/* Pointer to current found filename */
		DTA_BLK   dta;			/* The DTA used by findf/findn/findc */
		}  DTA_ELE;


typedef				/* DTA list header */
    struct
		{
		DTA_ELE  *link;			/* Link to active DTA_ELE element */
		int       mode;			/* Search file attribute */
		}  DTA_HDR;


typedef				/* File Pair (FP) structure */
	struct
		{
		int       sentinel;		/* Sentinel to detect free() error */
		int       index;		/* Index into fnp1 for tail part */
		int       dflag;		/* TRUE if fnp2 should be constructed */ 
		DTA_HDR  *hp;			/* Pointer to fwild structure */
		char     *s2;			/* Pointer to dest path */
		char     *fnp1;			/* Pointer to first filename */
		char     *fnp2;			/* Pointer to second filename */
		}  FP;


#define  DHV	DTA_HDR
#define  FPV	FP
#else
#define  DHV	char
#define  FPV	void
#endif

#if defined(_FILE_DEFINED)
#define  _PTOFILE	FILE *
#else
#define  _PTOFILE	void *
#endif

/* ------------------------------------------------------------------------ *\
|  Miscellaneous definitions
\* ------------------------------------------------------------------------ */

#ifndef _TIME_T_DEFINED
typedef long time_t;
#define _TIME_T_DEFINED
#endif

#ifndef  TRUE
#define  TRUE	1
#define  FALSE	0
#endif

#ifndef  MATCH					/* Used by strcmp() */
#define  MATCH	0
#endif

typedef							/* strtoken() support */
	struct
		{
		char  *ptr;     		/* Pointer for search resumption */
		char   ch;      		/* Saved delimiter character */
		}  TOKENCB;

/* ------------------------------------------------------------------------ *\
|  Function prototypes
\* ------------------------------------------------------------------------ */

#define  finit(a,b)  fwinit((a),(b))		/* Backward name compatibility */

extern	DHV    *fwinit (char *, int);		/* Init the wild filename server */
extern	char   *fwild (DHV *);				/* Return a wild filename */
extern	char   *fwildexcl (DHV *);			/* Return a wild filename (respects exclusion) */
extern	int     fexcludeInit (void);		/* Initialize the default exclusions subsystem */
extern	int     fexclude (char *);			/* Register an exclude path (exclude) */
extern	int     fexclude (char *);			/* Register an exclude path (exclude) */
extern	int     fexcludeCheck (char *);		/* Check if an excluded file/path */
extern	void	fexcludeDefEnable (int);	/* Enable/disable default file exclusion(s) */
extern	void	fexcludeShowExcl (int);		/* Show file exclusion(s) */
extern	DHV    *fwfree (DHV *);				/* Close and free an fwild instance */
extern	char   *fwfirst (char *);			/* Return the first matching wild filename */
extern  int	fwtype (DHV *);					/* Return the type of the current fn */
extern  char   *fwdate (DHV *);				/* Return the date of the current fn */
extern  char   *fwtime (DHV *);				/* Return the time of the current fn */
extern  char   *fwtimes (DHV *);			/* Return the time of the current fn (secs) */
extern  long	fwgetfdt (DHV *);			/* Return the current file timedate */
extern  UINT64	fwsize (DHV *);				/* Return the size of the current fn */
extern	int	fwvalid (char *);				/* Validate a file/path string */
extern	char   *fwerror (void);				/* Return the fwerrno error string */
extern  char   *vol_name (char *);			/* Return vol name of the spec drive */
extern  char   *_volname (int);				/* Return vol name of the spec drive */
extern	int	fgetattr (char *fnp);			/* Get file attributes (filename) */
extern	int	hgetattr (int fh);				/* Get file attributes (handle) */
extern	long	fgetfdt (char *fnp);		/* Get timedate (filename) */
extern	long	hgetfdt (int fh);			/* Get timedate (handle) */
extern	int	fgetsize (char *fnp, PUINT64 size);  /* Get file size (filename) */
extern	int	hgetsize (int fh, PUINT64 size);     /* Get file size (handle) */
extern	int	fsetattr (char *fnp, int attr);	/* Set file attr (filename) */
extern	int	fsetfdt (char *fnp, long fdt);	/* Set timedate (filename) */
extern	long	zerotime (void);			/* Return lowest usable UNIX time */

extern	int	getopt (int, char **, char *);	/* Decode command options */
extern	int	optvalue			/* Get numeric option value */
		    (char *, long *, long, long);
extern	char   *optvalerror (void);			/* Return optval() error string */
extern	int	expression (char *, long *);	/* Evaluate numeric expression */
extern	char   *expression_error (void);	/* Return expression() error string */

extern  char   *fnabspth (char *);			/* Return absolute name of the fn */
extern  char   *fncatpth (char *, char *);	/* Return concatenated name */
extern  char   *fntail (char *);			/* Return the tail of the filename */
extern	int	fnsubpth (char *, char *);		/* Check if s1 is a subpath of s2 */
extern	int	_fnchkdir (char *);				/* Test if a directory exists (no UNC) */
extern	int	fnchkdir (char *);				/* Test if a directory exists */
extern	int	fnchkfil (char *);				/* Test if a file exists */
extern  int     fnchkunc (char *);			/* Test if a UNC root */
extern  void	fnreduce (char *);			/* Reduce a filename */
extern  int	fndot (char *);					/* Test if a "dot" directory */
extern	int	fnmatch (char *, char *, int);	/* Match a wild file name */
extern  int	iswild (char *);				/* Test if a "wild" path */
extern	int	pmatch (char *, char *, int);	/* Test two fnames for match */
extern	int	psubmatch (char *, char *, int);	/* Test two fnames for submatch */

extern	int	pathmake (char *);				/* Build a path */
extern	int	pcreat (char *, int);			/* Build path and creat() a file */
extern	int	popen (char *, int, ...);		/* Build path and open() a file */
extern	int	prename (char *, char *);		/* Build path and rename() a file */

#ifdef _FILE_DEFINED
extern  FILE  *pfopen (const char *pathname, const char *mode);
extern	void	HexDump (FILE *, void *, unsigned); /* Perform hexdump to file */
#endif

extern  char	strpath (char *);			/* Return the path char of the fn */
extern  void	strsetp (char *, char);		/* Set the path char in a string */

extern  char	egetpath (void);			/* Return the path char of the env */
extern  char	getpath (void);				/* Return the path char of DOS */
extern  char	egetswch (void);			/* Return the switch char of the env */

extern	int	getdir (int, char *);			/* Get the CWD for a drive */
extern	int	getdrive (void);				/* Get the default drive */
extern	int	getcols (void);					/* Get the console column count */
extern	int	getrows (void);					/* Get the console row count */

extern  char   *stpblk (char *);			/* Skip leading white space */
extern  char   *fmalloc (unsigned int);		/* Malloc() with a fatal error */
extern  void	fatalerr (char *);			/* Die an unnatural death */

extern  int	execute (char *, int);						/* Execute a child process */
extern	int	fcopy (char *src, char *dst, int flg);		/* Copy a file */
extern	int	fp_init (FPV **, int flag, char *, char *);	/* Init fpair */
extern	int	fp_pair (FPV *, char **, char **);			/* Return fpair */

/* Far memory manipulation support */

#if defined(M_I86CM) || defined(M_I86LM) || defined(_WIN32)
#define  farcmp(a, b, c)	memcmp((a), (b), (c))
#define  farcpy(a, b, c)	memcpy((a), (b), (c))
#define  farset(a, b, c)	memset((a), (b), (c))
#else
extern	int	farcmp (void far *dst, void far *src, unsigned int n);
extern	void	farcpy (void far *dst, void far *src, unsigned int n);
extern	void	farset (void far *dst, int value,     unsigned int n);
#endif

/* Time/date string parsing support */

extern	char   *strdate  (time_t dt);		/* Convert a DOS timedate to a date string */
extern	char   *strtime  (time_t dt);		/* Convert a DOS timedate to a time string */
extern	char   *strtime2 (time_t dt);		/* Convert a DOS timedate to a time string */
extern	long	sgettd   (char *s);			/* Parse a TIMEDATE string */
extern	long	fwsgettd (char *s);			/* Parse a TIMEDATE string (filename) */
extern	char   *serrtd   (void);			/* Return TD error string */
extern	char   *fwserrtd (void);			/* Return TD error string (filename) */
extern  char   *strtoken (char *string, char *control, TOKENCB *tokenblk);

#ifdef  FWILD_INTERNAL

extern	int	_findf (DTA_BLK *, char *, int);	/* Dual mode */
extern	int	_findn (DTA_BLK *);
extern	void	_findc (DTA_BLK *);
extern  int	AttrFromWin32 (DWORD attr); 	/* Convert WIN32 attr to FWILD attr */
extern  DWORD	AttrToWin32 (int attr);		/* Convert FWILD attr to WIN32 attr */
#endif

extern	char	optswch;					/* The option switch character */
extern	char	optchar;					/* The last option character */
extern	int		optind;						/* The current argv[] index */
extern	char   *optarg;						/* Pointer to option parameter */
extern	char   *optenv;						/* Pointer to environment options */
extern	int	fwerrno;						// fwvalid() error result
extern	int	xporlater;						// TRUE if Windows XP or later

/* Template program support */

extern	char   *usagedoc [];				/* Usage documentation pointer array */

extern	void	cantfind (char *fnp);		/* Report can't find inout file */
extern	void	cantopen (char *fnp);		/* Report can't open inout file */
extern	void	usage    (void);			/* Report usage following error */
extern	void	help     (void);			/* Report help on request */
extern	void	docprint (char *s[]);		/* Print help support */

extern	void	fmemdump  (					/* Dump n bytes of memory */
		    _PTOFILE fp, char *p, int n, long offset);

/* ------------------------------------------------------------------------ *\
|  fcopy () definitions
\* ------------------------------------------------------------------------ */

/* Control flags (bit-mapped) */

#define  FCF_ROK	(1)	/* Copy even if dest is RO */
#define  FCF_FDT	(2)	/* Duplicate the src fdt */
#define  FCF_ATTR	(4)	/* Duplicate the src attributes */

/* Result (value) */

#define  FCR_SUCCESS	(0)	/* Successful copy */
#define  FCR_RO		(1)	/* RO and not FCF_ROK */
#define  FCR_OPN1	(2)	/* Can't open file 1 (src) */
#define  FCR_OPN2	(3)	/* Can't open file 2 (dest) */
#define  FCR_READ	(4)	/* Error reading file 1 (src) */
#define  FCR_WRITE	(5)	/* Error writing file 2 (dest) */
#define  FCR_GFDT	(6)	/* Unable to get src datetime */
#define  FCR_GATTR	(7)	/* Unable to get src attribute */
#define  FCR_SFDT	(8)	/* Unable to set dest datetime */
#define  FCR_SATTR	(9)	/* Unable to set dest attribute */

#define  FPR_SUCCESS	(0)	/* Successful */
#define  FPR_NOFILE	(1)	/* No file pair returned */
#define  FPR_P2WILD	(2)	/* Path2 cannot be wild */
#define  FPR_P2FILE	(3)	/* Path2 cannot be a file */
#define  FPR_MEMORY	(4)	/* Insufficient memory */
#define  FPR_FWERROR	(5)	/* fwild() error */

/* ------------------------------------------------------------------------ *\
|  fwerrno definitions
\* ------------------------------------------------------------------------ */

#define  FWERR_NONE	 (0)		// No error, pathname is good
#define  FWERR_NULL	 (1)		// NULL pathname pointer passed
#define  FWERR_EMPTY	 (2)		// Empty pathname passed
#define  FWERR_DRIVE	 (3)		// Improper drive spec
#define  FWERR_SEPARATOR (4)		// Improper separator spec
#define  FWERR_ELEMENT	 (5)		// Improper path element spec
#define  FWERR_SIZE	 (6)		// Invalid name field size
#define  FWERR_TRAIL	 (7)		// Trailing "\" following element

/* ------------------------------------------------------------------------ *\
|  Exit code definitions
\* ------------------------------------------------------------------------ */

#define  FWEXIT_SUCCESS			(0)	// Never fatal
#define  FWEXIT_FILE_NOT_FOUND		(1)	// Optional fatal
#define  FWEXIT_MINOR_ERROR		(2)	// Optional fatal

#define  FWEXIT_MAJOR_ERROR		(3)	// Always fatal
#define  FWEXIT_COMMAND_LINE		(4)	// Always fatal
#define  FWEXIT_PROGRAM_NOT_FOUND	(5)	// Always fatal

/* ------------------------------------------------------------------------ *\
|  Somehow missing from stdio.h included here to squelch compiler errors
\* ------------------------------------------------------------------------ */

extern char * gets(char *s);

#if defined(__cplusplus)
}
#endif

#endif // __FWILD_H__

/* ------------------------------------------------------------------------ */
