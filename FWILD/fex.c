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

#include  "fWild.h"

// -----------------------------------------------------------------------
// Private data
// -----------------------------------------------------------------------

// Internal default file exclusion list for Windows 10
// This is used only if there is no external default file list file.

static	char	*DefExcludeFiles [] = 
					{
					"D0$RECYCLE.BIN",
					"D0System*",
					"D0C:$Win*",
					"F0C:DumpStack*",
					"F0C:pagefile*",
					"F0C:swapfile*",
					"F0C:hiberfile*",
					NULL
					};

// Default configuration file:

#define	INIFILE	"fex.ini"

// Exclusion files can contain comment lines, lines beginning with '~'.

// -----------------------------------------------------------------------
// Private definitions
// -----------------------------------------------------------------------
//	#define DEBUG	// Define this for debug output

#define NULCH		('\0')
#define PATHCH		('\\')

// -----------------------------------------------------------------------
//	Private methods
// -----------------------------------------------------------------------
//	Copy the pathspec pattern segment into an exclusion record
// -----------------------------------------------------------------------
	static void
segCopy (					// Copy the path segment into the exclusion record
	char   *pDst,			// Pointer to the exclusion name buffer
	char   *pSrc)			// Pointer to the name string

	{						// Copy length; leave room for "*NUL"
	int		length = (MAX_XCHAR - 2);
	char	ch;				// Character copy buffer

	while (((ch = *pSrc++) != NULCH) && (ch != PATHCH))
		{
		if (--length <= 0)	// If oversize copy,
			{
			*pDst++ = '*';	//   append '*' to the pattern string
			break;
			}
		*pDst++ = ch;		// Copy the string character
		}
	*pDst = NULCH;			// Terminate the string
	}

// -----------------------------------------------------------------------
//	Configure the exclusion name list item
// -----------------------------------------------------------------------
	static int				// Return 0, or -1 for segment overflow
configExclRecord (			// Configure an exclusion record
	PEX		xp,				// Pointer to the exclusion table
	char   *pPattern)		// Pointer to the name pattern

	{
	if (xp->listCount >= MAX_XNAME)	// Prevent overflow
		return (-1);


	int drive = 0;				// Assume all drives
	int type  = ATT_DIR;		// Assume DIR
	int depth = 0;				// Assume at root

	char *p  = pPattern;		// Point the pattern
	char  ch = toupper(*(p++));

	if (ch == 'F')				// Configure type: 'D' or 'F'
		type  = ATT_FILE;
	else if (! (ch == 'D'))
		return (-1);			// Neither 'D' nor 'F'

	int syntaxOK = FALSE;		// Configure depth, default 0
	while ((ch = *p) != NULCH)
		{
		if (! isdigit(ch))
			break;
		depth    = ((depth * 10) + (ch - '0'));
		syntaxOK = TRUE;
		++p;
		}
	if (syntaxOK == FALSE)
		return (-1);

	if (isalpha(ch = *p))	// Configure drive, default all:
		{
		if ( (*(p+1) == ':'))
			{
			drive = DriveIndex(ch);
			p +=2;
			}
		}

	// Entry validated; configure the parameters and name

	PEXI	pXname = &xp->list[xp->listCount];
 	char   *pDst   = pXname->name;

	pXname->drive = drive;	// Copy the drive info
	pXname->type  = type;	// Copy the file typ info
	pXname->depth = depth;	// Copy the septh info
	segCopy(pDst, p);		// Copy the name,
	strupr(pDst);			//   and make it monocase upper

	++(xp->listCount);		// Increment the name count

	return (0);
	}

// -----------------------------------------------------------------------
//	Show the exclusion configuration
// -----------------------------------------------------------------------
	static void
ShowConfiguration (
	PEX		xp)

	{
	PEXI	pShow;		// Working pointer to the exclusion name records
	PEXI	pLast = &xp->list[xp->listCount];

	printf("Configured exclusions [%d]:\n", xp->listCount);
	for (pShow = &xp->list[0]; (pShow < pLast); (++pShow))
		{
		printf("%c %s %d:  \"%s\"\n",
			((pShow->drive == 0) ? '-' : ('A' + (pShow->drive -1))),
			((pShow->type & ATT_FILE) ? "FILE" : "DIR "),
			pShow->depth,
			pShow->name);
		}
	}
	
