/* ----------------------------------------------------------------------- *\
|
|		C-callable functions to perform findf/findn operations under either
|		DOS V2.0 or OS/2
|
|							Brian W Johnson
|							   26-May-90
|								9-May-91
|							   16-Aug-97 for Win32
|							   25-Sep-97 for UNC
|							   27-Sep-07 for 64 bit file sizes
|							   20-Dec-08 for FAT filetime corrections
|
|		Calling sequences:
|
|				int return = _findf (pDta, pString, Attr);
|
|					pDta:		Pointer to the special DTA
|					pString:	Pointer to path and file name string
|					Attr:		Integer file search attribute bitmap
|
|					return:		Return status:
|									0 if success
|									2 if file not found
|								   18 if no more files
|
|				return = _findn (pDta);
|
|					pDta:		Pointer to the special DTA
|
|					return:		Return status:
|									0 if success
|								   18 if no more files
|
\* ----------------------------------------------------------------------- */

#include  <windows.h>

#if defined(TEST) || defined(DEBUG)
#include <stdio.h>
#endif

#define	 FWILD_INTERNAL

#include  "fWild.h"

//----------------------------------------------------------------------------
//	#define TEST		// Define to build a test program
//	#define DEBUG		// Define to include debug output

#define ATT_MASK (ATT_RONLY | ATT_HIDDEN | ATT_SYSTEM | ATT_DIR | ATT_ARCH)

//----------------------------------------------------------------------------
//	Translate file search attributes to a diagnostic string
//----------------------------------------------------------------------------
#ifdef	DEBUG
	static char *
showSrchType (
	int mode)

	{
static char  modeStr [30];	// The returned mode string

	modeStr[0] = '\0';
	if (mode & FW_DIR)
		strcat(modeStr, "DIR ");
	if (mode & FW_FILE)
		strcat(modeStr, "FILE ");
	if (mode & FW_HIDDEN)
		strcat(modeStr, "HIDDEN ");
	if (mode & FW_SYSTEM)
		strcat(modeStr, "SYSTEM ");

	return (modeStr);
	}

//----------------------------------------------------------------------------
//	Translate found file attributes to a diagnostic string
//----------------------------------------------------------------------------
	static char *
showFileType (
	int mode)

	{
static char  modeStr [40];	// The returned mode string

	modeStr[0] = '\0';
	if (mode & ATT_DIR)
		strcat(modeStr, "DIR ");
	else // (not DIR)
		strcat(modeStr, "FILE ");
	if (mode & ATT_RONLY)
		strcat(modeStr, "RONLY ");
	if (mode & ATT_HIDDEN)
		strcat(modeStr, "HIDDEN ");
	if (mode & ATT_SYSTEM)
		strcat(modeStr, "SYSTEM ");
	if (mode & ATT_ARCH)
		strcat(modeStr, "AR ");

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
	printf("Type: %s\n",		showFileType(pDta->dta_type));
//	printf("Time: %04X\n",		pDta->dta_time);
//	printf("Date: %04X\n",		pDta->dta_date);
	printf("Size: %-8llu\n",	pDta->dta_size);
	printf("Hand: %-01X\n",		(unsigned)(pDta->sh));
	}

#endif
/* ----------------------------------------------------------------------- *\
|  wfd_to_dta () - Translate the WIN32_FIND_DATA to the FW_DTA structure
\* ----------------------------------------------------------------------- */
	static void
wfd_to_dta (
	FW_DTA	 *pDta)					/* The pointer to the DTA structure */

	{
//	FILETIME   LocalTime;			/* The localized time */
	
//	FileTimeToLocalFileTime(&pDta->wfd.ftLastWriteTime, &LocalTime);
//	FileTimeToDosDateTime(&LocalTime, &pDta->dta_date, &pDta->dta_time);
	pDta->dta_size  = (((UINT64)(pDta->wfd.nFileSizeHigh)) << 32)
					|  ((UINT64)(pDta->wfd.nFileSizeLow));
	strcpy(&pDta->dta_name[0], &pDta->wfd.cFileName[0]);
	}

