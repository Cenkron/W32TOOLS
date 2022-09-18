/* ----------------------------------------------------------------------- *\
|
|							 FWINIT - FWILD - FWFREE
|						   Wild Card File Name Server
|
|			Copyright (c) 1985, 1990, 1991, 1993, all rights reserved
|								Brian W Johnson
|									8-Jun-90
|								   14-May-91
|								   21-Feb-93
|								   17-Aug-97  Win32
|								   24-Feb-98  Memory leak fixed
|									1-Aug-06  Accept ".*" file and directory names
|
|			void *				Return a pointer to an fwild header
|		hp = fwinit (s, mode);	Initialize the wild filename system
|			char  *s;			Drive/path/filename string
|			int	   mode;		Search mode to use (FW_* from fwild.h)
|
|			char *				Return a drive/path/filename string
|		fnp = fwild (hp);		Return the next filename
|			void  *hp;			Pointer to the fwild header
|
|			void *				Always returns NULL
|		fwfree (hp);			Close and free the fwild system instance
|			void  *hp;			Pointer to the fwild header
|
|		fwfree() need only be called if aborting the fwild sequence early
|
\* ----------------------------------------------------------------------- */

#ifdef _WIN32
#include  <windows.h>
#include  <VersionHelpers.h>
#endif
#include  <stdio.h>
#include  <ctype.h>
#include  <string.h>
#include  <stdlib.h>

#define	 FWILD_INTERNAL

#include  "fwild.h"

// #define	VERBOSEOUT	 1	  // Define this in the makefile for verbose output
// #define	MEMORYWATCH	 1	  // Define this in the makefile to watch for memory leaks

#ifdef	MEMORYWATCH
#define	 mwprintf(a,b)	printf(a,b)
#else
#define	 mwprintf(a,b)
#endif

//define  SHOWSRCH

#ifdef	VERBOSEOUT
static void		m_disp (char *s1, char *s2);
static void		h_disp (DTA_HDR *p, char *s);
static void		e_disp (DTA_ELE *p, char *s, int flag);
#endif

#define	 FRESH		0					/* Initial state of DTA_ELE */
#define	 NONW_FX	1					/* Do findf for not wild (.* added) */
#define	 NONW_F		2					/* Do findf for not wild */
#define	 NONW_N		3					/* Do findn for not wild */
#define	 NONW_T		4					/* Non-wild transition state */
#define	 WILD_F		5					/* Do findf for ordinary wild */
#define	 WILD_N		6					/* Do findn for ordinary wild */
#define	 RECW_F		7					/* Do findf for recursive wild */
#define	 RECW_N		8					/* Do findn for recursive wild */
#define	 RECW_T		9					/* Recursive wild transition state */

#define	 NOT_WILD	0					/* Not a wild expression */
#define	 ORD_WILD	1					/* Ordinary wild expression */
#define	 REC_WILD	2					/* Recursive wild expression */

#define	 FW_ALL		(FW_HIDDEN | FW_SYSTEM | FW_SUBD)
#define	 FW_FLS		(FW_HIDDEN | FW_SYSTEM | FW_FILE)

static  char	 rwild_str [] = "/**";
static  char	 owild_str [] = "/*.*";

// -----------------------------------------------------------------------
// Private methods
// -----------------------------------------------------------------------

static	void	 build_fn (DTA_ELE *);
static	void	 copyn (char *, char *, int);
static  DTA_HDR *new_header (void);
static	DTA_ELE *new_element (void);
static	char	*new_object (int);
static	char	*dispose_object (char *);
static	void	 release_header (DTA_HDR *);
static	DTA_ELE *unnest_element (DTA_ELE *);
static	void	 fat_err (int);
static 	void     CheckVersion (void);

#ifdef MEMORYWATCH
static	unsigned int	AllocCount = 0;
#endif

// -----------------------------------------------------------------------
// Private variables
// -----------------------------------------------------------------------

int			    xporlater = 0;					// TRUE if Windows XP or later (global)

// Default file exclusion management

/* ----------------------------------------------------------------------- */

// Force the inclusion of the corrected DTOXTIME library file
// Removed because no longer in use
//extern int	ForceDtoxtime;
//static int *pForceDtoxtime = &ForceDtoxtime;


/* ----------------------------------------------------------------------- *\
|  fwinit () - Initialize the fwild system for a wild search
\* ----------------------------------------------------------------------- */
	DTA_HDR *					/* Return a pointer to a DTA header */
fwinit (						/* Initialize the wild filename system */
	char *pattern,				/* Drive/path/filename search string */
	int fmode)					/* Find search mode to use */

	{
	DTA_HDR	 *hp;
	DTA_ELE	 *ep;
	char	  ch;
	char	 *p;

		// Allocate a DTA header, and allocate the first DTA element for
		// the DTA list.  Initialize the first element with the supplied
		// prototype file name pattern.

	if (fwvalid(pattern) != FWERR_NONE)
		return	(NULL);

	CheckVersion();
	owild_str[0] = rwild_str[0] = strpath(pattern);
	hp = new_header();
	hp->xmode = 0;					// File exclusion mode, updated by fexclude
	hp->mode  = fmode;				// Find mode to use
	hp->link  = ep = new_element();
	strcpy(ep->proto, pattern);
	fnreduce(ep->proto);

		// Fix up the file name pattern string if it is not complete.
		// If the file name ends with ':', '/', or '\', then it has an
		// implied "*.*" file name, so append "*.*" to the pattern file
		// name string.	 If the string is empty, fnreduce() has stripped
		// off the ".", so convert it to "*.*".	 If the string is "..",
		// append "\*.*" to it.	 In every case, space for this was
		// allotted when the prototype string was allocated.

	p = ep->proto;
	if (*p)
		{
		ch = p[strlen(p) - 1];			/* Guaranteed non-null here */
		if (ch == '/' || ch == ':' || ch == '\\')
			strcat(p, "*.*");
		else if (strcmp(fntail(p), "..") == MATCH)
			strcat(p, "\\*.*");
		}
	else
		strcpy(p, "*.*");

	fexcludeInit(&(hp->xmode));	// Init the file exclusion system, effective once only

#ifdef	VERBOSEOUT
h_disp(hp, "FWINIT");
#endif

	return (hp);
	}

/* ----------------------------------------------------------------------- *\
|  fwfree () - Close and free the fwild system instance
\* ----------------------------------------------------------------------- */
	DTA_HDR *					/* Always returns NULL */
fwfree (						/* Close and free the fwild system instance */
	DTA_HDR	 *hp)				/* Pointer to the DTA header */

	{
	fexcludeClean();			// Terminate the fexclude mechanism
	release_header(hp);
mwprintf("Allocs (done) %d\n", AllocCount);
	return (NULL);
	}

/* ----------------------------------------------------------------------- */

/* ----------------------------------------------------------------------- *\
|  fwExclEnable () - Enable the file exclusion mechanism for the fwild object
\* ----------------------------------------------------------------------- */
	void
fwExclEnable (			// Enable/disable file exclusion
	DTA_HDR	 *hp,		// Pointer to the DTA header
	int       enable)	// TRUE to enable exclusion

	{
	if (hp)
		hp->exActive = enable;
	}

/* ----------------------------------------------------------------------- *\
|  fwild () - Request the next (first) matching pathname
\* ----------------------------------------------------------------------- */
	char *						/* Return a drive/path/filename string */
