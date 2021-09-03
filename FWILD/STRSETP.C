/* ----------------------------------------------------------------------- *\
|
|				    STRSETP
|
|		    Copyright (c) 1985, 1990, all rights reserved
|				Brian W Johnson
|				   26-May-90
|
|	    void
|	strsetp (s, ch);	Sets the path character in a string
|	    char  *s;		Pointer to the string
|	    char   ch;		The desired path character
|
|	All instances of a path character in string s are changed to ch
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>

#include  "fwild.h"

/* ----------------------------------------------------------------------- */
	void
strsetp (				/* Set the path characters in a string */
    char  *s,			/* Pointer to the string */
    char   ch)			/* The desired path character */

	{
	char  testch;

	if (ch == '/')
		testch = '\\';
	else
		testch = '/';

	for ( ; *s; ++s)
		{
		if (*s == testch)
			*s = ch;
		}
	}

/* ----------------------------------------------------------------------- */
