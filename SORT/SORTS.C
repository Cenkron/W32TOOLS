/* ----------------------------------------------------------------------- *\
|
|				SORT Subsystem
|
|		   Copyright (c) 1993, all rights reserved
|				Brian W Johnson
|				   15-Apr-93
|				   28-Aug-93
|				   31-Oct-93
|				   17-Aug-97
|
\* ----------------------------------------------------------------------- */

#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <malloc.h>
#include  <ctype.h>
#include  <errno.h>
#include  <limits.h>
#include  <fcntl.h>
#include  <sys\types.h>
#include  <sys\stat.h>
#include  <io.h>

#include  "fwild.h"
#include  "sort.h"

// #define  TESTMODE

/* ----------------------------------------------------------------------- *\
|  Private definitions
\* ----------------------------------------------------------------------- */

#ifndef	 UINT
#define	 UINT	unsigned int
#endif

#ifndef	 BYTE
#define	 BYTE	unsigned char
#endif

typedef  const char *PCSTR;

/* ----------------------------------------------------------------------- */

#define	 COLUMNS_MAX	 (1025)		/* Maximum number of text columns */
//#define OBJECTS_MAX	 (1600)		/* Maximum number of objects per pass */
#define	 OBJECTS_MAX	 (10000)	/* Maximum number of objects per pass */
#define	 METHODS_MAX	   (10)		/* Maximum number of sort methods */
#define	 SORT_METHOD_MAX   (10)		/* Maximum number of sort methods */

#define	 SORT_TYPE_NONE	    (0)		/* Unused method */
#define	 SORT_TYPE_STRING   (1)		/* Use a collation sort */
#define	 SORT_TYPE_NUMERIC  (2)		/* Use a numeric sort */
#define	 SORT_TYPE_DATE	    (3)		/* Use a date sort */

#define	 TEMPNAME  "_SORT_.TMP"		/* Default name of the temporary sort file */
#define	 TEMPOPEN  (O_BINARY | O_RDWR | O_CREAT | O_TRUNC)
#define	 TEMPPROT  (S_IREAD | S_IWRITE)

/* ----------------------------------------------------------------------- *\
|  Private structure definitions
\* ----------------------------------------------------------------------- */

typedef					/* The static method structure */
    struct				/* (one set, built statically) */
	{
	BYTE  type;
	BYTE  order;
	UINT  field1;
	UINT  column1;
	UINT  max1;
	UINT  field2;
	UINT  column2;
	UINT  max2;
	UINT  base;
	}  SMETHOD;

static	SMETHOD	 method [SORT_METHOD_MAX] = {0};

typedef	 SMETHOD  *PSMETHOD;		/* Pointer to an SMETHOD structure */


typedef					/* The dynamic method structure */
    struct				/* (built into each sorted object) */
	{
	BYTE	      type;		/* Sort type */
	BYTE	      order;		/* Sort order */
	union
	    {
	    long      value;            /* Numeric or Date sort */
	    struct                      /* String sort */
		{
		UINT  offset;
		UINT  length;
		} s;
	    } u;
	}  IMETHOD;

typedef	 IMETHOD  *PIMETHOD;		/* Pointer to an IMETHOD structure */


typedef					/* The constructed sort item */
    struct
	{
	UINT	 size;			/* Dynamic size of the SORTDATA structure */
/*	IMETHOD	 method [sortcb.methods];  The array of method structures */
/*	char	 data [N];		   The actual string being sorted on */
	}  SORTDATA;

typedef SORTDATA  *PSORTDATA;		/* Pointer to a SORTDATA structure */

#define	 PTR_TO_IMETHOD(p)	(PIMETHOD)((char *)(p) + sizeof(UINT))
#define	 PTR_TO_LINE_DATA(p)	((char *)(p) + loffset)


typedef
    struct HEAP_RECORD
	{
	struct HEAP_RECORD  *link;	/* Link to the next HEAP_RECORD */
	long		     seek;	/* Offset within the temporary file */
	int		     id;	/* The heap record ID */
	int		     count;	/* Number of remaining sort objects */
	PSORTDATA	     data;	/* Pointer to the current object */
	} HEAP_RECORD;


typedef					/* The sort control block */
    struct
	{
	BYTE	      _skipwhite;	/* TRUE to ignore leading white space */
	BYTE	      _completed;	/* TRUE when sorting is completed */
	UINT	      _verbose;		/* The message verbosity level */
	UINT	      _methods;		/* The number of sort methods */
	UINT	      _loffset;		/* The excess for the method data */
	UINT	      _maxstack;	/* Maximum stack available */
	UINT	      _minstack;	/* Minimum stack available */
	UINT	      _nesting;		/* Current sort nesting */
	UINT	      _maxnest;		/* Maximum sort nesting */
	UINT	      _heap_count;	/* The number of heaps built */
	UINT	      _line_count;	/* The number of lines in the current heap */
	UINT	      _line_total;	/* The total number of lines sorted */
	int	      _tab_size;	/* The tab size */
	int	      _tfd;		/* The temporary FILE, when open */
	PSORTDATA    *_table;		/* Pointer to the table array */
	HEAP_RECORD  *_crp;		/* Current run pointer */
	HEAP_RECORD  *_frp;		/* Pointer to first run */
	HEAP_RECORD  *_lrp;		/* Pointer to last run */
	void (*_fnsort)(int, int);	/* Pointer to the sort function */
        int (*_fnstrcmp)(PCSTR, PCSTR, size_t);  /* Pointer to the string compare function */
	}  SORTCB;

static	SORTCB	sortcb = {0};		/* The sort system structure */

