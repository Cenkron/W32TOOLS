/* ----------------------------------------------------------------------- *\
|
|				     CDiff
|		  A C++ class used to analyze file differences
|			     (Implementation file)
|
|		   Copyright (c) 1997, 1998, all rights reserved
|				Brian W Johnson
|				   21-Aug-97
|				    1-Jan-98
|
\* ----------------------------------------------------------------------- */

#define  WINVER  0x0A00

#include <afx.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
// #include <process.h>

#include "cdiff.h"

// #define CONSOLEMODETEST 1	// Define in the makefile to test in console mode

// #define USETIMESTAMPS 1	// Define in the makefile to use timestamps in test mode

#define  OPTIMIZER (3)		// Determine which optimizer to use

#if USETIMESTAMPS
extern void _TimeStamp (PSTR p);
#define  TimeStamp(s)  _TimeStamp(s)
#else
#define  TimeStamp(s)
#endif

/* ----------------------------------------------------------------------- *\
|  CDiff () - The class constructor
\* ----------------------------------------------------------------------- */

CDiff :: CDiff () :
	m_Error()

	{
	m_IgnoreAllBlanks       = false;
	m_IgnoreLeadingBlanks   = false;
	m_IgnoreTrailingBlanks  = false;
//??? m_InterleaveUniqueLines = false;
	m_MaxLength             = 0;
	m_TotalSize             = 0;
	m_TotalLines            = 0;
	m_EndFileA              = 1;
	m_PairRegionCount       = 0;

	m_FileData              = NULL;
	m_LineTable             = NULL;
	m_HashTable             = NULL;
	m_HashTableSize         = 0;
	m_RegionTable           = NULL;

	// Property variables

	m_IgnoreAllBlanks       = false;
	m_IgnoreLeadingBlanks   = false;
	m_IgnoreTrailingBlanks  = false;

	m_Failure               = 0;
	m_Success               = 0;
	m_Hwnd                  = NULL;

//	m_Sem = CreateSemaphore(NULL, 0, 1, NULL);
	}

/* ----------------------------------------------------------------------- *\
|  ~CDiff () - The class destructor
\* ----------------------------------------------------------------------- */

CDiff :: ~CDiff ()

	{
	m_Error.Empty();
	Reset();

//	if (m_Sem != INVALID_HANDLE_VALUE)
//		CloseHandle(m_Sem);
	}

/* ----------------------------------------------------------------------- *\
|  Reset () - Client reset the class				public interface
\* ----------------------------------------------------------------------- */
	void
CDiff :: Reset (void)

	{
	ReleaseFiles();
	ReleaseTables();
	}

/* ----------------------------------------------------------------------- *\
|  LoadFiles () - Client load the file pair			public interface
\* ----------------------------------------------------------------------- */
	bool
CDiff :: LoadFiles (
	PSTR  pFileNameA,		// Ptr to the filename for file A
	PSTR  pFileNameB)		// Ptr to the filename for file B

	{
	Reset();

	m_FileInfo[A].FileName = pFileNameA;
	m_FileInfo[B].FileName = pFileNameB;

	if ((LoadFilesThread()   != Success)
	||  (BuildTablesThread() != Success))
		{
		Reset();
		return (false);
		}

	return (true);
	}

/* ----------------------------------------------------------------------- *\
|  RebuildTables () - Client rebuild the data tables		public interface
\* ----------------------------------------------------------------------- */
	bool
CDiff :: RebuildTables (void)

	{
	ReleaseTables();

	if ((m_FileData == NULL)
	||  (BuildTablesThread() != Success))
		{
		ReleaseTables();
		return (false);
		}

	return (true);
	}

/* ----------------------------------------------------------------------- *\
|  GetError () - Get the error string following a failure	public interface
\* ----------------------------------------------------------------------- */
	CString &
CDiff :: GetError (void)

	{
	return (m_Error);
	}

/* ----------------------------------------------------------------------- *\
|  SetTabSize () - Set the tab interpretation size		public interface
\* ----------------------------------------------------------------------- */
	void
CDiff :: SetTabSize (
	int  FileIndex,		// File index [0..1]			
	int  TabSize)		// The provided tab size

	{
	if ((FileIndex >= 0)  &&  (FileIndex <= 1)
	&&  (TabSize > 0)     &&  (TabSize < 21))
		m_FileInfo[FileIndex].TabSize = TabSize;
	}

/* ----------------------------------------------------------------------- *\
|  SetIgnoreAllBlanks () - Set the ignore all blanks flag	public interface
\* ----------------------------------------------------------------------- */
	void
CDiff :: SetIgnoreAllBlanks (
	bool  Value)		// true to ignore all blanks

	{
	m_IgnoreAllBlanks = Value;
	}

/* ----------------------------------------------------------------------- *\
|  SetIgnoreLeadingBlanks () - Set the ignore leading flag	public interface
\* ----------------------------------------------------------------------- */
	void
CDiff :: SetIgnoreLeadingBlanks (
	bool  Value)		// true to ignore leading blanks

	{
	m_IgnoreLeadingBlanks = Value;
	}

/* ----------------------------------------------------------------------- *\
|  SetIgnoreTrailingBlanks () - Set the ignore trailing flag	public interface
\* ----------------------------------------------------------------------- */
	void
CDiff :: SetIgnoreTrailingBlanks (
	bool  Value)		// true to ignore trailing blanks

	{
	m_IgnoreTrailingBlanks = Value;
	}

/* ----------------------------------------------------------------------- *\
|  SetHwnd () - Set the associated window handle		public interface
\* ----------------------------------------------------------------------- */
	void
CDiff :: SetHwnd (
	HWND  Hwnd)			// The provided window handle

	{
	m_Hwnd = Hwnd;
	}

/* ----------------------------------------------------------------------- *\
|  SetFailureMessage () - Set the failure window message	public interface
\* ----------------------------------------------------------------------- */
	void
CDiff :: SetFailureMessage (
	int  Message)		// The provided window message

	{
	m_Failure = Message;
	}

/* ----------------------------------------------------------------------- *\
|  SetSuccessMessage () - Set the success window message	public interface
\* ----------------------------------------------------------------------- */
	void
