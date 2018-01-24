
#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <getoptns.h>

/*--------------------------------------------------------------------*/
    char
get_key (int  Flag, int ResponseSet)
    {
    char	c;

    if (Flag)
		c = (char)(getchar());
	
    else
	{
	for (;;)
	    {
	    while (((c = (char)(_getch())) == 0)  ||  (c == (char)(0xE0)))
			_getch();

	    c = tolower(c);
	    if ((c == 'y')
	    ||  (c == 'n'))
			break;

	    if ( ! ResponseSet)
			continue;

	    if ((c == 'r')
	    ||  (c == 'q')
	    ||  (c == 'c'))
			break;
	    }
	}

    putchar(c);
    putchar('\n');
    return (c);
    }

/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
