/* ----------------------------------------------------------------------- *\
|
|	Definitions of the IBM keys
|
|	These definitions are used with the KSI function getkey().
|	Note that the getkey() return variable must be 16 bit.
|
\* ----------------------------------------------------------------------- */

#define  HOME	0x4700		/* Cursor keys */
#define  UPARR	0x4800
#define  PGUP	0x4900
#define  LTARR	0x4B00
#define  RTARR	0x4D00
#define  END	0x4F00
#define  DNARR	0x5000
#define  PGDN	0x5100
#define  INSERT	0x5200
#define  DELETE	0x5300
#define  SHFTAB	0x0F00

#define  ALT1	0x7800		/* Alternate numeral keys */
#define  ALT2	0x7900
#define  ALT3	0x7A00
#define  ALT4	0x7B00
#define  ALT5	0x7C00
#define  ALT6	0x7D00
#define  ALT7	0x7E00
#define  ALT8	0x7F00
#define  ALT9	0x8000
#define  ALT0	0x8100
#define  ALTMIN	0x8200
#define  ALTEQ	0x8300

#define  ALTA	0x1E00		/* Alternate alphabetic keys */
#define  ALTB	0x3000
#define  ALTC	0x2E00
#define  ALTD	0x2000
#define  ALTE	0x1200
#define  ALTF	0x2100
#define  ALTG	0x2200
#define  ALTH	0x2300
#define  ALTI	0x1700
#define  ALTJ	0x2400
#define  ALTK	0x2500
#define  ALTL	0x2600
#define  ALTM	0x3200
#define  ALTN	0x3100
#define  ALTO	0x1800
#define  ALTP	0x1900
#define  ALTQ	0x1000
#define  ALTR	0x1300
#define  ALTS	0x1F00
#define  ALTT	0x1400
#define  ALTU	0x1600
#define  ALTV	0x2F00
#define  ALTW	0x1100
#define  ALTX	0x2D00
#define  ALTY	0x1500
#define  ALTZ	0x2C00

#define  F1	0x3B00		/* Base function keys */
#define  F2	0x3C00
#define  F3	0x3D00
#define  F4	0x3E00
#define  F5	0x3F00
#define  F6	0x4000
#define  F7	0x4100
#define  F8	0x4200
#define  F9	0x4300
#define  F10	0x4400

#define  SF1	0x5400		/* Shifted function keys */
#define  SF2	0x5500
#define  SF3	0x5600
#define  SF4	0x5700
#define  SF5	0x5800
#define  SF6	0x5900
#define  SF7	0x5A00
#define  SF8	0x5B00
#define  SF9	0x5C00
#define  SF10	0x5D00

#define  CF1	0x5E00		/* Control function keys */
#define  CF2	0x5F00
#define  CF3	0x6000
#define  CF4	0x6100
#define  CF5	0x6200
#define  CF6	0x6300
#define  CF7	0x6400
#define  CF8	0x6500
#define  CF9	0x6600
#define  CF10	0x6700

#define  AF1	0x6800		/* Alternate function keys */
#define  AF2	0x6900
#define  AF3	0x6A00
#define  AF4	0x6B00
#define  AF5	0x6C00
#define  AF6	0x6D00
#define  AF7	0x6E00
#define  AF8	0x6F00
#define  AF9	0x7000
#define  AF10	0x7100

#define  CBREAK	0x0000		/* Miscellaneous keys */
#define  CPRSCR	0x7200
#define  CSTAB	0x7300
#define  CTAB	0x7400
#define  CEND	0x7500
#define  CPGDN	0x7600
#define  CHOME	0x7700
#define  CPGUP	0x8400

#define  functkey(ch)	((ch & 0x00FF) == 0)

#if defined(__cplusplus)
extern "C" {
#endif

unsigned int	getkey (void);

#if defined(__cplusplus)
}
#endif

/* ----------------------------------------------------------------------- */
