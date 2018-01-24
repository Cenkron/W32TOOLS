////////////////////////////////////////////////////////////////////////////
// File:        MORE_LIB.C
// AUTHOR:      Mike Miller
////////////////////////////////////////////////////////////////////////////
// Copyright (c) 1993-2018 Miller Micro Systems - All Rights Reserved
////////////////////////////////////////////////////////////////////////////
#define VERSION "930322.154651"
////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <io.h>
#include <conio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <fwild.h>
#include "more_lib.h"

////////////////////////////////////////////////////////////////////////////

#define CR      '\r'
#define LF      '\n'
#define ESC     '\033'

////////////////////////////////////////////////////////////////////////////

static  int     initialized     = FALSE;
static  int     limit           = 25;
static  int     current         = 0;
static  int     ttyflag         = TRUE;

////////////////////////////////////////////////////////////////////////////
    int
more (int n)
    {

    if (n < 0)
        ttyflag = FALSE;
    else
        ttyflag = isatty(fileno(stdout));

    if (n == 0)
        n = getrows();

    limit = n - 1;

    initialized = TRUE;

    return (limit);
    }

////////////////////////////////////////////////////////////////////////////
    static void
_more (void)
    {
    if ( ! initialized)
        more(0);
    
    if ( ! ttyflag)
        return;

    if (++current >= limit)
        {
        printf("<< more -- press <CR> for line, <ESC> for all, others for page >> ...");
        fflush(stdout);

	const int c = getch();

        switch (c)
            {
            case CR:
            case LF:
                current = limit - 1;
                break;

            case ESC:
                ttyflag = FALSE;
                break;

            default:
                current = 0;
                break;
            }

        printf("\r                                                                     \r");
        }
    }

////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
    int
mputchar (int c)
    {

    putchar(c);

    if (c == LF)
        _more();

    return (c);
    }

////////////////////////////////////////////////////////////////////////////
    static int
_mputs (char *s)
    {

    while (s && *s)
        mputchar(*s++);
    
    return (0);
    }

////////////////////////////////////////////////////////////////////////////
    int
mputs (char *s)
    {

    _mputs(s);
    return (mputchar('\n'));
    }

////////////////////////////////////////////////////////////////////////////
    int
mprintf (char *pszFmt, ...)
    {
    va_list     arg_ptr;
    char        buffer [512];

    va_start(arg_ptr, pszFmt);
    const int result = vsprintf(buffer, pszFmt, arg_ptr);
    va_end(arg_ptr);

    _mputs(buffer);

    return result;
    }

////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
    void
mhelp (void)
    {
    char **     dp = usagedoc;

    while (*dp)
        mputs(*dp++);

    exit(0);
    }

////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
#ifdef TEST_MAIN

    int
main (int argc, char *argv[])
    {
    int iResult = 0;
    int i;

    for (i=0; i < 100; ++i)
        mprintf("i = %d\n", i);

    return (iResult);
    }

#endif
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
