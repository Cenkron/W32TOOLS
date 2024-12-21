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

#include <fWild.h>

#include "dtypes.h"
#include "xcopy.h"

/**********************************************************************/
#define VERSION "940602.091928"
/**********************************************************************/

static	UINT64	spaceAvailable = 0;


/**********************************************************************/
	static int
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
	static int
CheckSpace (
	char   *path,
	UINT64	chkSize)

	{
	if (chkSize >= spaceAvailable)
		spaceAvailable = dfree(path);
	if (chkSize < spaceAvailable)
		{
		spaceAvailable -= chkSize;
		return TRUE;
		}
	return FALSE;
	}

/**********************************************************************/

	EXPORT int
can_copy (
	char *src,
	char *dst,
	char *path)

	{
	int 	retval = TRUE;
	int		srcfh;
	UINT64	dstSize;
	

	if (v_flag >= 2)
		{
printf("CANcopy src: \"%s\"\n", src);
printf("CANcopy dst: \"%s\"\n", dst);
printf("CANcopy pth: \"%s\"\n", path);
		}

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
			
#if 1 // 24-Sep-23 No need to unlink it, just overwrite it
			if ((unlink(dst) != 0)  &&  (errno == EACCES))
				{
				error(src, "cannot delete output file");
				retval = FALSE;
				goto exit;
				}
#endif
			}
		}

	if (!AZ_Flags.s)
		{
		fgetsize(dst, &dstSize);


//printf("dstSize %lldd\n", dstSize);

	if ((! azFlags.p)  &&  (! AZ_Flags.s))
		spaceAvailable += dstSize;	

	while ( ! CheckSpace(path, dstSize))
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
/******************************* EOF **********************************/
/**********************************************************************/
