/* ----------------------------------------------------------------------- *\
|
|									FinderOpen
|									FinderFile
|									FinderClose
|
|								Brian W Johnson
|								   15-Oct-23
|
|
|		int						Returns TRUE for success, FALSE for failure
|	FinderOpen (				Open a Finder instance
|		PFW_DTA		pDta,		Pointer to the search DTA
|		const char *pPattern)	Pointer to the search pattern string
|
|		Finder					Returns FND_FILE, FND_DIR or FND_FAIL fo fail
|	FinderFile (
|		PFW_DTA		pDta,		Pointer to the search DTA
|			char   *pFound)		Pointer to returned found filename
|
|		void
|	FinderClose (				Close the Finder instance
|		PFW_DTA		pDta)		Pointer to the search DTA
|
\* ----------------------------------------------------------------------- */

#include  <windows.h>

#define	 FWILD_INTERNAL

#include  "fWild.h"

//----------------------------------------------------------------------------
//	#define TEST		// Define to build a test program
//	#define DEBUG		// Define to include debug output

#if defined(TEST)
#define DEBUG
#endif

#if defined(DEBUG)
#include <stdio.h>
#endif

//----------------------------------------------------------------------------

#define ATTR_RESULT_MASK	(ATT_FILE | ATT_DIR)

//----------------------------------------------------------------------------

#define ATT_MASK (ATT_RONLY | ATT_HIDDEN | ATT_SYSTEM | ATT_DIR | ATT_ARCH | FW_AD)

//----------------------------------------------------------------------------
//	Translate file search attributes to a diagnostic string
//----------------------------------------------------------------------------
#ifdef	DEBUG
	static char *
showSrchAttr (
	int mode)

	{
static char  modeStr [30];	// The returned mode string

	strcpy(modeStr, "{ ");
	if (mode & FW_DIR)
		strcat(modeStr, "DIR ");
	if (mode & FW_FILE)
		strcat(modeStr, "FILE ");
	if (mode & FW_HIDDEN)
		strcat(modeStr, "HIDDEN ");
	if (mode & FW_SYSTEM)
		strcat(modeStr, "SYSTEM ");
	if (mode & FW_AD)
		strcat(modeStr, "AD");
	strcat(modeStr, " }");

	return (modeStr);
	}

//----------------------------------------------------------------------------
//	Translate found file attributes to a diagnostic string
//----------------------------------------------------------------------------
	static char *
showFileAttr (
	int mode)

	{
static char  modeStr [40];	// The returned mode string

	strcpy(modeStr, "{ ");
	if (mode & ATT_DIR)
		strcat(modeStr, "DIR ");
	if (mode & ATT_FILE)
		strcat(modeStr, "FILE ");
	if (mode & ATT_RONLY)
		strcat(modeStr, "RONLY ");
	if (mode & ATT_HIDDEN)
		strcat(modeStr, "HIDDEN ");
	if (mode & ATT_SYSTEM)
		strcat(modeStr, "SYSTEM ");
	if (mode & ATT_ARCH)
		strcat(modeStr, "AR ");
	strcat(modeStr, " }");

	return (modeStr);
	}

/* ----------------------------------------------------------------------- *\
|  d_disp ()  -	 Display the details of a FW_DTA
\* ----------------------------------------------------------------------- */
	static void
d_disp (
	FW_DTA	 *pDta)				/* The pointer to the DTA structure */	

	{
	printf("Name: \"%s\"\n",	pDta->dta_name);
	printf("Type: %s\n",		showFileAttr(pDta->dta_type));
//	printf("Time: %04X\n",		pDta->dta_time);
//	printf("Date: %04X\n",		pDta->dta_date);
	printf("Size: %-8llu\n",	pDta->dta_size);
//	printf("Hand: %-01X\n",		(unsigned)(pDta->sh));
	}

#endif
/* ----------------------------------------------------------------------- *\
|  wfd_to_dta () - Translate the WIN32_FIND_DATA to the FW_DTA structure
\* ----------------------------------------------------------------------- */
	static void
