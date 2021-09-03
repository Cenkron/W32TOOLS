/* ----------------------------------------------------------------------- *\
|
|	fwerror () - Return the fwerrno description string
|
|	Copyright (c) 1991, all rights reserved, Brian W Johnson
|
|	15-May-91
|
\* ----------------------------------------------------------------------- */

#include  "fwild.h"

/* ----------------------------------------------------------------------- *\
|  fwerror () - Return the fwerrno description string
\* ----------------------------------------------------------------------- */
	char *				// Return the error string pointer
fwerror (void)

	{
	char  *p;


	switch (fwerrno)
		{
		case FWERR_NONE:
			p = "No error";
			break;

		case FWERR_NULL:
			p = "NULL path pointer";
			break;

		case FWERR_EMPTY:
			p = "Empty path string";
			break;

		case FWERR_DRIVE:
			p = "Misplaced drive specification";
			break;

		case FWERR_SEPARATOR:
			p = "Invalid separator";
			break;

		case FWERR_ELEMENT:
			p = "Invalid path element";
			break;

		case FWERR_SIZE:
			p = "Invalid name field size";
			break;

		case FWERR_TRAIL:
			p = "Illegal trailing separator";
			break;

		default:
			p = "???";
			break;
		}

	return (p);
	}

/* ----------------------------------------------------------------------- */
