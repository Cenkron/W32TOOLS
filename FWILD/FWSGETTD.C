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
#include  <fWild.h>

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

static  time_t    td_of_file  (char *s);
static  time_t    td_in_file  (char *s);

/* ----------------------------------------------------------------------- *\
|  fwsgettd () - Parse a string to a UNIX time - Use file extensions
\* ----------------------------------------------------------------------- */
	time_t				/* Return the UNIX timedate, or 0 if error */
fwsgettd (
	char  *s)   		/* Pointer to the time/date string or filename */

	{
	time_t   timedate;			/* The returned UNIX time/date */


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
	static time_t		// Returns file fdt, or 0 if error
td_of_file (
	char  *s)   		/* Pointer to the time/date search filename */

	{
	time_t	fdt = 0;	/* The returned result */
	
	if ((s == NULL) || (fgetfdt(s, &fdt) != 0))
		{
		sprintf(buffer, nofind, s);
		return (0);
		}

	buffer[0] = '\0';			/* No error from here */
	return (fdt);
	}

/* ----------------------------------------------------------------------- *\
|  td_in_file () - Return the timedate IN the specified file
\* ----------------------------------------------------------------------- */
	static time_t		// Returns fdt from file, or 0 if error
td_in_file (
	char  *s)			/* Pointer to the time/date search filename */

	{
	FILE  *fp;                  /* Pointer to the opened FILE structure */
	time_t   timedate = -1L;	/* The returned result */

	if ((s == NULL) || ((fp = fopen(s, "r")) == NULL))
		sprintf(buffer, nofind, s);
	else
		{
		if (fgets(buffer, sizeof(buffer), fp) == NULL)
			sprintf(buffer, noread, s);
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
#ifdef  TEST

#include  <time.h>

	void
main (int argc, char *argv [])

	{
	time_t  retval;


	printf("String: %s\n", *++argv);
		retval = fwsgettd(*argv);

	printf("Timedate: %08llX;  Message: %s\n", retval, fwserrtd());

	if (retval > 0L)
		printf("Time: %s", ctime(&retval));
	}
#endif
/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */
