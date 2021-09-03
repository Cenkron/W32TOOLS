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
|	    long  return;					The returned File TimeDate
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
	long								/* Returns the file date/time, or -1 */
fgetfdt (
	char  *fnp)							/* Pointer to the path/filename */

	{
	long         fdt;					/* The returned date/time value */
	struct _stat32i64  s;				/* The stat structure */
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
