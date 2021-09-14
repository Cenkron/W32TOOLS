// ----------------------------------------------------------------------------
//
// LPtypes.h
//
// ----------------------------------------------------------------------------

#ifndef _LPtypes_H_
#define _LPtypes_H_

// ----------------------------------------------------------------------------

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

// ----------------------------------------------------------------------------

#endif // _LPtypes_H_

// ----------------------------------------------------------------------------
//									EOF
// ----------------------------------------------------------------------------
