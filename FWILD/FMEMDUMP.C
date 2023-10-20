/* ----------------------------------------------------------------------- *\
|
|				    MEMDUMP
|
|		    Copyright (C) 1993; All rights reserved
|				Brian W. Johnson
|				   20-May-93
|				   17-Aug-97
|
\* ----------------------------------------------------------------------- */

#include  <windows.h>
#include  <stdio.h>
#include  <stdlib.h>

#define  FWILD_INTERNAL

#include  "fWild.h"
#include  "ptypes.h"

#define  LINLEN		(16)

/* ----------------------------------------------------------------------- */
	void
fmemdump (				/* Dump n bytes of memory */
	FILE   *fp,			/* The output file */
	char   *p,			/* Pointer to the byte buffer */
	int		n,			/* The number of bytes to dump */
	long	offset)		/* The offset field value */

	{
	int    i;			/* Byte counter */
	int    m;			/* Bytes in the current line */
	unsigned char  *q;	/* Pointer to the current byte */


	while (n > 0)
		{
		m = min(n, LINLEN);				/* Calculate the line length */
		n -= m;							/* Update the remaining length */

		fprintf(fp, "%08lX ", offset);	/* Print the byte number */
		offset += LINLEN;				/* Update it */

		q = (unsigned char *)(p);		/* Dump in hexadecimal */
		for (i = 0; i < LINLEN; ++i)
			{
			if ((i & 0x0003) == 0)
				fputc(' ', fp);
			if (i < m)
				fprintf(fp, "%02X ", *(q++));
			else
				fprintf(fp, "   ");
			}

		fputc(' ', fp);				/* Dump in ASCII */
		for (i = 0; i < LINLEN; ++i)
			{
			if (i >= m)
				fputc(' ', fp);
			else if ((*p >= ' ')  &&  (*p <= '~'))
				fputc(*p, fp);
			else
				fputc('.', fp);
			++p;
			}
		fputc('\n', fp);
		fflush(fp);
		}
    }

/* ----------------------------------------------------------------------- */
