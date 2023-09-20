/* ----------------------------------------------------------------------- *\
|
|	ptypes.h
|
|	Portable definitions for 8, 16 and 32 bit integers, for the
|	Intel 808X family processors, and the Microsoft compiler.
|
|	Portable byte access structures for 16 and 32 bit integers.
|	
|	Access to 32-bit numbers:
|	
|	  [U]INT32:	  MSB				    LSB
|		  	 -----------------------------------
|	  [u]int8 	|   b1   |   b2   |   b3   |   b4   |
|	          	 -----------------------------------
|	  [u]int16 	 \----hiword-----/ \----loword-----/
|	        
|		      	    0        1        2        3    MC68000
|		  	    3        2        1        0    VAX11, I8086
|
|	Access to 16-bit numbers:
|
|	  [U]INT16:  MSB             LSB
|		      ----------------- 
|	  [u]int8:   | hibyte | lobyte |
|		      -----------------
|
|		         0        1		MC68000
|		         1        0		VAX11, I8086
|
|	Symbols which must be predefined:
|
|		None
|
|	Symbols which are defined:
|
|		BYTE		Signed 8-bit byte.
|		UBYTE		Unsigned 8-bit byte.
|		INT8		Signed 8-bit byte.
|		UINT8		Unsigned 8-bit byte.
|
|		INT		Signed hardware integer.
|		UINT		Unsigned hardware integer.
|		INT16		Signed 16-bit integer.
|		UINT16		Unsigned 16-bit integer.
|
|		LONG		Signed 32-bit integer.
|		ULONG		Unsigned 32-bit integer.
|		INT32		Signed 32-bit integer.
|		UINT32		Unsigned 32-bit integer.
|
|	(union)	ACCESS32	Provides machine independent
|				access to 32-bit integers.
|
|	(union)	ACCESS16	Provides machine independent
|				access to 16-bit integers.
|
|	History:
|
|		20-Feb-89	Adapted by bwj
|		17-Aug-97	Adapted for NT
|		 9-Oct-01	UINT16 corrected for W32
|
\* ----------------------------------------------------------------------- */

#ifndef __PTYPES_H__
#define __PTYPES_H__

/* ----------------------------------------------------------------------- *\
|  8, 16, 32 bit simple types
\* ----------------------------------------------------------------------- */

	/* 8 bit items */

#ifndef  _WINDEF_
	typedef char			BYTE, *PBYTE;
#endif
	typedef unsigned char	UBYTE;

	typedef char			INT8;
	typedef unsigned char	UINT8;

	/* 16 bit items */

	typedef short			INT16;
	typedef unsigned short	UINT16;

	/* 32 bit items */

#ifndef  _WINDEF_
	typedef	int				INT;
	typedef long			LONG;
#endif
	typedef	unsigned int	UINT;
	typedef unsigned long	ULONG;
#ifndef _WINDEF_
	typedef long			INT32;
	typedef unsigned long	UINT32;
#endif

/* ----------------------------------------------------------------------- *\
|  16 bit access types
\* ----------------------------------------------------------------------- */

	typedef union {
		INT16 int16;
		UINT16 uint16;
		struct {
			BYTE lobyte;
			BYTE hibyte;
		} int8; 
		struct {
			UBYTE lobyte;
			UBYTE hibyte;
		} uint8; 
	} ACCESS16;

/* ----------------------------------------------------------------------- *\
|  32 bit access types
\* ----------------------------------------------------------------------- */

	typedef union {
		INT32 int32;
		UINT32 uint32;
		struct {
			INT16 loword;
			INT16 hiword;
		} int16;
		struct {
			UINT16 loword;
			UINT16 hiword;
		} uint16;
		struct {
			BYTE b4; 
			BYTE b3; 
			BYTE b2; 
			BYTE b1; 
		} int8;
		struct {
			UBYTE b4; 
			UBYTE b3; 
			UBYTE b2; 
			UBYTE b1; 
		} uint8;
	} ACCESS32;

/* ----------------------------------------------------------------------- *\
|  Utility definitions
\* ----------------------------------------------------------------------- */

#ifndef  TRUE			/* Boolean constants */
#define  TRUE		  1
#define  FALSE		  0
#endif

#ifndef  MATCH			/* strcmp(), strncmp() match result */
#define  MATCH		  0
#endif

#ifdef  LATTICE			/* NULL pointer value */
#ifndef  NULL
#if (defined(M_I86SM) || defined(M_I86MM))
#define  NULL		  0
#else
#define  NULL		  0L
#endif
#endif
#endif

#ifndef  REGISTER
#define  REGISTER		/* No register support */
#endif

#endif // __PTYPES_H__

/* ----------------------------------------------------------------------- */
