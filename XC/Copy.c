/*********************************************************************\
						XCOPY
 ---------------------------------------------------------------------
				Extended COPY utility
 ---------------------------------------------------------------------
					COPY routine
 ---------------------------------------------------------------------
 Copyright (c) 1986-2018 Miller Micro Systems - All Rights Reserved
                    Written by Michael S. Miller
 Copyright (c) 2007 by Brian Johnson, TX - All Rights Reserved
\*********************************************************************/

#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "dtypes.h"
#include "fwild.h"
#include "xcopy.h"

/**********************************************************************/
#define VERSION "940602.092145"
/**********************************************************************/
	EXPORT int
copy (char *src, char *dst, char *path)
	{
	int		dstfh;
	int		errorflg;   // = FALSE;
	const int	p_flag		= azFlags.p;
	int		pmode;
	int		srcfh;
	long	src_dt;

	// Set the mode for the input file
	if (AZ_Flags.a)
		pmode = O_TEXT;
	else
		pmode = O_BINARY;

	// Set the mode for the output file
	if (AZ_Flags.a && !(AZ_Flags.u|AZ_Flags.r))
		_fmode = O_TEXT;
	else
		_fmode = O_BINARY;

	if (!azFlags.l)
		{
		if (azFlags.v && !azFlags.q)
			printf("\n");

		if (!azFlags.q)
			notify(COPYING, src, path, dst);
		}

	int dst_attr = fgetattr(dst);

	if (dst_attr != -1
	&&  (azFlags.h || azFlags.s)
	&&  (dst_attr & (_A_HIDDEN | _A_SYSTEM)) != 0)
		fsetattr(dst, (dst_attr & ~(_A_HIDDEN | _A_SYSTEM)));

	if (is_readonly(dst))
		{
		if ( !azFlags.r )
			{
			error(dst, "read-only: use -r switch");
			errorflg = TRUE;
			goto err_exit;
			}
		else
			{
			if (!azFlags.n)
			clr_readonly(dst);
			}
		}

	if ( (srcfh=open(src, O_RDONLY | pmode)) < 0 )
		{
		error(src, "cannot open input file");
		errorflg = TRUE;
		goto err_exit;
		}

	if ((!azFlags.n)
	&&  ( (dstfh=pcreat(dst, S_IWRITE)) < 0 ))
		{
		error(dst, "cannot open output file");
		errorflg = TRUE;
		goto err_exit;
		}

	if (!init_buffer())
		{
		errorflg = TRUE;
		goto err_exit;
		}

	// ReSharper disable once CppDeclaratorMightNotBeInitialized
	errorflg = copy_loop(src, srcfh, dst, dstfh);

	if (errorflg)
		goto err_exit;

	if (!azFlags.n)
		{
		if (p_flag)
			unlink(temp_name);
#if 1 // BWJ THIS IS A TEST FOR W7 - retry this until it takes?
		{
		int count = 25;
		do  {
			if ((src_dt=fgetfdt(src)) < 0)
				error(dst, "error getting source time/date");

			else if (fsetfdt(dst, src_dt) != 0)
				error(dst, "error setting destination time/date");
			} while ((fgetfdt(dst) != fgetfdt(src))  &&  (--count > 0));
		}
#else
	if ((src_dt=fgetfdt(src)) < 0)
		error(dst, "error getting source time/date");

	else if (fsetfdt(dst, src_dt) != 0)
		error(dst, "error setting destination time/date");
#endif
		}

err_exit:

	if (!azFlags.n)
		{
		int src_attr = fgetattr(src);
		if (dst_attr == (-1))
			dst_attr = 0;

		if ((azFlags.a && !errorflg)
		&& (fsetattr(src, (src_attr &= ~_A_ARCH)) < 0))
			{
			error(src, "cannot change attributes");
			errorflg = TRUE;
			}

		if (errorflg && p_flag)
			{
			unlink(dst);
			if (rename(temp_name,dst) == (-1))
				error(dst, "unsuccessful unprotect");
			}
		else
			{
			if (!errorflg && (src_attr != (-1)))
				if (fsetattr(dst, src_attr|dst_attr) < 0)
					{
					error(dst, "cannot change attributes");
					errorflg = TRUE;
					}
			}
		}

	if (!azFlags.l && !azFlags.q)
		putchar('\n');
	    
	return (errorflg);
	}

/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*\

	---left---   srclen -middle- dstlen --------right----------

	Copy_______    %s    __to__    %s   ?[Y/N/R/QC]_:_  %c<ret>
	Copying____                         OK<ret>
	Moving_____
	Verifying__
	Deleting___

\*--------------------------------------------------------------------*/
	static void