#define	 skipwhite	sortcb._skipwhite
#define	 completed	sortcb._completed
#define	 verbose	sortcb._verbose
#define	 methods	sortcb._methods
#define	 loffset	sortcb._loffset
#define	 maxstack	sortcb._maxstack
#define	 minstack	sortcb._minstack
#define	 nesting	sortcb._nesting
#define	 maxnest	sortcb._maxnest
#define	 heap_count	sortcb._heap_count
#define	 line_count	sortcb._line_count
#define	 line_total	sortcb._line_total
#define	 tab_size	sortcb._tab_size
#define	 tfd		sortcb._tfd
#define	 table		sortcb._table
#define	 crp		sortcb._crp
#define	 frp		sortcb._frp
#define	 lrp		sortcb._lrp
#define	 fnsort		sortcb._fnsort
#define	 fnstrcmp	sortcb._fnstrcmp
#define	 sort(a1,a2)	(*fnsort)((a1), (a2))
#define	 strcompare(s1,s2,n) \
			(*fnstrcmp)((s1), (s2), (n))

static	char	line [COLUMNS_MAX];	 /* Output line buffer (multiple heaps) */

static	char   *tempfile = TEMPNAME;

/* ----------------------------------------------------------------------- *\
|  Private function prototypes
\* ----------------------------------------------------------------------- */

static	void	quicksort	(int, int);
static	void	shakersort	(int, int);
static	int	compare		(PSORTDATA, PSORTDATA);
static	void   *fat_alloc	(int);
static	void	save_to_heap	(void);
static	int	get_table_line	(char **ppstr);
static	int	get_heap_line	(char **ppstr);
static	void	get_heap_record (HEAP_RECORD *);

static	int	sort_xxx
	(BYTE type, BYTE order, UINT field1, UINT column1, UINT max1,
	 UINT field2, UINT column2, UINT max2, UINT base);

static	int	find_field
	(UINT rfield, UINT rcolumn, char *pstr, char **pret, UINT *nret);

static	int	find_column
	(UINT rcolumn, char *pstr, char **pret, UINT *nret);

static	int	configure_string
	(PSORTDATA p, PIMETHOD pim, PSMETHOD psm);

static	int	configure_numeric
	(PSORTDATA p, PIMETHOD pim, PSMETHOD psm);

static	int	configure_date
	(PSORTDATA p, PIMETHOD pim, PSMETHOD psm);

#if defined(TESTMODE)
static	void	showconfig	(void);
static	void	showalllines	(void);
static	void	showline	(int line, PSORTDATA p);
static	void	showtype	(BYTE type);
#endif

/* ----------------------------------------------------------------------- *\
|  sort_init () - Initialize the sort subsystem
\* ----------------------------------------------------------------------- */
    int
sort_init (void)	/* Initialize the sort system */

    {
    int	 result = 0;	/* The returned result, 0 for success */


    if (crp == NULL)
	crp = (HEAP_RECORD *)(fat_alloc(sizeof(HEAP_RECORD)));
    if (table == NULL)
	table = (PSORTDATA *)(fat_alloc(OBJECTS_MAX * sizeof(char *)));

    nesting    = 0;
    maxnest    = 0;
//    maxstack   = _stackavail();
//    minstack   = maxstack;
    fnsort     = quicksort;	/* Default fast, unstable sort */
    fnstrcmp   = strncmp;	/* Default case sensitive string compare */
    verbose    = 0;		/* Default zero verbosity */
    methods    = 0;		/* No sort methods yet */
    completed  = FALSE;		/* Sort not complete yet */

    heap_count = 0;
    line_count = 0;
    line_total = 0;

    frp	       = NULL;
    lrp	       = NULL;

    memset(&method[0], 0, (sizeof(SMETHOD) * SORT_METHOD_MAX));

    atexit(sort_terminate);
    return (result);
    }

/* ----------------------------------------------------------------------- *\
|  sort_parms () - Set sort system parameters
\* ----------------------------------------------------------------------- */
    int
sort_parms (			/* Set sort system flags */
    UINT  req_stable,		/* TRUE for a stable (slow) sort */
    UINT  req_verbose,		/* The verbosity level */
    UINT  req_skipwhite,	/* Ignore leading white space */
    UINT  req_ignorecase,	/* Ignore case flag */
    int	  order,		/* The sort order */
    int	  tabsize)		/* The tab size */

    {
    int	 result = 0;		/* The returned result */

    if (req_stable)
	fnsort = shakersort;	/* Stable, slow sort */
    else
	fnsort = quicksort;	/* Fast, unstable sort */

    if (req_ignorecase)
	fnstrcmp = strnicmp;	/* Case insensitive string compare */
    else
	fnstrcmp = strncmp;	/* Case sensitive string compare */

    verbose   = req_verbose;	/* Set the verbosity level */
    skipwhite = req_skipwhite;	/* Set the ignore white flag */
    tab_size  = tabsize;	/* Set the tab size */

    if (methods == 0)		/* Ensure that at least one method is specified */
	result = sort_string(0, 0, SHRT_MAX, order);

#if defined(TESTMODE)
showconfig();
#endif

    verbose = req_verbose;
    return (result);
    }

/* ----------------------------------------------------------------------- *\
|  sort_set_temp () - Set the non-default name of the temporary file
\* ----------------------------------------------------------------------- */
    void
sort_set_temp (
    char *ptempname)		/* The supplied temporary file name */

    {
    tempfile = ptempname;
    }

/* ----------------------------------------------------------------------- *\
|  sort_stats () - Display sort statistics
\* ----------------------------------------------------------------------- */
    void
