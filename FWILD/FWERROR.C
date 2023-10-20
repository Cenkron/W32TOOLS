/* ----------------------------------------------------------------------- *\
|
|	fwerror () - Return the fwerrno description string
|
|	Copyright (c) 1991, all rights reserved, Brian W Johnson
|
|	15-May-91
|
\* ----------------------------------------------------------------------- */

#include  "fWild.h"

	int	fwerrno = FWERR_NONE;

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

  		case FWERR_INVALID:
			p = "Invalid pattern";
			break;

  		case FWERR_INSTANCE:
			p = "Invalid instance pointer";
			break;

  		case FWERR_PHYSICAL:
			p = "Non-physical device";
			break;

  		case FWERR_NOMEM:
			p = "Not enough memory";
			break;

		default:
			p = "???";
			break;
		}

	return (p);
	}

/* ----------------------------------------------------------------------- */
