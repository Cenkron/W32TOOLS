/*********************************************************************\
                                XCOPY
 ---------------------------------------------------------------------
                        Extended COPY utility
 ---------------------------------------------------------------------
                            SHCOPY routine
 ---------------------------------------------------------------------
 Copyright (c) 1986-2018 Miller Micro Systems - All Rights Reserved
                    Written by Michael S. Miller
|	 4-Nov-03 /Tnnn option (for NTFS problems)
Copyright (c) 2007-2010 by Brian Johnson, TX - All Rights Reserved
\*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dtypes.h>
#include <fwild.h>

#include "xcopy.h"

/**********************************************************************/
#define VERSION "940602.092146"
/**********************************************************************/

#define EXIST(x)        (x!=(-1L))
#define MISSING(x)      (x==(-1L))
#define NEWERDT(x)      (x>=y_time)
#define OLDERDT(x)      (x<=o_time)

#define NEWER(x,y)      ((x+timedelta)>y)
#define OLDER(x,y)      ((x+timedelta)<y)
#define DIFFERENT(x,y)  ((x+timedelta)!=y)

#define ARCHIVE(x)      (x&ATT_ARCH)

/**********************************************************************/
	EXPORT int
should_copy (
	char *src,
	void *hp,
	char *dst)

	{
	BOOL        copy_flag = FALSE;
	int         attrib	    = 0;
	long        dst_dt;
	long        src_dt;
	UINT64      dst_size;
	UINT64      src_size;
	char       *abssrc;


	if ((abssrc = fnabspth(src)) == NULL)
		fatal("src filespec error");

	if (stricmp(abssrc, dst) == 0)
		{
		free(abssrc);
		fatal("Cannot copy file to self");
		}
	free(abssrc);

	if (azFlags.o || azFlags.y)
		azFlags.e = FALSE;

	if (! (azFlags.m|azFlags.y|azFlags.o|AZ_Flags.y|AZ_Flags.o|azFlags.e|azFlags.d|AZ_Flags.d|azFlags.u) )
		{
		copy_flag = TRUE;
		goto exit;
		}


	if (azFlags.u)
		attrib = fwtype(hp);

	src_dt   = fgetfdt(src);
	dst_dt   = fgetfdt(dst);
	fgetsize(src, &src_size);
	fgetsize(dst, &dst_size);
//	printf("%%ld %ld %ld %ld\n", src_dt, dst_dt, src_size, dst_size);

	if ((azFlags.m  && MISSING(dst_dt) )
	||  (azFlags.e  && EXIST(dst_dt) )
	||  (azFlags.y  && EXIST(dst_dt) && NEWER(src_dt, dst_dt) )
	||  (azFlags.o  && EXIST(dst_dt) && OLDER(src_dt, dst_dt) )
	||  (azFlags.d  && EXIST(dst_dt) && DIFFERENT(src_dt, dst_dt) )
	||  (AZ_Flags.d && EXIST(dst_dt) && DIFFERENT(src_size, dst_size) )
	||  (azFlags.u  && ARCHIVE(attrib) )
	||  (AZ_Flags.y && NEWERDT(src_dt) )
	||  (AZ_Flags.o && OLDERDT(src_dt) ) )
		{
		copy_flag = TRUE;
		}

exit:

	return (copy_flag);
	}

/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
