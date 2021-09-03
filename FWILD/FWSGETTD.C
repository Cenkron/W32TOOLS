/* ----------------------------------------------------------------------- *\
|
|	Time string parse library subsystem - file extensions
|
| - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
|
|	time = fwsgettd (s);		Return the parsed UNIX time
|
|	    char  *s;			Pointer to the string to be parsed
|
|	    long  time;			The returned UNIX time
|					(or -1L for failure)
| - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
|
|	s = fwserrtd (void);		Return the error message text
|
|	    char  *s;			Pointer to the returned text string
|
| - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
|
|	Note:  All restrictions of sgettd() apply:
|               The TIMEDATE library is not reentrant.
|	Note:  serrtd() must be called following the return from sgettd().
|
| - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
|
|	17-Jan-93	Copyright (c) 1993, Brian W. Johnson,
|			All rights reserved
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <fcntl.h>
#include  <fwild.h>

/* ----------------------------------------------------------------------- *\
|  Private variables
\* ----------------------------------------------------------------------- */

static  char    buffer [128] = {0};       /* The data/error message buffer */
static  char    nofind [] = "Unable to find timedate file: \"%s\"\n";
static  char    noopen [] = "Unable to open timedate file: \"%s\"\n";
static  char    noread [] = "Unable to read timedate file: \"%s\"\n";

/* ----------------------------------------------------------------------- *\
|  Private function prototypes
\* ----------------------------------------------------------------------- */

static  long    td_of_file  (char *s);
static  long    td_in_file  (char *s);

/* ----------------------------------------------------------------------- *\
|  fwsgettd () - Parse a string to a UNIX time - Use file extensions
\* ----------------------------------------------------------------------- */
	long				/* Return the UNIX timedate, or -1L if err */
fwsgettd (
	char  *s)   		/* Pointer to the time/date string or filename */

	{
	long   timedate;			/* The returned UNIX time/date */


	if (*s == '@')
		timedate = td_in_file(s + 1);

	else if (*s == '#')
		timedate = td_of_file(s + 1);

	else
		{
		timedate = sgettd(s);
		buffer[0] = '\0';       /* No error from here */
		}

	return  (timedate);			/* Return the specified timedate */
	}

/* ----------------------------------------------------------------------- *\
|  td_of_file () - Return the timedate OF the specified file
\* ----------------------------------------------------------------------- */
	static long
td_of_file (
	char  *s)   		/* Pointer to the time/date search filename */

	{
	long   timedate = -1L;	/* The returned result */
	char  *p;				/* Pointer to the found filename */


	if ((p = fwfirst(s)) == NULL)
		sprintf(buffer, nofind, s);
	else
		{
		timedate  = fgetfdt(p);
		buffer[0] = '\0';
		}

	return (timedate);
	}

/* ----------------------------------------------------------------------- *\
|  td_in_file () - Return the timedate IN the specified file
\* ----------------------------------------------------------------------- */
	static long
td_in_file (
	char  *s)				/* Pointer to the time/date search filename */

	{
	long   timedate = -1L;		/* The returned result */
	char  *p;					/* Pointer to the found filename */
	FILE  *fp;                  /* Pointer to the opened FILE structure */


	if (((p  = fwfirst(s))    == NULL)
	||  ((fp = fopen(p, "r")) == NULL))
		sprintf(buffer, nofind, s);
	else
		{
		if (fgets(buffer, sizeof(buffer), fp) == NULL)
			sprintf(buffer, noread, p);
		else
			{
			timedate  = sgettd(strtok(buffer, " \t"));
			buffer[0] = '\0';
			}
		fclose(fp);
		}

	return (timedate);
	}

/* ----------------------------------------------------------------------- *\
|  fwserrtd () - Return the text string explaining the last parse completion
\* ----------------------------------------------------------------------- */
	char *
fwserrtd (void)

	{
	if (buffer[0] != '\0')
		return (buffer);        /* Return the previously constructed error */
	else
		return (serrtd());      /* Get the error from the lower layer */
	}

/* ----------------------------------------------------------------------- *\
|  Test main
\* ----------------------------------------------------------------------- */
#ifdef  TESTMODE

#include  <time.h>

	void
main (int argc, char *argv [])

	{
	long  retval;


	printf("String: %s\n", *++argv);
		retval = fwsgettd(*argv);

	printf("Timedate: %08lX;  Message: %s\n", retval, fwserrtd());

	if (retval > 0L)
		printf("Time: %s", _ctime32(&retval));
	}
#endif
/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */
