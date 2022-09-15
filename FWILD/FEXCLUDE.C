// -----------------------------------------------------------------------
//
//						   FEXCLUDE.C
//				Wild Card File Name Exclusion Server
//
//			Copyright (c) 1993, all rights reserved
//						Brian W Johnson
//							17-Feb-93
//							17-Aug-97
//							16-Jun-22	Added default exclusion path mechanism
//							 1-Sep-22	Revised the exclusion file subsystem
//											for higher performance
//
//		For now, only local files are considered for exclusion
//
// -----------------------------------------------------------------------
//
//	Each exclusion spec can specify the following:
//		Drive letter (and colon) to specify only on the specified drive.
//		Rooted path to specify the pattern is root anchored.
//		Otherwise the pattern is tail anchored.
//		Ordinary wild name, but not recursive wild filenames.
//		Filename extensions may not be inferred; must be explicit.
//
// -----------------------------------------------------------------------

#include  <windows.h>
#include  <stdio.h>
#include  <stdlib.h>
#include  <ctype.h>
#include  <string.h>

#define  FWILD_INTERNAL		/* Enable FWILD internal definitions */

#include  "fwild.h"
#include  "fexclude.h"

// -----------------------------------------------------------------------
// Private variables
// -----------------------------------------------------------------------

static int			InstanceCount = 0;			// Instance count, suppress nested instances

static PExclusion	root 		= NULL;			// Root of the Exclusion structure list
static int			showConf	= FALSE;		// Show excluded config,  default no
static int			showExcl	= FALSE;		// Show excluded paths,   default no
static int			enbExcl		= TRUE;			// Enable excluded paths, default yes
static int			onceOnly	= TRUE;

static int			UnRootedPatternCount = 0;	// Count of non-root patterns

// -----------------------------------------------------------------------
// Private data
// -----------------------------------------------------------------------

// Internal default file exclusion list for Windows 10
// This is used only if there is no external default file list file.

static	char	*DefExcludeFiles [] = 
					{
					"\\$RECYCLE.BIN",
					"\\System Volume Information",
					"\\$WinREAgent",
					"\\DumpStack.log",
					"\\DumpStack.log.tmp",
					"\\pagefile.sys",
					"\\swapfile.sys",
					"\\hiberfile.sys",
					NULL
					};

// Exclusion files can contain comment lines, lines beginning with '~'.

// -----------------------------------------------------------------------
// Private definitions
// -----------------------------------------------------------------------

#define isPathChar(ch) ((ch == '\\')  ||  (ch == '/'))	// Match either possible path character

// -----------------------------------------------------------------------
// File classification data for the target path
// -----------------------------------------------------------------------

typedef
	enum
		{
		Local = 0,				// And is the startup default
		Remote,
		Foreign					// Effectively undefined as of now
		}
	Access;

typedef
	struct targetprops
		{
		Access	access;			// Target path Access category
		char	drive;			// UC drive letter if local, else zero if network
		Mode	mode;			// Mode: Rooted or UnRooted
		int     depth;			// Directory depth
		char   *begPath;		// Beginning ptr to filename buffer
		char   *endPath;		// Ending ptr to filename buffer
		Segments path;			// Path segment bodies
		} TargetProps;

TargetProps	targetProps;

// -----------------------------------------------------------------------
// Private methods
// -----------------------------------------------------------------------
// Show target props
// -----------------------------------------------------------------------
	static void
ShowTargetProps (void)			// Show Target object properties

	{
	printf("Target:\n");
	printf("  Access  %02X\n",  targetProps.access);
	printf("  Drive   %02X\n",  targetProps.drive);
	printf("  Mode    %02X\n",  targetProps.mode);
	printf("  Depth   %02X\n",  targetProps.depth);
	printf("  begPath %08lu\n", (int)targetProps.begPath);
	printf("  endPath %08lu\n", (int)targetProps.endPath);
	printf("  Path0  \"%s\"\n", targetProps.path[0]);
	printf("  Path1  \"%s\"\n", targetProps.path[1]);
	}

// -----------------------------------------------------------------------
// Show target props
// -----------------------------------------------------------------------
	static void