fwild (							/* Find the next filename */
	DTA_HDR	 *hp)				/* Pointer to the DTA header */

	{
	DTA_ELE	 *ep;
	DTA_ELE	 *xep;
	int		  findFail ;
	int		  chflag;
	int		  m_mode;
	char	 *p;
	char	 *ps;

	if (hp == NULL)
		return (NULL);

	if ((ep = hp->link) == NULL)
		fat_err(2);

	for (;;)
		{

#ifdef	VERBOSEOUT
e_disp(ep, "Main switch", FALSE);
#endif

		switch (ep->state)
			{

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

			case FRESH:			/* Freshly initialized DTA element */
				p = ep->proto;
				chflag = FALSE;
				do	{
					if (*p == '?')
						{
						if (ep->wild != REC_WILD)
							ep->wild = ORD_WILD;
						}
					else if (*p == '*')
						{
						if ((chflag == FALSE)  &&  (*(p + 1) == '*'))
							{
							++p;
							ep->wild = REC_WILD;
							}
						else
							ep->wild = ORD_WILD;
						}
					else if ((*p == '/') || (*p == ':') || (*p == '\\'))
						{
						chflag = FALSE;
						if (ep->wild)
							break;
						ep->pLast = p;	// Point the last replaced proto separator
						}
					else
						chflag = TRUE;
					} while (*(++p));	// Point end of active proto string
				ep->pNext = p;			// Point end of the proto string

				if (ep->wild)
					{
					int  n;			// Copy length

					if (ep->wild == REC_WILD)	/* Make match pattern */
						ep->state = RECW_F;
					else
						{
						ep->state = WILD_F;
						if (ep->pLast)
							{
							n = (p - (ep->pLast + 1));
							copyn(ep->pattern, (ep->pLast + 1), n);
							}
						else
							{
							n = (p - (ep->proto));
							copyn(ep->pattern, ep->proto, n);
							}
						}
					if (ep->pLast)				/* Make search pattern */
						{
						n = ((ep->pLast + 1) - (ep->proto));
						copyn(ep->search, ep->proto, n);
						strcat(ep->search, "*.*");
						}
					else
						strcpy(ep->search, "*.*");
					}
				else			/* Not wild, make only a search string */
					{
					int  n;		// Copy length

					ep->state = NONW_F;
					ps = ep->search;
					n = (p - ep->proto);
					copyn(ps, ep->proto, n);
// printf("proto: %s\n", ep->proto	? ep->proto	 : "null" );
// printf("last:  %s\n", ep->pLast	? ep->pLast	 : "null" );
// printf("next:  %s\n", ep->pNext	? ep->pNext	 : "null" );
// printf("srch:  %s\n", ep->search ? ep->search : "null" );
					if (hp->mode & FW_DSTAR)	/* Check for implied ".*" */
						{
						p = (ep->pLast) ? (ep->pLast) : (ep->proto);
						if ((strchr(p, '.') == NULL)  &&  ( ! _fnchkdir(ep->proto)))
							{
							if (fnchkunc(ps))	// UNC fixup
								{
								ep->dta.sh = INVALID_HANDLE_VALUE;
								strcpy(ep->found, ep->proto);
								ep->state = NONW_T;
								}
							else
								{
								strcat(ps, ".*");
								ep->state = NONW_FX;
								}
							}
						}

					else if (fnchkunc(ps))	 // UNC fixup
						{
#ifdef SHOWSRCH
printf("srch:  %s\n", ep->search ? ep->search : "null" );
#endif
						if (_findf(&ep->dta, ps, (hp->mode & FW_ALL) | FW_SUBD) != 0)
							{
							strcat(ps, "\\*.*");
							ep->state = NONW_FX;
							ep->pLast  = ep->proto + strlen(ep->proto);
							}
						}
					}
				break;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

			case RECW_F:						/* Recursive wild case */
			case RECW_N:

#ifdef	VERBOSEOUT
e_disp(ep, "RECW", FALSE);
#endif
				if (ep->state == RECW_F)		/* Search for file */
					{
#ifdef SHOWSRCH
printf("srch:  %s\n", ep->search ? ep->search : "null" );
#endif
					findFail  = _findf(&ep->dta, ep->search, (hp->mode & FW_ALL) | FW_SUBD);
					ep->state = RECW_N;
					}
				else
					findFail  = _findn(&ep->dta);

				if (findFail )						/* If no file found... */
					{
					if ((hp->link = ep = unnest_element(ep)) == NULL)
						{
						fwfree(hp);				/* Terminate the object */
						return (NULL);			/* Return failure */
						}
					break;						/* Continue unnested */
					}


#ifdef	VERBOSEOUT
e_disp(ep, "RECW", TRUE);
#endif

				if (ep->dta.dta_type & ATT_SUBD)		/* Check if SUBD */
					{
					if ((strcmp(ep->dta.dta_name,  ".") == 0)
					||	(strcmp(ep->dta.dta_name, "..") == 0))
						break;

					if (*ep->pNext)			/* Process non-terminal subd */
						{
						if (fnmatch(ep->pNext + 1, ep->dta.dta_name, hp->mode & FW_DSTAR))
							{
							if (hp->exActive && (hp->xmode & XD_BYNAME) && fexcludeCheck(ep->dta.dta_name))
								break;
							build_fn(ep);
							if (hp->exActive && (hp->xmode & XD_BYPATH) && fexcludeCheck(ep->found))
								break;

							ep->state = RECW_T;
							if (hp->mode & FW_SUBD)
								{
								ep->fdt = fgetfdt(ep->found);
								return (ep->found);
								}
							}
						}
					else // (not *ep->pNext)					/* Process terminal subd */
						{
						if (hp->exActive && (hp->xmode & XD_BYNAME) && fexcludeCheck(ep->dta.dta_name))
							break;
						build_fn(ep);		/* Terminal match case */
						if (hp->exActive && (hp->xmode & XD_BYPATH) && fexcludeCheck(ep->found))
							break;

						ep->state = RECW_T;
						if (hp->mode & FW_SUBD)
							{
							ep->fdt = fgetfdt(ep->found);
							return (ep->found);
							}
						}
					ep->state = RECW_T;
					break;
					}

				else if (hp->mode & FW_FLS)		/* Process file case */
					{
					if (*ep->pNext)				/* Process non-terminal file */
						{
						if (fnmatch(ep->pNext + 1, ep->dta.dta_name, hp->mode & FW_DSTAR))
							{
							if (hp->exActive && (hp->xmode & XF_BYNAME) && fexcludeCheck(ep->dta.dta_name))
								break;
							build_fn(ep);		/* Terminal match case */
							if (hp->exActive && (hp->xmode & XF_BYPATH) && fexcludeCheck(ep->found))
								break;

							ep->fdt = fgetfdt(ep->found);
							return (ep->found);
							}
						}
					else // (not *ep->pNext)		/* Process terminal file */
						{
						if (hp->exActive && (hp->xmode & XF_BYNAME) && fexcludeCheck(ep->dta.dta_name))
							break;
						build_fn(ep);		/* Terminal match case */
						if (hp->exActive && (hp->xmode & XF_BYPATH) && fexcludeCheck(ep->found))
							break;

						ep->fdt = fgetfdt(ep->found);
						return (ep->found);
						}
					}
				break;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

			case RECW_T:

				ep->state = RECW_N;
//				if (ep->dta.dta_name[0] != '.')
				if ((strcmp(ep->dta.dta_name,  ".") != 0)
				&&	(strcmp(ep->dta.dta_name, "..") != 0))
					{			/* Wild "." and ".." don't nest */
					build_fn(ep);
					xep = ep;			/* Nest the DTA element list */
					hp->link = ep = new_element();
					ep->link = xep;
					strcpy(ep->proto, xep->found);
					strcat(ep->proto, rwild_str);
					strcat(ep->proto, xep->pNext);
					}
				break;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


			case WILD_F:						/* Ordinary wild case */
			case WILD_N:

#ifdef	VERBOSEOUT
e_disp(ep, "WILD", FALSE);
#endif
				if (ep->state == WILD_F)		/* Search for file */
					{
#ifdef SHOWSRCH
printf("srch:  %s\n", ep->search ? ep->search : "null" );
#endif
					findFail  = _findf(&ep->dta, ep->search, (hp->mode & FW_ALL) | FW_SUBD);
					ep->state = WILD_N;
					}
				else
					findFail  = _findn(&ep->dta);

				if (findFail )						/* If no file found... */
					{
					if ((hp->link = ep = unnest_element(ep)) == NULL)
						{
						fwfree(hp);				/* Terminate the object */
						return (NULL);			/* Return failure */
						}
					break;						/* Continue unnested */
					}

#ifdef	VERBOSEOUT
e_disp(ep, "WILD", TRUE);
#endif
				if (*ep->pNext)			/* If terminal name, set match mode */
					m_mode = 0;
				else // (not *ep->pNext)
					m_mode = hp->mode & FW_DSTAR;

				if (!fnmatch(ep->pattern, ep->dta.dta_name, m_mode))
					break;

				if (ep->dta.dta_type & ATT_SUBD)		/* Check if SUBD */
					{
					if (*ep->pNext)					/* Yes, process non-terminal subd */
						{
						if ((strcmp(ep->dta.dta_name,  ".") == 0)	// No
						||	(strcmp(ep->dta.dta_name, "..") == 0))
							break;		/* Wild "." and ".." don't nest */

						if (hp->exActive && (hp->xmode & XD_BYNAME) && fexcludeCheck(ep->dta.dta_name))
							break;
						build_fn(ep);
						if (hp->exActive && (hp->xmode & XD_BYPATH) && fexcludeCheck(ep->found))
							break;
						xep = ep;				/* Nest the DTA element list */
						hp->link = ep = new_element();
						ep->link = xep;
						strcpy(ep->proto, xep->found);
						strcat(ep->proto, xep->pNext);
						}
					else // (not *ep->pNext)		/* Process terminal subd */
						{
						if (hp->exActive && (hp->xmode & XD_BYNAME) && fexcludeCheck(ep->dta.dta_name))
							break;
						build_fn(ep);
						if (hp->exActive && (hp->xmode & XD_BYPATH) && fexcludeCheck(ep->found))
							break;

						if (hp->mode & FW_SUBD)	/* If reading subd's... */
							{
							ep->fdt = fgetfdt(ep->found);
							return (ep->found);
							}
						}
					}

				else								/* Process file */
					{
					if ((*ep->pNext == '\0')  &&  (hp->mode & FW_FLS))
						{
						if (hp->exActive && (hp->xmode & XF_BYNAME) && fexcludeCheck(ep->dta.dta_name))
							break;
						build_fn(ep);
						if (hp->exActive && (hp->xmode & XF_BYPATH) && fexcludeCheck(ep->found))
							break;

						ep->fdt = fgetfdt(ep->found);
						return (ep->found);
						}
					}
				break;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

			case NONW_FX:						/* Non wild search */
			case NONW_F:
			case NONW_N:

#ifdef	VERBOSEOUT
e_disp(ep, "NONW", FALSE);
#endif
				if (ep->state == NONW_FX)		/* Search for file */
					{
// printf("\nNONW: findf on \"%s\" (1)\n", ep->search);
#ifdef SHOWSRCH
printf("srch:  %s\n", ep->search ? ep->search : "null" );
#endif
					findFail  = _findf(&ep->dta, ep->search, (hp->mode & FW_ALL) | FW_SUBD);
					if (findFail )
						{
						*(ep->search + (strlen(ep->search) - 2)) = '\0';
						ep->state = NONW_F;
						}
					}

				if (ep->state == NONW_F)
					{
// printf("\nNONW: findf on \"%s\" (2)\n", ep->search);
#ifdef SHOWSRCH
printf("srch:  %s\n", ep->search ? ep->search : "null" );
#endif
					findFail  = _findf(&ep->dta, ep->search, (hp->mode & FW_ALL) | FW_SUBD);
					}

				if (ep->state == NONW_N)
					{
// printf("\nNONW: findn on \"%s\" (3)\n", ep->search);
					findFail  = _findn(&ep->dta);
					}

				ep->state = NONW_N;

				if (findFail )						/* If no file found... */
					{
					if ((hp->link = ep = unnest_element(ep)) == NULL)
						{
						fwfree(hp);				/* Terminate the object */
						return (NULL);			/* Return failure */
						}
					break;						/* Continue unnested */
					}

#ifdef	VERBOSEOUT
e_disp(ep, "NONW", TRUE);
#endif

				if (ep->dta.dta_type & ATT_SUBD)
					{
					if (hp->exActive && (hp->xmode & XD_BYNAME) && fexcludeCheck(ep->dta.dta_name))
						break;
					build_fn(ep);
					if (hp->exActive && (hp->xmode & XD_BYPATH) && fexcludeCheck(ep->found))
						break;

					ep->state = NONW_T;
					if (hp->mode & FW_SUBD)
						{
						ep->fdt = fgetfdt(ep->found);
						return (ep->found);	/* Return the directory */
						}
					break;						// Return to top of loop
					}
				else if (hp->mode & FW_FLS)		// And is a file (because not a directory)
					{
					if (hp->exActive && (hp->xmode & XF_BYNAME) && fexcludeCheck(ep->dta.dta_name))
						break;
					build_fn(ep);
					if (hp->exActive && (hp->xmode & XF_BYPATH) && fexcludeCheck(ep->found))
						break;

					ep->fdt = fgetfdt(ep->found);
					return (ep->found);		/* Return the filename */
					}
				break;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

			case NONW_T:

// printf("\nNONW_T: entered, mode = %04X, DTA.type = %04X\n", hp->mode, ep->dta.dta_type);
				ep->state = NONW_N;
				if (hp->mode & (FW_FILE | FW_ALL))
					{
// printf("\nNONW_T: nesting\n");
					xep = ep;			/* Nest the DTA element list */
					hp->link = ep = new_element();
					ep->link = xep;
					strcpy(ep->proto, xep->found);
					strcat(ep->proto, owild_str);
					}
				break;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

			default:
				fat_err(3);
			}					// End of the state switch table
		}						// Return to top of the state loop

//	fwfree(hp);					// Terminate the object (if it were a real exit)
	return	(NULL);				// Dummy to make the compiler happy, never taken
	}

/* ----------------------------------------------------------------------- */

/* ----------------------------------------------------------------------- *\
|  build_fn () - Build the found pathname from the DTA
\* ----------------------------------------------------------------------- */
	static void
build_fn (ep)					/* Build the found filename in the DTA */
	DTA_ELE	 *ep;

	{
	int	  n;
	char  ch;

	if (ep->pLast)
		{
		n = ep->pLast - ep->proto + 1;
		copyn(ep->found, ep->proto, n);
		if (strlen(ep->found) > 0)
			{
			ch = ep->found[strlen(ep->found) - 1];
			if ((ch != ':')	&&	(ch != '/')	&&	(ch != '\\'))
				strcat(ep->found, "\\");
			}
		strcat(ep->found, ep->dta.dta_name);
		}
	else
		{
		strcpy(ep->found, ep->dta.dta_name);
		}
	}

/* ----------------------------------------------------------------------- */
	static void
copyn (p2, p1, n)				/* Copy n bytes from p1 to p2 */
	char  *p2;					/* and NULL terminate the string */
	char  *p1;
	int	   n;

	{
	while (n--)
		*(p2++) = *(p1++);
	*p2 = '\0';
	}

/* ----------------------------------------------------------------------- */
	static DTA_HDR *			/* Return a pointer to the new DTA header */
