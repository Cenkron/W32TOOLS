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

// #define	TEST				// Define in the makefile to include debug output
// #define	CONSOLEMODETEST		// Define in the makefile to build a test program

#include  <windows.h>

#if defined(CONSOLEMODETEST) || defined(TEST)
#include <stdio.h>
#endif

#define	 FWILD_INTERNAL

#include  "fwild.h"

#ifdef	TEST
static	void	d_disp (DTA_BLK *pDta);
static	int		i;
#endif

/* ----------------------------------------------------------------------- *\
|  wfd_to_dta () - Translate the WIN32_FIND_DATA to the DTA_BLK structure
\* ----------------------------------------------------------------------- */
	static void
wfd_to_dta (
	DTA_BLK	 *pDta)					/* The pointer to the DTA structure */	

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
	DTA_BLK	 *pDta)				/* The pointer to the DTA structure */	

	{
	BOOL	   result	= TRUE;					/* The returned result */
	int		   NewAttr	= 0;					/* The returned bitmap */
	int		   SrchAttr = pDta->SearchAttr;		/* The search attribute bitmap */


	pDta->dta_type = NewAttr = AttrFromWin32(pDta->wfd.dwFileAttributes);

	if (NewAttr & ATT_SUBD)
		{
		if ((SrchAttr & ATT_SUBD)				/* Check if want subdirectories */
		&& (    ((NewAttr & SrchAttr) & (FW_HIDDEN | FW_SYSTEM))
				== ((NewAttr           ) & (FW_HIDDEN | FW_SYSTEM))))
			result = FALSE;
		}
	else /* (it's a file) */
		{
		if (((NewAttr & (ATT_HIDDEN | ATT_SYSTEM)) == 0)
		||	((NewAttr & ATT_HIDDEN)	 &&	 (SrchAttr & ATT_HIDDEN))
		||	((NewAttr & ATT_SYSTEM)	 &&	 (SrchAttr & ATT_SYSTEM)))
			result = FALSE;
		}

	return (result);
	}

/* ----------------------------------------------------------------------- *\
|  __findf ()  -  Initiate a wild card search (internal version)
\* ----------------------------------------------------------------------- */
	static int
__findf (
	DTA_BLK	 *pDta,				/* The pointer to the DTA structure */	
	char	 *pName)			/* The pointer to the search path/file name */

	{
	int		  result;			/* The returned result */


	pDta->sh = FindFirstFile(pName, &pDta->wfd);
	result	 = (pDta->sh == INVALID_HANDLE_VALUE) ? 2 : 0;

	return (pDta->result = result);
	}

/* ----------------------------------------------------------------------- *\
|  __findn ()  -  Continue a wild card search (internal version)
\* ----------------------------------------------------------------------- */
	static int
__findn (
	DTA_BLK	 *pDta)				/* The pointer to the DTA structure */	

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

	return (pDta->result = result);
	}

/* ----------------------------------------------------------------------- *\
|  _findf ()  -	 Initiate a wild card search (public version)
\* ----------------------------------------------------------------------- */
	int
_findf (
	DTA_BLK	 *pDta,				/* The pointer to the DTA structure */	
	char	 *pName,			/* The pointer to the search path/file name */
	int		   Attr)			/* The attribute bitmap */

	{
	int		   result;			/* The returned result */


#ifdef TEST
	printf("_findf():  Attr: %04X  Pattern: %s\n", Attr, pName);
#endif

	pDta->SearchAttr = Attr;
	result = __findf(pDta, pName);

	while ((result == 0)  &&  Invalid(pDta))
		result = __findn(pDta);

	if (result == 0)
		{
		wfd_to_dta(pDta);
#ifdef TEST
		d_disp(pDta);
#endif
		}

	return (result);
	}

/* ----------------------------------------------------------------------- *\
|  _findn ()  -	 Continue a wild card search (public version)
\* ----------------------------------------------------------------------- */
	int
_findn (
	DTA_BLK	 *pDta)				/* The pointer to the DTA structure */	

	{
	int		   result;			/* The returned result */

	do	result = __findn(pDta);
		while ((result == 0)  &&  Invalid(pDta));

	if (result == 0)
		{
		wfd_to_dta(pDta);
#ifdef TEST
		d_disp(pDta);
#endif
		}

	return (result);
	}

/* ----------------------------------------------------------------------- *\
|  _findc ()  -	 Close a wild card search (public version)
\* ----------------------------------------------------------------------- */
	void
_findc (
	DTA_BLK	 *pDta)				/* The pointer to the DTA structure */	

	{
	if (pDta->sh != INVALID_HANDLE_VALUE)
		{
		FindClose(pDta->sh);
		pDta->sh = INVALID_HANDLE_VALUE;
		}
	}

/* ----------------------------------------------------------------------- *\
|  d_disp ()  -	 Display the details of a DTA_BLK
\* ----------------------------------------------------------------------- */
#ifdef	TEST
	static void
d_disp (
	DTA_BLK	 *pDta)				/* The pointer to the DTA structure */	

	{
	printf("Type:  %02X\n",	 pDta->dta_type);
	printf("Time:  %04X\n",	 pDta->dta_time);
	printf("Date:  %04X\n",	 pDta->dta_date);
	printf("Size:  %-8lu\n", pDta->dta_size);
	printf("Name:  %s\n",	 pDta->dta_name);
	}

#endif
/* ----------------------------------------------------------------------- *\
|  main ()	-  Console mode test main
\* ----------------------------------------------------------------------- */
#ifdef	CONSOLEMODETEST
	void
main (
	int		 argc,		// Argument count
	char	**argv)		// Argument list

	{
	int		 result;	// The findf/findn result
	int		 Attr = 0;	// The search attribute
	char	*s;			// Ptr to the search string
	DTA_HDR	 Hdr;		// The test DTA_HDR structure
	DTA_ELE	 Ele;		// The test DTA_ELE structure


	s = (argc > 1) ? (argv[1]) : ("test*");
	Hdr.link = &Ele;

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
