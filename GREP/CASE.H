#if !defined(_CASE_INCLUDED)

#define  _CASE_INCLUDED
#define  CASE_TABLE_SIZE        (256)
#define  adjust_case(c)    _case_adjust_table[c]

static  char    _case_adjust_table [CASE_TABLE_SIZE];

    static void
build_case_table (
    int  ignorecase)

    {
    int  i;

    for (i = 0; i < CASE_TABLE_SIZE; ++i)
        _case_adjust_table[i] = (ignorecase)
	    ? (tolower((char)(i)))
	    : (char)(i);
    }

#undef  CASE_TABLE_SIZE
#endif
