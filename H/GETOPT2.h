/* ----------------------------------------------------------------------- *\
|
|		     Modified getopt library function
|			  for Microsoft C on the IBM PC
|
|	Copyright (c) 1985, 1990, 1991, 1993, 1997, 1998, 2022, all rights reserved
|				Brian W Johnson
|				   20-Jun-2022 added pfopen()
|
\* ----------------------------------------------------------------------- */

#ifndef __GETOPT2_H__
#define __GETOPT2_H__

#if defined(__cplusplus)
extern "C" {
#endif

/* ------------------------------------------------------------------------ *\
|  getopt2() public interface
\* ------------------------------------------------------------------------ */

typedef enum			// getopt2 returned error code definitions
	{
	OPTERR_NONE = 0,			// No error reorted
	OPTERR_UNCONFIGURED,		// No OPTINIT structures are configured
	OPTERR_MISSING_SWITCH,		// Invalid switch character
	OPTERR_MISSING_OPTION,		// Missing option character
	OPTERR_INVALID_OPTION,		// Invalid option character
	OPTERR_MISSING_PARM,		// Missing option parameter string
	} OPTERR;

typedef int (conf_t)(	// getopt2 option configuration function
	char option,
	char *pArg);

typedef struct optinit
	{
	struct optinit *link;		// List linkable structure
	char			switchChar;	// The switch character for these options
	char		   *pOptions;	// The option character list for this switch character
	conf_t		   *pConfig; 	// Pointer to the option configure function for this switch
	} OPTINIT, *POPTINIT;

extern	void	getoptInit (POPTINIT pInitData);
extern	int		getopt2(int argc, char *argv[], char *envp, int *pArgIndex);
extern	int		getoptErrorCode (void);	// Returns the error code, 0 if no error
extern	char   *getoptErrorStr (void);	// Returns descriptive string after error

#endif // __GETOPT2_H__

/* ------------------------------------------------------------------------ */
