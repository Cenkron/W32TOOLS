/*********************************************************************\
				XCOPY
 ---------------------------------------------------------------------
                        Extended COPY utility
 ---------------------------------------------------------------------
			    CANCOPY routine
 ---------------------------------------------------------------------
Copyright (c) 1986-2018 by J & M Software, Dallas TX - All Rights Reserved
                    Written by Michael S. Miller
Copyright (c) 2007 by Brian Johnson, TX - All Rights Reserved
\*********************************************************************/

#include "stdio.h"
#include "ctype.h"

#include <io.h>
#include <fcntl.h>
#include <errno.h>

#include <fwild.h>

#include "dtypes.h"
#include "xcopy.h"

/**********************************************************************/
#define VERSION "940602.091928"
/**********************************************************************/

int	ch_disk (void);

/**********************************************************************/
    EXPORT int
can_copy (char *src, char *dst, char *path)
    {
    int 	retval = TRUE;
    int		srcfh;
    UINT64	dstSize;

    if (AZ_Flags.p)
	{
	if ((srcfh=open(src, O_RDONLY)) < 0)
	    {
	    error(src, "cannot open input file");
	    retval = FALSE;
	    goto exit;
	    }
	close(srcfh);

	if (!azFlags.n)
	    {
	    if (azFlags.r  &&  is_readonly(dst))
		clr_readonly(dst);
	    if ((unlink(dst) != 0)  &&  (errno == EACCES))
		{
		error(src, "cannot delete output file");
		retval = FALSE;
		goto exit;
		}
	    }
	}

    if (!AZ_Flags.s)
	{
	fgetsize(dst, &dstSize);

	while ( (filesize - dstSize) > (UINT64)dfree(path) )
	    {
	    error(dst, "destination disk full");
	    if ( !azFlags.c || !ch_disk() )
		{
		retval = FALSE;
		goto exit;
		}
	    }
	}

exit:

    return (retval);
    }

/**********************************************************************/
    int
ch_disk (void)
    {
    char buffer[256];

    printf(" -- change disks?  [Y/N] : ");
    gets(buffer);

    return ( (tolower(buffer[0])=='y')
             ? TRUE
             : FALSE );
    }

/**********************************************************************/
/******************************* EOF **********************************/
/**********************************************************************/
