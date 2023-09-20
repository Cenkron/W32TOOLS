/* ----------------------------------------------------------------------- *\
|
|			    EXECUTE library function
|
|		    Copyright (c) 1985, 1990, all rights reserved
|		       Brian W Johnson and Mike S. Miller
|				   26-May-90
|				    9-May-91
|
\* ----------------------------------------------------------------------- */

#include  <stdlib.h>
#include  <stdio.h>
#include  <ctype.h>
#include  <string.h>
#include  <process.h>

#include  "fwild.h"

#define	LINESIZE	1024

#ifndef MATCH
#define	MATCH		0
#endif

/* ----------------------------------------------------------------------- */

static	char	cmd [LINESIZE];		/* The built command line buffer */
static	char   *comspec  = NULL;	/* The command proc name */
static	char   *cmd_pfx  = "/c";	/* The command prefix */

/* ----------------------------------------------------------------------- */
	int					/* Returns 0, or (-1) if failure */
execute (				/* Execute a command line */
	char  *p,			/* The command line to be executed */
	int    z_flag)		/* TRUE to ignore child's exit() code */

	{
	int    error;
	char  *s;
	char  *arg;


	if (comspec == NULL)
		comspec = getenv("COMSPEC");	/* Identify the command processor */
	strcpy(cmd, stpblk(p));		/* Skip over leading white space */

	for (s = cmd; *s; ++s)		/* Eliminate any carriage control */
		{
		if ((*s == '\n') || (*s == '\r'))
			{
			*s = '\0';
			break;
			}
		}

	arg = cmd;				/* Parse off the initial token */
	for (;;)
		{
		if (*arg == '\0')
			break;			/* Point no arguments */
		if (isspace(*arg))
			{
			*(arg++) = '\0';		/* Point the arguments */
			break;
			}
		*arg = tolower(*arg);
		++arg;
		}

	if (stricmp(cmd, "sh") == MATCH)	/* Check if a "sh" request */
		error = (int)(spawnl(P_WAIT, comspec, cmd, cmd_pfx, arg, NULL));
	else
		error = (int)(spawnlp(P_WAIT, cmd, cmd, arg, NULL));

	if (error < 0)			/* Try as an intrinsic or batch */
		error = (int)(spawnl(P_WAIT, comspec, cmd, cmd_pfx, cmd, arg, NULL));

	if ((error >= 0)  &&  z_flag)
		error = 0;

	return (error);
	}

/* ----------------------------------------------------------------------- */