new_header ()

	{
	DTA_HDR *hp = (DTA_HDR *)(new_object(sizeof(DTA_HDR)));
mwprintf("New header %d\n", AllocCount);
	return (hp);
	}

/* ----------------------------------------------------------------------- */
	static DTA_ELE *			/* Return a pointer to the new DTA element */
new_element ()					/* Allocate an initialized DTA element */

	{
	DTA_ELE	 *ep;

	ep = (DTA_ELE *)(new_object(sizeof(DTA_ELE)));
	ep->link	   = NULL;
	ep->wild	   = NOT_WILD;
	ep->state	   = FRESH;
	ep->pLast	   = NULL;
	ep->pNext	   = NULL;
	ep->search[0]  = '\0';
	ep->pattern[0] = '\0';
	ep->proto[0]   = '\0';
	ep->found[0]   = '\0';
mwprintf("New element %d\n", AllocCount);
	return (ep);
	}

/* ----------------------------------------------------------------------- */
	static char *				/* Return a pointer to the new block */
new_object (size)				/* Allocate a block of memory */
	int	 size;					/* Size of memory to allocate */

	{
	char  *p;

	if ((p = calloc(size, 1)) == NULL)
		fat_err(1);
#ifdef MEMORYWATCH
	else
		++AllocCount;
#endif
	return (p);
	}

