// -------------------------------------------------------------------
//
// GMLinePrint.h: interface for the GMLinePrint class.
//
// -------------------------------------------------------------------

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <vector>

// these are really BFI magic numbers that make line height and page width
// calculations easier for me.  We are part of the US construction industry and are
// mired in old imperial measurements, so all our page dimensions are in inches.

#define LINEHEIGHT_15	 -12
#define LINEHEIGHT_12	 -15
#define LINEHEIGHT_10	 -17
#define MM_TO_INCH		 3.937		// mm to 1/100 inch
#define FOOTER_LINES	 2         // lines in the footer
#define HEADER_SEPARATOR 1		// lines for separator
#define INCH			 100	// used to convert inch values to MM_LOENGLISH mapping

typedef enum
	{
	LT_DEFAULT = 0,
	LT_HEAD = 1,
	LT_BODY = 2,
	LT_NEWPAGE = 3
	} LINE_TYPE;

typedef enum
	{
	RS_NONE = 0,
	RS_HEAD = 1,
	RS_BODY = 2,
	RS_ALL = 3
	} RESET_TYPE;

typedef enum
	{
	DEFAULT = 0,
	PAGE_RANGE = 1,
	NO_PAGE_BREAK = 2,
	NO_HEADING = 3
	} PRINT_TYPE;

typedef enum
	{
	PORTRAIT = 0,
	LANDSCAPE = 1
	} ORIENT;

typedef enum
	{
	FONT_10 = 0,
	FONT_15 = 1,
	FONT_12 = 2
	} FONT_CPI;

typedef enum
	{
	SINGLE		 = 1,
	DOUBLE_SHORT = 2,
	DOUBLE_LONG	 = 3
	} DUPLEX;


#ifndef COMPILING	// Prevents needing to declare all internals to the main program
#define CDC   int
#define CFont int
#endif // COMPILING


class GMLinePrint
	{
	public:
		bool print(PRINT_TYPE print_type);
		bool reset(RESET_TYPE reset_type);
//		bool write(LINE_TYPE line_type, BSTR text);
		bool write(LINE_TYPE line_type, char *text);
		GMLinePrint();
		virtual ~GMLinePrint();
		double m_punch_margin;
		bool m_b_use_page_breaks;
		bool m_b_print_head;
		CString m_title;
		short m_orientation;
		short m_font;
		bool  m_Ready;
		short m_duplex;

	protected:
		short m_top_offset;
		short m_left_offset;
		bool new_page(CDC *dc);
		short m_line;
		short m_last_body_line;

		short m_page_height;
		short m_page_width;
		short m_line_height;
		short m_max_lines;
		short m_page;
		bool page_loop();
		CFont *C10;
		CFont *C12;
		CFont *C15;

		std::vector<CString> *head_lines;
		std::vector<CString> *body_lines;

	protected:
		short m_cpi;
	};

// -------------------------------------------------------------------
//								EOF
// -------------------------------------------------------------------
