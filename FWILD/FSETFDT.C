/* ----------------------------------------------------------------------- *\
|
|								fsetfdt
|
|							Brian W Johnson
|								26-May-90
|								17-Aug-97
|								 4-Apr-00 NTFS vs FAT timestamp resolution fix
|								21-Dec-08 FAT timestamp correction
|								13-Dec-13 Add diagnostics for fsetfdt() problem
|								25-Oct-23 New version for new fWild design
|
|		int					Returns 0 for success, -1 for failure	
|	fsetfdt (				Set a new timedate (write time) for a file
|		const char  *pName, Pointer to the path/filename
|		time_t fdt)			The new file timedate
|
|	For historical reasons, the fWild system uses the unix time format
|	internally, even though it has much lower resolution than the native
|	Windows timestamp.  The write timestamp is used because it is the time
|	that is visibly used by Windows, and because it best represents the
|	origin date of the data contained in the file.
|
|	The conversion from UNIX time to Windows time is don here explicitly.
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
#define	DEBUG
#endif

#if defined(DEBUG)
#include <stdio.h>
#endif

#if 1
typedef
	union _FTU
		{
		FILETIME	dummy;
		FILETIME	ft;
		UINT64		u64;
} FTU;

#define  FA_WRITE	(FILE_WRITE_ATTRIBUTES)

// ---------------------------------------------------------------------------
//	fsetfdt ()  -  Set the file time/date (via filename)
// ---------------------------------------------------------------------------
	int								// Returns success (0) fail (-1) result
fsetfdt (
	const char  *pName,				// Pointer to the path/filename
	time_t		 fdt)				// The new File date/time

	{
	int		result = 0;						// The returned result, assume success
	HANDLE  hFile = INVALID_HANDLE_VALUE;	// Handle of the open file
	FTU		ftu;							// Integer format converter

	ftu.u64 = UnixTimeToTime(fdt);	// Convert the timestamp to Windows

	if (pName)
		hFile = CreateFile(pName, FA_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

	if ((hFile == INVALID_HANDLE_VALUE)
	||  ((SetFileTime(hFile, &ftu.ft, &ftu.ft, &ftu.ft)) == 0))
//	||  ((SetFileTime(hFile, NULL, NULL, &ftu.ft)) == 0))
		{
#ifdef DEBUG
printf("fsetfdt failed \"%s\"\n", pName);
printf("Error: %d\n", GetLastError());
#endif
		result = (-1);
		}

#ifdef DEBUG
	else
printf("fsetfdt suceeded [%lld] \"%s\"\n", fdt, pName);		// if (pSize)
printf("mfdt:  %014llX\n", ftu.u64);
#endif

	if (hFile != INVALID_HANDLE_VALUE)
		CloseHandle(hFile);

	return (result);
	}

#else
// ---------------------------------------------------------------------------
//	fsetfdt ()  -  Set the file time/date (via filename)
//	This is the old method used mostly in the DOS days, saved for posterity
// ---------------------------------------------------------------------------
	int									// Returns success (0) fail (-1) result
fsetfdt (
	const char  *fnp,					// Pointer to the path/filename
	time_t		 fdt)					// The new File date/time

	{
	int						result;		// The returned result
	struct _utimbuf			ut;			// The time structure
	TIME_ZONE_INFORMATION	TZinfo;		// Retrieved TZ info (currently unused)
	char  Buffer [1024];				// Path prefix


	fdt = ((fdt + 1) & ~1);				// FAT vs NTFS resolution */

	if (GetTimeZoneInformation(&TZinfo) == TIME_ZONE_ID_DAYLIGHT)
		{
		char  *pBuffer;					// Path pointer
		char    FileSystem [32];		// Filesystem name

		pathCopy(Buffer, fnp, MAX_COPY);
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
			fprintf(stderr, "fsetfdt: Error getting file system type info\n");
			}
		else if (_strnicmp(FileSystem, "FAT", 3) == 0)
			{
			fdt -= 3600;				// Move FAT files back one hour during DST
			}
		}

	ut.actime  = fdt;
	ut.modtime = fdt;
	result     = _utime(fnp, &ut);
	if (result == -1)
		fprintf(stderr, "fsetfdt: Error setting file datetime (errno = %d)\n", errno);

	return (result);
	}
#endif
// ---------------------------------------------------------------------------
//	Test program
// ---------------------------------------------------------------------------
#ifdef TEST

#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>

	void
main ()					/* Test main program */

	{
	char   in [100];
	PFI pFi;
	time_t a_fdt;
	time_t b_fdt;
	time_t mfdt;
	

	for (;;)
		{
		printf("\nFile: ");
		gets(in);
		if (tolower(in[0]) == 'q')
			break;

		if ((fgetfdt(in, &a_fdt)) != 0)
			printf("Error getting time\n");

		printf("a_fdt: %010llX\n", a_fdt);

		if (((pFi = FileInfoOpen(FW_FILE, in)) != NULL)
		&&	((FileInfoAttr(pFi) & ATT_FILE) == ATT_FILE))
			{
			b_fdt	= FileInfoFDT(pFi);
			mfdt	= FileInfoMtime(pFi);
			}
		if (pFi)
			FileInfoClose(pFi);

		printf("b_fdt: %010llX\n", b_fdt);
		printf("mfdt:  %014llX\n", mfdt);

		a_fdt += 1;

		if (fsetfdt(in, a_fdt) != 0)
			printf("Error setting time\n");
		}
	}

#endif // TEST
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
