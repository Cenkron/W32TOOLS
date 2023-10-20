/* ----------------------------------------------------------------------- */
/* GREPLIB.C - support routines for grep and sed                           */
/* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <io.h>
#include  <stdlib.h>
#include  <string.h>
#include  <ctype.h>

#include  "fWild.h"
#include  "sr_exter.h"
#include  "greplib.h"

/* ----------------------------------------------------------------------- */
    static char *
class_macros [] =
	{
	"[a-zA-Z]",         /* :a   alpha                   */
	"[01]",             /* :b   binary                  */
	"[a-zA-Z0-9_]",     /* :c   c identifier            */
	"[0-9]",            /* :d   decimal                 */
	"e",
	"[-+.eE0-9]",       /* :f   float                   */
	"g",
	"h",
	"i",
	"j",
	"k",
	"[a-z]",            /* :l   lower case alpha        */
	"m",
	"[a-zA-Z0-9]",      /* :n   alphanumeric            */
	"[0-7]",            /* :o   octal                   */
	"[~a-zA-Z0-9]",     /* :p   punctuation             */
	"q",
	"r",
	"[ \t\r\n]",        /* :s   space                   */
	"t",
	"[A-Z]",            /* :u   upper case alpha        */
	"v",
	"w",
	"[0-9a-fA-F]",      /* :x   hex                     */
	"y",
	"z"
	};

static  char    lbuf [LMAX];
static  char    pbuf [LMAX];

/* ----------------------------------------------------------------------- */
	char *
precompile (
	char *pattern)

	{
	char *      s       = pbuf;
	char        c;
	FILE *      fp;

	if (*pattern == INDIRECT_CHAR)
		{
		if ((fp=fopen(pattern+1, "r")) == NULL)
			cantopen(pattern+1);
        
		if (!fgetss(lbuf, sizeof(lbuf), fp))
			fatalerr("Error reading expression file\n");

		pattern = lbuf;
		fclose(fp);
		}

    /* If there are no class macros, just return the original pattern */

	if (strchr(pattern, CLASS_PFX) == NULL)
		return (pattern);

	memset(pbuf, 0, sizeof(pbuf));

	while (*pattern)
		{
		if (!Sr_metaflag)       /* -m */
			{
			if ((*pattern == ESCCHAR) && (*(pattern+1) == CLASS_PFX))
				{
				c = *(pattern+2);
				if (isalpha(c))
					{
					strcat(s, class_macros[ tolower(c) - 'a' ]);
					s += strlen(s);
					}
				else
					*s++ = c;
        
				pattern += 3;
				}
			else
				*s++ = *pattern++;
			}
		else
			{
			if (*pattern == CLASS_PFX)
				{
				c = *(pattern+1);
				if (isalpha(c))
					{
					strcat(s, class_macros[ tolower(c) - 'a' ]);
					s += strlen(s);
					}
				else
					*s++ = c;
        
				pattern += 2;
				}
			else if (*pattern == ESCCHAR)
				{
				++pattern;
				*s++ = *pattern++;
				*s++ = *pattern++;
				}
			else
				*s++ = *pattern++;
			}
		}

	*s = '\0';
	return (pbuf);
	}

/* ----------------------------------------------------------------------- */

/* ----------------------------------------------------------------------- */
	char *
fgetss (
	char  *buf,
	int    len,
	FILE  *fp)

	{
	char *p;
	char *q;

	if (p = fgets(buf, len, fp))
		{
		q = buf + strlen(buf) - 1;      /* Remove final newline, if any */
		if (*q == '\n')
			*q = '\0';
		}
	return (p);
	}

/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */
