/* ----------------------------------------------------------------------- *\
|
|  SORTS.H  - Header file for the sort subsystem
|
|  30-Mar-93
|  28-Aug-93
|  31-Oct-93
|
|  Copyright 1993 by Brian W. Johnson, all rights reserved
|
\* ----------------------------------------------------------------------- */

#define  SORT_ERR_NONE			(0)
#define  SORT_ERR_COLUMN_NOT_FOUND	(1)
#define  SORT_ERR_DATE_SYNTAX		(2)
#define  SORT_ERR_FIELD_NOT_FOUND	(3)
#define  SORT_ERR_LENGTH_ZERO		(4)
#define  SORT_ERR_SORT_METHOD_OVERFLOW	(5)
#define  SORT_ERR_TOO_LONG		(6)
#define  SORT_ERR_VALUE_SYNTAX		(7)

extern	int	sort_init	(void);
extern	int	sort_parms	(unsigned int stable, unsigned int verbose, unsigned int white,
				 unsigned int ignorecase, int order, int tabsize);
extern	int	sort_string	(unsigned int field,  unsigned int column, unsigned int colmax, int order);
extern	int	sort_numeric	(unsigned int field,  unsigned int column, unsigned int colmax, unsigned int base, int order);
extern	int	sort_date	(unsigned int field1, unsigned int field2, int order);
extern	void	sort_set_temp	(char *pfilename);
extern	void	sort_stats	(void);
extern	void	sort_terminate	(void);

extern	char   *sort_error	(int error_code);

extern	int	sort_put_line	(char *pstring);
extern	int	sort_get_line	(char **ppstring);

/* The main program must provide the following function */

extern	void	exitmsg		(int code, char *fmt, ... );

/* ----------------------------------------------------------------------- */
