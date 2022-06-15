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
#define	 mwfree(a)		dispose(a)
#define	 mwprintf(a,b)	printf(a,b)
#else
#define	 mwfree(a)		free(a)
#define	 mwprintf(a,b)
#endif

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

static char	 rwild_str [] = "/**";
static char	 owild_str [] = "/*.*";

static	void	 build_fn (DTA_ELE *);
static	void	 copyn (char *, char *, int);
static	DTA_ELE *new_element (void);
static	char	*new_string (int);
static	char	*dispose (char *);
static	void	 release_header (DTA_HDR *);
static	DTA_ELE *unnest_element (DTA_ELE *);
static	void	 fat_err (int);
static 	void     CheckVersion (void);

#ifdef MEMORYWATCH
static	unsigned int	AllocCount = 0;
#endif

int	    xporlater = 0;					// TRUE if Windows XP or later (global)

// Default file exclusion management

static	int		 FexcludeDefaultEnable	= TRUE;	

/* ----------------------------------------------------------------------- */

// bwj Force the inclusion of the corrected DTOXTIME library file
// Removed because no longer in use
//extern int	ForceDtoxtime;
//static int *pForceDtoxtime = &ForceDtoxtime;
 

/* ----------------------------------------------------------------------- *\
|  excludeDefFiles () - Exclude the system default files, if enabled
\* ----------------------------------------------------------------------- */
	void
fexcludeDefEnable (
	int  enable)

	{
	FexcludeDefaultEnable = enable;
	}

/* ----------------------------------------------------------------------- *\
|  fwinit () - Initialize the fwild system for a wild search
\* ----------------------------------------------------------------------- */
	DTA_HDR *					/* Return a pointer to a DTA header */
fwinit (s, xmode)				/* Initialize the wild filename system */
	char  *s;					/* Drive/path/filename string */
	int	   xmode;				/* Search mode to use */

	{
	DTA_HDR	 *hp;
	DTA_ELE	 *ep;
	char	  ch;
	char	 *p;

		// Allocate a DTA header, and allocate the first DTA element for
		// the DTA list.  Initialize the first element with the supplied
		// prototype file name pattern.

	if (fwvalid(s) != FWERR_NONE)
		return	(NULL);

	CheckVersion();
	owild_str[0] = rwild_str[0] = strpath(s);
mwprintf("New header\n", AllocCount);
	hp = (DTA_HDR *)(new_string(sizeof(DTA_HDR)));
	hp->mode = xmode;
	hp->link = ep = new_element();
mwprintf("New proto\n", AllocCount);
	ep->proto = new_string(strlen(s) + 5);		/* Allow for name fixup later */
	strcpy(ep->proto, s);
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

	if (FexcludeDefaultEnable)	// if enabled
		fexcludeDefault();		//   protect system special files/directories,

#ifdef	VERBOSEOUT
h_disp(hp, "FWINIT");
#endif

	return	(hp);
	}

/* ----------------------------------------------------------------------- *\
|  fwfree () - Close and free the fwild system instance
\* ----------------------------------------------------------------------- */
	DTA_HDR *					/* Always returns NULL */
fwfree (						/* Close and free the fwild system instance */
	DTA_HDR	 *hp)				/* Pointer to the DTA header */

	{
	release_header(hp);
	return (NULL);
	}

/* ----------------------------------------------------------------------- */

/* ----------------------------------------------------------------------- *\
|  fwild () - Request the next (first) matching pathname
\* ----------------------------------------------------------------------- */
	char *						/* Return a drive/path/filename string */
