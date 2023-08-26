/**********************************************************************\
                                 XCOPY
 ----------------------------------------------------------------------
                         Extended COPY utility
 ----------------------------------------------------------------------
                             VERIFY routine
 ----------------------------------------------------------------------
   Copyright (c) 1986-2018 Miller Micro Systems - All Rights Reserved
                      Written by Michael S. Miller
\**********************************************************************/

#include <stdio.h>
#include <io.h>
#include <string.h>
#include <dtypes.h>
#include <fcntl.h>

#include "xcopy.h"

/**********************************************************************/
#define VERSION "940602.092147"
/**********************************************************************/
	EXPORT int
verify (
	char *src,
	char *dst,
	char *path)

	{
	int			dstfh		=   -1;
	int			errorflg	= FALSE;
	int			srcfh;
	__int64		lsize		= filesize;

	if (!azFlags.v)
		return(0);

	if (!azFlags.l)
		{
		notify(VERIFYING, src, path, dst);
		}

	if ( (srcfh=open(src, O_RDONLY|O_BINARY)) < 0 )
		{
		error(src,"cannot open input file");
		errorflg = TRUE;
		goto err_exit;
		}

	if ( (dstfh=open(dst, O_RDONLY|O_BINARY)) < 0 )
		{
		error(dst,"cannot open output file");
		errorflg = TRUE;
		goto err_exit;
		}

	if (!init_buffer())
		{
		errorflg = TRUE;
		goto err_exit;
		}

	int		verify_size = bsize/2;
	char *	srcbuf = buffer;
	char *	dstbuf = buffer + verify_size;

	while (TRUE)
		{
		const int srccnt = read(srcfh, srcbuf, verify_size);

		if (srccnt == 0)
			goto done;
		if (srccnt == (-1))
			{
			error(src,"read error");
			errorflg = TRUE;
			goto err_exit;
			}

		const int dstcnt = read(dstfh, dstbuf, srccnt);
		if ((dstcnt == (-1))
		||  (dstcnt != srccnt))
			{
			error(dst,"read error");
			errorflg = TRUE;
			goto err_exit;
			}

		if (memcmp(dstbuf, srcbuf, srccnt) != 0)
			{
			error(dst,"verify error");
			errorflg = TRUE;
			goto err_exit;
			}

		lsize -= srccnt;
		if (lsize < (long) verify_size)
			verify_size = (unsigned int) lsize;
		if (lsize == 0)
			break;
		}

done:
	if (!azFlags.l)
	printf(" OK");
err_exit:

	close(srcfh);
	if (dstfh >= 0)
		close(dstfh);

	if (!azFlags.l)
		putchar('\n');

	return (errorflg);
	}

/*--------------------------------------------------------------------*/
/*---------------------------EOF--------------------------------------*/
/*--------------------------------------------------------------------*/
