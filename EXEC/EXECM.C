/* ----------------------------------------------------------------------- *\
|
|				     EXEC
|
|		    Copyright (c) 1985, 1990, 1992, all rights reserved
|				Brian W Johnson
|				   27-Apr-92
|				   21-Aug-97
|				   12-Aug-00 Quoted argument support
|
\* ----------------------------------------------------------------------- */

#include  <stdlib.h>
#include  <stdio.h>
#include  <ctype.h>

#include  "fWild.h"

#ifndef TRUE
#define FALSE	  0
#define TRUE	  1
#endif

char	copyright [] =
"Copyright (c) 1992 by J & M Software, Dallas TX - All Rights Reserved";

#define	LINESIZE	512		/* Text line buffer size */

char	buffer [BUFSIZ];		/* Buffer for stdout */

int	c_flag  = FALSE;			/* Command file flag */
int	e_flag  = TRUE;				/* Process empty lines flag */
int	q_flag  = FALSE;			/* Query flag */
int	Q_flag  = TRUE;		        /* Quote flag */
int	w_flag  = FALSE;			/* Wait code flag */
int	x_flag  = FALSE;			/* Execute flag */
int	y_flag  = TRUE;				/* Echo command flag */
int	z_flag  = FALSE;			/* Ignore child error flag */

char	swch    = '-';			/* The user's switch character */
char	metach  = '%';			/* The meta-character */

char   *cfnp    = NULL;			/* Command input filename */
char   *ifnp    = NULL;			/* Input lines filename */

void dprint (char **);

extern	void	init	(void);
extern	void	process (char *infile, int argc, char **argv);

/* ----------------------------------------------------------------------- */
    void
main (
    int    argc,
    char  *argv [])

	{
	char  *s;				/* Parser temporary */


	setbuf(stdout, buffer);
	init();

	while (--argc > 0 && (*++argv)[0] == swch)
		{
		for (s = argv[0] + 1; *s; s++)
			{
			switch (tolower(*s))
				{
				case 'c':
					++c_flag;
					if ((--argc > 0) && (*++argv)[0] != '-')
						cfnp = *argv;
					else
						{
						fprintf(stderr, "\7Missing command filename\n");
						usage();
						}
					goto next;

				case 'i':
					if ((--argc > 0) && (*++argv)[0] != '-')
						ifnp = *argv;
					else
						{
						fprintf(stderr, "\7Missing input filename\n");
						usage();
						}
					break;

				case 'm':
					if ((metach = *(s + 1)) == '\0')
						usage();
					++s;
					break;

				case 'q':
					if (*s == 'q')
						++q_flag;
					else
						Q_flag = ( ! Q_flag);
					break;

				case 's':
					e_flag = FALSE;
					break;

				case 'w':
					++w_flag;
					break;

				case 'x':
					++x_flag;
					break;

				case 'y':
					y_flag = FALSE;
					break;

				case 'z':
					z_flag = TRUE;
					break;

				case '?':
					help();

				default:
					usage();
				}
			}
next:;
		}

	process(ifnp, argc, argv);		/* Perform the real work */
	}

/* ----------------------------------------------------------------------- */
	void
usage (void)			/* Display usage documentation */

	{
	static char  *udoc [] =
		{
		"Usage:  exec  [%c?qswxyz]  [%cm#]  [%cc cmdfile]  [pattern]",
		"        exec %c?  for help",
		NULL
		};

	dprint(udoc);
	exit(1);
	}

/* ----------------------------------------------------------------------- */
	void
help (void)				/* Display help documentation */

	{
	static char  *hdoc [] =
	{
	"Usage:  exec [%c?qswxyz] [%cm#] [%cc cmdfile] [%ci infile] [pattern]",
	"",
	"exec constructs a sequence of DOS commands.  The commands are",
	"constructed from a supplied pattern, using a supplied list",
	"of arguments and filename macro expansions.",
	"",
	"    %cc <cmdfile>  directs exec to take the prototype pattern",
	"         from the named file instead of from the command line",
	"    %ci <infile> directs exec to take the input lines from",
	"         the named file instead of from stdin",
	"    %cm#  sets the meta-character to the character specified;",
	"         the default meta-character is '%%'",
	"    %cq   queries before executing commands",
	"    %cQ   don't quote arguments received as quoted",
	"    %cs   skips empty input lines from stdin",
	"    %cw   reports failing child exit(n) codes",
	"    %cx   executes the commands as they are constructed",
	"    %cy   does not echo the command as they are executed (%cx)",
	"    %cz   ignores command exit (%cx) codes",
	"",
	"For each input line read from stdin, the pattern supplied on the",
	"command line is used to construct a DOS command line.  Each DOS",
	"command constructed is written to stdout.  If the %cx option is",
	"used, the command is also executed immediately.",
	"",
	"Macro expansions are performed using the following constructs,",
	"where N is a single decimal digit in the range [0...9],",
	"and the meta-character may be changed as described above:",
	"",
	"    %%[dpnelu]N  assumes that token N is a filename, and extracts",
	"         the specified portions from token N as follows:",
	"    %%N   copies token N intact from the current input line",
	"    %%dN  discards the drive specification from token N",
	"    %%pN  discards the drive/path specification from token N",
	"    %%nN  discards the drive/path/name specification from token N",
	"    %%eN  discards the file extension specification from token N",
	"    l     converts the filename to lower case",
	"    u     converts the filename to upper case",
	"",
	"    %%-xxxN selectively discards individual fields from token N",
	"    %%+xxxN selectively includes individual fields from token N",
	"",
	"The input lines are always read from stdin.  stdin may be",
	"piped in, or redirected.  The input line tokens are separated",
	"by whitespace, and numbered starting from zero.",
	"",
	"The DOS commands are always written to stdout.  stdout may be",
	"piped out, or redirected.",
	"",
	copyright,
	NULL
	};

	dprint(hdoc);
	exit(0);
	}

/* ----------------------------------------------------------------------- */
	void
dprint (				/* Print documentation text */
	char  **dp)			/* Document array pointer */

	{
	while (*dp)
		{
		printf(*(dp++), swch, swch, swch, swch);
		putchar('\n');
		}
	}

/* ----------------------------------------------------------------------- */
