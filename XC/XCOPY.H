/*********************************************************************\
                                XCOPY
 ---------------------------------------------------------------------
                        Extended COPY utility
 ---------------------------------------------------------------------
 Copyright (c) 1986-2018 Miller Micro Systems - All Rights Reserved
                    Written by Michael S. Miller
|	 4-Nov-03 /Tnnn option (for NTFS problems)
Copyright (c) 2007 by Brian Johnson, TX - All Rights Reserved
\*********************************************************************/

/*********************************************************************/
//#define VERSION "940602.092321"
/*********************************************************************/

#pragma once

#include <dtypes.h>

#define GET_MODE        0
#define MIN_BUF_SIZE    (long)(2048)
//#define MAX_BUF_SIZE    (long)(0x8000 - 512)
#define MAX_BUF_SIZE    (long)(1000000 - 512)
#define BUF_INC         512

typedef unsigned _int64  UINT64;
typedef   signed _int64  INT64;

extern          A_Z_FLAGS azFlags;
extern          A_Z_FLAGS AZ_Flags;

extern          int     mode;
extern          INT64   filesize;
extern          char    temp_name [80];
extern          char    path_char;
extern          int     cols;

extern          time_t  o_time;
extern          time_t  y_time;

extern char *           buffer;
extern unsigned int     bsize;

extern time_t	timedelta;
#define  OneHour (60L * 60L)

extern  int     can_copy (char *, char *, char *);
extern  void    catpth (char *s, char *t);
extern  int     ch_disk (void);
extern  int     clr_readonly (char *filename);
extern  int     copy (char *, char *, char *);
extern  int     copy_loop (char *src, int srcfh, char *dst, int dstfh);
extern  INT64   dfree (char *);
extern  void    error (char *, char *);
extern  void    fatal (char *);
extern  int     get_screen_width (void);
extern  time_t  gfildt();
extern  char *  gfn (char*);
extern  int     init_buffer (void);
extern  int     is_readonly (char *filename);
extern  void    kill (char *);
extern  int     main (int, char * *);
extern  int     may_copy (char *, char *, char *);
extern  void    notfound (char *fn);
extern  void    process (char *src, void *hp, char *dst, char *path);
extern  int     set_readonly (char *filename);
extern  int     should_copy (char *src, void *hp, char *dst);
extern  int     verify (char *, char *, char *);

extern	void	notify (int operation, char *src, char *path, char *dst);
#define	COPYING		0
#define	MAYCOPY		1
#define	MOVING		2
#define	VERIFYING	3
#define	DELETING	4

/*********************************************************************/
/*********************************************************************/
/*********************************************************************/