InterSpace (char *src, char *dst)
	{
	int		spacing;
	const int	left	= 11;
	const int	middle	= 6;
	int		right;

	if (azFlags.q)
		right = 18;
	else if (azFlags.v)
		right = 4;
	else
		right = 1;

	const int srclen = (int)(strlen(src));
	const int dstlen = (int)(strlen(dst));

// Note that 'cols' is half the screen width (40 on 80x25 screen)

	if ((srclen+dstlen+left+middle+right) > (2*cols))
		spacing = 0;
	else if ((dstlen+middle+right) > cols)
		spacing = 2*cols - srclen - dstlen - left - middle - right;
	else
		spacing = cols - srclen - left;

	while (spacing--  >  0)	// takes care of negative case too
		putchar(' ');
	}

/*--------------------------------------------------------------------*/
	void
notify (int operation, char *src, char *path, char *dst)
	{
	char *	s	= (azFlags.b ? path : dst);

	if (AZ_Flags.l  ||  (operation != COPYING))
		{
		switch (operation)
			{
			case MAYCOPY:
				printf("Copy       ");
				break;
	    
			case COPYING:
				printf("Copying    ");
				break;

			case MOVING:
				printf("Moving     ");
				break;
	    
			case VERIFYING:
				printf("Verifying  ");
				break;
	    
			case DELETING:
				printf("Deleting   ");
				break;

			default: 
				;
			}
		}

	printf("%s", src);

	if ((AZ_Flags.l  ||  (operation != COPYING))  &&  (s))
		{
		InterSpace(src, s);
		printf("  to  %s", s);
		}

	if (operation == MAYCOPY)
		printf("?  [Y/N/R/QC] : ");
	}

/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
	int
copy_loop (char *src, int srcfh, char *dst, int dstfh)
	{
	int		errorflg	= FALSE;
	UINT64	readsize;

	if ((!azFlags.n)  &&  AZ_Flags.v)
		{
	if (fgetsize(src, &readsize) == 0)
		printf(" [%I64u]", readsize);
	else
		printf(" Cannot size %s", src);
	}

	if (hgetsize(srcfh, &readsize) != 0)
		{
		if (!azFlags.n)
			{
			if (AZ_Flags.v)
				printf(" (forced to 0)");
			(void)chsize(dstfh, 0);
			}
		}
	else
		{
		if (AZ_Flags.v)
			printf(" (%llu)", readsize);
		while (TRUE)
			{
			const int readcnt = read(srcfh, buffer, bsize);
//BWJ
//printf("\nReadsize: %ld\n", readcnt);
			if (readcnt == (-1))
				{
				error(src, "read error");
				errorflg = TRUE;
				goto err_exit;
				}

			if (readcnt == 0)
				break;

			if (AZ_Flags.r)
				{
				// convert all '\n' to '\r' for OS-9
				int	i;
				char *p;

				for (i=readcnt,p=buffer;  i;  --i,++p)
					if (*p == '\n')
						*p =  '\r';
				}

			int writecnt = readcnt;
    
			if (!azFlags.n)
				{
				writecnt = write(dstfh, buffer, writecnt);
				if ( readcnt != writecnt )
					{
					error(dst, "write error");
					errorflg = TRUE;
					goto err_exit;
					}
				}
			}
		}

err_exit:;

	close(srcfh);

	if (!azFlags.n)
		close(dstfh);

	return (errorflg);
	}

/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
	int
init_buffer (void)
	{
	int		size	= MAX_BUF_SIZE;

	while (!buffer)
		{
		if ( (buffer = (char *)malloc(size)) != NULL)
			{
			bsize = size;
			return (size);
			}
		else
			{
			size -= BUF_INC;
			if (size < MIN_BUF_SIZE)
				{
				fatal("Internal error");
				return (0);
				}
			}
		}

	return (size);
	}

/*--------------------------------------------------------------------*/
	int
is_readonly (char *filename)
	{
	int	    attrib; // = (-1);

	if ((attrib=fgetattr(filename)) < 0)
		return (0);
	else
		return (attrib & _A_RDONLY);
	}

/*--------------------------------------------------------------------*/
	int
clr_readonly (char *filename)
	{
	int	    attrib; // = (-1);

	if (((attrib=fgetattr(filename)) >= 0)
	&&  (fsetattr(filename, attrib & ~_A_RDONLY) < 0))
		error(filename, "cannot get attributes");

	return (0);
	}

/*--------------------------------------------------------------------*/
	int
set_readonly (char *filename)
	{
	int	    attrib; // = (-1);

	if (((attrib=fgetattr(filename)) >= 0)
	&&  (fsetattr(filename, attrib | _A_RDONLY) < 0))
		error(filename, "cannot get attributes");

	return (0);
	}

/*--------------------------------------------------------------------*/
/*---------------------------EOF--------------------------------------*/
/*--------------------------------------------------------------------*/