CDiff :: SetSuccessMessage (
	int  Message)		// The provided window message

	{
	m_Success = Message;
	}

/* ----------------------------------------------------------------------- *\
|  GetTextLine () - Get a text line by logical line number	public interface
\* ----------------------------------------------------------------------- */
	PSTR
CDiff :: GetTextLine (
	FileDesc   f,			// The requested logical file
	int        LineNumber,		// The requested line number
	int      *pLineNumber,		// Ptr to the returned line number
	RegType  *pRegType)			// Ptr to the returned region type

	{
	PLINETBL   pLine;			// Ptr into the LINETBL array

	switch (f)
		{
		case A:
			if ((m_LineTable == NULL)
			||  (LineNumber  <= 0)
			||  (LineNumber  >  m_FileInfo[A].Lines))
				break;

			pLine        = &m_LineTable[LineNumber];
			*pLineNumber = LineNumber;
			*pRegType    = pLine->Type;
			return (pLine->pLine);

		case B:
			if ((m_LineTable == NULL)
			||  (LineNumber  <= 0)
			||  (LineNumber  >  m_FileInfo[B].Lines))
				break;

			pLine        = &m_LineTable[LineNumber + m_FileInfo[A].Lines];
			*pLineNumber = LineNumber;
			*pRegType    = pLine->Type;
				return (pLine->pLine);

		case C:
			if ((m_LineTable == NULL)
			||  (LineNumber  <= 0)
			||  (LineNumber  >  m_FileInfo[C].Lines))
				break;

			pLine        = &m_LineTable[LineNumber];
			LineNumber   = pLine->Combined;
			pLine        = &m_LineTable[LineNumber];
			if (LineNumber >  m_FileInfo[A].Lines)
				LineNumber -= m_FileInfo[A].Lines;
			*pLineNumber = LineNumber;
			*pRegType    = pLine->Type;
			return (pLine->pLine);
		}

	*pLineNumber = 0;
	*pRegType    = Common;
	return (NULL);
	}

/* ----------------------------------------------------------------------- *\
|  GetFileMatch () - Get the file match flag			public interface
\* ----------------------------------------------------------------------- */
	bool
CDiff :: GetFileMatch (void)

	{
	return ((m_TotalLines == 0)
	|| ((m_PairRegionCount == 1)
		&&  (m_FileInfo[A].Lines == m_FileInfo[B].Lines)));
	}

/* ----------------------------------------------------------------------- *\
|  GetFileSize () - Get the file match flag			public interface
\* ----------------------------------------------------------------------- */
	int
CDiff :: GetFileSize (
	FileDesc  f)		// The requested logical file

	{
	int  Length;	// The returned length (in lines)

	switch (f)
		{
		case A:	    Length = m_FileInfo[A].Lines;   break;
		case B:	    Length = m_FileInfo[B].Lines;   break;
		case C:	    Length = m_FileInfo[C].Lines;   break;
		default:    Length = 0;			    break;
		}

	return (Length);
	}

/* ----------------------------------------------------------------------- */

/************************************************************/
/* The following functions are in the second (table) thread */
/************************************************************/

/* ----------------------------------------------------------------------- *\
|  LoadFilesThread () - Load the file pair
\* ----------------------------------------------------------------------- */
	bool
CDiff :: LoadFilesThread (void)

	{
	if ( ! OpenSpecifiedFile(&m_FileInfo[A]))
		return (false);

	if ( ! OpenSpecifiedFile(&m_FileInfo[B]))
		return (false);

	m_TotalSize = m_FileInfo[A].FileLength + m_FileInfo[B].FileLength + 2;
	m_FileData  = new char [m_TotalSize];

	if (m_FileData == NULL)
		{
		m_Error = "Can't allocate memory for files";
		return (false);
		}

	m_FileInfo[A].pFileData = m_FileData;
	m_FileInfo[B].pFileData = m_FileData + m_FileInfo[A].FileLength + 1;

	if ( ! ReadSpecifiedFile(&m_FileInfo[A]))
		return (false);

	if ( ! ReadSpecifiedFile(&m_FileInfo[B]))
		return (false);

	return (true);
	}

/* ----------------------------------------------------------------------- *\
|  ReleaseFiles () - Release the file dynamic memory and clean up
\* ----------------------------------------------------------------------- */
	void
CDiff :: ReleaseFiles (void)

	{
	if (m_FileData != NULL)
		{
		delete[]  m_FileData;
		m_FileData = NULL;
		}

	if (m_FileInfo[A].hFile != INVALID_HANDLE_VALUE)
		{
		CloseHandle(m_FileInfo[A].hFile);
		m_FileInfo[A].hFile = INVALID_HANDLE_VALUE;
		}

	if (m_FileInfo[B].hFile != INVALID_HANDLE_VALUE)
		{
		CloseHandle(m_FileInfo[B].hFile);
		m_FileInfo[B].hFile = INVALID_HANDLE_VALUE;
		}

	// Also clean up various file variables

	m_FileInfo[A].FileLength = 0;
	m_FileInfo[A].Lines      = 0;
	m_FileInfo[B].FileLength = 0;
	m_FileInfo[B].Lines      = 0;
	m_FileInfo[C].Lines      = 0;

	m_MaxLength     = 0;
	m_TotalSize     = 0;
	m_TotalLines    = 0;
	m_EndFileA      = 0;
	m_HashTableSize = 0;
	}

/* ----------------------------------------------------------------------- *\
|  OpenSpecifiedFile () - Open the specified file and get its length
\* ----------------------------------------------------------------------- */
	bool			// Returns true if successful
CDiff :: OpenSpecifiedFile (
	PFileInfo  pFileInfo)	// Ptr to the file info structure

	{
	pFileInfo->hFile = CreateFile(
		pFileInfo->FileName,	// The file name
		GENERIC_READ,		// Read only
		0,				// No file sharing limitation
		NULL,			// No security attributes
		OPEN_EXISTING,		// File must already exist
		0,				// No attributes
		NULL);			// No template
	if (pFileInfo->hFile == INVALID_HANDLE_VALUE)
		{
		m_Error  = "Can not open file: ";
		m_Error += pFileInfo->FileName;
		return (false);
		}

	pFileInfo->FileLength = ::GetFileSize(pFileInfo->hFile, NULL);

	if (pFileInfo->FileLength == 0xFFFFFFFF)
		{
		m_Error  = "Can not size file: ";
		m_Error += pFileInfo->FileName;
		return (false);
		}

	return (true);
	}

