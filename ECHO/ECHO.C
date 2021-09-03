/*--------------------------------------------------------------------*\
|  ECHO - Echo the command line arguments
\*--------------------------------------------------------------------*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <fwild.h>

/*--------------------------------------------------------------------*/

	char * 
usagedoc [] = {
	"Usage:  echo [-c] arguments",
	"",
	"-c   print argument count (argc)",
	"",
	"Version 2.2 Copyright (c) 1993 J & M Software, Dallas TX - All Rights Reserved",
	NULL};

/*--------------------------------------------------------------------*/

int	c_flag	= 0;

/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
	int
main (
	int   argc,
	char *argv[])

	{
	int		c;

	optenv = getenv("ECHO");

	while ( (c=getopt(argc,argv,"cC?")) != EOF )
		{
		switch (tolower(c))
			{
			case 'c':
				++c_flag;
				break;

			case '?':
				help();

			default:
				fprintf(stderr, "invalid option '%c'\n", optchar);
				usage();
			}
		}

	while (optind < argc)
		printf("%s ", argv[optind++]);

	printf("\n");

	if (c_flag)
		printf("%d arguments\n", argc);

	return(0);
	}

/*--------------------------------------------------------------------*/
/*-------------------------- EOF -------------------------------------*/
/*--------------------------------------------------------------------*/