wfd_to_dta (
	FW_DTA	 *pDta)					/* The pointer to the DTA structure */

	{
	LARGE_INTEGER N;

	strcpy(&pDta->dta_name[0], pDta->wfd.cFileName);

	pDta->dta_type	= (ATT_MASK & pDta->wfd.dwFileAttributes);
	if ( ! (pDta->dta_type & ATT_DIR))
		pDta->dta_type |= ATT_FILE;

	N.u.LowPart		= pDta->wfd.nFileSizeLow;
	N.u.HighPart	= pDta->wfd.nFileSizeHigh;
	pDta->dta_size	= N.QuadPart;

	N.u.LowPart		= pDta->wfd.ftLastWriteTime.dwLowDateTime;
	N.u.HighPart	= pDta->wfd.ftLastWriteTime.dwHighDateTime;
	pDta->dta_mtime	= N.QuadPart;
	pDta->dta_fdt	= TimeToUnixTime(N.QuadPart);

//printf("FinderFile X:  [%d] %s\n", pDta->valid, showFileAttr(pDta->dta_type));
	}

/* ----------------------------------------------------------------------- *\
|  TypeMatch () - Determine whether the found file matches the search profile
\* ----------------------------------------------------------------------- */
	static BOOL					/* Returns TRUE if NOT valid */
TypeMatch (
	FW_DTA	 *pDta)				/* The pointer to the DTA structure */

	{
	BOOL	   result	= FALSE;				// The returned result
	int		   SrchAttr = pDta->SearchAttr;		// The search attribute bitmap
	int		   fileAttr	= pDta->dta_type;		// The returned bitmap */

#ifdef DEBUG
	printf("TypeMatch(): filename:  \"%s\"\n", pDta->dta_name);
	printf("TypeMatch(): Srch Attr: %s\n",     showSrchAttr(SrchAttr));
	printf("TypeMatch(): Fnd  Attr: %s\n",     showFileAttr(fileAttr));
#endif
	if ((fileAttr & ATT_DIR)  &&  (SrchAttr & ATT_DIR))
		{
		if (((fileAttr & (ATT_HIDDEN | ATT_SYSTEM)) == 0)
		||	((fileAttr & ATT_HIDDEN)  &&  (SrchAttr & ATT_HIDDEN))
		||	((fileAttr & ATT_SYSTEM)  &&  (SrchAttr & ATT_SYSTEM)))
			result = TRUE;
		}

	else if ((fileAttr & ATT_FILE)  &&  (SrchAttr & ATT_FILE))
		{
		if (((fileAttr & (ATT_HIDDEN | ATT_SYSTEM)) == 0)
		||	((fileAttr & ATT_HIDDEN)  &&  (SrchAttr & ATT_HIDDEN))
		||	((fileAttr & ATT_SYSTEM)  &&  (SrchAttr & ATT_SYSTEM)))
			result = TRUE;
		}

#ifdef DEBUG
	printf("TypeMatch(): %s\n", (result ? "Matched" : "Unmatched"));
#endif
	return (result);
	}

/* ----------------------------------------------------------------------- *\
|  FindOpen ()	Open a wild card search Version 2 (public version)
\* ----------------------------------------------------------------------- */
	int				// TRUE for success, FALSE for failure
FinderOpen (
	PFW_DTA		pDta,		// Pointer to the search DTA
	const char *pPattern)	// Pointer to the search string

	{
	HANDLE  hFind;					// The returned file handle

	pDta->valid		 = FALSE;
	pDta->sh = hFind = FindFirstFile(pPattern, &pDta->wfd);
	if (hFind == INVALID_HANDLE_VALUE)
		return (FALSE);

	pDta->valid = TRUE;

#ifdef DEBUG
printf("FinderOpen:    \"%s\"\n", pPattern);
#endif
	return (TRUE);
	}

/* ----------------------------------------------------------------------- *\
|  FindFile ()	Find the first/next file Version 2 (public version)
\* ----------------------------------------------------------------------- */
	int			// Returns ATTR bitmap: ATT_FILE, ATT_DIR, or 0 for failure
