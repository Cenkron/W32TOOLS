/* ----------------------------------------------------------------------- *\
|
|					   fnFileType
|
|			    Copyright (c) 2023, all rights reserved
|					Brian W Johnson
|				   30-Sep-23 (New)
|
|	    int			Returns Filetype bitmap, or -1 if error
|	fnFileType (
|	    char  *s)	Path/filespec
|
\* ----------------------------------------------------------------------- */

#include  <windows.h>
#include  <stdio.h>
#include  <sys\stat.h>
#include  <tchar.h>

#include  "fWild.h"

/* ------------------------------------------------------------------------ */
//	#define TEST	// Define this to include the test main
//	#define DEBUG	// Define this to include the diagnostics

#ifdef  TEST
#define DEBUG
#endif

#define ERROR_MASK	(0xFFFFFFFF)
#define FLAGS_MASK	(0xFFF0)
#define	PATHCH	('\\')

/* ------------------------------------------------------------------------ */
	int				// Return Filetype mask, or -1 if error
fnFileType (
	const char *pPathspec)	// Ptr to the pathspec

	{
	int				result;
	struct __stat64 buffer;


	result = _stat64(pPathspec, &buffer);

#ifdef DEBUG
printf("result 1: %04X\n", result);
#endif

	if ((result & ERROR_MASK) == ERROR_MASK)
		result = -1;
	else
		result = (buffer.st_mode & FLAGS_MASK);

#ifdef DEBUG
printf("result 2: %08X\n", result);
#endif

	return (result);			// Error, probably nonexistent file/directory
	}

/* ------------------------------------------------------------------------ */
	int				// Return TRUE if pPathspec is an existing directory
isDirectory (
	const char *pPathspec)	// Ptr to the pathspec

	{
	int	result = fnFileType(pPathspec);

	return ((result > 0)  &&  ((result & _S_IFDIR) != 0));
	}

/* ------------------------------------------------------------------------ */
	int				// Return TRUE if pPathspec is an existing file
isFile (
	const char *pPathspec)	// Ptr to the pathspec

	{
	int	result = fnFileType(pPathspec);

	return ((result > 0)  &&  ((result & _S_IFREG) != 0));
	}

/* ----------------------------------------------------------------------- */
#ifdef TEST
main ()					/* Test main program */

	{
	char  s [1024];
	int result;

	for (;;)
		{
		printf("\nPattern: ");
		gets(s);
		printf("\n");
		result = fnFileType(s);
		printf("\nfnFileType:  %d\n", (result));
		result = isFile(s);
		printf(  "isFile:      %s\n", (result ? "TRUE" : "FALSE"));
		result = isDirectory(s);
		printf(  "isDirectory: %s\n", (result ? "TRUE" : "FALSE"));
		}
	}

#endif // TEST
/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */
