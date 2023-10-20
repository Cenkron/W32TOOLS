/*********************************************************************\
				XCOPY
 ---------------------------------------------------------------------
			Extended COPY utility
 ---------------------------------------------------------------------
			    DFREE routine
 ---------------------------------------------------------------------
Copyright (c) 1986-2018 by J & M Software, Dallas TX - All Rights Reserved
		    Written by Michael S. Miller
\*********************************************************************/

#include <windows.h>
#include <stdio.h>

#include "fWild.h"
#include "xcopy.h"

/**********************************************************************/
#define VERSION "940602.092145"
/**********************************************************************/
	UINT64		// Returns free disk spcae, or MAXLONGLONG
dfree (
	char *path)

	{
	UINT64    retval  = MAXLONGLONG;	// Assume faillure
	UINT64    dummy;
	char    *pBuffer;
	char    *pPast;
	char     buffer [1024];


	strcpy(buffer, path);
	if ((pPast = QueryUNCPrefix(buffer)) != NULL)
		{
		*pPast = '\0';					// Terminate the string
		pBuffer = buffer;				// Point it
		}
	else if ((pPast = QueryDrivePrefix(buffer)) != NULL)
		{
		*pPast++ = '\\';				// Make it "X;\"
		*pPast = '\0';					// Terminate the string
		pBuffer = buffer;				// Point it
		}
	else // (no prefix)
		{
		pBuffer = NULL;					// Pass NULL for current drive
		}

	if ( ! GetDiskFreeSpaceEx(pBuffer,
			(PULARGE_INTEGER)(&retval),
			(PULARGE_INTEGER)(&dummy),
			(PULARGE_INTEGER)(&dummy)))
		{
		fprintf(stderr, "Disk free space test failed (%lu) for \"%s\"\n",
			GetLastError(), buffer);
		retval = MAXLONGLONG;		// Permits copying anyway
//		retval = 0;			// Prevents copying in case of error
		}

	return retval;
	}

/**********************************************************************/
#if defined(TEST_MAIN)

	int
main (
	int   argc,
	char *argv[])

	{
	printf("dfree(%s) returned %ld\n", argv[1], dfree(argv[1]));
	}

#endif
/**********************************************************************/
/******************************* EOF **********************************/
/**********************************************************************/