/* ----------------------------------------------------------------------- *\
|  ReadSpecifiedFile () - Read the specified file
\* ----------------------------------------------------------------------- */
	bool			// Returns true if successful
CDiff :: ReadSpecifiedFile (
	PFileInfo  pFileInfo)	// Ptr to the file info structure

	{
	bool   Result = true;	// The returned result, assuming success
	DWORD  ActualReadSize;	// The returned actual read size

	if ( ! ReadFile(
		pFileInfo->hFile,
		pFileInfo->pFileData,
		pFileInfo->FileLength,
		&ActualReadSize,
		NULL))
		{
		m_Error  = "Can not read file: ";
		m_Error += pFileInfo->FileName;
		Result   = false;
		}
	else

		// NUL terminate the last file line, in case there is no terminator

		pFileInfo->pFileData[pFileInfo->FileLength] = '\0';

	CloseHandle(pFileInfo->hFile);
	pFileInfo->hFile = INVALID_HANDLE_VALUE;
	return (Result);
	}

/* ----------------------------------------------------------------------- *\
|  BuildTablesThread () - Build the data structures for a file pair
\* ----------------------------------------------------------------------- */
	int
//  TableResult		// Returns result code as specified
CDiff :: BuildTablesThread (void)

	{
	int  ArraySize;	// Dynamic size of the LINETBL array

	// At this point, the files have been read into the file data buffer

	m_MaxLength     = 0;
	DetermineEOL(&m_FileInfo[A]);
	AnalyzeFile(&m_FileInfo[A]);
	DetermineEOL(&m_FileInfo[B]);
	AnalyzeFile(&m_FileInfo[B]);
	m_MaxLength     = min(m_MaxLength, MAXLINELENGTH);
	m_EndFileA      = m_FileInfo[A].Lines;

	m_TotalLines    = m_FileInfo[A].Lines + m_FileInfo[B].Lines;
	ArraySize       = m_TotalLines + 1;
	m_HashTableSize = (2 * m_TotalLines);


TimeStamp("Starting");

	// Allocate memory for the LINETBL and Hash Table arrays

	m_LineTable = new LINETBL [ArraySize];
	if (m_LineTable == NULL)
		{
		m_Error = "Can't allocate memory for the line table";
		return (Failure);
		}
	memset(m_LineTable, 0, (ArraySize * sizeof(LINETBL)));

	m_HashTable = new ULONG [m_HashTableSize];
	if (m_HashTable == NULL)
		{
		m_Error = "Can't allocate memory for the hash table";
		return (Failure);
		}
	memset(m_HashTable, 0, (m_HashTableSize * sizeof(ULONG)));


TimeStamp("Building the hash table");

	// Build the hash and line tables

	BuildLineHashTable(&m_FileInfo[A], 1);

//	if (WaitForSingleObject(m_Sem, 0) == WAIT_OBJECT_0)
//		return (Interrupted);

	BuildLineHashTable(&m_FileInfo[B], (m_EndFileA + 1));

TimeStamp("Chaining");

	ChainIdenticalLines();

TimeStamp("Finding unique pairs");

	FindUniquePairs();

	// Discard the hash table

#ifndef CONSOLEMODETEST
	delete[]  m_HashTable;
	m_HashTable = NULL;
#endif

TimeStamp("Expanding pair regions");

	ExpandPairRegions();

TimeStamp("Counting pair regions");

	CountPairRegions();

TimeStamp("Building the region table");

	// Build the region table

	m_RegionTable = new REGIONTBL [m_PairRegionCount];
	if (m_RegionTable == NULL)
		{
		m_Error = "Can't allocate memory for the region table";
		return (Failure);
		}
	memset(m_RegionTable, 0, (m_PairRegionCount * sizeof(REGIONTBL)));

	BuildRegionTable();

TimeStamp("Sequencing the regions");

	if (SequenceRegions() == Failure)
		return (Failure);

TimeStamp("Assigning line types");

	AssignLineTypes();

	// Discard the region table

#ifndef CONSOLEMODETEST
	delete[]  m_RegionTable;
	m_RegionTable = NULL;
#endif

	// Build the composite file index

TimeStamp("Building the composite file index");

	BuildCompositeIndex();

	return (Success);
	}

/* ----------------------------------------------------------------------- *\
|  ReleaseTables () - Release the dynamic table memory and clean up
\* ----------------------------------------------------------------------- */
	void
CDiff :: ReleaseTables (void)

	{
	if (m_LineTable != NULL)
		{
		delete[]  m_LineTable;
		m_LineTable = NULL;
		}

	if (m_HashTable != NULL)
		{
		delete[]  m_HashTable;
		m_HashTable = NULL;
		}

	if (m_RegionTable != NULL)
		{
		delete[]  m_RegionTable;
		m_RegionTable = NULL;
		}
	}

/* ----------------------------------------------------------------------- *\
|  DetermineEOL () - Determine the EOL treatment for this file
\* ----------------------------------------------------------------------- */
	void
CDiff :: DetermineEOL (
    PFileInfo  pFileInfo)

	{
	char  PrevCh = '\0';	// The previous character
	int   Length;		// The character down counter
	PSTR  pFileData;		// Ptr to the file data


	Length    = pFileInfo->FileLength;
	pFileData = pFileInfo->pFileData;
	while (Length-- > 0)
		{
		char  Char = *(pFileData++);

		if (Char == '\r')
			{
			if (PrevCh == '\n')
				{
				pFileInfo->NL_EOL = false;
				break;
				}
			}
		else if (Char == '\n')
			{
			if (PrevCh == '\r')
				{
				pFileInfo->CR_EOL = false;
				break;
				}
			}
		PrevCh = Char;
		}
	}

/* ----------------------------------------------------------------------- *\
|  AnalyzeFile () - Determine the file parameters,
|  and determine the maximum line length
\* ----------------------------------------------------------------------- */
	void
