// ----------------------------------------------------------------------------
//
// LPRC.h : Implementation of LPR C interface to GMlineprint class
//
// ----------------------------------------------------------------------------

#ifndef _LPR_H_
#define _LPR_H_

// ----------------------------------------------------------------------------

// The Write method places a line of text into the head array, the body array or 
// causes a newpage code to be appended to the body array

	bool
LprWrite (
	LINE_TYPE type,
	char *text);

// The Erase method deletes line data of specified type, or both

	bool
LprErase (
	RESET_TYPE type);

// The Print method loops to print the pages of the document

	bool
LprPrint (
	PRINT_TYPE type);

// Gets the ready flag (probably pointless
// Ready signifies successful construction

	bool
LprGetReady ();

// Sets the punch margin
// Default is to always leave a margin for 3 hole punching (0.6 inch)

	bool
LprSetPunchMargin (
	double margin);

// Set page breaks enable; If page_breaks is true (the default) the print_loop looks
// for the form feed token (@@FF) at the start of the string and forces a new page.

	bool
LprSetPageBreaks (
	bool usePageBreaks);

// If the print_header value is true (the default) the header and footer
// print for each new page.  If print_header is false only the body lines print.

	bool
LprSetWriteHeaders (
	bool printHeader);

// The title prints at the bottom of each sheet in the lower right corner

	bool
LprSetTitle (
	char *pTitle);

// Set the page orientation (portrait or landscape)

	bool
LprSetOrient (
	ORIENT orientation);

// Set the font (10, 12, or 15 cpi)

	bool
LprSetFont (
	FONT_CPI font);

// Set the duplex (single, double short, double long)

	bool
LprSetDuplex (
	DUPLEX duplex);

// ----------------------------------------------------------------------------

#endif //_LPR_H_

// ----------------------------------------------------------------------------
//									EOF
// ----------------------------------------------------------------------------
