#include "stdio.h"
#include "ctype.h"

#include "fwild.h"


    void
main (argc, argv)
    int    argc;
    char  *argv[];

    {
    char  *p;
    char  *path;

    if (argc <= 1)
	path = ".";
    else
	path = *++argv;

    if (p = vol_name(path))
	printf("Volume name is \"%s\"\n", p);
    else
	printf("The volume is unnamed\n");
    }