// -----------------------------------------------------------------------
//	Get the default exclusion file pathname 
// -----------------------------------------------------------------------
	static char *
GetInitPath (void)				// Get the pathname of the default exclusion file

	{
    DWORD	length;
static char Dest [MAX_PATH];
	if ((length = GetModuleFileName(NULL, Dest, sizeof(Dest))) == 0)
		return (NULL);	// No filespec

	if (fnreduce(Dest) < 0)
		return (NULL);	// Invalid filespec

	Dest[strlen(Dest) - strlen(fntail(Dest))] = '\0';
	strcat(Dest, INIFILE);
	return ((strlen(Dest)  > 0) ? Dest : NULL);
	}

// -----------------------------------------------------------------------
//	Create exclusion records from an exclusion file
// -----------------------------------------------------------------------
	static int					// Return non-zero if a file error
fExcludeFromFile (				// Install an exclusion pattern
	PEX		xp,					// Pointer to the exclusion instance
	char   *pfn)				// Exclusion File pathname

	{
	FILE *fp;					// Handle of the opened exclusion file
	char  line[1024];			// Init file line buffer

	if (xp->showConf)			// Display the filename, if enabled
		{
		printf("ExclFile:  %s\n", pfn);
		fflush(stdout);
		}

	if ((pfn == NULL)
	|| ((fp = fopen(pfn, "r")) == NULL))
		return (-1);		// File read failed, fall back to internal defaults

	while (fgets(line, sizeof line, fp))
		{
		if (strlen(line) > 0)
			fExclude(xp, line);
		}

	fclose(fp);
	return (0);			// File read suceeded
	}

//----------------------------------------------------------------------------
//	Validate a FW_HDR pointer
//----------------------------------------------------------------------------
	static int
validHeader (					// Validate a PEX header pointer
	PEX		xp)					// Pointer to the header

	{
	return ((xp) && (xp->magic == xp));
	}

//----------------------------------------------------------------------------
//	Free a PEX instance header pointer
//----------------------------------------------------------------------------
	static void
freeHeader (					// Release a header and all its elements
	PEX		xp)					// Pointer to the header

	{
	xp->magic = NULL;			// Make the object invalid
	free(xp);					// Return the memory
	}

// -----------------------------------------------------------------------
//	Test a candidate file for exclusion
// -----------------------------------------------------------------------
	static int				// Return TRUE to exclude the file, else FALSE
ItemTest (					// Test a file/directory item for exclusion
	PEXI	pTest,			// Pointer to the current config record
	PEXI	pCheck)			// Pointer to the EXI to check

	{
#ifdef DEBUG
printf("CheckingItem \"%s\"   \"%s\"\n", pTest->name,  pCheck->name);
printf("TestDepth %d  CheckDepth %d\n",  pTest->depth, pCheck->depth);
printf("TestType %02X CheckType %02X\n", pTest->type,  pCheck->type);
printf("TestDrive %d  CheckDrive %d\n",  pTest->drive, pCheck->drive);
#endif
	if ( (pTest->depth != pCheck->depth)
	||   (pTest->type  != pCheck->type)
	||  ((pTest->drive != 0)  &&  (pTest->drive != pCheck->drive)))
		return (FALSE);

	strupr(pCheck->name);	// Make it monocase upper
	return (_nameMatch(pTest->name, pCheck->name, 0));
	}

// -----------------------------------------------------------------------
//	Global methods
// -----------------------------------------------------------------------
//	Open/Create a PEX exclusion instance
// -----------------------------------------------------------------------
	PEX							// Return anonymous pointer to the exclusion instance
fExcludeOpen (void)				// Open the instance

	{
	PEXtable  xp;				// Exclusion header instance pointer

	if ((xp = (PEXtable)(calloc(sizeof(EXtable), 1))) == NULL)
		{
		fwerrno = FWERR_NOMEM;
		fwInitError("exclusion open");
		}
	else
		{
		xp->magic    = xp;			// Validate the instance header
		xp->onceOnly = 0;			// Ensure single initialization
		xp->enbExcl  = TRUE;		// TRUE to enable defaults (unless disabled)
		xp->showConf = FALSE;		// TRUE to show exclusion configuration
		xp->showExcl = FALSE;		// TRUE to show exclusions (real time)
		xp->listCount = 0;			// No configured exclusions yet
		}		
	return (xp);
	}

