/* ----------------------------------------------------------------------- *\
|
|				     FCOPY
|
|		    Copyright (c) 1985, 1990, all rights reserved
|				Brian W Johnson
|				   27-May-90
|
\* ----------------------------------------------------------------------- */

#include  <io.h>
#include  <fcntl.h>
#include  <sys\types.h>
#include  <sys\stat.h>

#include  "fwild.h"


#define	DATASIZE	16384		/* Size of the file copy buffer */

static	char	cbuffer [DATASIZE];

/* ----------------------------------------------------------------------- */
    int
fcopy (				/* Copy one file from fnp1 to fnp2 */
    char  *fnp1,		/* Input file name 1 */ 
    char  *fnp2,		/* Output file name 2 */ 
    int    flags)		/* Control flags (FCF_xxxx) */

    {
    int   result;		/* The returned result (FCR_xxxx) */
    int   aflag;		/* Altered dest attr flag */
    int   attr;			/* File attribute flags */
    int   fh1;			/* Input file handle 1 */
    int   fh2;			/* Output file handle 2 */
    int   len;			/* Length of data read */
    long  dt;			/* File datetime */


    result = FCR_SUCCESS;		/* Assume success */
    aflag  = FALSE;			/* Assume dest is not read only */
    if ((attr = fgetattr(fnp2)) >= 0)	/* Check if dest exists */
	{
	if (attr & ATT_RONLY)		/* Check if dest is read only */
	    {
	    if (flags & FCF_ROK)	/* Check if ok to copy anyway */
		{
		if (fsetattr(fnp2, (attr & ~ATT_RONLY))) /* Remove RO attr */
		    {
		    result = FCR_SATTR;	/* Unable to change attribute */
		    goto exit2;
		    }
		else
		    aflag = TRUE;	/* Remember it was read only */
		}
	    else
		{
		result = FCR_RO;	/* RO and not FCF_ROK */
		goto exit2;
		}
	    }
	}

    if ((fh1 = open(fnp1, (O_RDONLY | O_BINARY))) < 0)
	{
	result = FCR_OPN1;
	goto exit2;
	}

    if ((fh2 = popen(fnp2, (O_WRONLY | O_CREAT | O_TRUNC | O_BINARY),
	    (S_IREAD | S_IWRITE))) < 0)
	{
	result = FCR_OPN2;
	goto exit1;
	}

    while ((len = read(fh1, &cbuffer[0], DATASIZE)) > 0)
	{
	if (write(fh2, &cbuffer[0], len) != len)
	    {
	    result = FCR_WRITE;
	    break;
	    }
	}
    if (len < 0)
	result = FCR_READ;

    if (close(fh2) != 0)
	result = FCR_WRITE;

exit1:
    close(fh1);

    if ((result == FCR_SUCCESS)  &&  (flags & FCF_FDT))
	if ((dt = fgetfdt(fnp1)) < 0L)		/* Duplicate the date/time */
	    result = FCR_GFDT;
	else if (fsetfdt(fnp2, dt))
	    result = FCR_SFDT;

    if ((result == FCR_SUCCESS)  &&  (flags & FCF_ATTR))
	{
	if ((attr = fgetattr(fnp1)) < 0)	/* Duplicate the attributes */
	    result = FCR_GATTR;
	else if (fsetattr(fnp2, attr))
	    result = FCR_SATTR;
	}

    else if ((result == FCR_SUCCESS)  &&  aflag)
	{
	if (fsetattr(fnp2, attr))		/* Restore the RO attribute */
	    result = FCR_SATTR;
	}

exit2:
    return (result);
    }

/* ----------------------------------------------------------------------- */
