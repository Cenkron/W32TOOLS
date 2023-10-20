/* ----------------------------------------------------------------------- *\
|
|		     Wild Card File Name Server Include File
|			  for Microsoft C on the IBM PC
|
|		Copyright (c) 1985 - 2023, all rights reserved
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
|				   10-Sep-23 New fnreduce error
|				    4-Oct-23 Major update of fWild
|				    6-Oct-23 Major update of path construction, path analysis
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

/* ------------------------------------------------------------------------ *\
|  Special definitions
\* ------------------------------------------------------------------------ */

typedef signed   __int64  INT64,  *PINT64;
typedef unsigned __int64  UINT64, *PUINT64;

#ifdef  MAX_PATH
#undef  MAX_PATH	// 260
#endif

#ifndef MAX_PATH
#define MAX_PATH (1024)	// Per https://learn.microsoft.com/en-us/windows/win32
#endif

#define MAX_COPY (MAX_PATH - 64)

/* ------------------------------------------------------------------------ *\
|  fWild() internal / external definitions
\* ------------------------------------------------------------------------ */

// File/directory attribute request bit definitions

#define  FW_HIDDEN		(0x02)			// Hidden file request
#define  FW_SYSTEM		(0x04)			// System file request
#define  FW_DIR			(0x10)			// Subdirectory request
#ifdef  FWILD_INTERNAL
#define  FW_VOLU		(0x08)			// Volume name request
#endif
#define  FW_FILE		(0x40)			// Ordinary file request (synthetic)

// Allow fWild to print Access Denied warnings (synthetic)

#define  FW_AD			(0x8000)

#define FW_ALLAD	(FW_FILE | FW_DIR | FW_HIDDEN | FW_SYSTEM | FW_AD)
#define FW_ALL		(FW_FILE | FW_DIR | FW_HIDDEN | FW_SYSTEM)
#define ATT_ALL		(ATT_RONLY | ATT_HIDDEN | ATT_SYSTEM\
					| ATT_VOLU | ATT_DIR | ATT_ARCH | ATT_FILE

// File/directory  attribute bit definitions

#define  ATT_RONLY		(0x01)			// Read only file
#define  ATT_HIDDEN		(0x02)			// Hidden file
#define  ATT_SYSTEM		(0x04)			// System file
#define  ATT_VOLU		(0x08)			// Volume identification
#define  ATT_DIR		(0x10)			// Subdirectory file
#define  ATT_ARCH		(0x20)			// File not archived
#define  ATT_FILE		(0x40)			// Ordinary file request (synthetic)
#define  ATT_NONE		(0x00)			// FinderFile() failure (synthetic)

// File exclusion control attributes

#define  XD_BYNAME    0x01				// Check directories, unrooted
#define  XD_BYPATH    0x02				// Check directories, rooted
#define  XF_BYNAME	  0x04				// Check files, unrooted
#define  XF_BYPATH    0x08				// Check files, rooted

// ------------------------------------------------------------------------------------------------

#ifdef  FWILD_INTERNAL

#define isPath(ch)  ((ch == '\\') || (ch == '/'))  // Match either possible path character

#define NULCH		('\0')				// Standard NUL character
#define PATHCH		('\\')				// Standard path character

#define FWP_MAX		(MAX_PATH + 100)	// Fwild path buffers are larger than just barely adequate
#define SEG_MAX		(20)				// Number of fWild segment buffers
#define	LEV_MAX		(30)				// Max number of search levels

typedef	enum		// Segment types
	{
	TYPE_ROOT = 0,		// Base segment				// Prefix and possibly root sepearator
	TYPE_NORMAL,		// Non-wild segment			// Not officially wild
	TYPE_WILD,			// Wild segment				// Officially wild
	TYPE_RWLD			// Recursive wild segment	// Officially recursive wild
	} SegType;

typedef enum		// State machine states
	{
	ST_FINISHED = 0,
	ST_ROOT_NT_0,
	ST_ROOT_NT_1,
	ST_ROOT_TL_0,
	ST_NORM_NT_0,
	ST_NORM_NT_1,
	ST_NORM_TL_0,
	ST_NORM_TL_1,
	ST_NORM_TL_2,
	ST_WILD_NT_0,
	ST_WILD_NT_1,
	ST_WILD_NT_2,
	ST_WILD_TL_0,
	ST_WILD_TL_1,
	ST_WILD_TL_2,
	ST_RWLD_NT_0,
	ST_RWLD_NT_1,
	ST_RWLD_NT_2,
	ST_RWLD_NT_3,
	ST_RWLD_TL_0,
	ST_RWLD_TL_1,
	ST_RWLD_TL_2,
	ST_RWLD_TL_3
	} FwildState;