// -----------------------------------------------------------------------
//	Initialize and connect the Exclusion instance to the fWild instance
// -----------------------------------------------------------------------
	int							// Return non-zero if an error
fExcludeConnect (
	PEX		xp,					// Pointer to the Exclusion instance
	PHP		hp)					// Pointer to the fWild instance

	{
	if ((! validHeader(xp))		// Validate the PEX header
	|| (xp->onceOnly != 0))
		return (-1);			// Multiple calls are not allowed

#ifdef DEBUG
printf("fExcludeConnect called\n");
#endif

	if (xp->enbExcl)
		{

		// If default exclusions are enabled,
		// first try the default file,
		// but fail over to using the internal defaults

#ifdef DEBUG
printf("fExclude Init file\n");
#endif
		if (fExcludeFromFile(xp, GetInitPath()) != 0)

			// The default file failed, so use the internal defaults instead

			{
#ifdef DEBUG
printf("fExclude Init internal\n");
#endif
			for (char **p = &DefExcludeFiles[0]; (*p != NULL); ++p)
				fExclude(xp, *p);
			}
		}
	else
		printf("Excl: Default exclusions disabled\n");

	if (xp->showConf)			// if requested, display the configuration
		ShowConfiguration(xp);

	if (hp)		// Connection is optional in the case of no fWild instance
		fwExclEnable(hp, xp, TRUE);	// Connect Excl instance to the fWild instance

	xp->onceOnly = TRUE;			// Suppress multiple configurations
	return (0);
	}

// -----------------------------------------------------------------------
//	Close/Destroy the PEX exclusion instance
// -----------------------------------------------------------------------
	PEX							// Always returns NULL for reassignment
fExcludeClose (					// Close the instance
	PEX		xp)					// Exclusion header instance pointer

	{
	if (validHeader(xp))
		freeHeader(xp);

	return (NULL);
	}

// -----------------------------------------------------------------------
//	Enable default exclusion configuration
// -----------------------------------------------------------------------
	void
fExcludeDefEnable (				// Enable/disable default exclusion paths
	PEX		xp,					// Pointer to the exclusion instance
	int		flag)				// True to enable exclusions

	{
	if ( ! validHeader(xp))		// Validate the PEX instance pointer
		return;					// Invalid instance pointer, exclude nothing

	xp->enbExcl = flag;
	}

// -----------------------------------------------------------------------
//	Show the PEX configuration
// -----------------------------------------------------------------------
	void
fExcludeShowConf (				// Enable/disable showing exclusion paths
	PEX		xp,					// Pointer to the exclusion instance
	int		flag)				// True to show the exclusion list

	{
	if ( ! validHeader(xp))		// Validate the PEX instance pointer
		return;					// Invalid instance pointer, exclude nothing

	xp->showConf = flag;
	}

// -----------------------------------------------------------------------
//	Show excluded files at runtime
// -----------------------------------------------------------------------
	void
fExcludeShowExcl (				// Enable/disable showing excluded files in real time
	PEX		xp,					// Pointer to the exclusion instance
	int		flag)				// True to show exclusions

	{
	if ( ! validHeader(xp))		// Validate the PEX instance pointer
		return;					// Invalid instance pointer, exclude nothing

	xp->showExcl = flag;
	}

// -----------------------------------------------------------------------
// Create an exclusion record for a provided pattern, or file of patterns
// -----------------------------------------------------------------------
	int							// Return non-zero if an error
fExclude (						// Exclude a path or path file from the fWild search
	PEX		xp,					// Pointer to the exclusion instance
	char   *pattern)			// Pointer to the exclusion pattern string

	{
#ifdef DEBUG
printf("fExclude called \"%s\"\n", pattern);
#endif DEBUG
	if (pattern == NULL)		// Ignore NULL string pointers
		return (0);

	int patoffset = (int)(strlen(pattern) - 1);	// Trim '\n' from lines
	if (pattern[patoffset] == '\n')
		pattern[patoffset] = '\0';

	else if (pattern[0] == '@')	// '@' denotes pattern is the name of an exclusion file
		{
		return (fExcludeFromFile(xp, pattern + 1));
		}

	// Remove trailing newlines, white space, etc.

	char  *p = (pattern + strlen(pattern));
	while ((p > pattern)  &&  (isspace(*(--p))))
		*p = '\0';					// NUL terminate the pattern;

	if ((pattern[0] == '\0')		// Comment empty line
	||  (!isgraph(pattern[0]))		// Comment blank or empty line
	||  (isspace(pattern[0]))		// Comment line
	||  (pattern[0] == '~'))		// Explicit comment line
		{
		if (xp->showConf)			// Display the comment, if enabled
			{
			printf("ExComment: %s\n", pattern);
			fflush(stdout);
			}
		return (0);					// Ignore this request
		}

	return (configExclRecord(xp, pattern));		// Otherwise pattern IS a pattern
	}

