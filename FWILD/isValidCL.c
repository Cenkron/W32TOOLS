/* ----------------------------------------------------------------------- *\
|
|								isValidChar
|			Validates characters for a normal command line pathspec
|
|					Copyright (c) 2022, all rights reserved
|								Brian W Johnson
|									 7-Oct-23
|
|			int				Returns TRUE/FALSE
|		isValidCmdline (
|			char ch,		Candidate char for a command line pathspec
|			int  wild)		True for wild limits, false for physical limits
|
|	This is currently unfinished and unused.
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <ctype.h>

#include  "fWild.h"

// ---------------------------------------------------------------------------
//#define TEST	// Define this to build the data generator program

// ---------------------------------------------------------------------------
// Validate path characters (also excludes space and '"')
// ---------------------------------------------------------------------------
	int
isValidChar (	// Reports validity of 7-bit characters for use in a command line
	char ch,
	int  wild)

	{
	int		result = FALSE;		// The returned result
	UINT64 One			 = 1;
	UINT64 highValidTbl = 0x07FFFFFE97FFFFFF;	// High case
	UINT64 lowValidTblW = 0x87FFE73800000000;	// Wild case
	UINT64 lowValidTblP = 0x87FFE33800000000;	// Physical case


	if (ch < 0x80)
		{
		if (ch >= 64)
			result = ((highValidTbl | (__ll_lshift(One, (ch - 64)))) != 0);
		else if (wild)
			result = ((lowValidTblW  | (__ll_lshift(One, (ch)))) != 0);
		else
			result = ((lowValidTblP  | (__ll_lshift(One, (ch)))) != 0);
		}

	return (result);
	}

// ---------------------------------------------------------------------------
//
// Space and DEL or disallowed by the add range [ 0x21 - 0x7E ]
//	[ . & < > [ ] | { } ^ = ; ! ' " + , ` ~ ]
// In addition, for Fwild, physical paths may not contain '* or '?'
//           High word			 Low word
// Wild:     07FFFFFE 97FFFFFF - 87FFE738 00000000
// Physical: 07FFFFFE 97FFFFFF - 87FFE338 00000000
// ---------------------------------------------------------------------------
#ifdef  TEST
main ()		// This is the generator for the two constants used above
	{
	char clist [] =	// List of forbidden command line characters
		{
		' ', '&', '<', '>' , '[' , ']', '|', '{', '}', '^',
		'=', ';', '!', '\'', '\"', '+', ',', '`', '~',  0
		};

	UINT64 One = 1;
	UINT64 validChar [2] = { 0, 0 };

	// Add the seven-bit printable characters

	for (int i = (int)('!'); (i < 0x7F); ++i)
		{
		if (i < 64)
			validChar[0] |= (__ll_lshift(One, (i)));
		else if (i < 128)
			validChar[1] |= (__ll_lshift(One, (i - 64)));
		}
				
	// Subtract the characters that cannot be used within a wild path

	for (int i = 0; (clist[i] != 0); ++i)
		{
		char ch = clist[i];

		if (ch < 64)
			validChar[0] &= ~(__ll_lshift(One, (ch)));
		else if (i < 128)
			validChar[1] &= ~(__ll_lshift(One, (ch - 64)));
		}

	printf("          High word            Low word\n");
	printf("Wild:     0x%016llX - 0x%016llX\n", validChar[1], validChar[0]);

	// Subtract the characters that cannot be used within a physical path

	validChar[0] &= ~(__ll_lshift(One, (int)('*')));
	validChar[0] &= ~(__ll_lshift(One, (int)('?')));

	printf("Physical: 0x%016llX - 0x%016llX\n", validChar[1], validChar[0]);
	}
#endif

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