#define MAX_XNAME	(32)	// Max number of exclusion names
#define MAX_XCHAR	(32)	// Max number of exclusion name chars

typedef				// Exclusion interface item (fWild interface)
	struct
		{
		int		drive;				// Drive number; 0 if network
		int		type;				// File/Directory type (masked)
		int		depth;				// Directory depth
		char   	name [MAX_PATH];	// Item name (monocase upper)
		}
	EXITEM, *PEXI;

typedef				// Exclusion table (internal only)
	struct
		{
		PVOID	magic;				// Protect against invalid use
		int		onceOnly;			// Ensure single initialization
		int		enbExcl;			// TRUE to enable default exclusion config
		int		showConf;			// TRUE to show exclusion configuration
		int		showExcl;			// TRUE to show exclusions (real time)
		int		listCount;			// Number of exclusion names
		EXITEM	list [MAX_XNAME];	// Exclusion name list
		}
	EXtable, *PEXtable, *PEX;


typedef				// DTA object used by FindFirstFile() and cousins
	struct
		{
		int			valid;					// TRUE when WFD has found file data
		int			SearchAttr;				// Search file attribute bitmap
		int			dta_type;				// Found file attribute bitmaps
		UINT64		dta_size;				// Found file size
		time_t		dta_fdt;				// Found file date/time
		time_t		dta_mtime;				// Found file Microsoft date/time
		char		dta_name [FWP_MAX];		// Found file filename
		HANDLE		sh;						// The Windows Find* search handle
		WIN32_FIND_DATA	 wfd;				// The WIN32 FIND_DATA structure
		}  FW_DTA, *PFW_DTA;

typedef				// Segment object
	struct
		{
		int			index;					// The segment index within the array
		int			terminal;				// TRUE if the last segment
		SegType		type;					// The segment type
		int			delta;					// Applicable change to directory depth
		char	   *pBegin;					// Ptr to the beginning of the pattern segment
		FwildState	levelState;				// The initial search state
		char		buffer		[FWP_MAX];	// Segment buffer
		} FW_SEG, *PFW_SEG;					// Initialized to zero by fwInit()

typedef				// DTE level element
	struct // dtaele
		{
		int			index;					// Search level index (element index)
		void	   *pHdr;					// Point to the FW_HDR instance			 
		FwildState	state;					// The current level search state
		int			depth;					// Current path depth
		PFW_SEG		pSeg;					// Ptr to the current associated segment
		int			segIndex;				// Current associated segment index
		char	   *patConcat;				// Pointer to the current Found concat point
		char	   *fndConcat;				// Pointer to the current Search concat point
		FW_DTA		dta;					// The DTA used by findf/findn/findc
		char		pattern		[FWP_MAX];	// Current search path
		char	    found		[FWP_MAX];	// Current found path
		} FW_LEV, *PFW_LEV;

typedef				// Basic fWild instance root object
	struct
		{
		PVOID		magic;					// Protect against invalid use
		int			running;				// TRUE to keep the state machine running;
		int			SearchAttr;				// Attributes used for searching inside the library
		int			CallerAttr;				// Caller's search file attributes bitmask
		int			rootDepth;				// Initial path depth
		int			curLevel;				// Current element array active level
		int			segmentCount;			// Number of segments compliled

		int			foundFlag;				// Search was successful
		char	   *file_name;				// Pointer to the last found filespec
		time_t		file_fdt;				// The found file timestamp
		char		file_type;				// The found file type attribute
		UINT64		file_size;				// The found file size

		PEXtable	xp;						// Ptr to the file exclusion instance
		EXITEM		xItem;					// EXITEM structure for file exclusion interface

		FW_LEV		level		[LEV_MAX];	// Search element array
		FW_SEG		segment		[SEG_MAX];	// Segment element array
		char		pattern		[FWP_MAX];	// Working copy of the pattern
		} FW_HDR, *PFW_HDR, *PFWH, *PHP;	// Initialized to zero by fwOpen()


typedef				// FileInfo search instance
	struct
		{
		void	   *magic;					// Validity check value
		FW_DTA		dta;					// Finder instance search block
		} FI, *PFI;


