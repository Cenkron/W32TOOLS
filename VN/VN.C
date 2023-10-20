#include "stdio.h"
#include "ctype.h"

#include "fWild.h"


	void
main (
	int    argc,
	char  *argv[])

	{
	char  *p;
	char  *path;

	if (argc <= 1)
		path = ".";
	else
		path = *++argv;

	if (p = volName(path))
		printf("Volume name is \"%s\"\n", p);
	else
		printf("The volume is unnamed\n");
	}
