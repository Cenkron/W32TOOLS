// GMFontSet.cpp: implementation of the GMFontSet class.
//
//////////////////////////////////////////////////////////////////////

#undef _DLL

#define WINVER 0x0A00

#include "stdafx.h"
#include "GMFontSet.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

GMArial::GMArial(int height, int weight)
	{
	CreateFont (height,  //height
				0,	// width
				0,	// escapement
				0,	//orientation
				weight,	//weight
				FALSE,	//italic
				FALSE,	//underline
				FALSE,	//strikeout
				ANSI_CHARSET,
				OUT_DEVICE_PRECIS,
				CLIP_DEFAULT_PRECIS,
				DEFAULT_QUALITY,
				FF_SWISS,
				_T("Arial"));
	}

GMArial::~GMArial()
	{
	}


GMFixed::GMFixed(int height, int weight)
	{
	CreateFont (height,  //height
				0,	// width
				0,	// escapement
				0,	//orientation
				weight,	//weight
				FALSE,	//italic
				FALSE,	//underline
				FALSE,	//strikeout
				ANSI_CHARSET,
				OUT_DEVICE_PRECIS,
				CLIP_DEFAULT_PRECIS,
				DEFAULT_QUALITY,
				FIXED_PITCH | FF_MODERN,
				_T("Courier New"));
	}

GMFixed::~GMFixed()
	{
	}

GMFixedSS::GMFixedSS(int height, int weight)
	{
	CreateFont (height,  //height
				0,	// width
				0,	// escapement
				0,	//orientation
				weight,	//weight
				FALSE,	//italic
				FALSE,	//underline
				FALSE,	//strikeout
				ANSI_CHARSET,
				OUT_DEVICE_PRECIS,
				CLIP_DEFAULT_PRECIS,
				DEFAULT_QUALITY,
				FIXED_PITCH | FF_SWISS,
				_T("Lucida Console"));
	}

GMFixedSS::~GMFixedSS()
	{
	}
