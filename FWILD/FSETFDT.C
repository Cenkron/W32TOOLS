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
|			long   fdt)			The new file timedate
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
	long   fdt)							/* The new File date/time */

	{
	int                    result;		/* The returned result */
	struct __utimbuf32     ut;			/* The time structure */
	TIME_ZONE_INFORMATION  TZinfo;		/* Retrieved TZ info (currently unused) */


	fdt = ((fdt + 1) & ~1);				/* FAT vs NTFS resolution */

	if (xporlater  &&  (GetTimeZoneInformation(&TZinfo) == TIME_ZONE_ID_DAYLIGHT))
		{
		char  *pBuffer;					// Path pointer
		char    FileSystem [32];		// Filesystem name
		char    Buffer     [1024];		// Path prefix

		strcpy(Buffer, fnp);
		if ((Buffer[0] == '\\')  &&  (Buffer[1] == '\\'))	// If a UNC path...
			{
			int  Index = 2;				// Skip over the initial "\\"
			while ((Buffer[Index] != '\\')  &&  (Buffer[Index] != '\0'))
				++Index;				// Find the next '\'
			if (Buffer[Index] != '\0')
				++Index;				// Advance to the first destination path element (or name)
			while ((Buffer[Index] != '\\')  &&  (Buffer[Index] != '\0'))
				++Index;				// Skip over the first element
			Buffer[Index++] = '\\';		// Terminate it with a '\'
			Buffer[Index]   = '\0';		// Pass "\\compname\destprefix\" for a UNC path
			pBuffer         = Buffer;	// Point it
			}
		else if ((isalpha(Buffer[0]))  &&  (Buffer[1] == ':'))	// If a X: path
			{
			Buffer[2] = '\\';			// Pass "X:\" for an explicit drive letter spec
			Buffer[3] = '\0';
			pBuffer   = Buffer;			// Point it
			}
		else							// Not UNC and not X: => is a relative path
			pBuffer   = NULL;			// Pass NULL to use current drive

		if ( ! GetVolumeInformation(
						pBuffer,
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
	result     = _utime32(fnp, &ut);
	if (result == -1)
		fprintf(stderr, "\007fsetfdt: Error setting file datetime (errno = %d)\n", errno);

	return (result);
	}

/* ----------------------------------------------------------------------- */
