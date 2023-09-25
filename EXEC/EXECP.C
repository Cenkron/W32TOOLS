/* ----------------------------------------------------------------------- *\
|
|			     EXEC process module
|
|		Copyright (c) 1985, 1990, 1997, all rights reserved
|				Brian W Johnson
|				   30-Apr-92
|				   20-Feb-93
|				   21-Aug-97
|				   12-Aug-00 Quoted argument support
|				   26-Aug-10 Fix quoted argument support
|
\* ----------------------------------------------------------------------- */

#include  <stdlib.h>
#include  <stdio.h>
#include  <conio.h>
#include  <ctype.h>
#include  <string.h>
#include  <malloc.h>

#include  "fwild.h"

#define	 LINESIZE	1024	/* Text line buffer size */

#define	 INCLUDE	   0	/* Include specified fields mode */
#define	 EXCLUDE	   1	/* Exclude specified fields mode */
#define	 COMPATIBILITY 2	/* Compatibility mode */

#define	 MAXTOKEN	  10	/* Max number of tokens */

extern	int	z_flag;			/* Ignore child error flag */
extern	int	Q_flag;			/* Quoted argument flag */

extern	void	usage (void);
extern	void	cantopen (char *fnp);

int		query	(void);
void	batch	(char *p);
int		exec	(char *s);

/* ----------------------------------------------------------------------- */

extern	int	c_flag;			/* Command file flag */
extern	int	e_flag;			/* Process empty lines flag */
extern	int	q_flag;			/* Query flag */
extern	int	w_flag;			/* Wait code flag */
extern	int	x_flag;			/* Execute flag */
extern	int	y_flag;			/* Echo flag */

extern	char	metach;		/* The meta-character */
extern	char	swch;		/* The user's switch character */

extern	char   *cfnp;		/* Command input filename */

/* ----------------------------------------------------------------------- */

FILE   *cfp     = NULL;		/* Command file descriptor */
FILE   *ifp     = NULL;		/* Input line file descriptor */

char   *pattern = NULL;		/* The current pattern */
char   *indata  = NULL;		/* Input line buffer */
char   *command = NULL;		/* Command line buffer */
char   *token   = NULL;		/* Token buffer */

char   *tokenptr [MAXTOKEN] = { NULL };	/* Array of token pointers */
int     quoted   [MAXTOKEN] = { 0 };	/* Array of quote flags */

void buildpat (char *[]);
int  tokenize (void);
void buildcmd (void);
char *gettoken (char **, int *);
void batch (char *);

/* ----------------------------------------------------------------------- */
	void
init ()

	{
	if (((pattern = malloc(LINESIZE)) == NULL)
	||  ((indata  = malloc(LINESIZE)) == NULL)
	||  ((command = malloc(LINESIZE)) == NULL)
	||  ((token   = malloc(LINESIZE)) == NULL))
		{
		fprintf(stderr, "\7Insufficient memory\n");
		exit(1);
		}
	swch = egetswch();
	}

/* ----------------------------------------------------------------------- */
	void
process (			/* Main program segment */
	char  *infile,	// Ptr to the input filename
	int    argc,	// The argument count
	char  *argv [])	// Ptr to the argument ptr

	{

	if (infile == NULL)		/* Accept input from a file */
		ifp = stdin;
	else
		{
		if ((ifp = fopen(infile, "r")) == NULL)
			{
			cantopen(infile);
			usage();
			}
		}

	if (c_flag)				/* Accept patterns from a file */
		{
		if ((cfp = fopen(cfnp, "r")) == NULL)
			{
			cantopen(cfnp);
			usage();
			}
		}
	else if (argc > 0)		/* Build the pattern from the command line */
		buildpat(argv);
	else
		usage();			/* We MUST have a command ! */

    // Initialization is complete, generate and execute (write) the commands

	while (fgets(indata, LINESIZE, ifp))
		{
		if (e_flag || (strlen(indata) > 1))
			{
			if (tokenize())
				{
				fprintf(stderr, "\7Too many tokens, failed\n");
				goto error;
				}
			if (c_flag)
				{
				fseek(cfp, 0L, 0);
				while (fgets(pattern, LINESIZE, cfp))
					{
					buildcmd();
					if (exec(command))
						{
						fprintf(stderr, "\7Command execution failed\n");
						goto error;
						}
					}
				}
			else
				{
				buildcmd();
				if (exec(command))
					{
					fprintf(stderr, "\7Command execution failed\n");
					goto error;
					}
				}
			}
		}

	// Perform all needed cleanup

error:
	if (c_flag)
		fclose(cfp);
	if (infile != NULL)
		fclose(ifp);
	}