ShowExcProps (					// Show Exclusion object properties
	PExclusion pExc)

	{
	printf("Excl:\n");
	printf("  Drive  %02X\n",  pExc->drive);
	printf("  Mode   %02X\n",  pExc->mode);
	printf("  Depth  %02X\n",  pExc->depth);
	printf("  Path0 \"%s\"\n", pExc->path[0]);
	printf("  Path1 \"%s\"\n", pExc->path[1]);
	}

// -----------------------------------------------------------------------
// Copy the pathspec segment(s) into an exclusion record, or into a candidate record
// -----------------------------------------------------------------------
	static int					// Return depth, or -1 for too many segments
copy_segments (					// Copy up to 2 segments into the Segments structure
	char      *pattern,			// From: Pointer to the pattern string, first segment
	Segments  pSeg)				// To:   Pointer to the Segments structure

	{
	int   seg = 0;				// Count segments from zero (is depth - 1)
	char *p   = pattern;		// Init ptr p to the pattern
	char *q   = pSeg[seg];		// Init ptr q for the first segment

	*pSeg[0] = '\0';			// Init both segments to empty
	*pSeg[1] = '\0';

	for (;;)					// Copy the segments (up to MAX_EXCL_SEGMENT)
		{
		if (*p == '\0')			// If end of pattern reached, done
			{
			*q = '\0';			// Terminate the current segment
			break;				// Finished, successful
			}

		else if (isPathChar(*p))
			{
			*q = '\0';			// Terminate the current segment
			if (++seg >= MAX_EXCL_SEGMENT)	// Move to next segment
				break;			// Error, too many segments
			++p;				// Skip over the found path character		
			q = pSeg[seg];		// Init ptr q for the next segment
			}

		else
			*q++ = *p++;
        }

	// Returns the segment copy depth, or (-1) if too many segments to copy
	return ((seg < MAX_EXCL_SEGMENT) ? (seg + 1): (-1));
	}

// -----------------------------------------------------------------------
// Initialize and add an exclusion record to the exclusion record list
// -----------------------------------------------------------------------
	static int					// Return non-zero if an error
add_list (						// Add a string to the exclusion list
	char  *pattern)				// Pointer to the pattern string

	{
	int         length = (pattern != NULL) ? strlen(pattern) : (0);
	int         index;			// Index into the pattern
	int         depth  = 0;		// Depth of the pattern
	PExclusion  pExc;

	if ((length > 0)
	&&  ((pExc = (PExclusion)(calloc(sizeof(Exclusion), 1))) != NULL))
		{

		// Init the drive spec

		index = 0;						// Point the pattern buffer
		pExc->drive = 0;				// Assume no drive spec
		if ((length >= 2)
		&&  (pattern[1] == ':')
		&&  (isalpha(pattern[0])))
			{
			pExc->drive = toupper(pattern[0]);
			index    = 2;				// Skip over the drive spec
			}

		// Init the root mode

		pExc->mode = (isPathChar(pattern[index]))
			? Rooted
			: UnRooted;
		if (pExc->mode == Rooted)
			++index;					// Skip over the root path character

		// Copy each of up to 2 path elements, and update the path depth

		if ((pExc->depth = copy_segments(&pattern[index], &pExc->path[0])) < 0)
			{
			if (showConf)
				{
				printf("Exclusion pattern error: \"%s\"\n", pattern);
				fflush(stdout);
				}

			free(pExc);		// Error, free the exclusion buffer
			return (-1);	// and report fatal error for too many segments required
			}

		// Successful initialization, link it into the exclusion record list

//ShowExcProps(pExc);

		if (pExc->mode == UnRooted)
			++UnRootedPatternCount;
		pExc->link = root;				// List link the new Exclusion record
		root       = pExc;
		}

	if (showConf)
		{
		printf("Excluding: %s\n", pattern);
		fflush(stdout);
		}

	return (0);		// Candidate exclusion record accepted
	}

// -----------------------------------------------------------------------
// Clear the exclusion record list, freeing all exclusion records
// -----------------------------------------------------------------------
	static void