sort_stats (void)		/* Return sort statistics */

    {
    exitmsg(0, "Total lines sorted:      %u", line_total);
    exitmsg(0, "Initial stack available: %u", maxstack);
    exitmsg(0, "Minimum stack available: %u", minstack);
    exitmsg(0, "Maximum sort nesting:    %u", maxnest);
    }

/* ----------------------------------------------------------------------- *\
|  sort_terminate () - Terminate the sort subsystem
\* ----------------------------------------------------------------------- */
    void
sort_terminate (void)		/* Terminate the sort system */

    {
    if (tfd > 0)
	{
	close(tfd);
	if (verbose > 2)
	    exitmsg(0, "Deleting the temporary file \"%s\"", tempfile);
	unlink(tempfile);
	tfd = -1;
	}
    }

/* ----------------------------------------------------------------------- *\
|  sort_string () - Insert a string sort method into the method sequence
\* ----------------------------------------------------------------------- */
    int
sort_string (
    UINT  field,		/* The field number to sort on */
    UINT  column,		/* The column to sort on */
    UINT  colmax,		/* The column limit (or zero) */
    int   order)		/* The sort order for the field */

    {
    return (sort_xxx(SORT_TYPE_STRING, (BYTE)(order),
	field, column, colmax, 0, 0, 0, 0));
    }

/* ----------------------------------------------------------------------- *\
|  sort_numeric () - Insert a numeric sort method into the method sequence
\* ----------------------------------------------------------------------- */
    int
sort_numeric (
    UINT  field,		/* The field number to sort on */
    UINT  column,		/* The column to sort on */
    UINT  colmax,		/* The column limit (or zero) */
    UINT  base,			/* The base (or zero for c-language) */
    int   order)		/* The sort order for the field */

    {
    return (sort_xxx(SORT_TYPE_NUMERIC, (BYTE)(order),
	field, column, colmax, 0, 0, 0, base));
    }

/* ----------------------------------------------------------------------- *\
|  sort_date () - Insert a date sort method into the method sequence
\* ----------------------------------------------------------------------- */
    int
sort_date (
    UINT  field1,		/* The first date field to sort on */
    UINT  field2,		/* The second date field to sort on */
    int   order)		/* The sort order for the field */

    {
    return (sort_xxx(SORT_TYPE_DATE, (BYTE)(order),
	field1, 0, 0, field2, 0, 0, 0));
    }

/* ----------------------------------------------------------------------- *\
|  sort_xxx () - Request generic sorting
\* ----------------------------------------------------------------------- */
    static int
sort_xxx (
    BYTE  type,
    BYTE  order,
    UINT  field1,
    UINT  column1,
    UINT  max1,
    UINT  field2,
    UINT  column2,
    UINT  max2,
    UINT  base)

    {
    int	      result;		/* The returned result, 0 for success */
    PSMETHOD  pmethod;		/* Pointer to the method entry */


    if (methods >= METHODS_MAX)
	{
	exitmsg(FWEXIT_COMMAND_LINE, "Sort method overflow");
	result = SORT_ERR_SORT_METHOD_OVERFLOW;
	}

    else
	{
	pmethod		 = &method[methods];
	loffset		 = (++methods * sizeof(IMETHOD)) + sizeof(SORTDATA);
	result		 = 0;

	pmethod->type	 = type;
	pmethod->order   = order;
	pmethod->field1	 = field1;
	pmethod->column1 = column1;
	pmethod->max1	 = max1;
	pmethod->field2	 = field2;
	pmethod->column2 = column2;
	pmethod->max2	 = max2;
	pmethod->base	 = base;
	}

    return (result);
    }

/* ----------------------------------------------------------------------- *\
|  sort_put_line () - Insert one text line (prior to being sorted)
\* ----------------------------------------------------------------------- */
    int
sort_put_line (			/* Add a sort object to the set */
    char      *pstr)		/* Pointer to the sort string */

    {
    int	       result = 0;	/* The returned result, 0 for success */
    int	       length;		/* Length of the line */
    int	       mresult;		/* The returned method result, 0 for success */
    int	       objsize;		/* Size of the object */
    UINT       i;		/* Index through the method array */
    PSORTDATA  p;		/* Pointer to the allocated SORTDATA structure */
    PIMETHOD   pim;		/* Pointer to the IMETHOD structures */
    PSMETHOD   psm;		/* Pointer to the SMETHOD structures */


	/* Determine the size of the SORTDATA object */

    length = strlen(pstr);
    if (length >= COLUMNS_MAX)
	{
	length = COLUMNS_MAX - 1;
	exitmsg(FWEXIT_MINOR_ERROR,
	    "Line too long \"%.65s\"", pstr);
	result = SORT_ERR_TOO_LONG;
	}
    objsize = length + loffset + 1;

	/* Allocate the new sort object; if the current */
	/* sort set is full, sort it and save it first */

    if ((line_count >= OBJECTS_MAX)
    || ((p = calloc(1, objsize)) == NULL))
	{
	save_to_heap();
	crp = (HEAP_RECORD *)(fat_alloc(sizeof(HEAP_RECORD)));
	p   = fat_alloc(objsize);
	}

	/* Build the new sort object */

    p->size = objsize;
    strncpy(PTR_TO_LINE_DATA(p), pstr, length);

    pim = PTR_TO_IMETHOD(p);
    psm = &method[0];
    for (i = 0; i < methods; ++i)
	{
	pim->order = psm->order;
	pim->type  = psm->type;
	switch (pim->type)
	    {
	    case SORT_TYPE_STRING:	/* Use a collation sort */
		mresult = configure_string(p, pim, psm);
		break;

	    case SORT_TYPE_NUMERIC:	/* Use a numeric sort */
		mresult = configure_numeric(p, pim, psm);
		break;

	    case SORT_TYPE_DATE:	/* Use a date sort */
		mresult = configure_date(p, pim, psm);
		break;
	    }

	if (result == 0)		/* Report only the first error */
	    result = mresult;
	++pim;
	++psm;
	}

	/* Add the new sort object to the set */

    table[line_count++] = p;
    ++line_total;

    return (result);
    }

/* ----------------------------------------------------------------------- *\
|  sort_get_line () - Return one text line (following sort completion)
\* ----------------------------------------------------------------------- */
    int
sort_get_line (
    char  **ppstr)		/* Pointer to the returned line pointer */

    {
#if defined(TESTMODE)
// showconfig();
// showalllines();
#endif


    if (frp == NULL)
	return (get_table_line(ppstr));

    else
	return (get_heap_line(ppstr));
    }

/* ----------------------------------------------------------------------- *\
|  get_table_line () - Return one text line (following sort completion)
|  This function is used when the sort was contained entirely in one heap
\* ----------------------------------------------------------------------- */
    static int
get_table_line (
    char  **ppstr)		/* Pointer to the returned line pointer */

    {
    int	 result = 0;		/* The returned result */
static	PSORTDATA  *pcur;	/* Pointer to next sort object */
static	PSORTDATA  *pend;	/* Sentinel pointer */


    if ( ! completed)
	{
	sort(0, (line_count - 1));
	pcur = &table[0];
	pend = &table[line_count];
	completed = TRUE;
	}

    if (pcur < pend)
	{
	*ppstr = line;
	strcpy(line, PTR_TO_LINE_DATA(*pcur));
	free(*pcur);
	*pcur = NULL;
	++pcur;
	}
    else
	{
	sort_terminate();
	free(table);
	table  = NULL;
	*ppstr = NULL;
	result = -1;
	}

    return (result);
    }

/* ----------------------------------------------------------------------- *\
|  get_heap_line () - Return one text line (following sort completion)
|  This function is used when the sort required multiple heaps.
|  This is effectively performing a merge sort from the various heaps.
\* ----------------------------------------------------------------------- */
    static int
get_heap_line (
    char  **ppstr)		/* Pointer to the returned line pointer */

    {
    HEAP_RECORD	 *srp;		/* Pointer to the predecessor heap record */
    HEAP_RECORD	 *arp;		/* Pointer to the sorted heap record */
    HEAP_RECORD	 *prp;		/* Pointer to the previous heap record */
    HEAP_RECORD	 *xrp;		/* Pointer to the current heap record */


    if ( ! completed)		/* Perform transition to retrieval mode */
	{
	save_to_heap();
	free(table);
	table	  = NULL;
	completed = TRUE;

	if (verbose > 1)
	    exitmsg(0, "Reading the temporary file \"%s\"", tempfile);
	lseek(tfd, 0L, SEEK_SET);

// showconfig();
// exit(0);

	    /* Retrieve the first record for each heap */

	for (xrp = frp; (xrp != NULL); xrp = xrp->link)
	    get_heap_record(xrp);
	}

    if (frp == NULL)
	{
	sort_terminate();	/* There are no records left */
	*ppstr = NULL;
	return (-1);
	}

    /* This is the merge sort selection.  Note that it should be a */
    /* stable sort, so the earliest record of an equal pair must be used. */

    arp = NULL;
    for (xrp = frp; (xrp != NULL); xrp = xrp->link)
	{
	if (arp == NULL)
	    {
	    arp = xrp;
	    srp = NULL;
	    }
	else if (compare(xrp->data, arp->data) < 0)
	    {
	    arp = xrp;
	    srp = prp;
	    }
	prp = xrp;
	}

    /* arp now points to the next logical heap record to be returned, */
    /* so copy and return the line, and delete the sort object */

    *ppstr = line;
    strcpy(line, PTR_TO_LINE_DATA(arp->data));
    free(arp->data);
    arp->data = NULL;

    /* If the heap is still not empty, replenish the heap record */

    if (--arp->count > 0)
	get_heap_record(arp);

    /* If the heap is empty, delete it and collapse the heap chain */

    else
	{
	for (xrp = frp, prp = NULL; (xrp != NULL); xrp = srp)
	    {
	    srp = xrp->link;
	    if (xrp->count == 0)
		{
		if (xrp == frp)
		    frp = srp;
		else if (prp != NULL)
		    prp->link = srp;
		if (xrp == lrp)
		    lrp = prp;

		if (verbose > 2)
		    exitmsg(0, "Heap %d closed", xrp->id);
		free(xrp);
		--heap_count;
		break;
		}
	    prp = xrp;
	    }
	}

    return (0);
    }

/* ----------------------------------------------------------------------- *\
|  get_heap_record () - Get the first line from the specified heap
\* ----------------------------------------------------------------------- */
    static void
get_heap_record (
    HEAP_RECORD	 *rp)	/* Pointer to the heap record */

    /* Get a record into allocated storage, from the specified */
    /* heap on the temporary file,  returning a pointer to it, */
    /* or NULL if there are no lines left in the run. */

    {
    int	   size;


    if (rp->count <= 0)
	exitmsg(FWEXIT_MAJOR_ERROR, "Unexpected empty heap");

    lseek(tfd, rp->seek, SEEK_SET);
    if (read(tfd, &size, sizeof(rp->data->size)) < 0)
	exitmsg(FWEXIT_MAJOR_ERROR, "Error (%d) reading temporary file", errno);

    rp->data	   = fat_alloc(size);
    rp->data->size = size;
    size	  -= sizeof(rp->data->size);
    if (read(tfd, (((char *)(rp->data)) + sizeof(rp->data->size)), size) != size)
	exitmsg(FWEXIT_MAJOR_ERROR, "Unexpected end of temporary file");

    rp->seek = lseek(tfd, 0L, SEEK_CUR);
//  showline(0, p);
    }

/* ----------------------------------------------------------------------- *\
|  save_to_heap () - Save the current sort set into a disk heap sort set
\* ----------------------------------------------------------------------- */
    static void
save_to_heap (void)

    {
    int		size;
    PSORTDATA  *p;


	/* Sort the current heap before saving it */

    sort(0, (line_count - 1));

	/* If not already created, create the heap file */

    if (tfd <= 0)
	{
	if ((tfd = open(tempfile, TEMPOPEN, TEMPPROT)) < 0)
	    exitmsg(FWEXIT_MAJOR_ERROR,
		"Can't create the temporary file \"%s\"", tempfile);

	if (verbose > 1)
	    exitmsg(0, "Writing the temporary file \"%s\"", tempfile);
	}

	/* The run block (pointed by crp) was preallocated because */
	/* there may not be enough space to allocate it now. */

    crp->link  = NULL;
    crp->seek  = lseek(tfd, 0L, SEEK_CUR);
    crp->id    = ++heap_count;
    crp->count = line_count;

    if (frp == NULL)
	frp = crp;
    else
	lrp->link = crp;
    lrp = crp;

    if (verbose > 2)
	exitmsg(0, "Heap %d, %d lines", heap_count, line_count);

    for (p = table; (p < table + line_count); ++p)
	{
	size = (*p)->size;
	if (write(tfd, *p, size) != size)
	    exitmsg(FWEXIT_MAJOR_ERROR,
		"Error (%d) writing the temporary file \"%s\"", errno, tempfile);
	free(*p);
	*p = NULL;
	}

    line_count = 0;
    }

/* ----------------------------------------------------------------------- *\
|  configure_string () - Configure the substring for a sort object
\* ----------------------------------------------------------------------- */
    static int
configure_string (
    PSORTDATA  p,		/* Pointer to the sort object */
    PIMETHOD   pim,		/* Pointer to the IMETHOD structure */
    PSMETHOD   psm)		/* Pointer to the SMETHOD structure */

    {
    int	       result = 0;	/* The returned result, 0 for success */
    UINT       length;		/* Length of the found field */
    char      *s;		/* Pointer to the found field */


    result = find_field(
	psm->field1,
	psm->column1,
	PTR_TO_LINE_DATA(p),
	&s,
	&length);

    if (result == 0)
	{
	if (length > psm->max1)
	    length = psm->max1;
	pim->u.s.offset = (s - (char *)(p));	/* Offset from object origin */
	pim->u.s.length = length;
	}

    return (result);
    }

/* ----------------------------------------------------------------------- *\
|  configure_numeric () - Configure the numeric value for a sort object
\* ----------------------------------------------------------------------- */
    static int
configure_numeric (
    PSORTDATA  p,		/* Pointer to the sort object */
    PIMETHOD   pim,		/* Pointer to the IMETHOD structure */
    PSMETHOD   psm)		/* Pointer to the SMETHOD structure */

    {
    int	       result = 0;	/* The returned result, 0 for success */
    UINT       length;		/* Length of the found field */
    char       chsave;		/* The saved field terminator */
    char      *psave;		/* Pointer to the saved field terminator */
    char      *s;		/* Pointer to the numeric field */
    char      *sret;		/* Pointer to the field terminator */


    result = find_field(
	psm->field1,
	psm->column1,
	PTR_TO_LINE_DATA(p),
	&s,
	&length);

    if (result == 0)		// Temporarily limit the field length
	{
	if ((psm->column1 > 0)
	&&  (length       > psm->column1))
	    {
	    psave  = s + psm->column1;
	    chsave = *psave;
	    *psave = '\0';
	    }
	else
	    chsave = '\0';

	pim->u.value = strtol(s, &sret, 0);

	if (s == sret)
	    {
	    exitmsg(FWEXIT_MINOR_ERROR,
		"Numeric sort field error: \"%s\"", PTR_TO_LINE_DATA(p));
	    result = SORT_ERR_VALUE_SYNTAX;
	    }

	if (chsave != '\0')	// Restore the field length
	    *psave = chsave;
	}

    return (result);
    }

/* ----------------------------------------------------------------------- *\
|  configure_date () - Configure the numeric value (date) for a sort object
\* ----------------------------------------------------------------------- */
    static int