typedef				/* File Pair (FP) structure */
	struct
		{
		int			sentinel;				// Sentinel to detect free() error
		int			index;					// Index into fnp1 for tail part
		int			dflag;					// TRUE if fnp2 should be constructed
		FW_HDR	   *hp;						// Pointer to fWild structure
		char	   *s2;						// Pointer to the constructed dest path
		char	   *fnp1;					// Pointer to first filename
		char	   *fnp2;					// Pointer to second filename
		}  FP, *PFP;


#else // (not FWILD_INTERNAL)

typedef void   *PFWH;
typedef void   *PHP;
typedef void   *PEX;
typedef void   *PEXI;
typedef void   *PFP;
typedef void   *PFI;
typedef void   *PVOID;

#endif // FWILD_INTERNAL

#if defined(_FILE_DEFINED)
#define  _PTOFILE	FILE *
#else
#define  _PTOFILE	void *
#endif

/* ------------------------------------------------------------------------ *\
|  Miscellaneous definitions
\* ------------------------------------------------------------------------ */

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

#define DriveIndex(ch) ((toupper(ch) - 'A') + 1)

/* ------------------------------------------------------------------------ *\
|  Function prototypes
\* ------------------------------------------------------------------------ */

extern	PFI		FileInfoOpen (int, const char *);		// Open file information instance
extern	char   *FileInfoName (PFI);						// Get file Name (no path)
extern	int		FileInfoAttr (PFI);						// Get file Attributes
extern	UINT64	FileInfoSize (PFI);						// Get file Size
extern	time_t	FileInfoFDT (PFI);						// Get file FTD
extern	time_t	FileInfoMtime (PFI);					// Get Microsoft time FDT
extern	PFI		FileInfoClose (PFI);					// Close file information instance

extern	PEX		fExcludeOpen (void);					// Open the file inclusion instance
extern	PEX		fExcludeClose (PEX);					// Close the file inclusion insstance
extern	int		fExclude (PEX, char *);					// Exclude a file/dir name	
extern	int		fExcludeCheck (PEX, PEXI);				// Check exclusion of a file/dir name	
extern	int		fExcludeConnect (PEX, PFWH);			// Check exclusion of a file/dir name	
extern	void	fExcludeDefEnable (PEX, int);			// Enable default file exclusion config
extern	void	fExcludeShowConf (PEX, int);			// Show the file exclusion configuration
extern	void	fExcludeShowExcl (PEX, int);			// Show file/dir exclusions in stdout
extern	int		fExcludePathCheck (PEX, char *);		// Check a path for exclusion

extern	int		fwActive (PFWH);						/* Check a PFWH for valid and active */
extern	PFWH	fwOpen (void);							/* Open fWild instance */
extern	int		fwInit (PFWH, const char *, int);		/* Initialize fWild search */
extern	char   *fWild (PFWH);							/* Return a wild filename */
extern	void	fwExclEnable(PFWH, PEX, int);			/* Enable fWild file exclusion */
extern	PFWH	fwClose (PFWH);							/* Close fWild instance */
//extern char  *fwFirst (const char *);					/* Return the first matching wild filename (deprecated) */
extern  int		fwdepth (PFWH);							/* Return the directory depth of the current fn */
extern  int		fwtype (PFWH);							/* Return the type of the current fn */
extern  char   *fwdate (PFWH);							/* Return the date of the current fn */
extern	char   *fwLastFound (PFWH);						/* Return ptr to the last found filespec */
extern  char   *fwtime (PFWH);							/* Return the time of the current fn */
extern  char   *fwtimes (PFWH);							/* Return the time of the current fn (secs) */
extern  time_t	fwgetfdt (PFWH);						/* Return the current file timedate */
extern  UINT64	fwsize (PFWH);							/* Return the size of the current fn */
extern	int		fwValid (const char *);					/* Validate a file/path string */
extern	char   *fwerror (void);							/* Return the fwerrno error string */
extern  char   *volName (const char *);					/* Return vol name of the spec drive (path) */
extern  char   *_volName (int);							/* Return vol name of the spec drive */
extern	int		fgetattr (const char *);				/* Get file attributes (filename) */
extern	int		hgetattr (int fh);						/* Get file attributes (handle) */
extern	int		fgetfdt (const char *, time_t *);		/* Get timedate (filename) */
extern	time_t	hgetfdt (int);							/* Get timedate (handle) */
extern	int		fgetsize (const char *, PUINT64);  		/* Get file size (filename) */
extern	int		hgetsize (int, PUINT64);			    /* Get file size (handle) */
extern	int		fsetattr (const char *, int);			/* Set file attr (filename) */
extern	int		fsetfdt (const char *, time_t);			/* Set timedate (filename) */
extern	time_t	zerotime (void);						/* Return lowest usable UNIX time */

