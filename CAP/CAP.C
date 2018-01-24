//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>

#include "fwild.h"

//////////////////////////////////////////////////////////////////////
    char *
usagedoc [] = {
    "Usage:  function | cap file",
    "",
    "cap (pipe cap) copies stdin to a file",
    "",
    "Version 1.1 Copyright (c) 1993 J & M Software, Dallas TX - All Rights Reserved",
    NULL};

//////////////////////////////////////////////////////////////////////
    int
main (int argc, char *argv[])
    {
    FILE *	capout;
    char	record[512];

    if ((argc < 2)
    ||  (argv[1] == NULL)
    ||  (argv[1][0] == '?')
    ||  (argv[1][0] == '-'  &&  argv[1][1] == '?') )
	{
	usage();
	}
    
    if (!(capout = fopen(argv[1], "w")))
	{
	fprintf(stderr,"unable to open %s\n",argv[1]);
	exit(1);
	}

    while (fgets(record, sizeof record, stdin) != NULL)
	fputs(record, capout);

    return (0);
    }

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