configure_date (
    PSORTDATA	  p,		/* Pointer to the sort object */
    PIMETHOD	  pim,		/* Pointer to the IMETHOD structure */
    PSMETHOD	  psm)		/* Pointer to the SMETHOD structure */

    {
    int		  result = 0;	/* The returned result, 0 for success */
    int		  length;	/* Length of the found date field */
    char	 *s;		/* Pointer to the found date field */
    char	 *s1;		/* Pointer into the date buffer */
    char	  buffer [256]; /* Temporary date field buffer */


	/* Find and copy the first date field */

    buffer[0] = '\0';

#if defined(TESTMODE)
printf("Fields: %u, %u\n", psm->field1, psm->field2);
fflush(stdout);
#endif

    result = find_field(
	psm->field1,
	psm->column1,
	PTR_TO_LINE_DATA(p),
	&s,
	&length);

    if (result != 0)
	goto exxit;

    if (length > 127)
	length = 127;
    s1 = &buffer[0];
    strncpy(s1, s, length);
    s1 += length;
    *s1 = '\0';


	/* Find and copy the second date field (if configured) */

    if (psm->field2 != 0)
	{
	result = find_field(
	    psm->field2,
	    psm->column2,
	    PTR_TO_LINE_DATA(p),
	    &s,
	    &length);

	if (result != 0)
	    goto exxit;

	if (length > 127)
	    length = 127;
	strcpy(s1++, ";");
	strncat(s1, s, length);
	s1 += length;
	*s1 = '\0';
	}



	/* Determine the date value */

#if defined(TESTMODE)
printf("String: \"%s\"\n", buffer);
fflush(stdout);
#endif

    if ((pim->u.value = sgettd(buffer)) == -1)
	{
	exitmsg(FWEXIT_MINOR_ERROR,
	    "Date field syntax error (%s): \"%s\"",
		serrtd(),
		PTR_TO_LINE_DATA(p));
	result = SORT_ERR_DATE_SYNTAX;
	}

exxit:
    return (result);
    }

/* ----------------------------------------------------------------------- *\
|  find_field () - Find field M, column N of the pointed string
|  (Tabs within fields (>= 1) are interpreted as spaces)
\* ----------------------------------------------------------------------- */
    static int		/* Returns 0 for success */
find_field (		/* Find field M, column N of the pointed string */
    UINT    rfield,	/* Requested field number within the string */
    UINT    rcolumn,	/* Requested column number within the field */
    char   *pstr,	/* Pointer to the string */
    char  **pret,	/* Pointer to returned field pointer */
    UINT   *nret)	/* Pointer to returned length of the found field */

    {
    char    ch;		/* Temporary character */
    char   *p;		/* Pointer into the string */
    char   *px;		/* Pointer into the string */
    int	    flag   = FALSE;	/* Non-space flag */
    int	    result = 0; /* The returned result, 0 for success */
    int	    length = 0; /* Length of the found field */
    int     quoted = 0; /* In a quoted region */
    UINT    field  = 0; /* Field counter */
    UINT    column = 0; /* Search column counter */
    UINT    abscol = 0; /* Absolute column counter */


	/* The column-only case is handled specially */

    if (rfield == 0)
	return (find_column(rcolumn, pstr, pret, nret));


	/* Handle the general field and column search case */

    for (p = pstr; ((ch = *p) != '\0'); ++p)
	{
	if ((ch == '\n')  ||  (ch == '\f'))
	    continue;

	else if (ch == ' ')
	    {
	    if (quoted == 0)
		flag = FALSE;
	    ++abscol;
	    }

	else if (ch == '\t')
	    {
	    if (quoted == 0)
		flag = FALSE;
	    abscol += (tab_size - (abscol % tab_size));
	    }

	else if (quoted  &&  (ch == '\"'))
	    {
	    quoted = 0;		// (but is included in the field)
	    ++abscol;
	    }

	else if (flag == FALSE)
	    {
	    if (++field == rfield)
		break;		// Process quotes in the next section

	    if (ch == '\"')	// Begin quoted region, not right field yet
		quoted = 1;
	    flag = TRUE;
	    ++abscol;
	    }
	}

    if (field != rfield)
	{
	exitmsg(FWEXIT_MINOR_ERROR,
	    "Field %d not found: \"%s\"", rfield, pstr);
	result = SORT_ERR_FIELD_NOT_FOUND;
	goto exxit;
	}


	/* Do the column search within the field */

    if (rcolumn > 1)
	{
	column = rcolumn + abscol - 1;	/* Correct for 1-based column */
	for ( ; ((ch = *p) != '\0'); ++p)
	    {
	    if (abscol >= column)
		break;

	    if ((ch == '\n')  ||  (ch == '\f'))
		break;
		
	    else if (ch == ' ')
		{
		if (quoted == 0)
		    break;
		++abscol;
		}

	    else if (ch == '\t')
		{
		if (quoted == 0)
		    break;
		abscol += (tab_size - (abscol % tab_size));
		}

	    else
		{
		if (ch == '\"')
		    quoted = (1 - quoted);
		++abscol;
		}
	    }

	if (abscol != column)
	    {
	    exitmsg(FWEXIT_MINOR_ERROR,
		"Field %d column %d not found: \"%s\"",
		rfield, rcolumn, pstr);
	    result = SORT_ERR_COLUMN_NOT_FOUND;
	    goto exxit;
	    }
	}

    for (px = p; ((ch = *px) != '\0'); ++px)
	{
	if ((ch == '\n')  ||  (ch == '\f'))
	    break;
		
	else if ((ch == ' ')  ||  (ch == '\t'))
	    {
	    if (quoted == 0)
		break;
	    }

	else if (ch == '\"')
	    quoted = (1 - quoted);

	++length;
	}

    if ((length == 0)  &&  (rfield != 0))
	{
	exitmsg(FWEXIT_MINOR_ERROR,
	    "Field %d length is zero: \"%s\"", rfield, pstr);
	result = SORT_ERR_LENGTH_ZERO;
	}
    else
	result = 0;

exxit:
    *pret = (result == 0) ? (p) : (NULL);
    *nret = length;
    return (result);
    }

/* ----------------------------------------------------------------------- *\
|  find_column () - Find column N of the pointed string (in field 0)
|  (Tabs within field 0 are expanded appropriately)
\* ----------------------------------------------------------------------- */
    static int		/* Returns 0 for success */
