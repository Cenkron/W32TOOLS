/* ----------------------------------------------------------------------- *\
|
|								fgetfdt
|
|							Brian W Johnson
|								26-May-90
|								 3-Apr-00 NTFS vs FAT timestamp resolution fix
|								21-Dec-08 FAT timestamp correction
|								24-Oct-23 Change to FileInfo method
|
|		int					Returns 0 for success, or nonzero for failure
|	fgetfdt (				Get the timedate (write time) of the file
|		const char  *pName, Pointer to the path/filename
|		time_t		*pFdt	Pointer to the returned FDT, or NULL
|
|	For historical reasons, the fWild system uses the unix time format
|	internally, even though it has much lower resolution than the native
|	Windows timestamp.  The write timestamp is used because it is the time
|	that is visibly used by Windows, and because it best represents the
|	origin date of the data contained in the file.
|
|	The conversion from Windows time to UNIX time is done by FileInfo.
|
\* ----------------------------------------------------------------------- */

#include  <windows.h>
#include  <sys\types.h>
#include  <sys\utime.h>
#include  <stdio.h>
#include  <string.h>
#include  <time.h>

#include  "fWild.h"

// ---------------------------------------------------------------------------
//	#define TEST		// Define to build a test program
//	#define DEBUG		// Define to include debug output

#if defined(TEST)		// There is currently no test program
#define DEBUG
#endif

#if defined(DEBUG)
#include <stdio.h>
#endif

#if 1
// ---------------------------------------------------------------------------
//	fgetfdt ()  -  Get the file time/date (via filename)
// ---------------------------------------------------------------------------
	int						// Returns 0 for success, or nonzero for failure
fgetfdt (
	const char  *pName, 	// Pointer to the path/filename
	time_t		*pFdt)		// Pointer to the returned FDT, or NULL

	{
	PFI		pFi;
	time_t	fdt    = 0;		// The returned FDT, assume failure
	int		result = (-1);	// The returned result, assume failure
	
	if (pName)
		pFi = FileInfoOpen(FW_ALL, pName);

	if (pFi)
		{
		fdt = FileInfoFDT(pFi);
		FileInfoClose(pFi);
		result = 0;
#ifdef DEBUG
printf("fgetfdt suceeded [%lld] \"%s\"\n", fdt, pName);		// if (pSize)
#endif
		}

#ifdef DEBUG
	else
		{
printf("fgetfdt failed \"%s\"\n", pName);
		}
#endif

	if (pFdt && (result == 0))
		*pFdt = fdt;
	return (result);
	}

#else
// ---------------------------------------------------------------------------
//	fgetfdt ()  -  Get the file time/date (via filename)
//	This is the old method used mostly in the DOS days, saved for posterity
// ---------------------------------------------------------------------------
	int						// Returns 0 for success, or nonzero for failure
fgetfdt (
	const char  *pName, 	// Pointer to the path/filename
	time_t		*pFdt)		// Pointer to the returned FDT, or NULL

	{
	time_t				fdt;			// The returned date/time value
	struct _stat64		stat;			// The stat structure
	TIME_ZONE_INFORMATION  TZinfo;		// Retrieved TZ info (unused)
	int   result = 0;					// Returned result, assume success


	int statresult = _stat64(pName, &stat);
	if (statresult != 0)
		return (-1);					// Failed

	fdt = ((stat.st_mtime + 1) & ~1);	// FAT vs NTFS resolution

	if (GetTimeZoneInformation(&TZinfo) == TIME_ZONE_ID_DAYLIGHT)
		{
		char  *pBuffer;					// Path pointer
		char    FileSystem [32];		// Filesystem name
		char    Buffer     [1024];		// Path prefix

		strcpy(Buffer, pName);
		if ((pBuffer = QueryUNCPrefix(Buffer)) != NULL)
			{
            *pBuffer = '\0';
			}
		else if ((pBuffer = QueryDrivePrefix(Buffer)) != NULL)	// Single mode
			{
            *pBuffer++ = '\\';
            *pBuffer   = '\0';
			}
		else // No prefix, build path from default drive
			{
			sprintf(Buffer, "%c:\\", (char)(((int)('A') + getdrive() - 1)));
			}


		if ( ! GetVolumeInformation(
				Buffer,
				NULL,
				0,
				NULL,
				NULL,
				NULL,
				&FileSystem[0],
				sizeof(FileSystem)))
			{
			fprintf(stderr, "fgetfdt: Error getting file system type info\n");
			}
		else if (_strnicmp(FileSystem, "FAT", 3) == 0)
			{
			fdt += 3600;				// Move FAT files forward one hour during DST
			}
		}

	if (pFdt && (result == 0))
		*pFdt = fdt;
	return (result);
	}

#endif
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
