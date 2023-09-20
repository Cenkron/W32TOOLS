//////////////////////////////////////////////////////////////////////////
// Standard Data Type Definitions					//
//////////////////////////////////////////////////////////////////////////
// Copyright (c) 1985-90 Miller Micro Systems, Garland TX		//
//////////////////////////////////////////////////////////////////////////

#ifndef _DTYPES_H
#define _DTYPES_H

#include <limits.h>

	/***** 		storage classes		*****/
#define MICROSOFT

#ifdef  MICROSOFT
#define FAST	        register
#define CONSTANT        const
#ifndef _WIN32
#define CDECL           cdecl
#define DDECL
#endif

#define GLOBAL	extern	/* reference to function/data defined/used elsewhere */
#define IMPORT	extern	/* reference to function/data used here defined elsewhere */
#define EXPORT	      	/* reference to function/data defined here used elsewhere */
#define PRIVATE static	/* reference to function/data defined/used here only */

#define FAR	far
#define NEAR	near
#endif

#ifdef  LATTICE
#define FAST
#define CONSTANT
#define CDECL
#define DDECL

#define GLOBAL	extern	/* reference to function/data defined/used elsewhere */
#define IMPORT	extern	/* reference to function/data used here defined elsewhere */
#define EXPORT	      	/* reference to function/data defined here used elsewhere */
#define PRIVATE static	/* reference to function/data defined/used here only */

#define FAR
#define NEAR
#endif

	/***** 		Generic data types		*****/

#if !defined(APIENTRY)
typedef int		INT;
typedef unsigned int	UINT;
#endif

	/***** 		64 bit data types		*****/

typedef double                  DOUBLE;

	/***** 		32 bit data types		*****/

typedef float                   FLOAT;
typedef	unsigned long int       LONGWORD;

#ifndef _WINDEF_
typedef	unsigned long    	UINT32;
#endif
#define UINT32_MAX	0xffffffffL
#define UINT32_MIN	0L

#ifndef _WINDEF_
typedef	signed   long int	INT32;
#endif
#define INT32_MAX	2147483647
#define INT32_MIN	-2147483647

#ifndef	_TIME_T_DEFINED
typedef long time_t;			/* time value */
#define	_TIME_T_DEFINED			/* avoid multiple def's of time_t */
#endif

	/***** 		16 bit data types		*****/

typedef int                     LOOP;   /* fastest general loop variable */
typedef unsigned short          WORD;

typedef	signed   short int	INT16;
#define INT16_MAX	32767
#define INT16_MIN	-32767

typedef	unsigned short int	UINT16;
#define UINT16_MAX	0xFFFF
#define UINT16_MIN	0

#ifndef	_SIZE_T_DEFINED
typedef unsigned int size_t;		/* size value */
#define SIZE_T_MAX	0xFFFF
#define SIZE_T_MIN	0
#define	_SIZE_T_DEFINED			/* avoid multiple def's of size_t */
#endif

	/***** 		 8 bit data types		*****/

#if !defined(APIENTRY)
typedef signed	 char           BYTE;
#endif
#define BYTE_MAX	127
#define BYTE_MIN	-127

typedef	signed   char	        INT8;
#define INT8_MAX	127
#define INT8_MIN	-127

#if !defined(APIENTRY)
typedef	signed   char	        CHAR;
#endif
//#define CHAR_MAX	127
//#define CHAR_MIN	-127

typedef	unsigned char	        UCHAR;
//#define UCHAR_MAX	255
#define UCHAR_MIN	0

typedef	unsigned char	        UBYTE;
#define UBYTE_MAX	255
#define UBYTE_MIN	0

typedef	unsigned char	        UINT8;
#define UINT8_MAX	255
#define UINT8_MIN	0

	/***** 	machine dependent length data types	*****/

typedef int                     SMALL;  /* smallest, fastest integer */
typedef short int               SLOOP;  /* smallest, fastest loop variable */

	/***** 		general data types		*****/

#if !defined(APIENTRY)
typedef int                     BOOL;   /* only tested for zero or non-zero */
#endif

#if 0
typedef enum {true=(-1), false=0} bool;
#endif


	/***** 		splitting hairs		*****/

typedef
    union ACCESS16
	{
	INT16  int16;
	UINT16 uint16;
	struct
            {
	    INT8 lobyte;
	    INT8 hibyte;
	    } int8; 
	struct
	    {
	    UINT8 lobyte;
	    UINT8 hibyte;
	    } uint8; 
	} ACCESS16;

typedef
    union ACCESS32
	{
	INT32  int32;
	UINT32 uint32;
	struct
	    {
	    INT16 loword;
	    INT16 hiword;
	    } int16;
	struct
	    {
	    UINT16 loword;
	    UINT16 hiword;
	    } uint16;
	struct
	    {
	    INT8 b4; 
	    INT8 b3; 
	    INT8 b2; 
	    INT8 b1; 
	    } int8;
	struct
	    {
	    UINT8 b4; 
	    UINT8 b3; 
	    UINT8 b2; 
	    UINT8 b1; 
	    } uint8;
	} ACCESS32;

typedef
    struct 
	{
	unsigned second2: 5;
	unsigned minute : 6;
	unsigned hour   : 5;
	unsigned day    : 5;
	unsigned month  : 4;
	unsigned year   : 7;
	} DOS_DT;

typedef
    struct
	{
	unsigned day    : 5;
	unsigned month  : 4;
	unsigned year   : 7;
	} DOS_DATE;

typedef
    struct
	{
	unsigned second2: 5;
	unsigned minute : 6;
	unsigned hour   : 5;
	} DOS_TIME;




/* ----------------------------------------------------------------------- */
/* Data structure for A-Z flags */

typedef
    struct A_Z_FLAGS
	{
	unsigned	a : 1;
	unsigned	b : 1;
	unsigned	c : 1;
	unsigned	d : 1;
	unsigned	e : 1;
	unsigned	f : 1;
	unsigned	g : 1;
	unsigned	h : 1;
	unsigned	i : 1;
	unsigned	j : 1;
	unsigned	k : 1;
	unsigned	l : 1;
	unsigned	m : 1;
	unsigned	n : 1;
	unsigned	o : 1;
	unsigned	p : 1;
	unsigned	q : 1;
	unsigned	r : 1;
	unsigned	s : 1;
	unsigned	t : 1;
	unsigned	u : 1;
	unsigned	v : 1;
	unsigned	w : 1;
	unsigned	x : 1;
	unsigned	y : 1;
	unsigned	z : 1;
	} A_Z_FLAGS;



#if !defined(TRUE)
#define FALSE   	0
#define TRUE    	(!FALSE)
#endif

#if !defined(NUL)
#define  NUL    	'\0'
#endif

#if !defined(NULL)
#define  NULL    	((void *)0)
#endif

#if !defined(FNULL)
#define  NNULL  	((void near *)0)
#define  FNULL  	((void far *)0L)
#endif

#if !defined(NO_FILE)
#define	NO_FILE	(-1)
#endif

#endif

/****************************************************/
/*****		end				*****/
/****************************************************/