find_column (		/* Find column N of the pointed string */
    UINT    rcolumn,	/* Requested column number within the string */
    char   *pstr,	/* Pointer to the string */
    char  **pret,	/* Pointer to returned field pointer */
    UINT   *nret)	/* Pointer to returned length of the found field */

    {
    char    ch;		/* Temporary character */
    char   *p;		/* Pointer into the string */
    char   *pf;		/* Pointer into the string */
    UINT    abscol = 0; /* Absolute column counter */
    int	    length = 0; /* Length of the found field */
    int	    result = 0; /* The returned result, 0 for success */


    if (rcolumn > 0)		/* Correct for 0-based column */
	--rcolumn;

    if (skipwhite)
	pf = stpblk(pstr);
    else
	pf = pstr;

    for (p = pf; ((ch = *p) != '\0'); ++p)
	{
	if (abscol >= rcolumn)
	    break;

	if (ch == '\t')
	    abscol += (tab_size - (abscol % tab_size));
	else
	    ++abscol;
	}

    if (abscol != rcolumn)
	{
	exitmsg(FWEXIT_MINOR_ERROR,
	    "Absolute column %d not found: \"%s\"", rcolumn, pstr);
	result = SORT_ERR_COLUMN_NOT_FOUND;
	goto exxit;
	}

    length = strlen(p);
#if 0
    if (length == 0)
	{
	exitmsg(FWEXIT_MINOR_ERROR,
	    "Field 0 length is zero: \"%s\"", pstr);
	result = SORT_ERR_LENGTH_ZERO;
	}
    else
#endif
	result = 0;

exxit:
    *pret = (result == 0) ? (p) : (NULL);
    *nret = length;
    return (result);
    }

/* ----------------------------------------------------------------------- *\
|  quicksort () - Perform a quicksort (Fast, unstable, and recursive)
\* ----------------------------------------------------------------------- */
    static void
quicksort (
    int	  l,		/* Index of the first (leftmost) sort object */
    int	  r)		/* Index of the last (rightmost) sort object */

    {
//    UINT	  stk;
    int		  i;
static int	  j;
static PSORTDATA  p;
static PSORTDATA  t;

    if (verbose > 1)
	{
//	if ((stk = _stackavail()) < minstack)
//	    minstack = stk;
	if (++nesting > maxnest)
	    maxnest = nesting;
	}

    if (l < r)
	{
	i = l;
	j = r;
	p = table[(l + r) / 2];
	do  {
	    while (compare(table[i], p) < 0)
		++i;
	    while (compare(p, table[j]) < 0)
		--j;

	    if (i <= j)
		{
		if (compare(table[i], table[j]) != 0)
		    { t = table[i]; table[i] = table[j]; table[j] = t; }
		++i;
		--j;
		}
	    } while (i <= j);

	quicksort(l, j);
	quicksort(i, r);
	}

    --nesting;
    }

/* ----------------------------------------------------------------------- *\
|  shakersort () - Perform a shakersort (Stable, slow, and not recursive)
\* ----------------------------------------------------------------------- */
    static void
shakersort (
    int	 l,		/* Index of the first (leftmost) sort object */
    int	 r)		/* Index of the last (rightmost) sort object */

    {
    int	       j;
    int	       k;
    PSORTDATA  t;

    if (l == r)
	return;

    ++l;
    k = r;
    do	{
	for (j = r; (j >= l); --j)
	    {
	    if (compare(table[j - 1], table[j]) > 0)
		{
		t = table[j]; table[j] = table[j - 1]; table[j - 1] = t;
		k = j;
		}
	    }
	l = k + 1;

	for (j = l; (j <= r); ++j)
	    {
	    if (compare(table[j - 1], table[j]) > 0)
		{
		t = table[j]; table[j] = table[j - 1]; table[j - 1] = t;
		k = j;
		}
	    }
	r = k - 1;
	}  while (l <= r);
    }

/* ----------------------------------------------------------------------- *\
|  compare () - Compare two sort objects
\* ----------------------------------------------------------------------- */
    static int
compare (			/* Master compare routine */
    PSORTDATA  pa,		/* Pointer to the left object */
    PSORTDATA  pb)		/* Pointer to the right object */

    {
    int	       result = 0;	/* The returned result, 0 for equal */
    int	       cycles;		/* Number of sort cycles */
    int	       sdiff;		/* Temporary difference */
    long       ldiff;		/* Temporary difference */
    IMETHOD   *pia;		/* Pointers to the IMETHOD structures */
    IMETHOD   *pib;		/* Pointers to the IMETHOD structures */
    BYTE       order;		/* The sort order */


    pia = PTR_TO_IMETHOD(pa);
    pib = PTR_TO_IMETHOD(pb);
    for (cycles = methods; (cycles > 0); --cycles)
	{
	order = pia->order;
	if (pia->type == SORT_TYPE_STRING)
	    {
            result = strcompare(
                ((char *)(pa) + pia->u.s.offset),
                ((char *)(pb) + pib->u.s.offset),
                min(pia->u.s.length, pib->u.s.length));
	    if (result != 0)
		break;

	    /* Compare the lengths */

	    sdiff = (pia->u.s.length - pib->u.s.length);
	    if (sdiff < 0)			/* Convert to trinary */
		{
		result = (-1);
		break;
		}
	    else if (sdiff > 0)
		{
		result = (1);
		break;
		}
	    }

	else  /* SORT_TYPE_NUMERIC or SORT_TYPE_DATE */
	    {
	    ldiff = (pia->u.value - pib->u.value); /* Compare the values */
	    if (ldiff < 0)			   /* Convert to trinary */
		{
		result = (-1);
		break;
		}
	    else if (ldiff > 0)
		{
		result = (1);
		break;
		}
	    }

	++pia;
	++pib;
	}

	/* If requested, apply reversal */

    return ((order) ? (-result) : (result));
    }

