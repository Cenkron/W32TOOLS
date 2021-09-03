/* ----------------------------------------------------------------------- *\
|
|	C-callable function to get a key stroke under either MS-DOS or OS/2.
|
|			    Brian W Johnson
|			       10-Jun-90
|
|	Calling sequence:
|
|	return = getkey (void)
|
|	    unsigned int  key;		The returned key stroke
|
\* ----------------------------------------------------------------------- */

#include  <conio.h>

#include  "keys.h"

/* ----------------------------------------------------------------------- *\
|  getkey () - Get the next key
\* ----------------------------------------------------------------------- */
	unsigned int
getkey (void)

	{
	unsigned int  key;

	if ((key = getch()) == 0)
		key = (getch() << 8);

	return (key);
	}

/* ----------------------------------------------------------------------- */
