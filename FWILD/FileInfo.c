/* ----------------------------------------------------------------------- *\
|
|								FileInfoOpen
|								FileInfoType
|								FileInfoSize
|								FileInfoFDT
|								FileInfoClose
|
|								Brian W Johnson
|								   22-Oct-23 (new)
|
|
|		PFI						Returns ptr to the FileInfo instance
|	FileInfoOpen (				Create a new FileInfo instance
|		int   callerAttrMask,	File attribute search mask (FW_FILE, FW_DIR, .etc)
|		char *pFileSpec)		Ptr to the (usually non-wild) search filename
|
|		char *					Returns the found file name
|	FileInfoName (				Get the file attributes
|		PFI instance			Ptr to the FileInfo instance
|
|		int						Returns the found file fileType
|	FileInfoAttr (				Get the file attributes
|		PFI instance			Ptr to the FileInfo instance
|
|		UINT64					Returns the found file size
|	FileInfoSize (				Get the file size
|		PFI instance			Ptr to the FileInfo instance
|
|		time_t					Returns the found file fdt
|	FileInfoFDT (				Get the UNIX file FDT
|		PFI instance			Ptr to the FileInfo instance
|
|		time_t					Returns the Microsoft fdt
|	FileInfoMtime (				Get the Microsoft file FDT
|		PFI instance			Ptr to the FileInfo instance
|
|		void
|	FileInfoClose (				Close the FileInfo instance
|		PFI instance			Ptr to the FileInfo instance
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

#define ATT_MASK (ATT_RONLY | ATT_HIDDEN | ATT_SYSTEM | ATT_DIR | ATT_ARCH)

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
// ---------------------------------------------------------------------------
//	newFi() - Allocate a new FI instance
// ---------------------------------------------------------------------------
	static PFI					// Returns ptr to the new FileInfo instance
newFI (void)					// Allocate a new FI instance	

	{
	PFI pFi = ((PFI)(calloc(sizeof(FI), 1)));

	if (pFi)
		pFi->magic = pFi;		// Make it valid

	return (pFi);
	}

// ---------------------------------------------------------------------------
//	validFI() - Validate a FileInfo instance
// ---------------------------------------------------------------------------
	static int					// Returns TRUE if the passed PFI is valid
validFI (						// Open a FileInfo instance
	PFI pFi)					// FI pointer to be tested

	{
	return ((pFi) && (pFi->magic == pFi));
	}

// ---------------------------------------------------------------------------
//	validFI() - Free a valid FileInfo instance
// ---------------------------------------------------------------------------
	static void
freeFI (						// Free a valid FileInfo instance
	PFI pFi)					// FI pointer to instance to be freed

	{
	if (validFI(pFi))			// Make sure it is valid
		{
		pFi->magic = NULL;		// Make it invalid
		free(pFi);
		}
	}

// ---------------------------------------------------------------------------
//	FileInfoOpen() - Create a new FileInfo instance
// ---------------------------------------------------------------------------
	PFI							// Returns ptr to the FileInfo instance
FileInfoOpen (					// Open a FileInfo instance
	int			callerAttrMask,	// File attribute search mask (FW_FILE, FW_DIR, .etc)
	const char *pFileSpec)		// Ptr to the (usually noneild) search filename

	{
	PFI pFi = NULL;				// Pointer to the allocated FileInfo instance

	if ((pFileSpec == NULL)			// Non-NULL  filespec required
	||  (strlen(pFileSpec) == 0)	// Non-empty filespec required
	||  ((pFi = newFI()) == NULL))	// Need an allocated FI
		return (NULL);

	pFi->dta.SearchAttr = callerAttrMask;
	if ((! FinderOpen(&pFi->dta, pFileSpec))
	||  (FinderFile(&pFi->dta, NULL) == 0))
		{
		freeFI(pFi);				// Failure, filespec was not found
		return (NULL);
		}
	
	return (pFi);					// Success return the FI instance pointer
	}

// ---------------------------------------------------------------------------
//	FileInfoAttr() - Get the found file attributes
// ---------------------------------------------------------------------------
	char *						// Returns the found file name
FileInfoName (					// Get the found file name (no path)
	PFI  pFi)					// Ptr to the FileInfo instance

	{
	return (validFI(pFi) ? pFi->dta.dta_name : (NULL));
	}

// ---------------------------------------------------------------------------
//	FileInfoAttr() - Get the found file attributes
// ---------------------------------------------------------------------------
	int							// Returns the found file attributes
FileInfoAttr (					// Get the found file attributes
	PFI  pFi)					// Ptr to the FileInfo instance

	{
	return (validFI(pFi) ? pFi->dta.dta_type : (0));
	}

// ---------------------------------------------------------------------------
//	FileInfoSize() - Get the found file size
// ---------------------------------------------------------------------------
	UINT64						// Returns the found file size
FileInfoSize (					// Get the found file size
	PFI  pFi)					// Ptr to the FileInfo instance

	{
	return (validFI(pFi) ? pFi->dta.dta_size : 0);
	}

// ---------------------------------------------------------------------------
//	FileInfoFDT() - Get the UNIX file FDT
// ---------------------------------------------------------------------------
	time_t						// Returns the found file fdt
FileInfoFDT (					// Get the UNIX file FDT
	PFI  pFi)					// Ptr to the FileInfo instance

	{
	return (validFI(pFi) ? pFi->dta.dta_fdt : 0);
	}

// ---------------------------------------------------------------------------
//	FileInfoFDT() - Get the UNIX file FDT
// ---------------------------------------------------------------------------
	time_t						// Returns the found file fdt
FileInfoMtime (					// Get the Microsoft file FDT
	PFI  pFi)					// Ptr to the FileInfo instance

	{
	return (validFI(pFi) ? pFi->dta.dta_mtime : 0);
	}

// ---------------------------------------------------------------------------
//	FileInfoClose() - Close the FileInfo instance
// ---------------------------------------------------------------------------
	PFI							// Returns ptr to the FileInfo instance
FileInfoClose (					// Open a FileInfo instance
	PFI pFi)					// FI pointer to instance to be freed

	{
	freeFI(pFi);				// Validate and free it
	return (NULL);				// Returned for pointer reassignment
	}

// ---------------------------------------------------------------------------
//	main ()	-  Console mode test main
// ---------------------------------------------------------------------------
#ifdef	TEST

static 	char in  [MAX_PATH] = { 0 };

// ---------------------------------------------------------------------------
	void
process (void)	

	{
	PFI  pFi = NULL;
	int		Attr;		// The search attribute

//	int		result;		// The FinderOpen() result
//	int		fileType;	// The FinderFile() result

//	Attr = (0);
//	Attr = (FW_FILE);
//	Attr = (FW_DIR);
	Attr = (FW_FILE | FW_DIR | FW_HIDDEN | FW_SYSTEM);
//	Attr = (FW_FILE | FW_HIDDEN | FW_SYSTEM);
//	Attr = (FW_DIR | FW_HIDDEN | FW_SYSTEM);
//	Attr = (FW_HIDDEN | FW_SYSTEM);
//	Attr = (FW_FILE | FW_HIDDEN);
//	Attr = (FW_FILE | FW_SYSTEM);


	pFi = FileInfoOpen(Attr, in);
	printf("Search: %s\n", in);

	if (! pFi)
		printf("Open Failed\n");
	else
		{
		printf("Open Succeded\n");
		printf("\n");
		printf("  Name:  \"%s\"\n",	FileInfoName(pFi));
		printf("  Srch:  %s\n",		showSrchAttr(Attr));
		printf("  File:  %s\n",		showFileAttr(FileInfoAttr(pFi)));
		printf("  Size:  %lld\n",	FileInfoSize(pFi));
		printf("  Date:  %lld\n",	FileInfoFDT(pFi));
		printf("\nDone\n");
		}
	}

// ---------------------------------------------------------------------------
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
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