clear_list (void)				// Clear out the exclusion list

	{
//printf("clear_list called\n");
	PExclusion  p;
	
	while (root != NULL)
		{
		p = root->link;	// Point the second element
		free(root);		// Free the top element
		root = p;		// Update the root
		} 
	}

// -----------------------------------------------------------------------
// Get the default exclusion file pathname 
// -----------------------------------------------------------------------
	static char *
GetInitPath (void)				// Get the pathname of the default exclusion file

	{
    DWORD	length;
	char  *pDest;
static char Dest [MAX_PATH];

	if ((length = GetModuleFileName(NULL, Dest, sizeof(Dest))) == 0)
		return (NULL);

	fnreduce(Dest);
	Dest[strlen(Dest) - strlen(fntail(Dest))] = '\0';
	pDest = fncatpth(Dest, "exclude.ini");

	return ((strlen(pDest)  > 0) ? pDest : NULL);
	}

// -----------------------------------------------------------------------
// Create exclusion records from an exclusion file
// -----------------------------------------------------------------------
	static int					// Return non-zero if a file error
fexcludeFromFile (				// Install an exclusion pattern
	char* pfn)					// Exclusion File pathname

	{
	FILE* fp;			// Handle of the opened exclusion file
	char  line[4096];	// Init file line buffer

	if (showConf)				// Display the comment, if enabled
		{
		printf("ExFile:    %s\n", pfn);
		fflush(stdout);
		}

	if ((pfn == NULL)
	|| ((fp = fopen(pfn, "r")) == NULL))
		return (-1);		// File read failed, fall back to internal defaults

	while (fgets(line, sizeof line, fp))
		{
		if (strlen(line) > 0)
			fexclude(line);
		}

	fclose(fp);
	return (0);			// File read suceeded
	}

// -----------------------------------------------------------------------
// Count the path depth of a path, update properties
// -----------------------------------------------------------------------
	char *						// Return the ptr to the end of the path
countDepth (					// Count the path depth
	int init,					// Initial value of the count
	int  *pDepth,				// Ptr to yhe depth property
	char *s)					// Ptr to the path

	{
	int depth  = init;

	for ( ; (*s); ++s)
		if (isPathChar(*s))
			++depth;				// Count the depth

	// Update the targetProps

	if (pDepth)
		targetProps.depth = depth;	// depth property
	return(s);
	}

// -----------------------------------------------------------------------
// Classify a candidate excluded file
// -----------------------------------------------------------------------
	static Access				// Return the target file classification type