CDiff :: AnalyzeFile (
	PFileInfo  pFileInfo)	// Ptr to the file info structure

	{
	int   LineCount = 0;	// The returned line count
	int   Flag      = 0;	// 1 if last line is not terminated
	int   Length;		// The character down counter
	int   Column;		// The line length counter
	PSTR  pFileData;		// Ptr to the file data


	Column    = 0;
	Length    = pFileInfo->FileLength;
	pFileData = pFileInfo->pFileData;
	while (Length-- > 0)
		{
		char  ch = *(pFileData++);

		if (((ch == '\r')  &&  pFileInfo->CR_EOL)
		||  ((ch == '\n')  &&  pFileInfo->NL_EOL))
			{
			++LineCount;
			m_MaxLength = max(m_MaxLength, Column);
			Column = 0;
			Flag   = 0;
			}

		else // (not a potential line terminator)
			{
			Flag = 1;
			if (ch == '\t')
				{
				do
				++Column;
				while (((Column % pFileInfo->TabSize) != 0)  &&  (Column < MAXLINELENGTH));
				}
			else
				++Column;
			}
		}

	m_MaxLength = max(m_MaxLength, Column);
	pFileInfo->Lines = LineCount + Flag;
	}

/* ----------------------------------------------------------------------- *\
|  BuildLineHashTable () - Build the line table hash entries
|
|  The hashing is done by setting up a table of pseudo-random numbers (the
|  table is MAXLINELENGTH long, so we'll only pay attention to the first
|  MAXLINELENGTH chracters).  The ASCII value of each character is multiplied
|  by the next random number in the sequence, and added to the hash value for
|  the line.
|  
|  Mathematical diversion:
|  The random values and the hash values are 32 bits long; hopefully (but I'm
|  not going to write a thesis to prove it) this will produce a distribution
|  of hash values very close to perfect, in which case the chances of collision
|  for N lines are given by: 1 - (2^32 - 1)! / ((2^32 - N)! * (2^32)^(N - 1) .
|  (I think; my math is getting weak, but it tallies with the case given in a
|  text book of the chances of a shared birthday in a room of 23 people being
|  0.5). This works out at 0.03 for the maximum N of 16384.
|  
|  The values for the pseudo-random sequence satisfy rules given in Knuth (Art
|  of Computer Programming) for R(N) = (R(N-1) * A + C) mod M, i.e.
|  1) C and M are relatively prime
|  2) B = A - 1 is a multiple of every prime of M
|  3) if M is a multiple of 4, B must be as well.
|  Since M = 2^32, 2) is satisfied by satisfying 3), and 1) is satisfied by any
|  odd number. This should give a sequence which only repeats every M values.
|  To avoid 0 turning up in the ones used, I have started the sequence at C,
|  i.e. the value following 0. Using B = 4 speeds up computation, and using a
|  large C ensures that overflow will take place, which should help make the
|  distribution of hash values even.
|
\* ----------------------------------------------------------------------- */
	void
CDiff :: BuildLineHashTable (
	PFileInfo  pFileInfo,		// Ptr to the file info structure
	int         ltIndex)		// Starting index into the line table

	{
	PLINETBL   pLineTable;		// Ptr to the current LINETBL entry
	int         FileIndex;		// Index into the file data
	int         FileLength;		// Length of the file
	PSTR       pFileData;		// Ptr to the file data
	char        Char;			// Temporary character

	int     RandomIndex;		// Index into the random number table
	ULONG   HashVal, HashValNoBlanks;	// Working line hash values
	ULONG   Random [MAXLINELENGTH];	// The random number table


	// Build the random number table

	Random[0] = 0xABCDEF01;
	for (RandomIndex = 1; (RandomIndex < MAXLINELENGTH); ++RandomIndex)
		Random[RandomIndex] = (Random[RandomIndex - 1] * 5) + 0xABCDEF01;


	// Build the line table, and the line hash table

	FileIndex  = 0;
	FileLength = pFileInfo->FileLength;
	pFileData  = pFileInfo->pFileData;
	pLineTable = m_LineTable + ltIndex;
	while (FileIndex < FileLength)
		{
		HashVal           = 0;
		HashValNoBlanks   = 0;
		RandomIndex       = 0;
		pLineTable->pLine = pFileData;

		// If required, skip over any leading white space
		// White space is defined as a space, tab, or NUL

		if (m_IgnoreLeadingBlanks)
			{
			while ((FileIndex < FileLength)
			&&    (  ((Char = *pFileData) == ' ')
				||  (Char               == '\t')
				||  (Char               == '\0')))
				{
				++FileIndex;
				++pFileData;
				}
			}

		while (FileIndex < FileLength)
			{
			Char = *pFileData;

			if (Char == '\r')
				{
				++FileIndex;
				*(pFileData++) = '\0';
				if (pFileInfo->CR_EOL)
					break;
				else
					continue;
				}

			if (Char == '\n')
				{
				++FileIndex;
				*(pFileData++) = '\0';
				if (pFileInfo->NL_EOL)
					break;
				else
					continue;
				}

			if ((RandomIndex < MAXLINELENGTH)
			&& (( ! m_IgnoreAllBlanks)  ||  ((Char != ' ')  &&  (Char != '\t')  &&  (Char != '\0'))))
				{
				HashVal += Random[RandomIndex++] * Char;

				// Remember the hash value from the last graphic character.

				if (isgraph(Char))
					HashValNoBlanks = HashVal;
				}

			++FileIndex;
			++pFileData;
			}

		if (m_IgnoreTrailingBlanks)
			HashVal = HashValNoBlanks;

		pLineTable->Hash = HashVal;
		++pLineTable;
		}
	}

/* ----------------------------------------------------------------------- *\
|  ChainIdenticalLines () - Chain together all identical lines
\* ----------------------------------------------------------------------- */
	void
CDiff :: ChainIdenticalLines (void)

	{
	PLINETBL  pLineTable;


	// Build the hash and line table entries

	pLineTable = &m_LineTable[1];
	for (int  ltIndex = 1; (ltIndex <= m_TotalLines); ++ltIndex, ++pLineTable)
		{
		ULONG  HashVal;
		int    htIndex;
		int    ltPair;


		HashVal = pLineTable->Hash;
		htIndex = HashVal % m_HashTableSize;

		// Look for a free place in the hash table, or for a
		// collision with other lines having the same hash value

		while (((ltPair = m_HashTable[htIndex]) != 0)
		&&     ( (m_LineTable[ltPair].Hash != HashVal)
		||   ( ! VerifyPair(ltIndex, ltPair))))
			htIndex = (htIndex + 1) % m_HashTableSize;

		// Chain this line to any others with the same hash value

		pLineTable->Pair     = ltPair;
		m_HashTable[htIndex] = ltIndex;
		}
	}

/* ----------------------------------------------------------------------- *\
|  FindUniquePairs () - Circularly link all chains of length 2, whose
|  members are in the two respective files.  Delink all the remaining
|  chains of length 1, length 3+, and length 2 in the same file.
\* ----------------------------------------------------------------------- */
	void
CDiff :: FindUniquePairs (void)

	{
	int  htIndex, ltIndex, ltPair;


	for (htIndex = 0; (htIndex < m_HashTableSize); ++htIndex)
		{
		if ((ltIndex = m_HashTable[htIndex]) == 0)
			continue;					// No such line

		if (((ltPair = m_LineTable[ltIndex].Pair) != 0)
		&&   (m_LineTable[ltPair].Pair == 0)
		&&   (ltIndex > m_EndFileA)
		&&   (ltPair <= m_EndFileA))
			{
			m_LineTable[ltPair].Pair = ltIndex;		// One line in each file, so cross link
			}

		else						// Other cases, so delink them
			{
			while (ltIndex != 0)
				{
				ltPair = m_LineTable[ltIndex].Pair;
				m_LineTable[ltIndex].Pair = 0;
				ltIndex = ltPair;
				}
			}
		}
	}

/* ----------------------------------------------------------------------- *\
|  ExpandPairRegions () - Pair up identical lines before or after the unique
|  pairs already found.  These will be lines that have 2 or more duplicates.
\* ----------------------------------------------------------------------- */
	void
CDiff :: ExpandPairRegions (void)

	{
	int  ltIndex, ltPair, i;

	for (ltIndex = 0; (ltIndex <= (m_EndFileA + 1)); ++ltIndex)
		{

		// Pretend there are unique pairs before and after the files,
		// so the first and last lines will be paired, if identical

		if (ltIndex == 0)		// Line preceding FileA first line
			ltPair = m_EndFileA;	// Line preceding FileB first line
		else if (ltIndex > m_EndFileA)	// Line following FileA last line
			ltPair = m_TotalLines + 1;	// Line following FileB last line
		else
			ltPair = m_LineTable[ltIndex].Pair;    // Interior lines

		if (ltPair != 0)		// Scan forward pairing identical lines ...
			{
			i = 1;
			while (((ltPair + i) <= m_TotalLines)
			&&    ((ltIndex + i) <= m_EndFileA)
			&&    (m_LineTable[ltIndex + i].Hash == m_LineTable[ltPair + i].Hash)
			&&    (VerifyPair((ltIndex + i), (ltPair + i)))
			&&    (m_LineTable[ltIndex + i].Pair == 0)
			&&    (m_LineTable[ltPair  + i].Pair == 0))
				{
				m_LineTable[ltIndex + i].Pair = ltPair  + i;
				m_LineTable[ltPair  + i].Pair = ltIndex + i;
				++i;
				}

			i = -1;			// ... and also scan backward
			while (((ltPair + i) > m_EndFileA)
			&&    ((ltIndex + i) > 0)
			&&    (m_LineTable[ltIndex + i].Hash == m_LineTable[ltPair + i].Hash)
			&&    (VerifyPair((ltIndex + i), (ltPair + i)))
			&&    (m_LineTable[ltIndex + i].Pair == 0)
			&&    (m_LineTable[ltPair  + i].Pair == 0))
				{
				m_LineTable[ltIndex + i].Pair = ltPair  + i;
				m_LineTable[ltPair  + i].Pair = ltIndex + i;
				--i;
				}
			}
		}
	}

/* ----------------------------------------------------------------------- *\
|  VerifyPair () - Verify that a pair is really a pair
|  (This is to handke the rare case where the hash collides)
\* ----------------------------------------------------------------------- */
	bool		    // Returns true if the lines verify
CDiff :: VerifyPair (
	int  Index1,	    // First index into the line table
	int  Index2)	    // First index into the line table

	{
//??? PUT THE FUNCTIONALITY IN
	return (true);
	}

/* ----------------------------------------------------------------------- *\
|  CountPairRegions () - Count the number of paired identical file regions
\* ----------------------------------------------------------------------- */
	void
CDiff :: CountPairRegions (void)

	{
	int  ltIndex, ltPair, ltPairTest;


	m_PairRegionCount = 0;
	for (ltPairTest = 0, ltIndex = 1; (ltIndex <= m_EndFileA); ++ltIndex)
		{
		ltPair = m_LineTable[ltIndex].Pair;
		if (ltPair == 0)
			ltPairTest = 0;
		else if (ltPair == ltPairTest)
			++ltPairTest;
		else // (This is the first line of a file A shared region)
			{
			++m_PairRegionCount;
			ltPairTest = ltPair + 1;
			}
		}
	}

/* ----------------------------------------------------------------------- *\
|  BuildRegionTable () - Build a table of paired identical regions.
\* ----------------------------------------------------------------------- */
	void
CDiff :: BuildRegionTable (void)

	{
	int  ltIndex, ltPair, ltPairTest;
	int  RegionIndex;


	RegionIndex = 0;
	for (ltPairTest = 0, ltIndex = 1; (ltIndex <= m_EndFileA); ++ltIndex)
		{
		ltPair = m_LineTable[ltIndex].Pair;
		if (ltPair == 0)
			ltPairTest = 0;
		else if (ltPair == ltPairTest)
			{
			++(m_RegionTable[RegionIndex - 1].RegionSize);
			++ltPairTest;
			}
		else // (This is the first line of a file A shared region)
			{
			m_RegionTable[RegionIndex].LineIndexA = ltIndex;
			m_RegionTable[RegionIndex].LineIndexB = ltPair;
			m_RegionTable[RegionIndex].RegionSize = 1;
			++RegionIndex;
			ltPairTest = ltPair + 1;
			}
		}

	RegionIndex = 0;
	for (ltPairTest = 0, ltIndex = (m_EndFileA + 1); (ltIndex <= m_TotalLines); ++ltIndex)
		{
		ltPair = m_LineTable[ltIndex].Pair;
		if (ltPair == 0)
			ltPairTest = 0;
		else if (ltPair == ltPairTest)
			++ltPairTest;
		else // (This is the first line of a file B shared region)
			{
			for (int  Index = 0; (Index < m_PairRegionCount); ++Index)
				{
				if (m_RegionTable[Index].LineIndexB == ltIndex)
					m_RegionTable[Index].RegionIndexB = RegionIndex;
				}
			++RegionIndex;
			ltPairTest = ltPair + 1;
			}
		}
	}