/* ----------------------------------------------------------------------- */
	void
buildpat (				/* Build the pattern from the argv list */
	char *argv [])		/* Passed argv pointer */

	{
	strcpy(pattern, *argv++);
	while (*argv)
		{
		strcat(pattern, " ");
		strcat(pattern, *argv++);
		}
	}

/* ----------------------------------------------------------------------- */
	int							// Return 0, or (-1) if error
tokenize (void)             	// Separate the input line into tokens

    {
#define  ST_SPACE       (0)		// Processing whitespace
#define  ST_TOKENENTRY  (1)		// Entering a normal token
#define  ST_TOKEN       (2)		// Processing a normal token
#define  ST_QUOTEENTRY  (3)		// Entering a quoted token
#define  ST_QUOTE       (4)		// Processing a quoted token

	int    i     = 0;			// Token pointer index
	char  *p     = indata;		// Pointer to the input data line
	int    state = ST_SPACE;    // The parser state


	while (*p != '\0')
		{
		if (i >= MAXTOKEN)
			return (-1);                    // Too many tokens error

		switch (state)
			{
			case ST_SPACE:
				if (isspace(*p))            // Skip inter_token whitespace
					++p;
				else if (*p == '"')         // Check for a token
					{
					++p;                    // Skip over the quote
					state = ST_QUOTEENTRY;  // Enter a quoted token
					}
				else
					state = ST_TOKENENTRY;  // Enter an ordinary token
				break;

			case ST_TOKENENTRY:
				quoted[i] = 0;
				tokenptr[i++] = p;          // Point the ordinary token
				state = ST_TOKEN;           // Process the ordinary token
				break;

			case ST_TOKEN:
				if (isspace(*p))            // Check for whitespace
					{
					*p++ = '\0';            // NUL terminate the token
					state = ST_SPACE;       // Enter a whitespace region
					}
				else
					++p;                    // Skip over the token
				break;

			case ST_QUOTEENTRY:
				quoted[i] = Q_flag;
				tokenptr[i++] = p;          // Point the quoted token
				state = ST_QUOTE;           // Enter the quoted token
				break;

			case ST_QUOTE:
				if (*p == '"')              // Check for a quote
					{
					*p++ = '\0';            // NUL terminate the token
					state = ST_SPACE;       // Enter a whitespace region
					}
				else
					++p;                    // Skip over the token
				break;
			}
		}

	while (i < MAXTOKEN)		    // Clear the next pointer
		tokenptr[i++] = NULL;

	return (0);				    // Return success
	}

/* ----------------------------------------------------------------------- */
	void
buildcmd (void)			/* Build a command from the pattern and data */

	{
	char  *pPattern     = pattern;	/* Pointer to the current pattern */
	char  *pCommand     = command;	/* Pointer to the command string */
	char  *pQuote;					/* Pointer to the working begin quote ptr */
	char  *pBegin;					/* Pointer to the most recent first non-white char */
	char  *pToken;					/* Pointer to the macro token string */
	char   ch;						/* The current character */
	int    quoteFlag;				/* TRUE if the string is to be quoted */
	int    quoteActive  = FALSE;	/* TRUE if quoting is active */
	int    prevSpace    = TRUE;		// TRUE if the previous character was a space


//fprintf(stderr, "pattern: %s\n", pattern);
	while ((ch = *pPattern) != '\0')
		{
		if ( ! isspace(ch))					// If not white space...
			{
			if (prevSpace)
				pBegin = pCommand;			// Remember the first non-white char of the string
			prevSpace = FALSE;
			}
		else // (isspace(ch) is TRUE)		// It is whitespace...
			{
			if (quoteActive)				// If quoting is active
				*pCommand++ = '"';			// Write the suffix quote
			quoteActive = FALSE;			// End the active quote region
			prevSpace = TRUE;
			}

		if (ch == metach)					// If it is the Meta-character...
			{
			++pPattern;
			if (*pPattern == metach)		// Quoted meta-character
				*pCommand++ = *pPattern++;
			else
				{
				pToken = gettoken(&pPattern, &quoteFlag);	/* Macro token */
				if (pToken)	/* Macro token */
					{
//fprintf(stderr, "token(%d): %s\n", quoteFlag, pToken);
					if (quoteFlag && ! quoteActive)	// If quoting requested for the first time
						{
						quoteActive = TRUE;
						for (pQuote = pCommand++; (pQuote > pBegin); --pQuote)
								*(pQuote) = *(pQuote-1);	// Shift the argument buffer up one byte
							*pQuote = '"';	// Write the prefix quote
						}
					while (*pToken)			// Copy the token to the command buffer
						*pCommand++ = *pToken++;
					}
				else						// Invalid token
					{
					fprintf(stderr, "\7Invalid token: %s\n", pPattern);
					usage();
					}
				}
			}
		else if (ch == '\n')				// Ignored LF character
			++pPattern;
		else						/* Ordinary character */
			*pCommand++ = *pPattern++;
		}

	if (quoteActive)						// If quoting is not yet complete
		*pCommand++ = '"';					// Suffix quote

	*pCommand = '\0';						// Terminator
	}

/* ----------------------------------------------------------------------- */
	char *				/* Returns pointer to token */
gettoken (				/* Expand a macro expression */
	char  **pp,			/* Pointer to the input line pointer */
	int    *quote)		/* Pointer to the returned quote request */

	{
	char   ch;			/* Temporary character */
	int    i;			/* Token index */
	int    dflag;		/* Drive flag */
	int	   pflag;		/* Path flag */
	int	   nflag;		/* Name flag */
	int    eflag;		/* Extension flag */
	int    mode;		/* Metacharacter mode */
	int    lflag = 0;	/* Lower case flag */
	int    uflag = 0;	/* Upper case flag */
	char  *tokp;		/* Pointer to the token */

static	char	drive     [1024];       // The drive identification
static	char	path      [1024];       // The path identification
static	char	basename  [1024];       // The base name identification
static	char	extension [1024];       // The extension identification


	ch = **pp;					// Check for a mode determining switch
	if (ch == '+')
		{
		dflag = FALSE;				// Use INCLUDE mode
		pflag = FALSE;
		nflag = FALSE;
		eflag = FALSE;
		mode  = INCLUDE;
		++(*pp);				// Skip over the switch character
		}
	else if (ch == '-')
		{
		dflag = TRUE;				// Use EXCLUDE mode
		pflag = TRUE;
		nflag = TRUE;
		eflag = TRUE;
		mode  = EXCLUDE;
		++(*pp);				// Skip over the switch character
		}
	else
		{
		dflag = TRUE;				// Use COMPATIBILITY mode (default)
		pflag = TRUE;
		nflag = TRUE;
		eflag = TRUE;
		mode  = COMPATIBILITY;
		}

	i = -1;							// Set the sentinel value
	while (ch = *((*pp)++))
		{
		ch = tolower(ch);
		if (isdigit(ch))			// Accept the arg index
			{
			i = ch - '0';			// Select the argument
			*quote = quoted[i];		// Return the quote request
			break;
			}
		else if (ch == 'd')			// Accept the 'd' flag
			{
			if (mode == INCLUDE)
			++dflag;				// Include the drive
			else
			dflag = FALSE;			// Exclude the drive
			}
		else if (ch == 'p')			// Accept the 'p' flag
			{
			if (mode == INCLUDE)
			++pflag;				// Include the path
			else
			{
			pflag = FALSE;			// Exclude the path
			if (mode == COMPATIBILITY)
				dflag = FALSE;		// Exclude the drive
			}
			}
		else if (ch == 'n')			// Accept the 'n' flag
			{
			if (mode == INCLUDE)
				++nflag;			// Include the filename
			else
				{
				nflag = FALSE;		// Exclude the filename
				if (mode == COMPATIBILITY)
					{
					pflag = FALSE;	// Exclude the path
					dflag = FALSE;	// Exclude the drive
					}
				}
			}

		else if (ch == 'e')			// Accept the 'e' flag
			{
			if (mode == INCLUDE)
				++eflag;			// Include the extension
			else
				eflag = FALSE;		// Exclude the extension
			}
		else if (ch == 'l')			// Accept the 'l' flag
			{
			lflag = 1;
			uflag = 0;
			}
		else if (ch == 'u')			// Accept the 'u' flag
			{
			lflag = 0;
			uflag = 1;
			}
		else
			break;
		}

	if (i < 0)						// If invalid syntax, return NULL
		return (NULL);

	tokp = tokenptr[i];				// If no token, return empty string
	if (tokp == NULL)
		{
		*quote = FALSE;
		return ("");
		}

	if ((dflag == FALSE)			// If no flag set, return empty string
	&&  (pflag == FALSE)
	&&  (nflag == FALSE)
	&&  (eflag == FALSE))
		{
		*quote = FALSE;
		return ("");
		}

	if ((dflag)						// If all flags set, return the original token
	&&  (pflag)
	&&  (nflag)
	&&  (eflag))
		{
		strcpy(token, tokp);
		}
	else
		{
		_splitpath(tokp, drive, path, basename, extension);

		*token = '\0';				// Initialize the target
		if (dflag)					// Conditionally include the drive
			strcat(token, drive);

		if (pflag)					// Conditionally include the path
			strcat(token, path);

		if (nflag)					// Conditionally include the base name
			strcat(token, basename);

		if (eflag)					// Conditionally include the extension
			{
			if (nflag)
				strcat(token, extension);	// with "dot"
			else
				strcat(token, (extension + 1));	// without "dot"
			}

		if (pflag)
			{
			char  *p = &token[strlen(token) - 1];
			if ((*p == '\\')  ||  (*p == '/'))
			*p = '\0';				// Delete the terminal '\' or '/'
			}
		}


	if (lflag)						// Conditionally convert to lower case
		strlwr(token);
	else if (uflag)					// Conditionally convert to upper case
		strupr(token);

	return (token);
	}

/* ----------------------------------------------------------------------- */
	int						/* Return 0, or (-1) if error */
exec (						/* Execute the built command */
	char  *s)				/* Pointer to the command */

	{
	int   result = 0;		/* Return result code */


	if (q_flag)
		{
		printf("%s  [y/n] ? ", s);
		fflush(stdout);
		if (query() == FALSE)
			return  (0);
		}
	else
		batch(s);			/* Batch the command */

	strcat(s, "\n");
	if (x_flag)				/* Execute the command */
		{
		if (((result = execute(s, z_flag)) > 0) && (w_flag))
			printf("EXEC: exit code = %d\n", result);
		}
	return (result);
	}

/* ----------------------------------------------------------------------- */
	void
batch (						/* Write batch output from the command line */
    char  *p)				/* The command line to be batched */

    {
	p = stpblk(p);			/* Skip over leading white space */

	if ((tolower(*(p + 0)) == 's')	/* Skip a "sh" prefix */
	&&  (tolower(*(p + 1)) == 'h')
	&&  ((*(p + 2) == '\0') || isspace(*(p + 2))))
		p = stpblk(p + 2);

	if (y_flag)
		{
		puts(p);			/* Write out the command */
		fflush(stdout);
		}
	}

/* ----------------------------------------------------------------------- */
	int
query (void)

	{
	char  ch;
	char  response;

	response = 0;
	do  {
		if (ch = (char)(getch()))
			{
			if (response == 0)
				response = tolower(ch);
			if (ch == '\r')
				fputc('\n', stdout);
			else
				fputc(ch, stdout);
			fflush(stdout);
			}
		}  while (ch != '\r');		/* Await CR */

	return  (response == 'y');
	}

/* ----------------------------------------------------------------------- */