// -----------------------------------------------------------------------
//	Test a candidate file for exclusion
// -----------------------------------------------------------------------
	int							// Return TRUE to exclude the target, else FALSE to include it
fExcludeCheck (					// Check if a file/path is in the exclusion list
	PEX		xp,					// Pointer to the exclusion instance
	PEXI	pCheck)				// Pointer to the name to check

	{
	if (( ! validHeader(xp))	// Validate the PEX instance pointer
	||	(xp->enbExcl == FALSE))
		return (FALSE);			// Invalid instance pointer, exclude nothing
#ifdef DEBUG
printf("Checking \"%s\"\n", pCheck->name);
#endif
	PEXI	 pTest;				// Working pointer to the exclusion name records
	PEXI	 pLast = &xp->list[xp->listCount];
	for (pTest = &xp->list[0]; (pTest < pLast); (pTest += 1))
		{
		if (ItemTest(pTest, pCheck))
			{
			if (xp->showExcl)
				printf("Excluded [%s]: \"%s\"\n", ((pCheck->type & ATT_DIR) ? "DIR" : "FILE"), pCheck->name);
			return (TRUE);
			}
		}

	return (FALSE);
	}

// -----------------------------------------------------------------------
//	Test a candidate file/pathspec for exclusion
// -----------------------------------------------------------------------
	int							// Return TRUE to exclude the target, else FALSE to include it
fExcludePathCheck (				// Check if a file/path is excluded
	PEX		xp,					// Pointer to the exclusion instance
	char	*pPath)				// Pointer to the candidate path


	{
	char   *pBody;				// Working pointer into the body
	char   *pTemp;				// Working pointer into the body
	char   *pSep;				// Pointer to an element separator
	EXITEM  item;				// Exclusion item structure
static char path [MAX_PATH];


	if (pPath == NULL)					// NULL pointer not allowed
		return (TRUE);					// Excluded by definition

	if ((pBody = QueryUNCPrefix(pPath)) != NULL)
		return (FALSE);					// Can't exclude a network path

	// Make a working copy of the passed File/PathSpec string

	pathCopy(path, pPath, MAX_COPY);	// Copy the filespec
	strsetp(pPath, PATHCH);				// Standardize the path characters

	if ((item.depth = GetDepth(path, &item.drive)) < 0)
		return (TRUE);					// Excluded because path invalid

	pBody  = PointPastPrefix(path);		// Skip over the path prefix (and any root separator)
	pTemp  = pBody;						// Working pointer into the body
	item.type = ATT_DIR;				// Assume there will be directory segments

	// The path body contains some number (>= 0) of directories and a file
	// Check each segment of the path body for exclusion
	
	while ((pSep = strchr(pTemp, PATHCH)) != NULL)
		{
		*pSep = NULCH;					// NUL the segment separator
		if (strcmp(pTemp, "..") == 0)	// Check for a ".." segment
			{
			--item.depth;				// ".." reduces the depth by one
			if (item.depth < 0)			// If depth goes negative,
				return (TRUE);			//   path is invalid, excluded by definition
			}
		else
			{	
			pathCopy(item.name, pTemp, MAX_COPY);	// Copy the segment for checking
			if (fExcludeCheck(xp, &item))
				{
//printf("Excluded %s\n", pPath);
				return (TRUE);			// Segment is excluded, so path is excluded
				}
			++item.depth;				// Advance the depth for the next segment		
			}
		pTemp = (pSep+1);				// Point the next segment
		}

	// At this point, pTemp points the filename segment

	pathCopy(item.name, pTemp, MAX_COPY);	// Copy the segment for checking
	item.type = ATT_FILE;
	return (fExcludeCheck(xp, &item));	// if filename is excluded, path is excluded
	}

// -----------------------------------------------------------------------
//									EOF
// -----------------------------------------------------------------------
