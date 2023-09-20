/* ----------------------------------------------------------------------- *\
|
|		C-callable function to set the file time/date under either
|		MS-DOS or OS/2.
|
|							Brian W Johnson
|								26-May-90
|								17-Aug-97
|								 4-Apr-00 NTFS vs FAT timestamp resolution fix
|								21-Dec-08 FAT timestamp correction
|								13-Dec-13 Add diagnostics for fsetfdt() problem
|
|		Calling sequences:
|
|		return = fsetfdt (
|			char  *fnp,			Pointer to the path/filename
|			time_t fdt)			The new file timedate
|
|			int	   result;		0 for success, -1 for failure	
|
\* ----------------------------------------------------------------------- */

#include  <windows.h>

#include  <sys\types.h>
#include  <sys\utime.h>
#include  <stdio.h>
#include  <string.h>
#include  <time.h>

#include  "fwild.h"

/* ----------------------------------------------------------------------- *\
|  fsetfdt ()  -  Set the file time/date (via filename)
\* ----------------------------------------------------------------------- */
	int									/* Returns success (0) fail (-1) result */
fsetfdt (
	char  *fnp,							/* Pointer to the path/filename */
	time_t fdt)							/* The new File date/time */

	{
	int						result;		/* The returned result */
	struct _utimbuf			ut;			/* The time structure */
	TIME_ZONE_INFORMATION	TZinfo;		/* Retrieved TZ info (currently unused) */


	fdt = ((fdt + 1) & ~1);				/* FAT vs NTFS resolution */

	if (xporlater  &&  (GetTimeZoneInformation(&TZinfo) == TIME_ZONE_ID_DAYLIGHT))
		{
		char  *pBuffer;					// Path pointer
		char    FileSystem [32];		// Filesystem name
		char    Buffer     [1024];		// Path prefix

		strcpy(Buffer, fnp);
		if ((pBuffer = QueryUNCPrefix(Buffer)) != NULL)
			{
            *pBuffer = '\0';
			}
		else if ((pBuffer = QueryDrivePrefix(Buffer, TRUE)) != NULL)	// Single mode
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
			fprintf(stderr, "\007fsetfdt: Error getting file system type info\n");
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
		fprintf(stderr, "\007fsetfdt: Error setting file datetime (errno = %d)\n", errno);

	return (result);
	}

/* ----------------------------------------------------------------------- */
