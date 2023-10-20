// -----------------------------------------------------------------------
//
//						   FEXCLUDE.H
//				Wild Card File Name Exclusion Server
//
//			Copyright (c) 1993, all rights reserved
//						Brian W Johnson
//							17-Feb-93
//							17-Aug-97
//							16-Jun-22	Added default exclusion path mechanism
//							 1-Sep-22	Revised the parser subsystem
//											for higher performance
//
//
//	Each exclusion spec can specify the following:
//		Drive letter (and colon) to specify only on the specified drive.
//		Rooted path to specify the pattern is root anchored.
//		Otherwise the pattern is tail anchored.
//		Ordinary wild name, but not recursive wild filenames.
//		Filename extensions may not be inferred; must be explicit.
//
// -----------------------------------------------------------------------
//
//	static	char	*DefExcludeFiles [] = 
//					{
//					"\\$RECYCLE.BIN\\**",
//					"\\$WinREAgent\\**",
//					"\\System Volume Information\\**",
//					"\\pagefile.sys",
//					"\\swapfile.sys",
//					"\\hiberfile.sys",
//					NULL
//					};
//
// Exclusion files can contain comment lines; lines beginning with '~'.
//
// -----------------------------------------------------------------------
// Public definitions
// -----------------------------------------------------------------------

#ifndef __FEXCLUDE_H__
#define __FEXCLUDE_H__

#if defined(__cplusplus)
extern "C" {
#endif

// -----------------------------------------------------------------------
// Public interface definitions
// -----------------------------------------------------------------------

typedef
	enum
		{
		UnRooted = 0,
		Rooted
		}
	Mode;

#define MAX_EXCL_BUFFER		(4096)
#define MAX_EXCL_SEGMENT	(2)

typedef char      Segment[MAX_EXCL_BUFFER];
typedef Segment   Segments[MAX_EXCL_SEGMENT];
typedef Segments *PSegments;

typedef
	struct excl
		{
		struct excl *link;	// Link to put them in a single linked list
		char  drive;		// UC drive letter or zero
		Mode  mode;			// Mode values
		int   depth;		// Directory depth
		Segments path;		// Path segment bodies
		}
	Exclusion, *PExclusion;

// -----------------------------------------------------------------------
// Public methods
// -----------------------------------------------------------------------
	int
fexcludeCheck (			// Check if a pathspec matches any name in the exclusion list
	char *path);		// Path to check

// -----------------------------------------------------------------------
	void
fexcludeDefEnable (		// Enable/disable default exclusion paths
	int flag);			// True to enable exclusions
	
// -----------------------------------------------------------------------
	void
fexcludeShowConf (		// Enable/disable showing exclusion path configuration
	int flag);			// True to show configuration
// -----------------------------------------------------------------------
	void
fexcludeShowExcl (		// Enable/disable showing excluded paths at run time
	int flag);			// True to show exclusions

// -----------------------------------------------------------------------
	void				// Return non-zero if an error
fexcludeClean
	(void);

// -----------------------------------------------------------------------
	int					// Return non-zero if an error
fexclude (				// Exclude a path or path file from the fWild search
	char  *pattern);	// Pointer to the pattern string

// -----------------------------------------------------------------------
	int					// Return non-zero if an error
fexcludeInit (			// Initialize the eclusion system
	int  *xmode);		// Pointer to the fWild.xmode variable

// -----------------------------------------------------------------------

#if defined(__cplusplus)
}
#endif

#endif // __FEXCLUDE_H__

// -----------------------------------------------------------------------
//									EOF
// -----------------------------------------------------------------------
