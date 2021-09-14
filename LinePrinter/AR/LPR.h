// ----------------------------------------------------------------------------
//
// LPR.h : Implementation of LPR C interface to GMlineprint class
//
// ----------------------------------------------------------------------------

#ifndef _LPR_H_
#define _LPR_H_

#include "GMFontSet.h"
#include "GMLinePrint.h"


// ----------------------------------------------------------------------------

class LPR : public GMLinePrint
	{
	public:
		// Methods

		LPR();

		~LPR();

		bool	Write(
			LINE_TYPE type,
			char *text);

		bool	Erase(
			RESET_TYPE type);

		bool	Print(
			PRINT_TYPE type);

		bool	SetPunchMargin(
			double margin);

		bool	SetPageBreaks(
			bool usePageBreaks);

		bool	SetWriteHeaders(
			bool printHeader);

		bool	SetTitle(
			char *pTitle);

		bool	SetOrient(
			ORIENT orientation);

		bool	SetFont(
			FONT_CPI font);

	private:

		GMLinePrint* m_lpr;

	};

// ----------------------------------------------------------------------------

#endif //_LPR_H_

// ----------------------------------------------------------------------------
//									EOF
// ----------------------------------------------------------------------------