/* ----------------------------------------------------------------------- *\
|  SequenceRegions () - Sequence the paired identical regions
\* ----------------------------------------------------------------------- */
	int				// Returns Success/Failure
CDiff :: SequenceRegions (void)

	{
	int  FirstRegion, LastRegion, MatchRegion;

	FirstRegion = 0;
	while (FirstRegion < m_PairRegionCount)
		{
		LastRegion  = FirstRegion;
		MatchRegion = 0;
		while (LastRegion < m_PairRegionCount)
			{
			int  RegionB = m_RegionTable[LastRegion].RegionIndexB;

			MatchRegion = max(RegionB, MatchRegion);
			if (MatchRegion <= LastRegion)
				break;

			++LastRegion;
			}

		if (FirstRegion == LastRegion)
			m_RegionTable[FirstRegion].Type = Common;

		else if (OptimizeRegions(FirstRegion, LastRegion) == Failure)
			return (Failure);

		// process the region

		FirstRegion = LastRegion + 1;
		}
	return (Success);
	}

/* ----------------------------------------------------------------------- *\
|  OptimizeRegions () - Optimize a sequence of disordered regions
|  Optimize first:  minimum cost
|  Optimize second: earliest common region takes priority
\* ----------------------------------------------------------------------- */
	int				// Returns Success/Failure
CDiff :: OptimizeRegions (
	int  FirstRegion,		// The first region index to be optimized
	int  LastRegion)		// The last  region index to be optimized

#if !(OPTIMIZER - 3)
	{
	// First, assume no region need be moved (which is obviously false)

	for (int i = FirstRegion; (i <= LastRegion); ++i)
		m_RegionTable[i].Type = Common;

//  return (Success);	// (to skip the optimizer)

	for (;;)
		{
		int   i, j;		// General purpose region array indices
		int   HighRegion;	// The index of the worst violator region
		int   HighCost = 0;	// The cost  of the worst violator region

		for (i = FirstRegion; (i <= LastRegion); ++i)
			{
			if (m_RegionTable[i].Type == Common)		// Don't process moved regions
				{
				int  Cost = 0;		// Determine the violation cost for each region
				for (j = FirstRegion; (j < i); ++j)		// Process (i > j) cases
					{
					if ((m_RegionTable[j].Type == Common)
					&&  (m_RegionTable[i].RegionIndexB < m_RegionTable[j].RegionIndexB))
						Cost += m_RegionTable[j].RegionSize;
					}

				for (j = (i + 1); (j <= LastRegion); ++j)	// Process (i < j) cases
					{
					if ((m_RegionTable[j].Type == Common)
					&&  (m_RegionTable[i].RegionIndexB > m_RegionTable[j].RegionIndexB))
						Cost += m_RegionTable[j].RegionSize;
					}

				if (HighCost  <= Cost)	// Remember the highest cost region
				{
				HighCost   = Cost;
				HighRegion = i;
				}
			}
		}

		if (HighCost == 0)		// If the configuration is valid, we are done
			break;

							// Otherwise, we move the highest cost region

		m_RegionTable[HighRegion].Type = MovedA;
		}

	return (Success);
	}

#elif !(OPTIMIZER - 2) // ------------ OLD OPTIMIZER VERSION ---------------
// This one doesn't always terminate

	{
	// Assume no region need be moved (which is obviously false)

	for (int i = FirstRegion; (i <= LastRegion); ++i)
		m_RegionTable[i].Type = Common;

//  return (Success); // (for skipping the optimizer)

	for (;;)
		{
		int   i, j;		// General purpose region array indices
		int   HighCost = 0;	// The cost  of the worst violator region
		int   HighRegion;	// The index of the worst violator region
		int   PrevHighRegion;	// The index of the previous worst violator region
		bool  Cross;		// true if the regions are crossed
		bool  Moved;		// true if exactly one region is moved
		bool  Valid = true;

		for (i = FirstRegion; (i <= LastRegion); ++i)
			{
			int  Cost = 0;		// Determine the violation cost for each region

			for (j = FirstRegion; (j < i); ++j)			// j < i cases
				{
				Cross = (m_RegionTable[i].RegionIndexB < m_RegionTable[j].RegionIndexB);
				Moved = (m_RegionTable[i].Type        != m_RegionTable[j].Type);
				if (Cross ^ Moved)
					{
					Valid = false;
					Cost += m_RegionTable[j].RegionSize;
					}
				}
			for (j = (i + 1); (j <= LastRegion); ++j)		// j > i cases
				{
				Cross = (m_RegionTable[i].RegionIndexB > m_RegionTable[j].RegionIndexB);
				Moved = (m_RegionTable[i].Type        != m_RegionTable[j].Type);
				if (Cross ^ Moved)
					{
					Valid = false;
					Cost += m_RegionTable[j].RegionSize;
					}
				}

			if (HighCost  <= Cost)	// Remember the highest cost region
				{
				HighCost   = Cost;
				HighRegion = i;
				}
			}

		if (Valid)			// If the configuration is valid, we are done
			break;

					// Otherwise cross the highest cost region

		RegType  CurrentType = m_RegionTable[HighRegion].Type;
		m_RegionTable[HighRegion].Type = ((CurrentType == Common) ? MovedA : Common);
		}

	return (Success);
	}

#elif !(OPTIMIZER - 1) // ------------ OLD OPTIMIZER VERSION ---------------
// This one can be very slow

	{
	int    i, j;		// General purpose array indices
	int    BestCost = INT_MAX;	// The best solution cost so far
	int    Cost;		// The cost of the current solution
	int    Count;		// Region count
	bool   Valid;		// Configuration valid when true
	bool   Trying;		// true while still trying combinations
	PBYTE  pArray;		// Ptr to the boolean region array


	Count = LastRegion + 1 - FirstRegion;
	if ((pArray = new BYTE [Count]) == NULL)
		return (Failure);
	memset(pArray, 1, (Count * sizeof(BYTE)));

	Trying = true;
	int  OneCount = Count;
	do  {

		// Advance to the next possible solution

		for (i = 0; (i < Count); ++i)
			{
			BYTE  Value = pArray[i];
			if (Value == 1)
				{
				pArray[i] = 0;
				--OneCount;
				}
			else
				{
				pArray[i] = 1;
				++OneCount;
				break;
				}
			}
		Trying = (OneCount < Count);


		// Determine the solution validity and cost

		Valid = true;
		Cost  = 0;
		for (i = 0; (Valid  &&  (i < Count)); ++i)
			{
			if (pArray[i] == 0)
				Cost += m_RegionTable[FirstRegion + i].RegionSize;

			else // (pArray[i] == 1)
				{
				for (j = (i + 1); (Valid  &&  (j < Count)); ++j)
					{
					if ((pArray[j] == 1)
					&&  (   m_RegionTable[FirstRegion + j].RegionIndexB
					< m_RegionTable[FirstRegion + i].RegionIndexB))
					Valid = false;
					}
				}
			}


		// If a better solution, update the region table

		if (Valid  &&  (Cost < BestCost))
			{
			for (i = 0; (i < Count); ++i)
				{
				m_RegionTable[FirstRegion + i].Type
					= ((pArray[i] == 1) ? Common : MovedA);
				}
			BestCost = Cost;
			}
		} while (Trying);

	delete[] pArray;
	return (Success);
	}
#endif
/* ----------------------------------------------------------------------- *\
|  AssignLineTypes () - Assign the line types to all of the lines
\* ----------------------------------------------------------------------- */
	void
CDiff :: AssignLineTypes (void)

	{
	int  ltIndex;		// Index into the line table


	for (ltIndex = 1; (ltIndex <= m_EndFileA); ++ltIndex)
		m_LineTable[ltIndex].Type = OnlyA;

	for (ltIndex = (m_EndFileA + 1); (ltIndex <= m_TotalLines); ++ltIndex)
		m_LineTable[ltIndex].Type = OnlyB;

	for (int Index = 0; (Index < m_PairRegionCount); ++Index)
		{
		int      RegionSize = m_RegionTable[Index].RegionSize;
		int      LineIndex  = m_RegionTable[Index].LineIndexA;
		RegType  LineType   = m_RegionTable[Index].Type;

		for (ltIndex = LineIndex; (ltIndex < (LineIndex + RegionSize)); ++ltIndex)
			m_LineTable[ltIndex].Type = LineType;

		LineIndex = m_RegionTable[Index].LineIndexB;
		if (LineType == MovedA)
			LineType  = MovedB;

		for (ltIndex = LineIndex; (ltIndex < (LineIndex + RegionSize)); ++ltIndex)
			m_LineTable[ltIndex].Type = LineType;
		}
	}

/* ----------------------------------------------------------------------- *\
|  BuildCompositeIndex () - Build the composite file line index
\* ----------------------------------------------------------------------- */
	void
CDiff :: BuildCompositeIndex (void)

	{
	int  ltIndexC = 1;
	int  ltIndexA = 1;
	int  ltIndexB = m_EndFileA + 1;

	while ((ltIndexA <= m_EndFileA)  ||  (ltIndexB <= m_TotalLines))
		{
		if (ltIndexA > m_EndFileA)
			m_LineTable[ltIndexC].Combined = ltIndexB++;	// File A is exhausted

		else if (ltIndexB > m_TotalLines)
			m_LineTable[ltIndexC].Combined = ltIndexA++;	// File B is exhausted

		else if (m_LineTable[ltIndexA].Type != Common)
			m_LineTable[ltIndexC].Combined = ltIndexA++;	// File A is not Common

		else if (m_LineTable[ltIndexB].Type != Common)
			m_LineTable[ltIndexC].Combined = ltIndexB++;	// File B is not Common

		else
			{
			m_LineTable[ltIndexC].Combined = ltIndexA++;	// Both files are Common
			++ltIndexB;
			}
		++ltIndexC;
		}

	m_FileInfo[C].Lines = ltIndexC - 1;
	}

/* ----------------------------------------------------------------------- *\
|  All following code is for testing the class in console mode
\* ----------------------------------------------------------------------- */
#ifdef CONSOLEMODETEST

/* ----------------------------------------------------------------------- *\
|  DisplayMiscInfo () - Display the miscellaneous member variables
\* ----------------------------------------------------------------------- */
	void
CDiff :: DisplayMiscInfo (void)

	{
	printf("Max length:       %d\n", m_MaxLength);
	printf("Total size:       %d\n", m_TotalSize);
	printf("Total lines:      %d\n", m_TotalLines);
	printf("Combined lines:   %d\n", m_FileInfo[C].Lines);
	printf("End of file A:    %d\n", m_EndFileA);
	printf("Hash table size:  %d\n", m_HashTableSize);
	printf("Pair regions:     %d\n", m_PairRegionCount);
	printf("\n");
	}

/* ----------------------------------------------------------------------- *\
|  DisplayFileInfo () - Display a specified FILEINFO structure
\* ----------------------------------------------------------------------- */
	void
CDiff :: DisplayFileInfo (
	int        Index)

	{
	PFileInfo  p = &m_FileInfo[Index];

	printf("(%d) Filename:  \"%s\"\n", Index, (LPCTSTR)(p->FileName));
	printf("    pData     Length  Lines  CREOL  NLEOF  Handle\n");
#ifdef _WIN64
	printf("    %08llX   %3d     %3d   %-5.5s  %-5.5s  %08llX\n",
		(UINT64)(p->pFileData), p->FileLength, p->Lines,
		(p->CR_EOL ? "true" : "false"),
		(p->NL_EOL ? "true" : "false"),
		(UINT64)(p->hFile));
#else
	printf("    %08X   %3d     %3d   %-5.5s  %-5.5s  %08X\n",
		(UINT32)(p->pFileData), p->FileLength, p->Lines,
		(p->CR_EOL ? "true" : "false"),
		(p->NL_EOL ? "true" : "false"),
		(UINT32)(p->hFile));
#endif
	}