fwild (hp)						/* Find the next filename */
	DTA_HDR	 *hp;				/* Pointer to the DTA header */

	{
	DTA_ELE	 *ep;
	DTA_ELE	 *xep;
	int		  stat;
	int		  n;
	int		  chflag;
	int		  m_mode;
	char	 *p;
	char	 *ps;

	if (hp == NULL)
		return	(NULL);
	if ((ep = hp->link) == NULL)
		fat_err(2);

	for (;;)
		{

#ifdef	VERBOSEOUT
e_disp(ep, "Main switch", FALSE);
#endif

		switch (ep->state)
			{

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
						ep->last = p;
						}
					else
						chflag = TRUE;
					} while (*(++p));

				ep->next = p;
				if (ep->wild)
					{
					if (ep->wild == REC_WILD)	/* Make match pattern */
						ep->state = RECW_F;
					else
						{
						ep->state = WILD_F;
						if (ep->last)
							{
mwprintf("New pattern\n", AllocCount);
							ep->pattern = new_string((n = p - (ep->last + 1)) + 1);
							copyn(ep->pattern, ep->last + 1, n);
							}
						else
							{
mwprintf("New pattern\n", AllocCount);
							ep->pattern = new_string((n = p - ep->proto) + 1);
							copyn(ep->pattern, ep->proto, n);
							}
						}
					if (ep->last)				/* Make search pattern */
						{
mwprintf("New search\n", AllocCount);
						ep->search = new_string((n = (ep->last + 1) - ep->proto) + 4);
						copyn(ep->search, ep->proto, n);
						strcat(ep->search, "*.*");
						}
					else
						{
mwprintf("New search\n", AllocCount);
						ep->search = new_string(4);
						strcpy(ep->search, "*.*");
						}
					}
				else			/* Not wild, make only a search string */
					{
					ep->state = NONW_F;
mwprintf("New search\n", AllocCount);
					ps = ep->search = new_string((n = p - ep->proto) + 5);
					copyn(ps, ep->proto, n);
// printf("proto: %s\n", ep->proto	? ep->proto	 : "null" );
// printf("last:  %s\n", ep->last	? ep->last	 : "null" );
// printf("next:  %s\n", ep->next	? ep->next	 : "null" );
// printf("srch:  %s\n", ep->search ? ep->search : "null" );
					if (hp->mode & FW_DSTAR)	/* Check for implied ".*" */
						{
						p = (ep->last) ? (ep->last) : (ep->proto);
// printf("xxxx 1 %s\n", p);
#ifdef _WIN32
						if ((strchr(p, '.') == NULL)
						&&	( ! _fnchkdir(ep->proto)))
							{
							if (fnchkunc(ps))	// UNC fixup
								{
								ep->dta.sh = INVALID_HANDLE_VALUE;
								ep->found = new_string(strlen(ep->proto) + 1);
								strcpy(ep->found, ep->proto);
								ep->state = NONW_T;
								}
							else
								{
								strcat(ps, ".*");
								ep->state = NONW_FX;
								}
							}
#else
						if ((strchr(p, '.') == NULL)
						&&	( ! fnchkdir(ep->proto)))
							{
							strcat(ps, ".*");
							ep->state = NONW_FX;
							}
#endif
						}
#ifdef _WIN32
					else if (fnchkunc(ps))	 // UNC fixup
						{
						if (_findf(&ep->dta, ps,
								(hp->mode & FW_ALL) | FW_SUBD) != 0)
							{
//							strcat(ep->proto, "\\");
							strcat(ps, "\\*.*");
							ep->state = NONW_FX;
							ep->last  = ep->proto + strlen(ep->proto);
							}
						}
#endif
					}
				break;


			case RECW_F:						/* Recursive wild case */
			case RECW_N:

#ifdef	VERBOSEOUT
e_disp(ep, "RECW", FALSE);
#endif

//xxx			ep->found = dispose(ep->found); /* Clean up the old filename */

				if (ep->state == RECW_F)		/* Search for file */
					{
					stat = _findf(&ep->dta, ep->search,
						(hp->mode & FW_ALL) | FW_SUBD);
					ep->state = RECW_N;
					}
				else
					stat = _findn(&ep->dta);

				if (stat)						/* If no file found... */
					{
					if ((hp->link = ep = unnest_element(ep)) == NULL)
						{
						release_header(hp);
mwprintf("Allocs (done) %d\n", AllocCount);
						return	(NULL);			/* Return failure */
						}
					break;						/* Continue unnested */
					}


#ifdef	VERBOSEOUT
e_disp(ep, "RECW", TRUE);
#endif

				if (ep->dta.dta_type & ATT_SUBD)		/* Check if SUBD */
					{
					ep->state = RECW_T;					/* Yes, and visible */
					if (hp->mode & FW_SUBD)
						{
						if (*ep->next)			/* Check if a terminal name */
							{
							if (fnmatch(ep->next + 1, ep->dta.dta_name,
								hp->mode & FW_DSTAR))
								{
								build_fn(ep);	/* Non-terminal match case */
mwprintf("Allocs %d\n", AllocCount);
								ep->fdt = fgetfdt(ep->found);
								return	(ep->found);
								}
							}
						else
							{
							build_fn(ep);		/* Terminal match case */
mwprintf("Allocs %d\n", AllocCount);
							ep->fdt = fgetfdt(ep->found);
							return	(ep->found);
							}
						}
					break;
					}
				else if (hp->mode & FW_FLS)		/* File case */
					{
					if (*ep->next)				/* Check if a terminal name */
						{
						if (fnmatch(ep->next + 1, ep->dta.dta_name,
							hp->mode & FW_DSTAR))
							{
							build_fn(ep);		/* Non-terminal match case */
mwprintf("Allocs %d\n", AllocCount);
							ep->fdt = fgetfdt(ep->found);
							return	(ep->found);
							}
						}
					else
						{
						build_fn(ep);			/* Terminal match case */
mwprintf("Allocs %d\n", AllocCount);
						ep->fdt = fgetfdt(ep->found);
						return	(ep->found);
						}
					}
				break;


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
mwprintf("New proto\n", AllocCount);
					ep->proto = new_string(strlen(xep->found)
						+ strlen(xep->next) + 4);
					strcpy(ep->proto, xep->found);
					strcat(ep->proto, rwild_str);
					strcat(ep->proto, xep->next);
					}
				break;


			case WILD_F:						/* Ordinary wild case */
			case WILD_N:

