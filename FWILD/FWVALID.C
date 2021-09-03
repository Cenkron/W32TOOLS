/* ----------------------------------------------------------------------- *\
|
|	fwvalid () - Syntactically validate a file path string
|
|	Copyright (c) 1991, all rights reserved, Brian W Johnson
|
|	15-May-91
|	17-Aug-97  Neutered for NT
|	24-Sep-97  UNC support
|       29-Aug-00  Filenames beginning with "."
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <ctype.h>

#include  "fwild.h"

#ifdef  DIAGNOSTICS
#define  diagprintf(a,b)	printf(a,b)
#else
#define  diagprintf(a,b)
#endif

#define  MAXNAME  (1024)
#define  MAXEXT   (1024)

/* ----------------------------------------------------------------------- */

#define  S_INIT		(0)		// Nothing scanned yet
#define  S_DRIVE	(1)		// Drive specification scanned
#define  S_SEPARATOR	(2)		// Separator last scanned
#define  S_ELEMENT	(3)		// Path element last scanned

static	int	sepcount = 0;		// Separator count
static	int	state    = S_INIT;	// Current FSM state
static	char   *s        = NULL;	// Pointer into the pathname

		int	fwerrno  = FWERR_NONE;	// fwvalid() error result


static	int	parse (void);
static	int	pathname (void);

/* ----------------------------------------------------------------------- *\
|  fwvalid () - Scan and validate an entire path
\* ----------------------------------------------------------------------- */
	int					// Return 0 for success, FWERR_??? for failure
fwvalid (
	char *pathname)		// Pointer to the pathname

	{
	s        = pathname;		// Install the pathname pointer
	state    = S_INIT;			// Initialize the FSM state
	fwerrno  = FWERR_NONE;		// Assume success
	sepcount = 0;				// Clear the slash count

	if (s == NULL)
		{
		fwerrno = FWERR_NULL;	// NULL pathname pointer passed
		goto exit;
		}

	if (*s == '\0')
		{
		fwerrno = FWERR_EMPTY;	// Empty pathname passed
		goto exit;
		}

	while (*s != '\0')
		{
		if (parse() != FWERR_NONE)
			goto exit;			// The parse failed
		}

	if ((state == S_SEPARATOR)  &&  (sepcount > 1))
		fwerrno = FWERR_TRAIL;	// Disallowed trailing slash

exit:
	return (fwerrno);
	}

/* ----------------------------------------------------------------------- *\
|  parse () - Scan and validate a path field
\* ----------------------------------------------------------------------- */
	static int		// Return 0 for success, FWERR_??? for failure
parse (void)

	{
	// Recognize a valid drive specification

	if (isalpha(*s)  &&  (*(s+1) == ':'))
		{
		s += 2;
		if (state == S_INIT)
			{
			state  = S_DRIVE;
			diagprintf("x: parsed; next:%c\n", *s);
			}
		else
			fwerrno = FWERR_DRIVE;
		}

	// Recognize a valid path separator ("/" or "\") (or maybe a UNC)

	else if ((*s == '\\')  ||  (*s == '/'))
		{
		++s;
		if (state != S_SEPARATOR)
			{
			if ((state == S_INIT)  &&  (*s == '\\'))
				++s;
			state  = S_SEPARATOR;
			++sepcount;
			diagprintf("/ parsed; next:%c\n", *s);
			}
		else
			fwerrno = FWERR_SEPARATOR;
		}

	// Recognize the "." or ".." path elements

//	else if (((*s == '.')                    &&  ((*(s+1) == '/')  ||  (*(s+1) == '\\')  ||  (*(s+1) == '\0')))
//	  ||  ((*s == '.')  &&  (*(s+1) == '.')  &&  ((*(s+2) == '/')  ||  (*(s+2) == '\\')  ||  (*(s+2) == '\0'))))

	else if ((*s == '.')
		 &&                          (((*(s+1) == '/')  ||  (*(s+1) == '\\')  ||  (*(s+1) == '\0'))
			 || ((*(s+1) == '.')  &&  ((*(s+2) == '/')  ||  (*(s+2) == '\\')  ||  (*(s+2) == '\0')))))
		{
		++s;
		if (*s == '.')
			++s;
		if (state != S_ELEMENT)
			{
			state  = S_ELEMENT;
			diagprintf(". or .. parsed; next:%c\n", *s);
			}
		else
			fwerrno = FWERR_ELEMENT;
		}

	// Recognize the "**" path element

	else if ((*s == '*')  &&  (*(s+1) == '*'))
		{
		s += 2;
		if (state != S_ELEMENT)
			{
			state  = S_ELEMENT;
			diagprintf("** parsed; next:%c\n", *s);
			}
		else
			fwerrno = FWERR_ELEMENT;
		}

	// Recognize a valid path element name

	else if (pathname() == FWERR_NONE)
		{
		if (state != S_ELEMENT)
			{
			state  = S_ELEMENT;
			diagprintf("name parsed; next:%c\n", *s);
			}
		else
			fwerrno = FWERR_ELEMENT;
		}

	return (fwerrno);
	}

/* ----------------------------------------------------------------------- *\
|  pathname () - Scan a valid path element name, 8.3 type
\* ----------------------------------------------------------------------- */
	static int			// Return 0 for success, FWERR_??? for failure
pathname ()

	{
	char  ch;			// The current character

	while ((ch = *s) != '\0')
		{
		if (iscntrl(ch))	// Allows 0x80 - 0xFE
//		if ( ! isprint(ch))        (previous version, fails for weird names in XP)
			{
			fwerrno = FWERR_ELEMENT;
			break;
			}
		if ((ch == '\\')  ||  (ch == '/'))
			break;
		++s;
		}

    return (fwerrno);
    }

#if 0	// Old DOS version
	{
	char  ch;			// The current character
	int   count;		// The character counter
	int   field;		// The field counter

    for (field = 0; field <= 1; ++field)
		{
		for (count = 0; ; ++count)
			{
			ch = *s;
			if (( ! isprint(ch))  ||  (ch == '.')
			||  (ch == '\\')      ||  (ch == '/'))
				break;
			++s;
			}

		if (field == 0)
			{
			if ((count < 1)  ||  (count > MAXNAME))
				{
			fwerrno = FWERR_SIZE;
			break;
			}
		if (ch != '.')
			break;
		++s;
		}
	else  // (field == 1)
		{
		if (count > MAXEXT)
			fwerrno = FWERR_SIZE;
		break;
		}
	}

	return (fwerrno);
	}
#endif

/* ----------------------------------------------------------------------- */