extern	int		getopt (int, char **, char *);			/* Decode command options */
extern	int		optvalue								/* Get numeric option value */
		    		(char *, long *, long, long);
extern	char   *optvalerror (void);						/* Return optval() error string */
extern	int		expression (char *, long *);			/* Evaluate numeric expression */
extern	char   *expression_error (void);				/* Return expression() error string */

extern  int		_fnabspth (char *, const char *);		/* Return absolute name of the fn */
extern  char   *fnabspth (const char *);				/* Return absolute name of the fn */
extern	int		fnBuildPath (const char *);				// Verifies/constructs a filespec path
extern  int		_fncatpth (char *, const char *, const char *);	/* Return concatenated name to buffer */
extern  char   *fncatpth (const char *, const char *);	/* Return concatenated name (allocated) */
extern  char   *fntail (const char *);					/* Return the tail of the filename (raw) 	*/
extern  char   *_fntail (const char *);					/* Return the tail of the filename (post prefix)*/
extern	int		fnsubpth (const char *, const char *);	/* Check if s1 is a subpath of s2 */
extern	int		fnchkdir (const char *);				/* Test if a directory exists */
extern	int		fnchkdir (const char *);				/* Test if a directory exists */
extern	int		fnCondense(char *);						/* Condense a non-prefix pathname */
extern	int		fnchkdir (const char *);				/* Test if a directory exists */
extern	int		fnchkfil (const char *);				/* Test if a file exists */
//extern int 	fnchkunc (const char *);				/* Test if a UNC root (deprecated) */
extern  int		_fnGetPath (const char *);				// Extracts buildable path (internal only)
extern  int		fnGetPath (char *, const char *);		// Extracts buildable path 
extern  int		fnMakePath (void);						// Verifies/constructs buildable path
extern  int		fnreduce (char *);						/* Reduce a filename */
extern	int		fnValidName (const char *);				/* Test if a path is valid nonwild filename syntax */
extern	int		fndot (const char *);					// Test if a "dot" directory
extern	int		fnFileType (const char *);				// Ptr to the file/pathspec
//extern int	fnmatch (const char *, const char *, int);	/* Match a wild file name segment  (deprecated) */
extern	int		fnMatch (const char *, const char *);	/* Match a wild file name segment*/
//extern int	fnmatch2 (const char *, const char *);	/* Match a wild file name segment (deprecated) */
extern	int		isDirectory (const char *);				// Test if an existing directory
extern	int		isFile (const char *);					// Test if an existing file
extern	int		isPhysical (const char *);				// Test if path is physically valid
extern  int		isWild (const char *);					// Test if a "wild" path (has '?' or '*')
extern	int 	fnParse (const char *, int *, int *);	/* Parse a filename path */
//extern int	pmatch (char *, char *, int);			/* Test two fnames for match (deprecated) */
//extern int	psubmatch (char *, char *, int);		/* Test two fnames for submatch (deprecated) */
//extern int	MakePath (const char *);				/* Build a path (deprecated) */

extern	int		_nameMatch (const char *pPat, const char *pNam, int); // Test file/dir name for acceptance
extern	int		pnMatch (const char *, const char *);	/* Match against a wild file name path */
extern	int		pnOverlap (const char *, const char *);	/* Check for overlap between two paths */
extern	int		pcreat (const char *, int);				/* Build path and creat() a file */
extern	int		popen (const char *, int, int);			/* Build path and open() a file */
extern	int		prename (const char *, const char *);	/* Build path and rename() a file */
//extern char  *QueryDrivePrefixMulti (const char *);	/* Query if pathspec points a drive spec (multi) */
extern	char   *QueryUNCPrefix (const char *);			/* Query if pathspec points a UNC spec */
extern	char   *QueryDrivePrefix (const char *);		/* Query if pathspec points a drive spec (single) */
extern	char   *QueryRootPrefix (const char *);			/* Query if pathspec points a drive spec (single) */
extern	char   *PointPastPrefix (const char *);			/* Returns ptr past any prefix */
extern	int		isPrefixed (const char *);				// Query if pathspec points any prefix
extern	int		isRooted (void);						// "rooted" property of last PointPastPrefix() call
extern	int		isSameDevice (const char *, const char *);	// Query if two pathspecs are same device

extern	char   *pathCat (char *, const char *, int);	// Concatenate a path (protected)
extern	char   *pathCopy (char *, const char *, int);	// Copy a path (protected)
extern	char   *pathDup (const char *);					// Duplicate a path (MAX_PATH compliant)
extern	char   *fpathDup (const char *);				// Duplicate a path but fatal if fail

extern	char   *showFileAttr (int);						// Show file attributes in a string */

extern	time_t	TimeToUnixTime (time_t);				// Returns the timestamp in unix format
extern	time_t	UnixTimeToTime (time_t);				// Returns the timestamp in Windows format

#ifdef _FILE_DEFINED
extern  FILE   *pfopen (const char *, const char *);
extern	void	HexDump (FILE *, void *, unsigned);		/* Perform hexdump to file */
#endif

extern  char	strpath (const char *);					/* Return the path char of the fn */
extern  void	strsetp (char *, char);					/* Set the path char in a string */

extern	int		isValidChar (char, int);				/* Validate command line characters */
extern	int		isValidPath (char);						/* Validate path characters */
extern	int		_isValidDrive (int);					/* Validate drive by drive number */
extern	int		isValidDrive (const char *);			/* Validate drive by drive name */
extern	int		isValidUNC (char);						/* Validates UNC \\server\share characters */
extern  char	egetpath (void);						/* Return the path char of the env */
extern  char	getpath (void);							/* Return the path char of DOS */
extern  char	egetswch (void);						/* Return the switch char of the env */

extern	int 	_getdepth (int);						/* Get the CWD directory depth from drive number */
extern	int 	GetDepth (const char *, int *);			/* Get the CWD directory depth from path */
extern	char   *__getDir (int);							/* Point the CWD by drive number */
extern	char   *_getDir (const char *);					/* Point the CWD by drive name */
extern	char   *getDir (const char *);					/* Get the allocated CWD by drive name */
extern	int		getdrive (void);						/* Get the default drive */
extern	int		MakePrefixLetter (char, char *);		/* Make a drive prefix (letter) */
extern	int		MakePrefixNumber (int, char *);			/* Make a drive prefix (number) */
extern	int		getcols (void);							/* Get the console column count */
extern	int		getrows (void);							/* Get the console row count */

extern  char   *stpblk (char *);						/* Skip leading white space */
extern  char   *fmalloc (unsigned int);					/* malloc() with a fatal error */
//extern char  *fstrdup (const char *);					/* strdup() with a fatal error */
//extern char  *strdupMax(const char *);				/* strdup always with size MAX_PATH (deprecated) */
//extern char  *fstrdupMax(const char *);				/* strdupMax with a fatal error (deprecated) */
extern  void	fatalerr (const char *);				/* Die an unnatural death */

extern  int		execute (char *, int);					/* Execute a child process */
extern	int		fcopy (const char *, const char *, int);	/* Copy a file */
extern	int		fp_init (PFP *, int, char *, char *);	/* Init fpair */
extern	int		fp_pair (PFP, char **, char **);		/* Return fpair */

/* Far memory manipulation support */

#if defined(M_I86CM) || defined(M_I86LM) || defined(_WIN32)
#define  farcmp(a, b, c)	memcmp((a), (b), (c))
#define  farcpy(a, b, c)	memcpy((a), (b), (c))
#define  farset(a, b, c)	memset((a), (b), (c))
#else
extern	int		farcmp (void far *, void far *, unsigned int);
extern	void	farcpy (void far *, void far *, unsigned int);
extern	void	farset (void far *, int value,  unsigned int);
#endif

/* Time/date string parsing support */

extern	char   *strdate  (time_t);						/* Convert a DOS timedate to a date string */
extern	char   *strtime  (time_t);						/* Convert a DOS timedate to a time string */
extern	char   *strtime2 (time_t);						/* Convert a DOS timedate to a time string */
extern	time_t	sgettd   (char *);						/* Parse a TIMEDATE string */
extern	time_t	fwsgettd (char *);						/* Parse a TIMEDATE string (filename) */
extern	char   *serrtd   (void);						/* Return TD error string */
extern	char   *fwserrtd (void);						/* Return TD error string (filename) */
extern  char   *strtoken (char *, char *, TOKENCB *);

#ifdef  FWILD_INTERNAL

extern	int		FinderOpen (PFW_DTA, const char *);	// Finder open
extern	int		FinderFile (PFW_DTA, char *);			// Finder get filename
extern	void	FinderClose (PFW_DTA);					// Finder close

