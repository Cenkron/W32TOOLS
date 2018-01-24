/*----------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <fwild.h>
#include <dtypes.h>
#include <getoptns.h>

/*----------------------------------------------------------------------*/

GETOPT_DATA	optdata	= {0};

#define	KILO	(1024L)
#define	MEGA	(KILO*KILO)

/*----------------------------------------------------------------------*/
    int
GetOptions (
    char *	optname,
    int		argc,
    char *	argv [],
    char *	optstring)
    {
    int		iResult	= 0;
    int		c;
    int		lc;
    int (*	p) (int c, char *optarg);

    optenv = getenv(optname);

    while ((c=getopt(argc, argv, optstring)) != EOF)
	switch (lc=tolower(c))
	    {
	    case 'a':	optdata.flags.a = !optdata.flags.a;	goto finish;
	    case 'b':	optdata.flags.b = !optdata.flags.b;	goto finish;
	    case 'c':	optdata.flags.c = !optdata.flags.c;	goto finish;
	    case 'd':	optdata.flags.d = !optdata.flags.d;	goto finish;
	    case 'e':	optdata.flags.e = !optdata.flags.e;	goto finish;
	    case 'f':	optdata.flags.f = !optdata.flags.f;	goto finish;
	    case 'g':	optdata.flags.g = !optdata.flags.g;	goto finish;
	    case 'h':	optdata.flags.h = !optdata.flags.h;	goto finish;
	    case 'i':	optdata.flags.i = !optdata.flags.i;	goto finish;
	    case 'j':	optdata.flags.j = !optdata.flags.j;	goto finish;
	    case 'k':	optdata.flags.k = !optdata.flags.k;	goto finish;
	    case 'l':	optdata.flags.l = !optdata.flags.l;	goto finish;
	    case 'm':	optdata.flags.m = !optdata.flags.m;	goto finish;
	    case 'n':	optdata.flags.n = !optdata.flags.n;	goto finish;
	    case 'o':	optdata.flags.o = !optdata.flags.o;	goto finish;
	    case 'p':	optdata.flags.p = !optdata.flags.p;	goto finish;
	    case 'q':	optdata.flags.q = !optdata.flags.q;	goto finish;
	    case 'r':	optdata.flags.r = !optdata.flags.r;	goto finish;
	    case 's':	optdata.flags.s = !optdata.flags.s;	goto finish;
	    case 't':	optdata.flags.t = !optdata.flags.t;	goto finish;
	    case 'u':	optdata.flags.u = !optdata.flags.u;	goto finish;
	    case 'v':	optdata.flags.v = !optdata.flags.v;	goto finish;
	    case 'w':	optdata.flags.w = !optdata.flags.w;	goto finish;
	    case 'x':	optdata.flags.x = !optdata.flags.x;	goto finish;
	    case 'y':	optdata.flags.y = !optdata.flags.y;	goto finish;
	    case 'z':	optdata.flags.z = !optdata.flags.z;

	finish:
		if (optarg)
		    {
		    optdata.szVal[lc - 'a'] = optarg;

		    if (isdigit(*optarg))
			{
			optdata.lVal[lc - 'a'] = strtol(optarg, &optarg, 0);
			switch (tolower(*optarg))
			    {
			    case 'k':
				optdata.lVal[lc - 'a'] *= KILO;
				break;
			    case 'm':
				optdata.lVal[lc - 'a'] *= MEGA;
				break;
			    default:;
			    }
			}

		    }

		if (((p=optdata.pProc[lc - 'a']) != NULL)
		&&  ((iResult=(*p)(c, optarg)) != 0))
		    goto done;
		break;

	    case '?':
		help();
		goto done;
	    
	    default:
		iResult = (-1);
		fprintf(stderr, "%s: FATAL: invalid switch option '%c'\n",
		    optname, optchar);
		usage();
		goto done;
	    }

done:
    return (iResult);
    }

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
