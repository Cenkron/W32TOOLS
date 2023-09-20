/* ----------------------------------------------------------------------- *\
|
|		C-callable function to get the UNIX file time/date under either
|		MS-DOS or OS/2.
|
|							Brian W Johnson
|								26-May-90
|								 3-Apr-00 NTFS vs FAT timestamp resolution fix
|								21-Dec-08 FAT timestamp correction
|
|		Calling sequence:
|
|		return = fgetfdt (char *fnp)	Pointer to the path/filename
|
|	    time_t  return;					The returned File TimeDate
|
\* ----------------------------------------------------------------------- */

#include  <windows.h>

#include  <sys\types.h>
#include  <sys\stat.h>
#include  <stdio.h>
#include  <string.h>
#include  <time.h>

#include  "fwild.h"

/* ----------------------------------------------------------------------- *\
|  fgetfdt ()  -  Get the file time/date (via filename)
\* ----------------------------------------------------------------------- */
	time_t								/* Returns the file date/time, or -1 */
fgetfdt (
	char  *fnp)							/* Pointer to the path/filename */

	{
	time_t				fdt;			/* The returned date/time value */
	struct _stat32i64	s;				/* The stat structure */
	TIME_ZONE_INFORMATION  TZinfo;		/* Retrieved TZ info (unused) */


	int statresult = _stat32i64(fnp, &s);
	if (statresult != 0)
		return (-1L);					/* Failed */

	fdt = ((s.st_mtime + 1) & ~1);	/* FAT vs NTFS resolution */

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
			fprintf(stderr, "\007fgetfdt: Error getting file system type info\n");
			}
		else if (_strnicmp(FileSystem, "FAT", 3) == 0)
			{
			fdt += 3600;				// Move FAT files forward one hour during DST
			}
		}

	return (fdt);
	}

/* ----------------------------------------------------------------------- */
