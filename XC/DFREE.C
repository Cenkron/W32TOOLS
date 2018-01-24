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
#include "xcopy.h"

/**********************************************************************/
#define VERSION "940602.092145"
/**********************************************************************/
	INT64
dfree (char *path)
	{
	INT64    retval  = (-1);
	INT64    dummy;
	char    *pBuffer;
	char     buffer [1024];


	strcpy(buffer, path);
	if ((buffer[0] == '\\')  &&  (buffer[1] == '\\'))	// If a UNC path...
		{
		int  Index = 2;				// Skip over the initial "\\"
		while ((buffer[Index] != '\\')  &&  (buffer[Index] != '\0'))
			++Index;				// Find the next '\'
		if (buffer[Index] != '\0')
			++Index;				// Advance to the first destination path element (or name)
		while ((buffer[Index] != '\\')  &&  (buffer[Index] != '\0'))
			++Index;				// Skip over the first element
		buffer[Index++] = '\\';		// Terminate it with a '\'
		buffer[Index]   = '\0';		// Pass "\\compname\destprefix\" for a UNC path
		pBuffer         = buffer;	// Point it
		}
	else if ((isalpha(buffer[0]))  &&  (buffer[1] == ':'))	// If a X: path
		{
		buffer[2] = '\\';			// Pass "X:\" for an explicit drive letter spec
		buffer[3] = '\0';
		pBuffer   = buffer;			// Point it
		}
	else							// Not UNC and not X: => is a relative path
		pBuffer   = NULL;			// Pass NULL to use current drive

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
main (int argc, char *argv[])
	{

	printf("dfree(%s) returned %ld\n", argv[1], dfree(argv[1]));
	}

#endif
/**********************************************************************/
/******************************* EOF **********************************/
/**********************************************************************/
