/*                      FILE  sr_exter.h                                  */
/*Copyright 1989, 1991, 1993 by English Knowledge Systems, Inc. All Rights Reserved.*/

#ifndef SR_EXTER

#define SR_EXTER 1

#define ESCCHAR '\\'

#define SR_MAX_REP_BUF  4096    /* The size of the temperary buffer to hold*/
                                /* the new string while a search and       */
                                /* replace is taking place.                */

extern char *Sr_assign[26];     /* assigned variable pointers             */
extern short Sr_metaflag;       /* whether metacharacters are on or off   */
extern char  Sr_eol;            /* additional end-of-line (e.g. newline)  */
extern short Sr_shortmatch;     /* Do a short or a longest match.         */

/* This is the structure of a compiled pattern.                           */
#define SR_CPAT struct pnode
SR_CPAT {
        SR_CPAT *parent;     /* parent pointer                            */
        SR_CPAT *left;       /* left  operand pointer                     */
        SR_CPAT *right;      /* right operand pointer                     */
        char    *asgpos;     /* the assigned postion                      */
        char    *asgstr;     /* the saved assign string.                  */
        short    alter;      /* alternative count                         */
        short    ileft;      /* left integer                              */
        short    iright;     /* right integer                             */
        char     type;       /* type field                                */
        };

char    *sr_s   (char *buf, char *pat);
int      sr_sr  (char *buf, char *pat,     char *replace);
int      sr_srg (char *buf, char *pat,     char *replace);

char    *sr_cs  (char *buf, SR_CPAT *cpat);
int      sr_csr (char *buf, SR_CPAT *cpat, char *replace);
int      sr_csrg(char *buf, SR_CPAT *cpat, char *replace);

SR_CPAT *sr_compile(char *pat);
int      sr_search(SR_CPAT *re, char *str, char **str_st);
int      sr_replace(char *old_str,
                    char *old_str_st,
                    int   len,
                    char *new_str,
                    char *sub_str);

void     sr_free_re(SR_CPAT *re);
void     sr_free_assign(void);

void     sr_treep(SR_CPAT *tree, SR_CPAT *p, short indent); 

#endif
