/* ----------------------------------------------------------------------- *\
|
|				     CDiff
|		  A C++ class used to analyze file differences
|			       (Definition file)
|
|		   Copyright (c) 1997, all rights reserved
|				Brian W Johnson
|				   21-Aug-97
|
\* ----------------------------------------------------------------------- */

enum RegType  { Common, OnlyA, OnlyB, MovedA, MovedB }; // Membership region types
enum FileDesc { A, B, C };

class  CFileInfo		// Helper class for CDiff
    {
public:

    CString  FileName;		// Ptr to the file name
    PSTR    pFileData;		// Ptr to the file data
    int      FileLength;	// Length of the file data (bytes)
    int      Lines;		// Length of the file data (lines)
    int      TabSize;		// Tab stop interval
    BOOL     CR_EOL;		// TRUE to treat CR as EOL
    BOOL     NL_EOL;		// TRUE to treat NL as EOL
    HANDLE   hFile;		// The file handle, while open

    CFileInfo () :		// The class constructor
	    FileName()
	{
	pFileData  = NULL;
	FileLength = 0;
	Lines      = 0;
	TabSize    = 4;
	CR_EOL     = TRUE;
	NL_EOL     = TRUE;
	hFile      = INVALID_HANDLE_VALUE;
	}

    ~CFileInfo ()		// The class destructor
	{
	FileName.Empty();
	}
    };

typedef CFileInfo  *PFileInfo;


class  CDiff
    {
private:

#define MAXLINELENGTH   (256)		// Maximum displayable line width


    CFileInfo  m_FileInfo [3];		// The three file info instances

    typedef
	struct linetable
	    {
	    PSTR          pLine;	// Ptr to the text line
	    int            Pair;	// Index of the paired counterpart line
	    union
		{
		ULONG      Hash;	// The line hash value
		RegType    Type;	// The region type
		};
	    int            Combined;	// Index of the combined file line
	    } LINETBL, *PLINETBL;

    enum TableResult { Failure, Success, Interrupted };
    PLINETBL  m_LineTable;		// Ptr to the virtual LINETBL array

    typedef
	struct regiontable
	    {
	    int      LineIndexA;	// Index of the first line in file A
	    int      LineIndexB;	// Index of the first line in file B
	    int      RegionIndexB;	// Index of the region in file B
	    int      RegionSize;	// Size of the region (lines)
	    RegType  Type;		// The determined region type
	    } REGIONTBL, *PREGIONTBL;

    PREGIONTBL  m_RegionTable;		// Ptr to the virtual REGIONTBL array

    int       m_MaxLength;		// Length of the longest line

    PSTR      m_FileData;		// Ptr to the file data (for both files)
    int       m_TotalSize;		// Total size of the file data buffer
    int       m_TotalLines;		// Total lines (in both files)
    int       m_EndFileA;		// Index of last line of file A
    int       m_PairRegionCount;	// Count of the number of pair regions

    PULONG    m_HashTable;		// Ptr to the virtual hash table array
    int       m_HashTableSize;		// Size of the hash table

//    HANDLE    m_Sem;			// The thread control semaphore
    CString   m_Error;			// Description of the most recent error


	// Property variables

    BOOL      m_IgnoreAllBlanks;	// TRUE to ignore all      blanks
    BOOL      m_IgnoreLeadingBlanks;	// TRUE to ignore leading  blanks
    BOOL      m_IgnoreTrailingBlanks;	// TRUE to ignore trailing blanks
//??? BOOL      m_InterleaveUniqueLines;	// ???

    int       m_Failure;		// Window message signifying success
    int       m_Success;		// Window message signifying failure
    HWND      m_Hwnd;			// Window handle to post messages to

public:
    CDiff ();
    ~CDiff ();

    void  Reset (void);
    BOOL  LoadFiles (PSTR pA, PSTR pB);
    BOOL  RebuildTables (void);
    CString &GetError (void);
    void  SetTabSize (int FileIndex, int TabSize);
    void  SetIgnoreAllBlanks (BOOL Value);
    void  SetIgnoreLeadingBlanks (BOOL Value);
    void  SetIgnoreTrailingBlanks (BOOL Value);
    void  SetHwnd (HWND Hwnd);
    void  SetFailureMessage (int Message);
    void  SetSuccessMessage (int Message);

    PSTR  GetTextLine (FileDesc f, int LineNumber, int *pLineNumber, RegType *pRegType);
    BOOL  GetFileMatch (void);
    int   GetFileSize (FileDesc f);


#if 0
    void BarChart  (HPS hps, LONG cxClient, LONG cyClient, LONG cxBar);
    LONG FillBlock (HPS hps, LONG cxClient, LONG cyClient, LONG cxBar,
		SHORT ltTop, SHORT ltBottom);
    int PrintableLine (int ltIndex, PLONG pTextColor, PLONG pBackColor);
    int RelativeLine (int ltIndex, int Offset);
#endif


private:
    BOOL  LoadFilesThread (void);
    void  ReleaseFiles (void);
//    TableResult  BuildTablesThread (void);
    int   BuildTablesThread (void);
    void  ReleaseTables (void);

    BOOL  OpenSpecifiedFile (PFileInfo pFileInfo);
    BOOL  ReadSpecifiedFile (PFileInfo pFileInfo);
    void  AnalyzeFile (PFileInfo pFileInfo);
    void  DetermineEOL (PFileInfo pFileInfo);
    void  BuildLineHashTable (PFileInfo pFileInfo, int ltIndex);
    void  ChainIdenticalLines (void);
    void  FindUniquePairs (void);
    void  ExpandPairRegions (void);
    int   VerifyPair (int Index1, int Index2);
    void  CountPairRegions (void);
    void  BuildRegionTable (void);
    int   SequenceRegions (void);
    int   OptimizeRegions (int FirstRegion, int LastRegion);
    void  AssignLineTypes (void);
    void  BuildCompositeIndex (void);

#ifdef CONSOLEMODETEST	// Diagnostic member functions
public:
    void  DisplayMiscInfo (void);
    void  DisplayFileInfo (int Index);
    void  DisplayFileTable (void);
    void  DisplayHashInfo (int Index);
    void  DisplayHashTable (void);
    void  DisplayLineInfo (int Index);
    void  DisplayLineTable (void);
    void  DisplayRegionInfo (int Index);
    void  DisplayRegionTable (void);
    PSTR  TranslateRegionType (RegType Type);
#endif
    };

/* --------------------------------- EOF --------------------------------- */