FinderFile (
	PFW_DTA	pDta,			// Pointer to the search DTA
	char   *pFound)			// Pointer to returned found filename bufffer

	{
	int		fileType;			// The found file type (assume failure)

	if ((pDta == NULL) || (pDta->sh == INVALID_HANDLE_VALUE))
		return (0);

	for (;;)
		{
		if (! pDta->valid)			// If no more files,
			{
			fileType = (0);			// No acceptance
			FinderClose(pDta);		// Close Finder
			break;
			}

//printf("FinderFile 0:  [%d] \"%s\"\n", pDta->valid, pDta->wfd.cFileName);

		wfd_to_dta(pDta);	// Transfer useful data to the DTA
		fileType = (pDta->dta_type & ATTR_RESULT_MASK);

//("FinderFile 1:  [%d] %s\n", pDta->valid, showFileAttr(fileType));

		// Queue the next entry, if any

		 pDta->valid = FindNextFile(pDta->sh, &pDta->wfd);

//printf("FinderFile 1:  [%d] GetNext\n", pDta->valid);

		if ((strcmp(pDta->dta_name, "..") != 0)		// Skip over these
		&&  (strcmp(pDta->dta_name, ".")  != 0)
		&&  (TypeMatch(pDta)))
			break;					// Accept this file/dir
		}

//printf("FinderFile 2:  [%d] %s\n", pDta->valid, showFileAttr(fileType));

	if (fileType != ATT_NONE)
		{
		if (pFound != NULL)
			strcpy(pFound, pDta->dta_name);
		}	
#ifdef DEBUG
printf("FinderFile:    \"%s\"\n", pDta->dta_name);
#endif

//printf("FinderFile 3:  [%d] %s\n", pDta->valid, showFileAttr(fileType));

	return (fileType);	// If valid, the return file type
	}

/* ----------------------------------------------------------------------- *\
|  FindClose ()	Close a wild card search Version 2 (public version)
\* ----------------------------------------------------------------------- */
	void
FinderClose (
	PFW_DTA	pDta)			// Pointer to the search DTA

	{
	if ((pDta)  &&  (pDta->sh != INVALID_HANDLE_VALUE))
		{
		FindClose(pDta->sh);
		pDta->sh = INVALID_HANDLE_VALUE;
		pDta->valid = FALSE;
		}
	
#ifdef DEBUG
printf("FinderClose\n");
#endif
	}

/* ----------------------------------------------------------------------- *\
|  main ()	-  Console mode test main
\* ----------------------------------------------------------------------- */

#ifdef	TEST
static	FW_HDR	Hdr;	// The test FW_HDR structure
static 	FW_DTA	Dta;	// The test FW_DTA structure

static 	char in  [MAX_PATH] = { 0 };
static 	char out [MAX_PATH] = { 0 };

/* ----------------------------------------------------------------------- */
	void
process (void)	

	{
	int		result;		// The FinderOpen() result
	int		fileType;	// The FinderFile() result
	int		Attr;		// The search attribute


//	Attr = (0);
//	Attr = (FW_FILE);
//	Attr = (FW_DIR);
	Attr = (FW_FILE | FW_DIR | FW_HIDDEN | FW_SYSTEM);
//	Attr = (FW_FILE | FW_HIDDEN | FW_SYSTEM);
//	Attr = (FW_DIR | FW_HIDDEN | FW_SYSTEM);
//	Attr = (FW_HIDDEN | FW_SYSTEM);
//	Attr = (FW_FILE | FW_HIDDEN);
//	Attr = (FW_FILE | FW_SYSTEM);

	Dta.SearchAttr = Attr;
	result = FinderOpen(&Dta, in);
	printf("Open:   %s\n", (result ? "TRUE" : "FALSE"));
	printf("Search: %s\n", in);
//	printf("Name:   %s\n", Dta.dta_name);

	if (result)
		{
		for (;;)
			{
			fileType = FinderFile(&Dta, out);

printf("FinderFile 4:      %s\n", showFileAttr(fileType));

			if (fileType == ATT_NONE)
				break;

			Hdr.file_fdt = fgetfdt(out);
			printf("\n");
			printf("  found: \"%s\"\n",	out);
			printf("  Srch:  %s\n",		showSrchAttr(Dta.SearchAttr));
			printf("  File:  %s\n",		showFileAttr(Dta.dta_type));
			printf("  Size:  %lld\n",	Dta.dta_size);
			printf("  Date:  %lld\n",	Dta.dta_fdt);
			printf("\n");
			out[0] = '\0';
			}
		}
			printf("Done\n");
	}

/* ----------------------------------------------------------------------- */
	void
main ()					/* Test main program */

	{
	for (;;)
		{
		printf("\nPattern: ");
		gets(in);
		printf("\n");
		process();
		}
	}

#endif // TEST
/* ----------------------------------------------------------------------- */