classify 						// Classify the target file
	(char *target)				// Target File pathspec

	{
	if ((target[0] == '\\')
	&&  (target[1] == '\\')
	&&  (isalpha(target[2])))
		{
		targetProps.access = Remote;	// No further exclusion testing for remote
		return (targetProps.access);
		}

	// Local access

	int index = 0;				// Index into the target string first element

	targetProps.access = Local;						// access property
	targetProps.drive = 0;		// Assume no drive spec
	if (isalpha(target[0])		// Check if local file access
	&&  (target[1] == ':'))
		{
		targetProps.drive = toupper(target[0]);		// drive property
		index = 2;	// Skip over the drivespec
		}

	if (isPathChar(target[index]))	// (If Rooted)
		{
		++index;	// Skip over the pathchar
		targetProps.mode = Rooted;					// mode property
		targetProps.begPath = &target[index];		// begPath property
		targetProps.endPath =						// endPath property
			countDepth(1, &targetProps.depth,		// depth   property
			&target[index]);
		}

	else // (the path is UnRooted)
		{
		targetProps.mode = UnRooted;				// mode property
		targetProps.begPath = &target[index];		// begPath property
		targetProps.endPath =						// endPath property
			countDepth(1, NULL, &target[index]);	// (ignore the depth property)

		if (onceOnly)	// Do this only once
			{
			onceOnly = FALSE;
			char *altTarget = fnabspth(target);		// Make an alternate rooted target

			targetProps.drive
				= toupper(altTarget[0]);			// drive property
			countDepth(1, &targetProps.depth,		// depth property (Initial target only)
				altTarget);							// (ignore the endPath value)
			free(altTarget);
			}
		}

	return (targetProps.access);

#if 0	// The following is not useful at the moment, and awaits another day
	else		// Must be network file access, if not local
		{
		char *p;				// Target path traversal pointer
		int sepCount = 0;		// Count of network separators
		for (p = target; (*p); ++p)
			if (*p == '/')		// Skipping over //host/share/rootpath...
				if (++sepCount >= 4)
					break;
#endif

    }

// -----------------------------------------------------------------------
// Test a candidate file for exclusion
// -----------------------------------------------------------------------
	static int				// Return TRUE to exclude the file, else FALSE
exclusionTest (				// Test this candidate for exclusion
	PExclusion pExc)		// Pointer to the target Exclusion struture

	{
	char *pBegin;

//printf("\nexclusionTest\n");
//ShowExcProps(pExc);

	if (pExc->mode == UnRooted)		// For the UnRooted case, count backwards from the end
		{
		pBegin = targetProps.endPath;
		int countDown = pExc->depth;
		for (;;)
			{
			if (isPathChar(*(pBegin-1)))
				if (--countDown <= 0)
					break;
			--pBegin;
			}
		}
	else							// For the Rooted case, start at the beginning
		pBegin = targetProps.begPath;

	copy_segments(pBegin, &targetProps.path[0]);

//ShowTargetProps();

	int result = TRUE;
	for (int seg = 0; (seg < pExc->depth); ++seg)
		{
		if ((pExc->path[seg][0] != '\0') && (fnmatch(pExc->path[seg], targetProps.path[seg], FALSE) == FALSE))
			{
			result = FALSE;
			break;
			}
		}

//printf("Target result: (%s) \n", (result ? "TRUE" : "FALSE"));
	if (showExcl  &&  result)
		{
		printf("Excluded: %s\n", targetProps.path[0]);
		fflush(stdout);
		}

	return (result);
	}

// -----------------------------------------------------------------------
// Test a candidate file for exclusion
// Patterns may be Rooted or UnRooted on entry
// All targets are Rooted on entry
// -----------------------------------------------------------------------
	static int				// Return TRUE to exclude the file, else FALSE
exclusionFilter (			// Filter this candidate for exclusion
	char *target)			// Target pathspec to check

	{
	PExclusion  pExc;				// The current candidate exclusion pattern
	int          result = FALSE;	// The returned result

	// Apply various exclusion rules

//printf("Target: \"%s\"\n", target);
//ShowTargetProps();

	for (pExc = root; (pExc != NULL); pExc = pExc->link)
		{
//printf("exclusionFilter testing against \"%s\"\n", pExc->path[0]);

		if ((isalpha(pExc->drive))  &&  (targetProps.drive != pExc->drive))
			{
//printf("Blocked drive check\n");
			continue;			// If valid drive spec, and target drive mismatches, then can't apply the exclusion
			}

		if ((pExc->mode == Rooted)  &&  (targetProps.depth != pExc->depth))
			{
//printf("Blocked rooted check\n");
			continue;			// Can't apply Rooted pattern at other than at root level
			}					// (The Rooted case requires equal depths; is only appllied at root level)

//BWJ needs work
		if ((pExc->mode == UnRooted)  &&  (targetProps.depth < pExc->depth))
			{
//printf("Blocked depth check\n");
			continue;			// Can't apply any pattern if insufficient target depth
			}

//printf("Target: \"%s\"\n", target);
//ShowTargetProps();
		result = (exclusionTest(pExc));	// Perform the actual exclusion test
    
		if (result)
			return (TRUE);
	    }

	return (result);
	}

// -----------------------------------------------------------------------
// Global methods
// -----------------------------------------------------------------------
//Test a candidate file for exclusion
// -----------------------------------------------------------------------
	int							// Return TRUE to exclude the target, else FALSE to include it
fexcludeCheck (					// Check if a file/path is in the exclusion list
	char *target)				// Target pathspec to check

	{
	int  result = FALSE;		// The returned result

//printf("target: \"%s\"\n", target);
	if (targetProps.access == Remote)		// No exclusion processing for remote paths
		return (FALSE);

	if (strcmp(fntail(target), ".") == 0)	// Don't examine "dot" directories
		return (FALSE);

	if (strcmp(fntail(target), "..") == 0)
		return (FALSE);

	if (classify(target) != Local)
		return (FALSE);			// For now, only consider local files for exclusion

//if (*target == '$')
//	printf("target: \"%s\"\n", target);


//printf("target: \"%s\"\n", target);
//ShowTargetProps();

	// If there are no unrooted patterns, unrooted targets need not be converted or tested

	if ((targetProps.mode == Rooted)
	||  (UnRootedPatternCount > 0))
		result = exclusionFilter(targetProps.begPath);

	return (result);
	}

// -----------------------------------------------------------------------
// Create an exclusion record for a provided pattern, or file of patterns
// -----------------------------------------------------------------------
	int							// Return non-zero if an error
fexclude (						// Exclude a path or path file from the fwild search
	char  *pattern)				// Pointer to the pattern string

	{
//printf("fexclude called \"%s\"\n", pattern);
	if (pattern == NULL)		// Ignore NULL string pointers
		return (0);

	else if (pattern[0] == '@')	// '@' denotes pattern is the name of an exclusion file
		{
		return (fexcludeFromFile(pattern + 1));
		}

	// Remove trailing newlines, white space, etc.

	char  *p = (pattern + strlen(pattern));
	while ((p > pattern)  &&  (isspace(*(--p))))
		*p = '\0';					// NUL terminate the pattern;

	if ((pattern[0] == '\0')	// Comment empty line
	||  (!isgraph(pattern[0]))		// Comment blank or empty line
	||  (isspace(pattern[0]))		// Comment line
	||  (pattern[0] == '~'))		// Explicit comment line
		{
		if (showConf)				// Display the comment, if enabled
			{
			printf("ExComment: %s\n", pattern);
			fflush(stdout);
			}
		return (0);					// Ignore this request
		}

	return (add_list(pattern));		// Otherwise pattern IS a pattern
	}

// -----------------------------------------------------------------------
	void
fexcludeDefEnable (				// Enable/disable default exclusion paths
	int flag)					// True to enable exclusions

	{
	enbExcl = flag;
	}

// -----------------------------------------------------------------------
	void
fexcludeShowExcl (				// Enable/disable showing excluded files in real time
	int flag)					// True to show exclusions

	{
	showExcl = flag;
	}

// -----------------------------------------------------------------------
	void
fexcludeShowConf (				// Enable/disable showing exclusion paths
	int flag)					// True to show exclusions

	{
	showConf = flag;
	}

// -----------------------------------------------------------------------
	void						// Return non-zero if an error
fexcludeClean (void)

	{
	if (--InstanceCount == 0)	// Suppress nested instances
		clear_list();
	}

// -----------------------------------------------------------------------
	int							// Return non-zero if an error
fexcludeInit (
	int  *xmode)				// Pointer to the fwild.xmode variable

	{
	if (++InstanceCount == 1)	// Suppress nested instances
		{
//printf("fexcludeInit called\n");
		if (showConf)			// Display the comment, if enabled
			{
			if (enbExcl)
				printf("ExInit:\n");
			else
				printf("ExInit:    Default exclusions disabled\n");
			fflush(stdout);
			}

		if (enbExcl)

			// If default exclusions are enabled,
			// first try the default file,
			// but fail over to using the internal defaults

			{
			if (fexcludeFromFile(GetInitPath()) != 0)

				// The default file failed, so use the internal defaults instead

				{
				for (char **p = &DefExcludeFiles[0]; (*p != NULL); ++p)
					fexclude(*p);
				}
			}
		}

//	*xmode = 0; // options
//	*xmode = XD_BYPATH;
//	*xmode = XD_BYNAME;
//	*xmode = XF_BYPATH;
//	*xmode = XF_BYNAME;

	*xmode = (XD_BYPATH | XF_BYPATH);
	return (0);
	}

// -----------------------------------------------------------------------
//									EOF
// -----------------------------------------------------------------------