#ifdef	VERBOSEOUT
e_disp(ep, "WILD", FALSE);
#endif

//xxx			ep->found = dispose(ep->found); /* Clean up the old filename */

				if (ep->state == WILD_F)		/* Search for file */
					{
					stat = _findf(&ep->dta, ep->search,
						(hp->mode & FW_ALL) | FW_SUBD);
					ep->state = WILD_N;
					}
				else
					stat = _findn(&ep->dta);

				if (stat)						/* If no file found... */
					{
					if ((hp->link = ep = unnest_element(ep)) == NULL)
						{
						release_header(hp);
mwprintf("Allocs (done) %d\n", AllocCount);
						return	(NULL);			/* Return failure */
						}
					break;						/* Continue unnested */
					}

#ifdef	VERBOSEOUT
e_disp(ep, "WILD", TRUE);
#endif

				if (*ep->next)			/* Set match mode if terminal */
					m_mode = 0;
				else
					m_mode = hp->mode & FW_DSTAR;
				if (!fnmatch(ep->pattern, ep->dta.dta_name, m_mode))
					break;

				if (ep->dta.dta_type & ATT_SUBD)		/* Check if SUBD */
					{
					if (*ep->next)				/* Yes, check if a terminal name */
						{
//						if (ep->dta.dta_name[0] == '.')			/* No */
						if ((strcmp(ep->dta.dta_name,  ".") == 0)
						||	(strcmp(ep->dta.dta_name, "..") == 0))
							break;		/* Wild "." and ".." don't nest */
						build_fn(ep);
						xep = ep;				/* Nest the DTA element list */
						hp->link = ep = new_element();
						ep->link = xep;
mwprintf("New proto\n", AllocCount);
						ep->proto = new_string(strlen(xep->found)
							+ strlen(xep->next) + 1);
						strcpy(ep->proto, xep->found);
						strcat(ep->proto, xep->next);
						}
					else if (hp->mode & FW_SUBD)		/* Terminal case */
						{
						build_fn(ep);
mwprintf("Allocs %d\n", AllocCount);
						ep->fdt = fgetfdt(ep->found);
						return	(ep->found);
						}
					}
				else	/* Handle the terminal file name case */
					{
					if ((*ep->next == '\0')
					&& (hp->mode & FW_FLS))
						{
						build_fn(ep);
mwprintf("Allocs %d\n", AllocCount);
						ep->fdt = fgetfdt(ep->found);
						return	(ep->found);
						}
					}
				break;


			case NONW_FX:						/* Non wild search */
			case NONW_F:
			case NONW_N:

