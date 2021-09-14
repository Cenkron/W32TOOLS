// -------------------------------------------------------------------
// GMLinePrint.cpp: implementation of the GMLinePrint class.
// Copyright 1999 by Gayle Mfg Co. 

// BFI = Brute Force and Ignorance.  Much of this code will appear so to
//       experienced programmers.  
// -------------------------------------------------------------------

#define COMPILING

#define WINVER 0x0A00

#include "stdafx.h"

#include "gmfontset.h"
#include <vector>
#include "GMLinePrint.h"
#include "lprc.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

using namespace std;
// -------------------------------------------------------------------
// Construction/Destruction
// -------------------------------------------------------------------

GMLinePrint::GMLinePrint ()
	{
	m_Ready = false;

	// initialize the fonts	 (see the GM font code for implementation details)
	C15 = (CFont*) new GMFixed(13, FW_BOLD);
	C12 = (CFont*) new GMFixed(15, FW_BOLD);
	C10 = (CFont*) new GMFixed(18, FW_BOLD);

	// and the rest of the object
	m_b_print_head = true;
	m_b_use_page_breaks = true;
	m_title = "GMPrintEZ";
	m_punch_margin = 0.60;
	m_top_offset = 0;
	m_left_offset = 0;
	m_cpi = 10;
	m_duplex = SINGLE;

	// keeping the line arrays off the stack
	body_lines = new vector<CString>;
	head_lines = new vector<CString>;

	m_Ready = true;
	}

// -------------------------------------------------------------------

GMLinePrint::~GMLinePrint ()
	{
	if (C15)
		delete C15;
	if (C10)
		delete C10;
	if (C12)
		delete C12;
	if (body_lines) 
		delete body_lines;
	if (head_lines)
		delete head_lines;
	}


// -------------------------------------------------------------------

	bool
GMLinePrint::page_loop ()
	{
	// loop thru the body lines and print to the page
	try
		{
		CPrintDialog pd(0);					// lazy way of getting the default printer
		DOCINFO di;							// must have DOCINFO for CDC::StartDoc(&DOCINFO)
		m_line = 0;
		m_max_lines = 20;
		m_last_body_line = m_max_lines;
		m_line_height = LINEHEIGHT_10;

		memset(&di, 0, sizeof(DOCINFO));	// make a clean start
		di.cbSize = sizeof(DOCINFO);
		di.lpszDocName = m_title;

		// if the body lines dont contain anything just return now
		if (0 == body_lines->size())
			return true;

		pd.GetDefaults();					// just get all the printer defaults - no display
											// so this COM object can run from MTS
	
		DEVMODE *pdm = pd.GetDevMode();
	
		// set orientation
		// print landscape or protrait?
		pdm->dmOrientation = m_orientation + 1;
		// signify te presence of orientation data
		pdm->dmFields |= DM_ORIENTATION | DM_DUPLEX;
		pdm->dmDuplex = m_duplex;

		// set punch margin
		switch (m_orientation)
			{
			case PORTRAIT:
				m_top_offset = 0;
				m_left_offset = (short)(INCH * m_punch_margin);
				break;

			case LANDSCAPE:
				m_top_offset = (short)(INCH * m_punch_margin);
				m_left_offset = 0;
				break;
			}

		// create the printer device context by getting values from the printdialog and the 
		// dm structure
		CDC dc;
		if (! dc.CreateDC(pd.GetDriverName(), pd.GetDeviceName(), pd.GetPortName(), pdm))
			{
			AfxMessageBox(_T("Can't create DC in print_loop"));
			return false;
			}
	
		dc.StartDoc(&di);

		// obtain the page dimensions from the Device Context
		m_page_height = (short)(dc.GetDeviceCaps(VERTSIZE) * MM_TO_INCH);
		m_page_width = (short)(dc.GetDeviceCaps(HORZSIZE) * MM_TO_INCH);

		CFont *oldfont;

		// select font and set line height
		switch (m_font)
			{
			case FONT_12:
				oldfont = dc.SelectObject(C12);
				m_line_height = LINEHEIGHT_12;
				m_cpi = 12;
				break;
			case FONT_15:
				oldfont = dc.SelectObject(C15);
				m_line_height = LINEHEIGHT_15;
				m_cpi = 15;
				break;
			case FONT_10:
			default:
				oldfont = dc.SelectObject(C10);
				m_line_height = LINEHEIGHT_10;
				m_cpi = 10;
				break;
			}


		// compute the lines per page
		m_max_lines = -(( m_page_height - m_top_offset) / m_line_height);

		// compute the last body line
		if (m_b_print_head)
			{
			m_last_body_line = (short)(m_max_lines - FOOTER_LINES - head_lines->size() - HEADER_SEPARATOR);
			}
		else
			{
			m_last_body_line = m_max_lines;
			}




		// the print loop
		// I like to use the Standard Library collections when I can. 
		vector<CString>::iterator itext;
		m_page = 0;
		for (itext = body_lines->begin(); itext < body_lines->end(); ++itext)
			{
			// look for a pagebreak
			if ("@@FF" == itext->Left(4))
				{
				if (m_b_use_page_breaks)
					{
					m_line = 0;
					dc.EndPage();
					new_page(&dc);
					}
				// otherwise just ignore the page_break token
				}
			else
				{
				if (m_line >= m_last_body_line)
					{
					m_line = 0;
					dc.EndPage();
					}
				if (m_line == 0)
					new_page(&dc);

				dc.TextOut(m_left_offset, (m_line++ * m_line_height) - m_top_offset, *itext);
				}

			}

		dc.EndPage();
		dc.EndDoc();
		dc.DeleteDC();
		return true;
		}

	catch(...)
		{
		return false;
		}
	}