/* ----------------------------------------------------------------------- *\
|  DisplayFileTable () - Display all FILEINFO structures in the array
\* ----------------------------------------------------------------------- */
	void
CDiff :: DisplayFileTable (void)

	{
	DisplayFileInfo(A);
	DisplayFileInfo(B);
	DisplayFileInfo(C);
	}

/* ----------------------------------------------------------------------- *\
|  DisplayHashInfo () - Display a specified hash table entry
\* ----------------------------------------------------------------------- */
	void
CDiff :: DisplayHashInfo (
	int       Index)

	{
	printf("(%3d)  %5d\n", Index, m_HashTable[Index]);
	}

/* ----------------------------------------------------------------------- *\
|  DisplayHashTable () - Display the hash table
\* ----------------------------------------------------------------------- */
	void
CDiff :: DisplayHashTable (void)

	{
	if (m_HashTable == NULL)
		printf("\nNo hash table\n");

	else // (display the header and the table)
		{
		printf("\nIndex  Value\n");
		for (int Index = 0; (Index < m_HashTableSize); ++Index)
			DisplayHashInfo(Index);
		}
	}

/* ----------------------------------------------------------------------- *\
|  DisplayLineInfo () - Display a specified LINETBL structure
\* ----------------------------------------------------------------------- */
	void
CDiff :: DisplayLineInfo (
	int       Index)

	{
	PLINETBL  p = &m_LineTable[Index];
	CString   Str;

	if (p->pLine != NULL)	// Prepare the line string
		{
		for (PSTR s = p->pLine; ((*s != '\0') && (*s != '\r') && (*s != '\n')); ++s)
			Str += *s;
		}

#ifdef _WIN64
	printf("(%3d)  %s %5d  %5d  %08llX  \"%s\"\n",
		Index, TranslateRegionType(p->Type), p->Pair, p->Combined, (UINT64)(p->pLine), (LPCTSTR)(Str));
#else // _WIN32
	printf("(%3d)  %s %5d  %5d  %08X  \"%s\"\n",
		Index, TranslateRegionType(p->Type), p->Pair, p->Combined, (UINT32)(p->pLine), (LPCTSTR)(Str));
#endif
	}

/* ----------------------------------------------------------------------- *\
|  DisplayLineTable () - Display all LINETBL structures in the array
\* ----------------------------------------------------------------------- */
	void
CDiff :: DisplayLineTable (void)

	{
	if (m_LineTable == NULL)
		printf("\nNo line table\n");

	else // (display the header and the table)
		{
		printf("\nIndex  Type     Pair  Index  pLine     Line\n");
		for (int Index = 1; (Index <= m_TotalLines); ++Index)
			{
			DisplayLineInfo(Index);
			if (Index == m_EndFileA)
				printf("\n");
			}
		}
	}

/* ----------------------------------------------------------------------- *\
|  DisplayRegionInfo () - Display a specified REGIONTBL structure
\* ----------------------------------------------------------------------- */
	void
CDiff :: DisplayRegionInfo (
	int       Index)

	{
	PREGIONTBL  p = &m_RegionTable[Index];

	printf("(%3d)  %5d  %5d  %7d  %4d  %s\n",
	Index, p->LineIndexA, p->LineIndexB, p->RegionIndexB, p->RegionSize,
	TranslateRegionType(p->Type));
	}

/* ----------------------------------------------------------------------- *\
|  DisplayRegionTable () - Display all REGIONTBL structures in the array
\* ----------------------------------------------------------------------- */
	void
CDiff :: DisplayRegionTable (void)

	{
	if (m_RegionTable == NULL)
		printf("\nNo region table\n");

	else // (display the header and the table)
		{
		printf("\nIndex  LineA  LineB  RegionB  Size  Type\n");
		for (int Index = 0; (Index < m_PairRegionCount); ++Index)
		DisplayRegionInfo(Index);
		}
	}

/* ----------------------------------------------------------------------- *\
|  TranslateRegionType () - Translate a region type spec into an ASCII string
\* ----------------------------------------------------------------------- */
	PSTR
CDiff :: TranslateRegionType (
	RegType  Type)			// The region type

	{
	return (
		(Type == Common) ? "Common " :
		(Type == OnlyA)  ? "A only " :
		(Type == OnlyB)  ? "B only " :
		(Type == MovedA) ? "A moved" :
							"B moved");
	}

/* ----------------------------------------------------------------------- *\
|  TimeStamp () - Output a timestamp
\* ----------------------------------------------------------------------- */
#if USETIMESTAMPS
	void
_TimeStamp (
	PSTR  p)		// Ptr to the description string

	{
static DWORD  BaseTime = 0;
	DWORD  CurrentTime = GetTickCount();

	if (BaseTime == 0)
		BaseTime = CurrentTime;
	CurrentTime -= BaseTime;

	printf("%6d mS. - %s\n", CurrentTime, p);
	}
#endif
/* ----------------------------------------------------------------------- *\
|  main () - The test program main()
\* ----------------------------------------------------------------------- */

CDiff  Difference;	// The class instance

	int
main (
	int  argc,
	char *argv[])

	{
	if (argc != 3)
		{
		printf("This program needs two filename arguments\n");
		return (1);
		}

//    Difference.SetIgnoreAllBlanks(true);

	if ( ! Difference.LoadFiles(argv[1], argv[2]))
		{
		printf("%s\n", (LPCTSTR)(Difference.GetError()));
		Difference.Reset();
		return (1);
		}

	Difference.DisplayMiscInfo();
	Difference.DisplayFileTable();
//  printf("GetFileSize():  %d  %d  %d\n",
//	Difference.GetFileSize(A),
//	Difference.GetFileSize(B),
//	Difference.GetFileSize(C));
//  Difference.DisplayHashTable();
	Difference.DisplayLineTable();
	Difference.DisplayRegionTable();

	Difference.Reset();

	return (0);
	}
#endif
/* --------------------------------- EOF --------------------------------- */