#ifdef	VERBOSEOUT
e_disp(ep, "NONW", FALSE);
#endif

//xxx			ep->found = dispose(ep->found); /* Clean up the old filename */

#if 1
				if (ep->state == NONW_FX)		/* Search for file */
					{
// printf("\nNONW: findf on \"%s\" (1)\n", ep->search);
					stat = _findf(&ep->dta, ep->search,
						(hp->mode & FW_ALL) | FW_SUBD);
					if (stat)
						{
						*(ep->search + (strlen(ep->search) - 2)) = '\0';
						ep->state = NONW_F;
						}
					}

				if (ep->state == NONW_F)
					{
// printf("\nNONW: findf on \"%s\" (2)\n", ep->search);
					stat = _findf(&ep->dta, ep->search,
						(hp->mode & FW_ALL) | FW_SUBD);
					}

				if (ep->state == NONW_N)
					{
// printf("\nNONW: findf on \"%s\" (3)\n", ep->search);
					stat = _findn(&ep->dta);
					}

				ep->state = NONW_N;
#endif
#if 0
				if ((ep->state == NONW_FX)		  /* Search for file */
				||	(ep->state == NONW_F))
					{
					stat = _findf(&ep->dta, ep->search,
						(hp->mode & FW_ALL) | FW_SUBD);
					ep->state = NONW_N;
					}
				else
					stat = _findn(&ep->dta);
#endif

				if (stat)						/* If no file found... */
					{
					if ((hp->link = ep = unnest_element(ep)) == NULL)
						{
						release_header(hp);
mwprintf("Allocs (done) %d\n", AllocCount);
						return	(NULL);			/* Return failure */
						}
					break;						/* Continue unnested */
					}

#ifdef	VERBOSEOUT
e_disp(ep, "NONW", TRUE);
#endif

				if (ep->dta.dta_type & ATT_SUBD)
					{
					build_fn(ep);				/* Yes, and visible */
					ep->state = NONW_T;
					if (hp->mode & FW_SUBD)
						{
mwprintf("Allocs %d\n", AllocCount);
						ep->fdt = fgetfdt(ep->found);
						return	(ep->found);	/* Return the directory */
						}
					break;
					}
				else if (hp->mode & FW_FLS)
					{
					build_fn(ep);
mwprintf("Allocs %d\n", AllocCount);
					ep->fdt = fgetfdt(ep->found);
					return	(ep->found);		/* Return the filename */
					}
				break;


			case NONW_T:

// printf("\nNONW_T: entered, mode = %04X, DTA.type = %04X\n", hp->mode, ep->dta.dta_type);
				ep->state = NONW_N;
				if (hp->mode & (FW_FILE | FW_ALL))
					{
// printf("\nNONW_T: nesting\n");
					xep = ep;			/* Nest the DTA element list */
					hp->link = ep = new_element();
					ep->link = xep;
mwprintf("New proto\n", AllocCount);
					ep->proto = new_string(strlen(xep->found) + 5);
					strcpy(ep->proto, xep->found);
					strcat(ep->proto, owild_str);
					}
				break;


			default:
				fat_err(3);
			}
		}
	return	(NULL);				/* Dummy to make the compiler happy */
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
	char  c;

	ep->found = dispose(ep->found);		/* Clean up the old filename */
