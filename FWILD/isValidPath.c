/* ----------------------------------------------------------------------- *\
|
|								isValidPath
|						Validates characters for a normal pathspec

|								isValidUNC
|						Validates characters for a UNC pathspec
|
|					Copyright (c) 2022, all rights reserved
|								Brian W Johnson
|									21-Sep-22
|
|			int				Returns TRUE/FALSE
|		isValidPath (
|			char ch);		Candidate char for a pathspec
|
\* ----------------------------------------------------------------------- */

#include  <windows.h>
//#include  <VersionHelpers.h>
#include  <stdio.h>
#include  <ctype.h>
#include  <string.h>
#include  <stdlib.h>
#include  "fwild.h"

// ------------------------------------------------------------------------------------------------
// Validate path characters (includes '?' and '*' as valid for the Pattern Compiler)
// ------------------------------------------------------------------------------------------------
	int
isValidPath (			// Reports validity of characters for use in a file\path string
	char ch)

	{
	return ((ch > 31)
		&& (ch < 127)
		&& (ch != '"')
		&& (ch != ':')
		&& (ch != '|')
		&& (ch != '<')
		&& (ch != '>'));
	}

// ------------------------------------------------------------------------------------------------
// Validate UNC path characters
// ------------------------------------------------------------------------------------------------
	int
isValidUNC (			// Reports validity of characters for use in a UNC \\server\share string
	char ch)

	{
	return (isalnum(ch));
	}

// ------------------------------------------------------------------------------------------------