// -------------------------------------------------------------------

	bool
GMLinePrint::write (LINE_TYPE line_type, char *text)
	{
	CString A(text);  
//	SysFreeString(text);

	 // builds the arrays that contain the data to be printed
	try
		{
		switch (line_type)
			{
			case LT_BODY:
				body_lines->push_back(A);
				break;
			case LT_HEAD:
				head_lines->push_back(A);
				break;
			case LT_NEWPAGE:
				body_lines->push_back("@@FF");     // the @@@ is a special flag FF means form feed
				break;
			}
	
		return true;
		}

	catch(...)
		{		  
		return false;
		}
	}

// -------------------------------------------------------------------

	bool
GMLinePrint::reset (RESET_TYPE reset_type)
	{
	try
		{
		switch(reset_type)
			{
			case RS_HEAD:
				head_lines->erase(head_lines->begin(), head_lines->end());
				break;
			case RS_BODY:
				body_lines->erase(body_lines->begin(), body_lines->end());
				break;
			case RS_ALL:
				head_lines->erase(head_lines->begin(), head_lines->end());
				body_lines->erase(body_lines->begin(), body_lines->end());
				break;
			}
		return true;
		}

	catch(...)
		{
		return false;
		}
	}

// -------------------------------------------------------------------

	bool
GMLinePrint::print (PRINT_TYPE print_type)
	{
	return page_loop();
	}

// -------------------------------------------------------------------

	bool
GMLinePrint::new_page(CDC* dc)
	{
		try
		{
			dc->StartPage();
			dc->SetMapMode(MM_LOENGLISH);		// mapmode settings made before starting the 
												// page	 seem to get lost on Win95 - not so on NT4.0

			++m_page;

			if (!m_b_print_head)	  // we aren't going to print the heading and footer
				return true;

			// the print loop  for the HEADING Lines
			vector<CString>::iterator itext;

			for (itext = head_lines->begin(); itext < head_lines->end(); ++itext)
			{

				dc->TextOut(m_left_offset, (m_line++ * m_line_height) - m_top_offset, *itext);
				if (m_line >= m_last_body_line)
					break;
			}

			// draw the separators
			dc->SelectStockObject(BLACK_PEN);
			int y = (m_line++ * m_line_height) - m_top_offset;
			dc->MoveTo(0, y);
			dc->LineTo(m_page_width, y);
			y = -(m_page_height - 32);
			dc->MoveTo(0, y);
			dc->LineTo(m_page_width, y);


			//		COleDateTime dt;
			//		dt = COleDateTime::GetCurrentTime();
			//		CString footer;
			//		footer.Format(_T("Pg. %d"),  m_page);

			y = -(m_page_height)-(m_line_height);
			//		dc->TextOut(m_left_offset, y , dt.Format(_T("%m/%d/%y  %H:%M")) );
			//		dc->TextOut((m_page_width - m_left_offset)/2, y,footer);
			//		dc->TextOut((m_page_width - ((m_title.GetLength()+2) *(INCH/m_cpi))), y , m_title);
			return true;
		}

		catch (...)
		{
			return false;
		}
	}

	// -------------------------------------------------------------------
	//								EOF
	// -------------------------------------------------------------------