/* ----------------------------------------------------------------------- *\
|  fat_alloc () - Allocate space; failure is fatal to sorting
\* ----------------------------------------------------------------------- */
    void *
fat_alloc (			/* Allocate space, if no space, abort */
    int	 n)

    {
    register void  *p;


    if ((p = calloc(1, n)) == NULL)
	exitmsg(FWEXIT_MAJOR_ERROR, "Insufficient memory");
    return (p);
    }

/* ----------------------------------------------------------------------- *\
|  sort_error () - Translate sort subsystem error codes
\* ----------------------------------------------------------------------- */
    char *
sort_error (
    int	 error_code)

    {
    char  *p;
static char  dummy [32];


    switch (error_code)
	{
	case SORT_ERR_NONE:
	    p = "no error";
	    break;

	case SORT_ERR_COLUMN_NOT_FOUND:
	    p = "column not found";
	    break;

	case SORT_ERR_DATE_SYNTAX:
	    p = "date syntax error";
	    break;

	case SORT_ERR_FIELD_NOT_FOUND:
	    p = "field not found";
	    break;

	case SORT_ERR_LENGTH_ZERO:
	    p = "line length is zero";
	    break;

	case SORT_ERR_SORT_METHOD_OVERFLOW:
	    p = "sort method table overflow";
	    break;

	case SORT_ERR_TOO_LONG:
	    p = "line too long";
	    break;

	case SORT_ERR_VALUE_SYNTAX:
	    p = "value syntax error";
	    break;

	default:
	    sprintf(p = dummy, "unknown sort error (%d)", error_code);
	    break;
	}

    return (p);
    }

/* ----------------------------------------------------------------------- *\
|  showconfig () - Print the interpreted configuration
\* ----------------------------------------------------------------------- */
#if defined(TESTMODE)

    static void
showconfig (void)

    {
    int	 i;

    printf("SORTCB:\n");
    printf("  skipwhite:  %u\n",   skipwhite);
    printf("  verbose:    %u\n",   verbose);
    printf("  completed:  %u\n",   completed);
    printf("  methods:    %u\n",   methods);
    printf("  loffset:    %u\n",   loffset);
    printf("  minstack:   %u\n",   minstack);
    printf("  maxstack:   %u\n",   maxstack);
    printf("  nesting:    %u\n",   nesting);
    printf("  maxnest:    %u\n",   maxnest);
    printf("  heap_count: %u\n",   heap_count);
    printf("  line_count: %u\n",   line_count);
    printf("  tfd:        %u\n",   tfd);
    printf("  table:      %04X\n", table);
    printf("  crp:        %04X\n", crp);
    printf("  frp:        %04X\n", frp);
    printf("  lrp:        %04X\n", lrp);
    printf("  fnsort:     %04X\n", fnsort);
    printf("  fnstrcmp:   %04X\n", fnstrcmp);

    for (i = 0; i < methods; ++i)
	{
	PSMETHOD p = &method[i];

	printf("SMETHOD %u:\n", i+1);
	showtype(p->type);
	printf("  order:      %u\n", p->order);
	printf("  field1:     %u\n", p->field1);
	printf("  column1:    %u\n", p->column1);
	printf("  max1:       %u\n", p->max1);
	printf("  field2:     %u\n", p->field2);
	printf("  column2:    %u\n", p->column2);
	printf("  max2:       %u\n", p->max2);
	printf("  base:       %u\n", p->base);
	}
    printf("\n");
    fflush(stdout);
    }

/* ----------------------------------------------------------------------- *\
|  showalllines () - Print all of the configured lines
\* ----------------------------------------------------------------------- */
    static void
showalllines (void)

    {
    int	 line;

    for (line = 0; line < line_count; ++line)
	showline(line, table[line]);

    printf("\n");
    fflush(stdout);
    exit(0);
    }

/* ----------------------------------------------------------------------- *\
|  showline () - Print one confingured line
\* ----------------------------------------------------------------------- */
    static void
showline (
    int	       line,
    PSORTDATA  p)

    {
    int	       meth;

    printf("Line %3d: \"%s\"\n", line, PTR_TO_LINE_DATA(p));

    for (meth = 0; (meth < methods); ++meth)
	{
	PIMETHOD pim = (PTR_TO_IMETHOD(p)) + meth;

	showtype(pim->type);
	switch (pim->type)
	    {
	    case SORT_TYPE_STRING:
		printf("  offset:     %u\n", pim->u.s.offset);
		printf("  length      %u\n", pim->u.s.length);
		break;

	    case SORT_TYPE_NUMERIC:
		printf("  value:      %lu\n", pim->u.value);
		break;

	    case SORT_TYPE_DATE:
		printf("  value:      %lu\n", pim->u.value);
		break;
	    }
	}
    }

/* ----------------------------------------------------------------------- *\
|  showtype () - Print the method type as a string
\* ----------------------------------------------------------------------- */
    static void
showtype (
    BYTE  type)

    {
    switch (type)
	{
	case SORT_TYPE_STRING:
	    printf("  type:       string (%u)\n", type);
	    break;

	case SORT_TYPE_NUMERIC:
	    printf("  type:       numeric (%u)\n", type);
	    break;

	case SORT_TYPE_DATE:
	    printf("  type:       date (%u)\n", type);
	    break;

	default:
	    printf("  type:       UNKNOWN (%u)\n", type);
	    break;
	}
    }

#endif
/* ----------------------------------------------------------------------- */