/* ----------------------------------------------------------------------- *\
|  Invalid () - Determine whether the found file matches the attribute profile
\* ----------------------------------------------------------------------- */
	static BOOL					/* Returns TRUE if NOT valid */
Invalid (
	FW_DTA	 *pDta)				/* The pointer to the DTA structure */

	{
	BOOL	   result	= TRUE;					/* The returned result */
	int		   SrchAttr = pDta->SearchAttr;		/* The search attribute bitmap */
	int		   fileAttr	= 0;					/* The returned bitmap */

	fileAttr = (ATT_MASK & AttrFromWin32(pDta->wfd.dwFileAttributes));

#ifdef DEBUG
	printf("Invalid(): filename:  \"%s\"\n", pDta->dta_name);
	printf("Invalid(): Srch Attr: %s\n",     showSrchType(SrchAttr));
	printf("Invalid(): Fnd  Attr: %s\n",     showFileType(fileAttr));
#endif
	if (fileAttr & ATT_DIR)
		{
		if ((SrchAttr & ATT_DIR)				/* Check if want subdirectories */
		&& (    ((fileAttr & SrchAttr) & (FW_HIDDEN | FW_SYSTEM))
			==  ((fileAttr           ) & (FW_HIDDEN | FW_SYSTEM))))
			result = FALSE;
		}

	else /* (it's a file) */
		{
		if (((fileAttr & (ATT_HIDDEN | ATT_SYSTEM)) == 0)
		||	((fileAttr & ATT_HIDDEN)	 &&	 (SrchAttr & ATT_HIDDEN))
		||	((fileAttr & ATT_SYSTEM)	 &&	 (SrchAttr & ATT_SYSTEM)))
			{
			fileAttr |= ATT_FILE;
			result = FALSE;
			}
		}

	if (! result)
		{
		pDta->dta_type = fileAttr;	// Valid; return properties
		wfd_to_dta(pDta);
		}
	
#ifdef DEBUG
	printf("Invalid(): %s\n", (result ? "Invalid" : "Valid"));
#endif
	return (result);
	}

/* ----------------------------------------------------------------------- *\
|  __findf ()  -  Initiate a wild card search (internal version)
\* ----------------------------------------------------------------------- */
	static int
__findf (
	FW_DTA		 *pDta,			/* The pointer to the DTA structure */	
	const char	 *pName)		/* The pointer to the search path/file name */

	{
	int		  result;			/* The returned result */


	pDta->sh = FindFirstFile(pName, &pDta->wfd);
	result	 = (pDta->sh == INVALID_HANDLE_VALUE) ? 2 : 0;

#ifdef DEBUG
	printf("__findf(): Handle: %01X;  Pattern: \"%s\"\n", (unsigned)(pDta->sh), pName);
#endif
	return (result);
	}

/* ----------------------------------------------------------------------- *\
|  __findn ()  -  Continue a wild card search (internal version)
\* ----------------------------------------------------------------------- */
	static int
__findn (
	FW_DTA	 *pDta)				/* The pointer to the DTA structure */	

	{
	int		   result;			/* The returned result */


	if (pDta->sh == INVALID_HANDLE_VALUE)
		result = 18;
	else if (FindNextFile(pDta->sh, &pDta->wfd))
		result = 0;
	else
		{
		FindClose(pDta->sh);
		pDta->sh = INVALID_HANDLE_VALUE;
		result = 18;
		}

#ifdef DEBUG
	printf("__findf(): [%d]  Found: %s\n", result, pDta->dta_name);
#endif
	return (result);
	}

/* ----------------------------------------------------------------------- *\
|	_findf () -	Initiate a wild card search (public version)
|	_findf () -	Also returns the first file/dir
|				This is a combination of _findo() and _findn()
\* ----------------------------------------------------------------------- */
	int
_findf (
	FW_DTA		 *pDta,			/* The pointer to the DTA structure */	
	const char	 *pName,		/* The pointer to the search path/file name */
	int			  Attr)			/* The attribute bitmap */

	{
	int		   result;			/* The returned result */

#ifdef DEBUG
	printf("_findf():  Attr: %04X  Pattern: %s\n", Attr, pName);
#endif

	pDta->SearchAttr = Attr;
	result = __findf(pDta, pName);

	while ((result == 0)  &&  Invalid(pDta))
		{
		if (result == 0)
			{
#ifdef DEBUG
			d_disp(pDta);
#endif
			}
		}
		result = __findn(pDta);

#ifdef DEBUG
	printf("_findf():  [%d]  Found: %s\n", result, pDta->dta_name);
#endif
	return (result);
	}

