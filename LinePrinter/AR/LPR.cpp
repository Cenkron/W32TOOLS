// ----------------------------------------------------------------------------
//
// LPR.cpp : Implementation of LPR C interface to GMlineprint.cpp
//
// ----------------------------------------------------------------------------

#undef _DLL

#include "stdafx.h"
#define WIN32_WINXP

#include "gmfontset.h"
#include <vector>
#include "GMLinePrint.h"
#include "lpr.h"

// ----------------------------------------------------------------------------

LPR::LPR()
	{
	}


LPR::~LPR()
	{
	}


// The Write method places a line of text into the head array, the body array or 
// causes a newpage code to be appended to the body array

	bool
LPR::Write (
	LINE_TYPE type,
	char *text)

	{
	return (m_lpr->write(type, text));
	}


// The Erase method deletes line data of specified type, or both

	bool
LPR::Erase (
	RESET_TYPE type)
	{
	return (m_lpr->reset(type));
	}


// The Print method loops to print the pages of the document

	bool
LPR::Print (
	PRINT_TYPE type)

	{
	return (m_lpr->print(type));
	}


// Sets the punch margin
// Default is to always leave a margin for 3 hole punching (0.6 inch)

	bool
LPR::SetPunchMargin (
	double margin)

	{
	m_lpr->m_punch_margin = margin;
	return true;
	}


// Set page breaks enable; If page_breaks is true (the default) the print_loop looks
// for the form feed token (@@FF) at the start of the string and forces a new page.

	bool
LPR::SetPageBreaks (
	bool usePageBreaks)

	{
	m_lpr->m_b_use_page_breaks = usePageBreaks;
	return true;
	}


// If the print_header value is true (the default) the header and footer
// print for each new page.  If print_header is false only the body lines print.

	bool
LPR::SetWriteHeaders (
	bool printHeader)

	{
	m_lpr->m_b_print_head = printHeader;
	return true;
	}


// The title prints at the bottom of each sheet in the lower right corner

	bool
LPR::SetTitle (
	char *pTitle)

	{
	m_lpr->m_title = pTitle;
	return true;
	}


// Set the page orientation (portrait or landscape)

	bool
LPR::SetOrient (
	ORIENT orientation)

	{
	m_lpr->m_orientation = orientation;
	return true;
	}

// Set the font (10, 12, or 15 cpi)

	bool
LPR::SetFont (
	FONT_CPI font)

	{
	m_lpr->m_font = font;
	return true;
	}

// ----------------------------------------------------------------------------
//									EOF
// ----------------------------------------------------------------------------
