// ----------------------------------------------------------------------------
//
// LPRC.cpp : Implementation of LPR C interface to GMlineprint.cpp
//
// ----------------------------------------------------------------------------

#include "stdafx.h"
#include "GMFontSet.h"
#include "GMLinePrint.h"
#include "lprc.h"

// ----------------------------------------------------------------------------

GMLinePrint *pLpr = new GMLinePrint;

// ----------------------------------------------------------------------------

// Gets the ready flag (probably pointless
// Ready signifies successful construction

	bool
LprGetReady ()

	{
	pLpr = new GMLinePrint;
	return pLpr->m_Ready;
	}

// The Write method places a line of text into the head array, the body array or 
// causes a newpage code to be appended to the body array

	bool
LprWrite (
	LINE_TYPE type,
	char *text)

	{
	return pLpr->write(type, text);
	}


// The Erase method deletes line data of specified type, or both

	bool
LprErase (
	RESET_TYPE type)

	{
	return (pLpr->reset(type));
	}


// The Print method loops to print the pages of the document

	bool
LprPrint (
	PRINT_TYPE type)

	{
	return (pLpr->print(type));
	}


// Sets the punch margin
// Default is to always leave a margin for 3 hole punching (0.6 inch)

	bool
LprSetPunchMargin (
	double margin)

	{
	pLpr->m_punch_margin = margin;
	return true;
	}


// Set page breaks enable; If page_breaks is true (the default) the print_loop looks
// for the form feed token (@@FF) at the start of the string and forces a new page.

	bool
LprSetPageBreaks (
	bool usePageBreaks)

	{
	pLpr->m_b_use_page_breaks = usePageBreaks;
	return true;
	}


// If the print_header value is true (the default) the header and footer
// print for each new page.  If print_header is false only the body lines print.

	bool
LprSetWriteHeaders (
	bool printHeader)

	{
	pLpr->m_b_print_head = printHeader;
	return true;
	}


// The title prints at the bottom of each sheet in the lower right corner

	bool
LprSetTitle (
	char *pTitle)

	{
	pLpr->m_title = CString(pTitle);
	return true;
	}


// Set the page orientation (portrait or landscape)

	bool
LprSetOrient (
	ORIENT orientation)

	{
	pLpr->m_orientation = orientation;
	return true;
	}


// Set the font (10, 12, or 15 cpi)

	bool
LprSetFont (
	FONT_CPI font)

	{
	pLpr->m_font = font;
	return true;
	}

// Set the duplex (single, double short, double long)

	bool
LprSetDuplex (
	DUPLEX duplex)

	{
	pLpr->m_duplex = duplex;
	return true;
	}

// ----------------------------------------------------------------------------
//									EOF
// ----------------------------------------------------------------------------