/* ----------------------------------------------------------------------- *\
|	_findo ()  -	 Initiate a wild card search (public version)
\* ----------------------------------------------------------------------- */
	int
_findo (
	FW_DTA		 *pDta,			/* The pointer to the DTA structure */	
	const char	 *pPattern,		/* The pointer to the search path/file name */
	int			  Attr)			/* The attribute bitmap */

	{
	int		   result;			/* The returned result */

	pDta->SearchAttr = Attr;
	result = __findf(pDta, pPattern);

#ifdef DEBUG
	printf("_findo():  [%d] Attr: %04X  Pattern: %s\n", result, Attr, pPattern);
#endif
	return (result);
	}

/* ----------------------------------------------------------------------- *\
|	_findn ()  -	 Continue a wild card search (public version)
\* ----------------------------------------------------------------------- */
	int
_findn (
	FW_DTA	 *pDta)				/* The pointer to the DTA structure */	

	{
	int		   result;			/* The returned result */

	do	{
		result = __findn(pDta);
		if (result == 0)
			{
#ifdef DEBUG
			d_disp(pDta);
#endif
			}
		} while ((result == 0)  &&  Invalid(pDta));

#ifdef DEBUG
	printf("_findn():  [%d]  Found: %s\n", result, pDta->dta_name);
#endif
	return (result);
	}

/* ----------------------------------------------------------------------- *\
|  _findc ()  -	 Close a wild card search (public version)
\* ----------------------------------------------------------------------- */
	void
_findc (
	FW_DTA	 *pDta)				/* The pointer to the DTA structure */	

	{
	if (pDta->sh != INVALID_HANDLE_VALUE)
		{
		FindClose(pDta->sh);
		pDta->sh = INVALID_HANDLE_VALUE;
		}

#ifdef DEBUG
	printf("_findc()\n");
#endif
	}

/* ----------------------------------------------------------------------- *\
|  main ()	-  Console mode test main
\* ----------------------------------------------------------------------- */
#ifdef	TEST
	void
main (
	int		 argc,		// Argument count
	char	**argv)		// Argument list

	{
	int		 result;	// The findf/findn result
	int		 Attr = 0;	// The search attribute
	char	*s;			// Ptr to the search string
	FW_HDR	 Hdr;		// The test FW_HDR structure
	FW_LEV	 Ele;		// The test FW_LEV structure


	s = (argc > 1) ? (argv[1]) : ("test*");

	Attr = ATT_HIDDEN | ATT_SYSTEM;

	result = _findo(&Ele.dta, s, Attr);
	printf("Result: %d\n", result);
	
	do	{
		result = _findn(&Ele.dta);
		printf("Found: %s\n", Ele.dta.dta_name);
		printf("Date:  %s\n", fwdate(&Hdr));
		printf("Time:  %s\n", fwtime(&Hdr));

		}	while (result == 0);

	}

#endif
/* ----------------------------------------------------------------------- */
#ifdef	TESTX
	void
main (
	int		 argc,		// Argument count
	char	**argv)		// Argument list

	{
	int		 result;	// The findf/findn result
	int		 Attr = 0;	// The search attribute
	char	*s;			// Ptr to the search string
	FW_HDR	 Hdr;		// The test FW_HDR structure
	FW_LEV	 Ele;		// The test FW_LEV structure


	s = (argc > 1) ? (argv[1]) : ("test*");

	Attr = ATT_HIDDEN | ATT_SYSTEM;

	result = _findf(&Ele.dta, s, Attr);
	while (result == 0)
		{
		printf("Found: %s\n", Ele.dta.dta_name);
		printf("Date:  %s\n", fwdate(&Hdr));
		printf("Time:  %s\n", fwtime(&Hdr));
		result = _findn(&Ele.dta);
		}
	}

#endif
/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */
