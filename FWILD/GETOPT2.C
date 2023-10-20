/* ----------------------------------------------------------------------- *\
|
|				   GETOPT2 ()
|
|		    Copyright (c) 1985, 1990, 2022, all rights reserved
|				Brian W Johnson
|				   20-Jun-2022
|
|			Modified from the fWild library version
|			Supports simultaneous use of multiple switch characters
|
|	    int
|	opt = getopt2 (argc, argv, dummy);
|	    int  argc;
|	    char  *argv [];
|	    char  *dummy;
|
|	    char   optswch;	(Global variable, declared in getopt() )
|	    char   optchar;	(Global variable, declared in getopt() )
|	    int    optind;	(Global variable, declared in getopt() )
|	    char  *optarg;	(Global variable, declared in getopt() )
|	    char  *optenv;	(Global variable, declared in getopt() )
|
|	getopt() parses command line arguments, looking for valid options.
|	For each valid option, getopt() returns the option character.
|	If there are no more valid options, getopt() returns EOF (-1).
|	If an option character is invalid, getopt() returns NUL (0).
|	If an option passes a parameter, optarg points to it.
|	If an option parameter is missing, getopt() returns NUL (0).
|	If an option does not pass a parameter, optarg is NULL.
|	After parsing of options is complete, the main program can find
|	the next command line argument pointed by argv[optind].
|
|	To recognize the options  -a -A -b -C -Ffilename -f filename
|	the optstr is formed as follows:
|
|	    char  *optstr = "aAbCF:f:";
|
|	"--" can be used to terminate option parsing, and returns EOF.
|	The alternate option form -a-C is also accepted.
|
|	If optenv is not NULL, that string is searched first before searching
|	the argv[] array.  The application can place a pointer to an option
|	string into optenv, prior to the first call to getopt().  This string
|	is usually obtained from the environment.
|
|	The current switch character is obtained from the environment
|	or operating system via egetswch().
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <ctype.h>
#include  <string.h>
#include  <stdlib.h>
#include  <ptypes.h>
#include  <getopt2.h>

#include  "ascii.h"
#include  "fWild.h"

#define DEBUG_LEVEL 0
//#define DEBUG_LEVEL 3

// --------------------------------------------------------------------------
// Private definitions
// --------------------------------------------------------------------------

typedef enum		// State machine state definitions
	{
	ST_DONE = 0,	// Processing completed, exits the state machine
	ST_SW,			// Switch character now
	ST_WHTSW,		// White space then switch character
	ST_TERM,		// Terminator character
	ST_OPT_SW,		// Option now, or white to switch
	ST_OPT,			// Option character
	ST_BEGINPARM,	// Parameter begin character
	ST_ENDPARM,		// Find parameter end and config
	ST_EOP			// EOP, adjust argIndex and terminate parsing
	} OPTSTATE;

typedef enum		// State machine mode definitions
	{
	SM_ENV = 0,		// ENV scan mode
	SM_ARG,			// ARG scan mode
	}  ScanMode;

#define	EOB		0xCC	// Buffer change signal
#define	EOP		0xB9	// Parse end signal

// --------------------------------------------------------------------------
// Private variables
// --------------------------------------------------------------------------


// Support for the state machine

static	int			configured 	= 0;			// Number of configured OPTINT structures
static	ScanMode	scanMode	= SM_ENV;		// The current scan mode
static	OPTSTATE	state		= ST_DONE;		// The state variable, initially disabled
static	OPTERR		errorCode 	= OPTERR_NONE;	// The getopt2 returned error status
static	int			m_argc;						// argc passed in from main()
static	char	  **m_argv;						// argv passed in from main()
static	int			argIndex 	= 0;			// The current argv[] index
static	char	   *pArg		= NULL;			// Pointer to the current option
static	char	   *pArgFirst	= NULL;			// Pointer to the beginning of current buffer
static	UINT8		ch			= NUL;			// Current active character
static	char		chSwitch	= NUL;			// The current active switch character
static	char		chOption	= NUL;			// The current active option character

//BWJ finish status
static	int			cfgStatus;

// Support for the parameter scanner

static	char	   *pParmBegin	= NULL;			// Pointer to first byte of an argument
static	char	   *pParmEnd	= NULL;			// Pointer to last byte + 1of an argument

// Support for multiple switch character configurations

static	conf_t	   *pConfig		= NULL;			// Ptr to the appropriate configuration function
static	char	   *pOptString	= NULL;			// Ptr to the active option string
static	OPTINIT    *root		= NULL;			// Root of the config type list

// --------------------------------------------------------------------------
// Private methods
// --------------------------------------------------------------------------
// Test and validate a possible switch character
// This is a helper function for IsValidSW()
// --------------------------------------------------------------------------
	static int
ValidateSwChar (void)			// Returns TRUE for a valid chSwitch, else FALSE

	{
	for (POPTINIT p = root; (p != NULL); (p = p->link))
		{
		if (ch == p->switchChar)
			{
			pOptString	= p->pOptions;
			pConfig		= p->pConfig;
			return (TRUE);
			}
		}

	return (FALSE);
    }

// --------------------------------------------------------------------------
// Return TRUE if the current character is a valid switch character
// When successful, IsValidSW() copies the switch char into chSwitch
// --------------------------------------------------------------------------
	static int
IsValidSW (void)	// CHANGE NAME AND INCLUDE PURE TEST VERSION

	{
	if ((ch != NUL)
	&&  ValidateSwChar())
		{
		chSwitch = ch;
#if DEBUG_LEVEL >= 1
printf("New SW %c\n", ch);
#endif
		return (TRUE);
		}
		
	chSwitch = NUL;		// No current switch character
#if DEBUG_LEVEL >= 2
printf("New SW FAIL\n");
#endif
	return (FALSE);
	}

// --------------------------------------------------------------------------
// Return TRUE if the current character is the current switch character
// --------------------------------------------------------------------------
	inline int
IsCurrentSW (void)	// CHANGE NAME AND INCLUDE PURE TEST VERSION

	{
	return ((ch != NUL)  &&  (ch == chSwitch));
	}

// --------------------------------------------------------------------------
// Return TRUE if the current character is a valid option character
// When successful, IsValidOPT() copies the option char into chOption
// --------------------------------------------------------------------------
	static int
IsValidOPT (void)

	{
	if ((ch != NUL)
	&&  (pOptString != NULL)
	&&  (strchr(pOptString, ch) != NULL))
		{
		chOption = ch;		// Save the option for option processing
#if DEBUG_LEVEL >= 1
printf("New OPT %c\n", ch);
#endif
		return (TRUE);
		}
	
#if DEBUG_LEVEL >= 2
printf("New OPT FAIL\n");
#endif
	return	(FALSE);
	}

// --------------------------------------------------------------------------
//	Return the white property of the current character
// --------------------------------------------------------------------------
	inline int
IsWhite (void)

	{
	return (isspace(ch) != 0);
	}

// --------------------------------------------------------------------------
//	Return the EOB property of the current character
// --------------------------------------------------------------------------
	inline int
IsEOB (void)

	{
#if DEBUG_LEVEL >= 2
printf("IsEOB: %s\n", ((ch == EOB) ? "True" : "False"));
#endif
	return (ch == EOB);
	}

// --------------------------------------------------------------------------
//	Return the EOP property of the current character
// --------------------------------------------------------------------------
	inline int
IsEOP (void)

	{
#if DEBUG_LEVEL >= 2
printf("IsEOP: %s\n", ((ch == EOP) ? "True" : "False"));
#endif
	return (ch == EOP);
	}

// --------------------------------------------------------------------------
// Return TRUE if the current chOption requires a Parm
// --------------------------------------------------------------------------
	static int
ParmRequired (void)

	{
	char *parmReq;		// Pointer into the option string

	if ((chOption != NUL)
	&&  (pOptString != NULL)
	&&  ((parmReq = strchr(pOptString, chOption)) != NULL)
	&&  (*(parmReq + 1) == ':'))
		{
#if DEBUG_LEVEL >= 2
printf("ParmRequired OPT %c True\n", chOption);
#endif
		return (TRUE);
		}

#if DEBUG_LEVEL >= 2
printf("ParmRequired OPT %c False\n", chOption);
#endif
	return (FALSE);
	}

// --------------------------------------------------------------------------
// Set the error code variable
// --------------------------------------------------------------------------
	inline void
SetError (
	OPTERR error)

	{
	errorCode = error;
	}

// --------------------------------------------------------------------------
//	If permitted, get the next ARG buffer, return ptr to it, NULL if failed
// --------------------------------------------------------------------------
	static char *	// Return pointer to new buffer, else NULL if fail
NewBuffer (void)

	{
	char *p;							// Pointer to the next buffer, if any

	if (scanMode != SM_ARG)				// This algorithm does not apply to ENV mode
		{
		p = NULL;
#if DEBUG_LEVEL >= 2
printf("NewBuffer not available ENV mode\n");
#endif
		return (p);
		}

	if (argIndex < m_argc)				// Can't increment above argc
		++argIndex;

	if (argIndex >= m_argc)				// argIndex must stay below argc for valid buffers
		p = NULL;						// Failure, points a real argument

	else
		p = m_argv[argIndex];			// Probable success (if p != NULL)

#if DEBUG_LEVEL >= 1
printf("NewBuffer pArg: %08X, argIndex: %d\n", (unsigned int)(p), argIndex);
#endif
	pArgFirst = p;						// Save the initial pointer
	return (p);
	}

// --------------------------------------------------------------------------
//
// When processing ARG and ENV buffers, end of buffer will be reported (once)
// as a BUFCHG character, to allow the state machine to complete Parm string
// processing, and decide SW strategy, before scanning the next buffer.
//
// --------------------------------------------------------------------------
// Advance to the next character for consideration
// --------------------------------------------------------------------------
	static void
NextCH (void)

	{
#if DEBUG_LEVEL >= 3
printf("NextCH entry ch (%c) pArg (%s)\n", ((ch) ? ch : 0), ((pArg) ? "valid" : "NULL"));
#endif
	if (pArg == NULL)					// (pArg is invalid)
		{
		ch = EOP;						// Permanent termination
#if DEBUG_LEVEL >= 3
printf("NextCH 1 (NUL)\n");
#endif
		}

	else if ((ch == NUL)		 		// First/only buffer (pArg is valid)
		 ||  (ch == EOB))				// Next buffer       (pArg is valid)
		{
		ch = *pArg;						// First character of first/next buffer
#if DEBUG_LEVEL >= 3
printf("NextCH 2 (%c)\n", ch);
#endif
		}

	else if (ch == EOB)		 			// (pArg is valid)
		{
		ch = *pArg;						// First character of first buffer
#if DEBUG_LEVEL >= 3
printf("NextCH 3 (%c)\n", ch);
#endif
		}

	else if (*(pArg+1) != NUL)			// (There is a next character)
		{
		ch = *(++pArg);					// Valid character, and increment
#if DEBUG_LEVEL >= 3
printf("NextCH 4 (%c)\n", ch);
#endif
		}

	else // (There is no next char in the current buffer)
		{
		ch   = EOB;						// Send the end of buffer signal once
#if DEBUG_LEVEL >= 3
printf("NextCH 5 (%c)\n", ch);
#endif
		pArg = NewBuffer();				// Try for another ARG buffer
		}
#if DEBUG_LEVEL >= 2
printf("NextCH exit  ch (%c) pArg (%s)\n", ((ch) ? ch : 0), ((pArg) ? "valid" : "NULL"));
#endif
	}

// --------------------------------------------------------------------------
//	Initialize the ch variable (once per Parser run)
// --------------------------------------------------------------------------
	inline void
InitCH (void)
	{
	ch = NUL;
	NextCH();
	}

// --------------------------------------------------------------------------
//	Return TRUE if the current character is valid
// --------------------------------------------------------------------------
	inline int
ValidateCH (void)

	{
	return (ch != NUL);
    }

// --------------------------------------------------------------------------
// Initialize the state machine buffer variables for the ARG scan
// --------------------------------------------------------------------------
	static int
InitARG (
	int    argc,			// argc passed from main()
	char  *argv [])			// argv passed from main()

	{
	m_argc   = argc;
	m_argv   = argv;
	argIndex = 0;

	pArg = NewBuffer();		// Get the first buffer

#if DEBUG_LEVEL >= 1
	if (pArg)
		printf("ARG: \"%s\"\n", (pArg ? pArg : "(NULL)"));
#endif

	ch = NUL;
	return (pArg != NULL);
	}

// --------------------------------------------------------------------------
// Initialize the state machine buffer variables for the ENV scan
// --------------------------------------------------------------------------
	static int
InitENV (
	char *envPtr)

	{
	pArg = envPtr;		// Advance to the first switch character, if any

#if DEBUG_LEVEL >= 1
	printf("ENV: \"%s\"\n", (pArg ? pArg : "(NULL)"));
#endif

	ch = NUL;
	return (pArg != NULL);	// No switch character found, abort the ENV scan
	}

// --------------------------------------------------------------------------
// State machine helper functions
// --------------------------------------------------------------------------
	inline void
ConfigOptionOnly (void)

	{
#if DEBUG_LEVEL >= 1
printf("  ConfigureOptionOnly (%c, %c)\n", chSwitch, chOption);
#endif
	cfgStatus = (*pConfig)(chOption, NULL);	// Configure the option
	}

// --------------------------------------------------------------------------
// Terminate the Parm buffer and configure the option
// --------------------------------------------------------------------------
// If this Parm was terminated by an EOB, then pParmEnd is NULL
// In this case there is no need to terminate the Parm text
// --------------------------------------------------------------------------
	void
ConfigOptionParm (void)

	{
	char savedCH;				// Saved current character

	if (pParmEnd != NULL)		// Will be NULL if the buffer ended
		{
		savedCH   = *pParmEnd;	// Temorarily terminate the parm
		*pParmEnd = NUL;	// else the Parm is already terminated
		}

#if DEBUG_LEVEL >= 1
printf("  ConfigureOptionParm (%c, %c, %s)\n", chSwitch, chOption, pParmBegin);
#endif
	cfgStatus = (*pConfig)(chOption, pParmBegin);	// Configure the option

	if (pParmEnd != NULL)
		*pParmEnd = savedCH;	// Restore the altered current character
	}

// --------------------------------------------------------------------------
// Found the beginning of the Parm
// --------------------------------------------------------------------------
	inline void
FoundParm (void)

	{
#if DEBUG_LEVEL >= 2
printf("FOUND_PARM  (%c)\n", *pArg);
#endif
	pParmBegin = pArg;		// Begins the parm
	}

// --------------------------------------------------------------------------
// The state machine
// --------------------------------------------------------------------------
	void
Parser (void)

	{
#if DEBUG_LEVEL >= 1
printf("\nPARSER BEGIN\n");
#endif

	while ((state != ST_DONE)  &&  (ValidateCH()))
		{
		switch (state)
			{
			case ST_SW:						// Require switch character now (always in a new buffer)
#if DEBUG_LEVEL >= 1
printf("ST_SW (%c)\n", ch);
#endif
				if (IsEOP())
					{
					state = ST_EOP;				// Input has ended, terminate scanning
					}
				else if (IsEOB())				// End of the current buffer
					{
					NextCH();					// Skip over EOB
					}
				else if (IsValidSW())
					{
					NextCH();
					state = ST_TERM;			// Valid switch character found
					}
				else
					{
					state = ST_EOP;				// Not a valid switch, terminate scanning
					}
				break;


			case ST_WHTSW:					// Require Valid switch character preceded by any amount of white space
#if DEBUG_LEVEL >= 1
printf("ST_WHTSW (%c)\n", ch);
#endif
				if (IsEOP())
					{
					state = ST_EOP;				// Input has ended, terminate scanning
					}
				else if (IsEOB())
					{
					NextCH();					// EOB requires that SW be in first byte of the next buffer
					state = ST_SW;
					}
				else if (IsWhite())
					{
					NextCH();					// Skip over white space
					}
				else if (IsValidSW())
					{
					NextCH();					// Switch character found
					state = ST_TERM;
					}
				break;


			case ST_TERM:					// Check for a terminator now
#if DEBUG_LEVEL >= 1
printf("ST_TERM (%c)\n", ch);
#endif
				if (IsEOP())
					{
					SetError(OPTERR_MISSING_OPTION);
					state = ST_EOP;				// Input has ended, fatal error
					}
				else if (IsEOB())
					{
					SetError(OPTERR_MISSING_OPTION);
					state = ST_EOP;				// Input has ended, fatal error
					}
				else if (IsCurrentSW())
					{
					state = ST_EOP;				// (--) case, terminate scanning
					}
				else
					{
					state = ST_OPT;				// Not a terminator, might be an option, leave character in play
					}
				break;


			case ST_OPT_SW:					// Require an option character now, or white space and switch
#if DEBUG_LEVEL >= 1
printf("ST_OPT_SW (%c)\n", ch);
#endif
				if (IsEOP())
					{
					state = ST_EOP;				// Input has ended, terminate scanning
					}
				else if (IsEOB())
					{
					NextCH();					// EOB found, advance to first char of new buffer
					state = ST_SW;				// EOB requires SW in the first byte of next buffer
					}
				else if (IsWhite())
					{
					NextCH();					// White space found
					state = ST_WHTSW;			// Not an option, try SW following white space
					}
				else
					{
					state = ST_OPT;				// Not white space, so require an option
					}
				break;


			case ST_OPT:					// Require a valid option character now
#if DEBUG_LEVEL >= 1
printf("ST_OPT (%c)\n", ch);
#endif
				if (IsEOP())
					{
					SetError(OPTERR_MISSING_OPTION);
					state = ST_EOP;				// Input has ended, fatal error
					}
				else if (IsEOB())
					{
					SetError(OPTERR_MISSING_OPTION);
					state = ST_EOP;				// OPT reqired, but EOB, fatal error
					}
				else if ( ! IsValidOPT())
					{
					SetError(OPTERR_INVALID_OPTION);
					state = ST_EOP;				// Not a valid option, fatal error
					}
				else if (ParmRequired())
					{
					NextCH();
					state = ST_BEGINPARM;		// Fetch a parm
					}
				else
					{
					ConfigOptionOnly();			// No parm required, configure the option
					NextCH();
					state = ST_OPT_SW;			// Either option or switch will be acceptable following
					}
				break;


			case ST_BEGINPARM:				// Require parm, possibly preceded by any amount of white space
#if DEBUG_LEVEL >= 1
printf("ST_BEGINPARM (%c)\n", ch);
#endif
				if (IsEOP())
					{
					SetError(OPTERR_MISSING_PARM);
					state = ST_EOP;				// Input has ended, fatal error
					}
				else if (( ! IsWhite())			// Skip over white space and EOB
				     &&  ( ! IsEOB()))
					{
					FoundParm();				// Parm beginning found
					state = ST_ENDPARM;
					}
				NextCH();
				break;


			case ST_ENDPARM:				// Require parm end
#if DEBUG_LEVEL >= 1
printf("ST_ENDPARM (%c)\n", ch);
#endif
				if (IsEOP())					// EOP terminates parm
					{
					ConfigOptionParm();			// Configure the option with parm
					state = ST_EOP;				// Input has ended, terminate scanning
					}
				else if (IsEOB())				// EOB terminates parm
					{
					ConfigOptionParm();			// Configure the option with parm
					NextCH();
					state = ST_SW;
					}
				else if (IsWhite())				// White space terminates parm
					{
					ConfigOptionParm();			// Configure the option with parm
					NextCH();
					state = ST_WHTSW;
					}
				else	
					NextCH();					// Skip over the active parm
				break;

			case ST_EOP:					// End parse
#if DEBUG_LEVEL >= 1
printf("ST_EOP (%c)\n", ch);
#endif
				state = ST_DONE;				// Terminates scanning
				break;

			} // End switch(state)
#if DEBUG_LEVEL >= 1
printf("NextCH Loop (%c)\n", ch);
#endif
		} // End state processing loop

#if DEBUG_LEVEL >= 1
printf("PARSER END, error (%u)\n\n", errorCode);
#endif
	} // End Parser

// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
// Public methods
// --------------------------------------------------------------------------
// Add a switch character and its associated optionList string
// --------------------------------------------------------------------------
	void
getoptInit (
	POPTINIT p)

	{
#if DEBUG_LEVEL >= 1
printf("Linking  \'%c\'\n", p->switchChar);
#endif

	p->link = root;
	root    = p;

	++configured;			// Show configuration count
	}

// --------------------------------------------------------------------------
// Process the options
// --------------------------------------------------------------------------
	int
getopt2 (
	int    argc,			// argc passed from main()
	char  *argv [],			// argv passed from main()
	char  *envPtr,			// Pointer to the environment string
	int   *pArgIndex)		// Pointer to the callers argIndex variable

	{
	*pArgIndex = 1;			// Init the default returned index
	errorCode = OPTERR_NONE;

	if ( ! configured)
		{
		SetError(OPTERR_UNCONFIGURED);
		return (errorCode);
		}

	scanMode = SM_ENV;
	if (InitENV(envPtr))		// Look for an valid ENV string
		{
		state    = ST_SW;		// Require initial switch
		InitCH();
		Parser();
		}

	scanMode = SM_ARG;
	if ((errorCode == OPTERR_NONE)
	&&  (InitARG(argc, argv)))	// Look for a ARGV string set
		{
		state    = ST_SW;		// Require initial switch
		InitCH();
		Parser();
		*pArgIndex = argIndex;	// Return the updated argv index
		}

	return (errorCode);
	}

// --------------------------------------------------------------------------
// Return the error code for the last call to getopt()
// --------------------------------------------------------------------------
	int
getoptErrorCode (void)

	{
	return (errorCode);
    }

// --------------------------------------------------------------------------
// Return an error message string describing the reported error code
// --------------------------------------------------------------------------
	char *
getoptErrorStr (void)

	{
	char *p;

	switch (errorCode)
		{
		case OPTERR_UNCONFIGURED:		// No OPTINIT structures are configured
			p = "GetOpt unconfigured";
			break;
		case OPTERR_MISSING_SWITCH:		// Missing switch character
			p = "Missing switch";
			break;
		case OPTERR_MISSING_OPTION:		// Missing switch option
			p = "Missing switch option";
			break;
		case OPTERR_INVALID_OPTION:		// Invalid switch option
			p = "Invalid switch option";
			break;
		case OPTERR_MISSING_PARM:		// Missing option parameter
			p = "Missing option parameter";
			break;
		default:
			p = "No error";
			break;
		}

	return (p);
    }

// --------------------------------------------------------------------------
//										EOF
// --------------------------------------------------------------------------