/* ----------------------------------------------------------------------- */
	static char *				/* Always returns NULL */
dispose_object (				/* Free a block of object memory */
	char  *s)					/* Pointer to object to free */

	{
	if (s != NULL)
		{
		free(s);
#ifdef MEMORYWATCH
		--AllocCount;
#endif
		}
	return (NULL);
	}

/* ----------------------------------------------------------------------- */
	static void
release_header (hp)				/* Release a header and all its elements */
	DTA_HDR	 *hp;				/* Pointer to the header */

	{
	while (hp->link != NULL)					/* Release all DTA elements */
		hp->link = unnest_element(hp->link);

	dispose_object((char *)(hp));						/* Then, release the header */
mwprintf("Free header %d\n", AllocCount);
	}

/* ----------------------------------------------------------------------- */
	static DTA_ELE *			/* Returned pointer to the successor element */
unnest_element (				/* Unnest a DTA element */
	DTA_ELE	 *ep)				/* Pointer to the element */

	{
	DTA_ELE	 *eplink;			/* Pointer to the next element */

	if (ep != NULL)
		{
		eplink = ep->link;						/* Point the successor */
		dispose_object((char *)(ep));					/* Then, release the element */
mwprintf("Free element %d\n", AllocCount);
		}
	else
		eplink = NULL;							/* There was no element */

	return (eplink);
	}

