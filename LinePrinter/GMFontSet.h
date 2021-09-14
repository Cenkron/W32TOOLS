// GMFontSet.h: interface for the GMFontSet class.
// These are some standard fonts we use here that make font changes easier to deal
// with. 
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GMFONTSET_H__66D0B39A_886C_11D2_BB2A_00104BCAD0E6__INCLUDED_)
#define AFX_GMFONTSET_H__66D0B39A_886C_11D2_BB2A_00104BCAD0E6__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


class GMArial : public CFont  
	{
	public:
	
		GMArial(int height , int weight = FW_NORMAL);
		virtual ~GMArial();

	};

class GMFixed : public CFont  
	{
	public:
	
		GMFixed(int height , int weight = FW_NORMAL);
		virtual ~GMFixed();

	};

class GMFixedSS : public CFont
	{
	public:
	
		GMFixedSS(int height , int weight = FW_NORMAL);
		virtual ~GMFixedSS();

	};

#endif // !defined(AFX_GMFONTSET_H__66D0B39A_886C_11D2_BB2A_00104BCAD0E6__INCLUDED_)
