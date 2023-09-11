/* ----------------------------------------------------------------------- *\
|
|				     FPAIR
|
|		    Copyright (c) 1985, 1990, all rights reserved
|				Brian W Johnson
|				   27-May-90
|				    9-May-91
|				   17-Aug-97
|				   10-Sep-23 Compatibility with new fnreduce
|
\* ----------------------------------------------------------------------- */

#include  <windows.h>
#include  <stdio.h>
#include  <stdlib.h>
#include  <malloc.h>
#include  <string.h>

#define  FWILD_INTERNAL

#include  "fwild.h"

/* ----------------------------------------------------------------------- */
	int
fp_init (					/* Initialize the FP system */
	FP   **fpp,				/* Pointer to the returned FP pointer */
	int    PermitFile,		/* TRUE to permit s2 to be a file name */
	char  *s1,				/* Pointer to the pathname1 string */
	char  *s2)				/* Pointer to the pathname2 string */

	{
	int    CatIndex = 0;	/* Concatenation index of the path */
	int    TermIndex;		/* Termination index of the path (not used here) */
	int    result;			/* Returned result */
	int    dflag;			/* TRUE if path 2 should be constructed */
	FP    *fp;				/* Pointer to the FP structure */


	result = FPR_SUCCESS;		/* Assume success */
	fp     = *fpp;				/* Init the FP pointer */
	if (iswild(s2))				/* Ensure non-wild path2 */
		{
		result = FPR_P2WILD;
		goto exit;
		}

    if (fnchkdir(s2))
		{
		dflag  = TRUE;			/* Using s2 as a path, constructing fnp2 */
		if (fnParse(s1, &CatIndex, &TermIndex) < 0)	/* Set pointer to construct path2 */
			{
			result = FPR_NOFILE;	/* Can't be a file */
			goto exit;
			}
		}
	else if ( ! PermitFile)
		{
		result = FPR_P2FILE;	/* Can't be a file */
		goto exit;
		}
	else if (fnchkfil(s2))
		{
		dflag = FALSE;			/* Using s2 as a filename */
		if (fnreduce(s2) < 0)
			{
			result = FPR_P2FILE;
			goto exit;
			}
		}
	else
		{
		result = FPR_NOFILE;	/* File s2 not found */
		goto exit;
		}

	if ((fp = malloc(sizeof(FP))) == NULL)
		{
		result = FPR_MEMORY;
		goto exit;
		}

	if ((fp->hp = fwinit(s1, FW_FILE)) == NULL)	/* Init the wild subsystem */
		{
		fp->sentinel = 0;
		free(fp);
		result = FPR_FWERROR;
		goto exit;
		}

	if (result == FPR_SUCCESS)
		{
		fp->sentinel = 0x55AA;
		fp->index    = CatIndex;
		fp->dflag    = dflag;
		fp->fnp2     = NULL;
		fp->s2       = s2;
		}

exit:
//	*fpp = fp;
	return (result);
	}

/* ----------------------------------------------------------------------- */
	int
fp_pair (				/* Return file pair filenames */
	FP     *fp,			/* Pointer to the FP structure */
	char  **s1,			/* Pointer to the pathname1 string */
	char  **s2)			/* Pointer to the pathname2 string */

	{
	int    result;		/* Returned result */


	result = FPR_SUCCESS;		/* Assume success */
	if (fp->sentinel != 0x55AA)
		{
		result = FPR_NOFILE;
		goto exit;
		}

	if (fp->fnp2 != NULL)
		{
		free(fp->fnp2);
		fp->fnp2 = NULL;
		}

	if ((fp->fnp1 = fwild(fp->hp)) == NULL)
		{
		fp->sentinel = 0;
		free(fp);
		result = FPR_NOFILE;
		}
	else if (fp->dflag)
		{
		if ((fp->fnp2 = fncatpth(fp->s2, (fp->fnp1 + fp->index))) == NULL)
			return (FPR_P2FILE);
		
		*s1 = fp->fnp1;
		*s2 = fp->fnp2;
		}
	else
		{
		*s1 = fp->fnp1;
		*s2 = fp->s2;
		}

exit:
	return (result);
	}

/* ----------------------------------------------------------------------- */