mwprintf("New found %04X\n", ep);
	if (ep->last)
		{
		n = ep->last - ep->proto + 1;
		ep->found = new_string(n + strlen(ep->dta.dta_name) + 2);
		copyn(ep->found, ep->proto, n);
		if (strlen(ep->found) > 0)
			{
			c = ep->found[strlen(ep->found) - 1];
			if ((c != ':')	&&	(c != '/')	&&	(c != '\\'))
				strcat(ep->found, "\\");
			}
		strcat(ep->found, ep->dta.dta_name);
		}
	else
		{
		ep->found = new_string(strlen(ep->dta.dta_name) + 1);
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
	static DTA_ELE *			/* Return a pointer to the new DTA element */
new_element ()					/* Allocate an initialized DTA element */

	{
	DTA_ELE	 *ep;

mwprintf("New element\n", AllocCount);
	ep = (DTA_ELE *)(new_string(sizeof(DTA_ELE)));
	ep->link	= NULL;
	ep->wild	= NOT_WILD;
	ep->state	= FRESH;
	ep->proto	= NULL;
	ep->last	= NULL;
	ep->next	= NULL;
	ep->search	= NULL;
	ep->pattern = NULL;
	ep->found	= NULL;
	return	(ep);
	}

/* ----------------------------------------------------------------------- */
	static char *				/* Return a pointer to the new block */
new_string (size)				/* Allocate a block of string memory */
	int	 size;					/* Size of memory to allocate */

	{
	char  *p;

	if ((p = malloc(size)) == NULL)
		fat_err(1);
#ifdef MEMORYWATCH
	else
		++AllocCount;
#endif
	return	(p);
	}

/* ----------------------------------------------------------------------- */
	static char *				/* Always returns NULL */
dispose (						/* Free a block of string memory */
	char  *s)					/* Pointer to string to free */

	{
	if (s != NULL)
		{
		free(s);
#ifdef MEMORYWATCH
mwprintf("Free object\n", AllocCount);
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

mwprintf("Free header\n", AllocCount);
	mwfree((char *)(hp));						/* Then, release the header */
	}

/* ----------------------------------------------------------------------- */
	static DTA_ELE *			/* Returned pointer to the successor element */
unnest_element (				/* Unnest a DTA element */
	DTA_ELE	 *ep)				/* Pointer to the element */

	{
	DTA_ELE	 *eplink;			/* Pointer to the next element */

	if (ep != NULL)
		{
		if (ep->proto)							/* Release the proto name */
			mwfree((char *)(ep->proto));
		if (ep->search)							/* Release the search name */
			mwfree((char *)(ep->search));
		if (ep->pattern)						/* Release the pattern name */
			mwfree((char *)(ep->pattern));
		if (ep->found)							/* Release the found name */
			mwfree((char *)(ep->found));
		eplink = ep->link;						/* Point the successor */
		mwfree((char *)(ep));					/* Release the element */
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
	printf("Pointer....%04x\n", hp);
	printf("Link.......%04x\n", hp->link);
	printf("Mode.......%04x\n", hp->mode);
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

	printf("Pointer____%04x\n", ep);

	printf("Link_______%04x\n", ep->link);

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

	if (ep->proto)
		printf("Prototype__%s\n",	ep->proto);

	if (ep->last)
		printf("Last_______%3d\n",	ep->last - ep->proto);
	else
		printf("Last_______  0\n");

	if (ep->next)
		printf("Next_______%3d\n",	ep->next - ep->proto);
	else
		printf("Next_______  0\n");

	if (ep->search)
		printf("Search_____%s\n",	ep->search);

	if (ep->pattern)
		printf("Pattern____%s\n",	ep->pattern);

	if (ep->found)
		printf("Found______%s\n",	ep->found);

	if (flag)
		{
		printf("DTA.name___%s\n",	ep->dta.dta_name);
		printf("DTA.type___%04X\n", ep->dta.dta_type);
		printf("DTA.time___%04X\n", ep->dta.dta_time);
		printf("DTA.date___%04X\n", ep->dta.dta_date);
		printf("DTA.size___%d\n",	ep->dta.dta_size);
		}

	printf("\n");
	fflush(stdout);
	}

#endif
/* ----------------------------------------------------------------------- */