/* ----------------------------------------------------------------------- */
	static void
fat_err (n)						/* Report fatal internal error and die */
	int	 n;						/* Error number */

	{
	char  *s;

	switch (n)
		{
		case 1:
			s = "Insufficient memory";	 break;

		case 2:
			s = "Element ptr error";	  break;

		case 3:
			s = "Invalid state"; break;

		default:
			s = "Invalid error code";
		}
	fprintf(stderr, "Fwild-F-%s\n\7", s);
	exit(1);
	}

/* ----------------------------------------------------------------------- */
	void
CheckVersion (void)

	{
	xporlater = IsWindowsXPOrGreater();
    }

/* ----------------------------------------------------------------------- */
#ifdef	VERBOSEOUT
	static void
m_disp(s1, s2)
	char  *s1;
	char  *s2;

	{
	printf("\n");
	printf("Match strings:\n");

	if (s1)
		printf("String1..%s\n", s1);
	else
		printf("String1 is NULL\n");

	if (s2)
		printf("String2..%s\n", s2);
	else
		printf("String2 is NULL\n");

	printf("\n");
	fflush(stdout);
	}

#endif
/* ----------------------------------------------------------------------- */
#ifdef	VERBOSEOUT
	static void
h_disp(hp, s)
	DTA_HDR	 *hp;
	char	 *s;

	{
	printf("\n");
	printf("From: %s\n", s);
	printf("Pointer....%04x\n", (int)(hp));
	printf("Link.......%04x\n", (int)(hp->link));
	printf("Mode.......%04x\n", (int)(hp->mode));
	printf("\n");
	fflush(stdout);
	}

#endif
/* ----------------------------------------------------------------------- */
#ifdef	VERBOSEOUT
	static void
e_disp(ep, s, flag)
	DTA_ELE	 *ep;				/* Pointer to the DTA element */
	char	 *s;				/* Originination string */
	int		  flag;				/* True for a search result */

	{
	printf("\n");

	if (flag)
		printf("Result from: %s\n", s);
	else
		printf("Entry to: %s\n", s);

	printf("Pointer____%04x\n", (int)(ep));

	printf("Link_______%04x\n", (int)(ep->link));

	switch (ep->wild)
		{
		case NOT_WILD:	printf("Wild_______NOT_WILD\n");  break;
		case ORD_WILD:	printf("Wild_______ORD_WILD\n");  break;
		case REC_WILD:	printf("Wild_______REC_WILD\n");  break;
		default:		printf("Wild_______BAD\n");
		}

	switch (ep->state)
		{
		case FRESH:		printf("State______FRESH\n");	break;
		case NONW_FX:	printf("State______NONW_FX\n"); break;
		case NONW_F:	printf("State______NONW_F\n");	break;
		case NONW_N:	printf("State______NONW_N\n");	break;
		case NONW_T:	printf("State______NONW_T\n");	break;
		case WILD_F:	printf("State______WILD_F\n");	break;
		case WILD_N:	printf("State______WILD_N\n");	break;
		case RECW_F:	printf("State______RECW_F\n");	break;
		case RECW_N:	printf("State______RECW_N\n");	break;
		case RECW_T:	printf("State______RECW_T\n");	break;
		default:		printf("State______BAD\n");
		}

	if (ep->proto[0])
		printf("Prototype__%s\n",	ep->proto);

	if (ep->pLast)
		printf("Last_______%3d\n",	ep->pLast - ep->proto);
	else
		printf("Last_______  0\n");

	if (ep->pNext)
		printf("Next_______%3d\n",	ep->pNext - ep->proto);
	else
		printf("Next_______  0\n");

	if (ep->search)
		printf("Search_____%s\n",	ep->search);

	if (ep->pattern)
		printf("Pattern____%s\n",	ep->pattern);

	if (ep->found[0])
		printf("Found______%s\n",	ep->found);

	if (flag)
		{
		printf("DTA.name___%s\n",	ep->dta.dta_name);
		printf("DTA.type___%04X\n", ep->dta.dta_type);
		printf("DTA.size___%u\n",	(int)(ep->dta.dta_size));
		}

	printf("\n");
	fflush(stdout);
	}

#endif
/* ----------------------------------------------------------------------- */