extern	int		_findf (FW_DTA *, const char *, int);	/* Dual mode (open + find) */
extern	int		_findo (FW_DTA *, const char *, int);	/* Open */
extern	int		_findn (FW_DTA *);						/* Find */
extern	void	_findc (FW_DTA *);						/* Close */
extern  int		AttrFromWin32 (DWORD);		 			/* Convert WIN32 attr to FWILD attr */
extern  DWORD	AttrToWin32 (int);						/* Convert FWILD attr to WIN32 attr */
#endif

extern	char	optswch;								/* The option switch character */
extern	char	optchar;								/* The last option character */
extern	int		optind;									/* The current argv[] index */
extern	char   *optarg;									/* Pointer to option parameter */
extern	char   *optenv;									/* Pointer to environment options */
extern	int		fwerrno;								// fwValid() error result

/* Template program support */

extern	char   *usagedoc [];							/* Usage documentation pointer array */

extern	void	fwInitError	(const char *fnp);			/* Report fwInit error */
extern	void	cantfind	(const char *fnp);			/* Report can't find inout file */
extern	void	cantopen 	(const char *fnp);			/* Report can't open inout file */
extern	void	usage    	(void);						/* Report usage following error */
extern	void	help     	(void);						/* Report help on request */
extern	void	docprint 	(const char *s[]);			/* Print help support */

extern	void	fmemdump  (								/* Dump n bytes of memory */
		    _PTOFILE, char *, int, long);

/* ------------------------------------------------------------------------ *\
|  fcopy () definitions
\* ------------------------------------------------------------------------ */

/* Control flags (bit-mapped) */

#define  FCF_ROK	(1)	/* Copy even if dest is RO */
#define  FCF_FDT	(2)	/* Duplicate the src fdt */
#define  FCF_ATTR	(4)	/* Duplicate the src attributes */

/* Result (value) */

#define  FCR_SUCCESS	(0)	/* Successful copy */
#define  FCR_RO			(1)	/* RO and not FCF_ROK */
#define  FCR_OPN1		(2)	/* Can't open file 1 (src) */
#define  FCR_OPN2		(3)	/* Can't open file 2 (dest) */
#define  FCR_READ		(4)	/* Error reading file 1 (src) */
#define  FCR_WRITE		(5)	/* Error writing file 2 (dest) */
#define  FCR_GFDT		(6)	/* Unable to get src datetime */
#define  FCR_GATTR		(7)	/* Unable to get src attribute */
#define  FCR_SFDT		(8)	/* Unable to set dest datetime */
#define  FCR_SATTR		(9)	/* Unable to set dest attribute */

#define  FPR_SUCCESS	(0)	/* Successful */
#define  FPR_NOFILE		(1)	/* No file pair returned */
#define  FPR_P2WILD		(2)	/* Path2 cannot be wild */
#define  FPR_P2FILE		(3)	/* Path2 cannot be a file */
#define  FPR_MEMORY		(4)	/* Insufficient memory */
#define  FPR_FWERROR	(5)	/* fWild() error */

/* ------------------------------------------------------------------------ *\
|  fwerrno definitions
\* ------------------------------------------------------------------------ */

#define  FWERR_NONE			(0)		// No error, pathname is good
#define  FWERR_NULL			(1)		// NULL pathname pointer passed
#define  FWERR_EMPTY		(2)		// Empty pathname passed
#define  FWERR_DRIVE		(3)		// Improper drive spec
#define  FWERR_SEPARATOR	(4)		// Improper separator spec
#define  FWERR_ELEMENT		(5)		// Improper path element spec
#define  FWERR_SIZE			(6)		// Invalid name field size
#define  FWERR_TRAIL		(7)		// Trailing "\" following element
#define  FWERR_INVALID		(8)		// Invalid pattern
#define  FWERR_INSTANCE		(9)		// Not a valid instance structure
#define  FWERR_PHYSICAL		(10)	// Non-physical device
#define  FWERR_NOMEM		(11)	// Not enough memory

/* ------------------------------------------------------------------------ *\
|  Exit code definitions
\* ------------------------------------------------------------------------ */

#define  FWEXIT_SUCCESS				(0)	// Never fatal
#define  FWEXIT_FILE_NOT_FOUND		(1)	// Optional fatal
#define  FWEXIT_MINOR_ERROR			(2)	// Optional fatal

#define  FWEXIT_MAJOR_ERROR			(3)	// Always fatal
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
		
