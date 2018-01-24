/*                    FILE "sr_engin.c"                         */
/*
 *      This program is the CONFIDENTIAL and PROPRIETARY property
 *      of English Knowledge Systems Inc.  Any unauthorized use,
 *      reproduction or transfer of this program is strictly prohibited.
 *
 *      Copyright (c) 1988 1989 1990 1992 English Knowledge Systems, Inc.
 *      This is an unpublished work, and is subject to limited distribution and
 *      restricted disclosure only. ALL RIGHTS RESERVED.
 *
 *                      RESTRICTED RIGHTS LEGEND
 *      Use, duplication, or disclosure by the Government is subject to
 *      restrictions set forth in subparagraph (c)(1)(ii) of the Rights in
 *      Technical Data and Computer Software clause at DFARS 252.227-7013.
 *              English Knowledge Systems Inc.
 *              5525 Scotts Valley Dr. #22
 *              Scotts Valley, CA 95066
 *
 *      The REGX PLUS package may be compiled into an application
 *      and the application sold without royalty payments.  If you 
 *      wish to resell the source code we would be happy to work
 *      out a per copy or a one time licensing fee so that you may 
 *      do so.
 *                              James H. Conklin
 *                              President
 *                              English Knowledge Systems Inc.
 */

/*
 *      9-20-91 JHC
 *      ADDITIONS:
 *      1.  Added the global variable Sr_shortmatch.  If set to 1, this
 *          variable will have * and + match the shortest strings found
 *          instead of the longest.
 *
 *      2.  Added more error checking to the function calls.
 *
 *      3.  Added more debugging information.
 *
 *      4.  Added more documentation.
 *
 *      CHANGES:
 *      1.  Changed the types of the interface routines.  The compiled
 *          pattern used to be passed as a char * .  It is now passed
 *          as SR_CPAT * .  This change will enhance the ability of the
 *          compiler to catch errors in usage.
 *
 *      2.  Changed the data structure that is used to store information 
 *          internally.  This change will make ports easier.
 *
 *      3.  Changed some of the code formatting to enhance readability.
 *
 *      FIXES:
 *      1.  A fix that makes sure all nodes will always be freed by
 *          sr_search() when apropriate.
 *
 *      2.  A fix that makes sure all nodes will always be freed by
 *          sr_free_re() whenever a pattern with an ASSIGN is given.

 *      2-12-93 JHC
 *      ADDITIONS:
 *      1.  New routines.
 *              sr_s(str, pat)          search
 *              sr_sr(str, pat, rep)    search and replace
 *              sr_srg(str, pat, rep)   search and replace global
 *              sr_cs(str, pat)         compiled pattern search
 *              sr_csr(str, pat, rep)   compiled pattern search and replace
 *              sr_csrg(str, pat, rep)  compiled pattern search and replace global
 *              sr_free_assign()        frees all memory in assigns.
 * 
 *      2.  Added a new internal debugging routine.
 * 
 *      3.  Added more examples to the documentation.
 * 
 *      4.  Provide the documentation in Microsoft word for windows
 *          and laser jet 2 formats as well as in ascii.
 * 
 *      5.  Added some code examples of the routines in use.
 * 
 *      6.  Allow extended character codes from 128 to 256 to be allowed 
 *          in character classes.
 * 
 *      CHANGES:
 *      1.  Made the existing parse tree print routine available 
 *          outside the search and replace routines.
 * 
 *      2.  Changed the data structure that is used to store information 
 *          internally.  This change will make ports easier.
 *
 *      3.  Changed some code to get rid of some more lint messages.
 *
 *      4.  Changed the name of this file.
 *
 *      5.  Changed the name of the header file.
 *
 *      FIXES:
 *      1.  A fix that frees any memory saved in the assign variables
 *          every time a new search is started.
 *      2.  A fix to the scanner when dealing with a highly embedded set
 *          of replications.
 *      3.  Fixed problem with some special characters not being able
 *          to be escaped.
 *      4.  Fix to unassign assingments that were made when a sub pattern
 *          which had done an assign fails.
 */

/*
 * A DETAILED DESCRIPTION OF PARSE TREE STRUCTURE
 *
 *   The normal use of the fields in a parse tree node is as described above.
 *   Exceptions to this use are described below.
 *
 *   ALT and CAT:
 *       For these two types of nodes, the left and right branches are actually
 *       of type (SR_CPAT *), and point to sub-parse trees.  alter is used
 *       in scanning as a flag to indicate which branch is being executed.
 *
 *   ASSIGN:
 *       The left branch is a (SR_CPAT *) pointing to a sub-tree.  iright
 *       is a (char) with value a-z.  The asgpos is used as (char *)
 *       pointing to beginning of the assignment location.
 *
 *   REP1 and REP2:
 *       The left branch is a (SR_CPAT *) pointing to a sub-tree.  The
 *       right branch is a (SR_CPAT *) pointing to a single node which
 *       has the replication information.  In that node, ileft is an
 *       (int) specifying the lower bound on replication; iright is an
 *       (int) specifying the upper bound.  Also in the sub-node, asgpos
 *       is a (char *) pointing to the place in the string being scanned
 *       where the current try at replication began.  It is included in order
 *       to prevent the replication from matching the null string an infinite
 *       number of times.  The parent and type fields are unused in the
 *       sub-node.  In the REP1 or REP2 node, the alter field is used as a
 *       count on number of replications.
 *
 *   CHARS1 and CHARS2:
 *       ileft is a (char) specifying which character to match.  The right
 *       branch and the alter flag are as in REP1 and REP2, except that
 *       the alter flag is unused in the sub-node.
 *
 *   CCLASS1 and CCLASS2:
 *       The left branch is a (SR_CPAT *) pointing to a description
 *       of the character class (see CCLASS).  The right branch and alter
 *       flags are as in CHARS1 and CHARS2.
 *
 *   ARB1 and ARB2:
 *       The left branch is unused, and the right branch and alter flags
 *       are as in CHARS1 and CHARS2.
 *
 *   CHARS:
 *       ileft is a (char) specifying which character to match. The
 *       right branch is a (SR_CPAT *) pointer to additional sequences
 *       of CHARS, CCLASS, ARBITRARY, POSITION, and VARIABLE nodes
 *       (since all of these can be processed linearly with no need
 *       to backtrack).  The alter flag is unused.

 *   CCLASS:
 *       The left branch is a (SR_CPAT *) pointer to a chain of
 *       character class ranges.  In each node in the chain, ileft and
 *       alter fields are the (char) lower and upper bounds on characters
 *       accepted, and the right branch is a (SR_CPAT *) to more
 *       links in the chain, with the parent field unused.  The chain of
 *       ranges is in ascending order, to shorten time needed to look.
 *       The right branch and alter flag are as in CHARS:
 *
 *   ARBITRARY:
 *       The left branch and alter flag are unused.  The right branch is
 *       as in CHARS.
 *
 *   POSITION:
 *       The left branch is a (SR_CPAT *) pointer to a chain of
 *       position ranges.  In each node in the chain, the ileft and
 *       alter fields are the (int) bounds on positions.  It is impossible
 *       to tell a priori which is the lower and which is the upper bound
 *       since it may depend on the length of the string being scanned.
 *       In the chained nodes, the right branch is a (SR_CPAT *)
 *       pointer to the rest of the chain.  In the POSITION node, the
 *       right branch and alter flag are as in CHARS.
 *
 *   VARIABLE:
 *       ileft is a (char) from a-z indicating which variable to expand.
 *       The alter flag is unused, and the right branch is as in CHARS.
 *
 *
 *
 *  USE OF NODES IN SCANNING:
 *
 *   In scanning, use is made of a linked list stack of free parse tree nodes
 *   in order to do backtracking.  Normally, the parent pointer points back
 *   to the previous entry on the stack.  The left branch is a (SR_CPAT *)
 *   pointer to the tree position to which to return.  The right branch is a
 *   (char *) pointing to the place in the string being scanned at which to
 *   continue the scanning process.  The alter flag is the saved alter flag
 *   of the node in the tree to which the return is to be made.
 *
 *   The exception to this is when the type of the tree position is the
 *   special value RESTORE (which does not occur in the tree).  In that case,
 *   the right branch is a (SR_CPAT *) pointer to a node in the tree
 *   and the alter flag is to be restored to that node.  This gives a
 *   mechanism to distinguish between just resetting the alter flag value
 *   (which is necessary sometimes, even when the node is not a generator)
 *   and a FAILURE return to a node which will try another choice.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "sr_exter.h"

/* LIST OF TOKENS                                                       */
#define END         0
#define ALT         1           /* alternation                          */
#define CAT         2           /* concatenation                        */
#define ASSIGN      3           /* assignment                           */
#define REP1        4           /* repetition count type 1              */
#define REP2        5           /* repetition count type 2              */
#define CHARS       6           /* characters                           */
#define CCLASS      7           /* character class                      */
#define ARBITRARY   8           /* arbitrary                            */
#define POSITION    9           /* position from beginning of string    */
#define VARIABLE   10           /* variable                             */
#define OPEN       11           /* (                                    */
#define SR_CLOSE   12           /* )                                    */

/*
 *  The above token values are used as indices into the precedence functions
 *  to determine when reductions should occur.  They are also used during
 *  scanning as the node type.
 *
 *  The following are not returned by gettoken(), but are node types used
 *  during scanning.
 */
#define CHARS1     13           /* CHARS repetition count type 1        */
#define CHARS2     14           /* CHARS repetition count type 2        */
#define CCLASS1    15           /* CCLASS repetition count type 1       */
#define CCLASS2    16           /* CCLASS repetition count type 2       */
#define ARB1       17           /* ARB repetition count type 1          */
#define ARB2       18           /* ARB repetition count type 2          */
#define RESTORE    19           /* special value for FAILURE in scanning*/

/* MACROS FOR EASY REFERENCE FOR REPLICATIONS */
#define LOWER(t)    (t->right->ileft)
#define UPPER(t)    (t->right->iright)
#define BEGIN(t)    (t->right->asgpos)

#define TRUE    1
#define FALSE   0
#define BOOL    short
#define FAILURE 0
#define SUCCESS 1
#define FIRST   2
#define EOS     '\0'
#define GETNODE 20              /* The number of nodes to malloc each time.*/

/* #define      SR_DEBUG 1       Remove this define to remove debugging code.   */
#ifdef SR_DEBUG
static  short Kntnode = 0;
#define TYPES   19
#endif


static SR_CPAT Restore = {
    NULL,   NULL,   NULL,   NULL,   NULL,  0,  0,  0,  RESTORE,
    } ;

/* These are the four external globals that the program gets to use.    */
BOOL     Sr_shortmatch = FALSE;
BOOL     Sr_metaflag   = TRUE;
char     Sr_eol        = '\0';
char    *Sr_assign[26] = {
    NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,
    NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,
    NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,
    } ;


/* This is the structure of a parse stack entry.                        */
struct  pstack {
        char     type;          /* type of stack entry (token type)     */
        BOOL     term;          /* terminal or not                      */
        short    newval;        /* character value from gettoken()      */
        SR_CPAT *val;           /* pointer to value                     */
        };

/* This is the pointer to the top of the scanning stack saved by generators */
static  SR_CPAT *Stacksc;       /* The stack for the scanner.           */
static  short   *Nextch;        /* A processed copy of the pattern.     */
static  BOOL    *Flags;         /* A set of flags that goes with Nextch.*/
static  short    Indexsc;       /* Index into the string being complied.*/

/* The following routines are used only in compiling the RE.            */
static  char     gettoken(struct pstack *stack, short last);
static  short    getint(short *indpt);
static  BOOL     getpos(short *indpt, short *valpt);
static  SR_CPAT *pnode(void);

/* The following routines are used only in searching the string.        */
static  char   *scanner(SR_CPAT *cpat, char *subj, char *start);
static  char   *pop(SR_CPAT **t);
static  BOOL    push(SR_CPAT *t, char *pos);
static  BOOL    pusher(SR_CPAT *t);
static  BOOL    inrange(short c, SR_CPAT *t);
static  BOOL    posscan(SR_CPAT *t, char **pos, char *subject);
static  void    scan_free(SR_CPAT *point);

/* The following routines are used in both scanning and compiling.      */
static  BOOL    decipher(char *s);
static  BOOL    special(short c);
static  void    freepnode(SR_CPAT *p);
static  void    freerange(SR_CPAT *tree);
static  void    fatal(char *s);

#ifdef SR_DEBUG
/* The following routines are used only for debugging.                  */
        void    sr_treep(SR_CPAT *tree, SR_CPAT *p, short indent);
static  void    stack_prn(char *call_from);
static  void    stackp(struct pstack stack[], short nstack);
static  void    rangep(SR_CPAT *t);
static  void    pp(short i);
static  void    swerr(char *cp, short val);
#define DEBUGS(cp, val) swerr(cp, val)
#else
#define DEBUGS(cp, val)
#endif

/*
 * Do a search on the input buffer.
 * Return the next position in str after the matched pattern.
 */
char *
sr_s(str, pat)
char    *str;                   /* The input string to do the search on.*/
char    *pat;                   /* The pattern to search for.           */
{

        int      len;           /* The length of the match.             */
        char    *found;         /* Where in str the match was found.    */
        SR_CPAT *cpat;          /* The compiled pattern.                */

        cpat = sr_compile(pat);
        len  = sr_search(cpat, str, &found);
        sr_free_re(cpat);       /* Free the compiled pattern.           */
        if (len == -1)
                return((char *)NULL);

        return(&found[len]);
        }


/*
 * Do a search and a replace once on the input buffer.
 */
int
sr_sr(str, pat, sub)
char    *str;                   /* The input string to do the search on.*/
char    *pat;                   /* The pattern to search for.           */
char    *sub;                   /* The replacement string.              */
{

        int      len;           /* The length of the match.             */
        char    *found;         /* Where in str the match was found.    */
        SR_CPAT *cpat;          /* The compiled pattern.                */
        char     buf[SR_MAX_REP_BUF];   /* Buffer for the changed string.       */

        cpat = sr_compile(pat);
        len  = sr_search(cpat, str, &found);
        sr_free_re(cpat);       /* Free the compiled pattern.           */
        if (len == -1)
                return(FALSE);

        sr_replace(str, found, len, buf, sub);
        strcpy(str, buf);
        return(TRUE);
        }

/*
 * Do a search and a replace globally on the input buffer.
 * Return the number of patterns that were matched in the string.
 */
int
sr_srg(str, pat, sub)
char    *str;                   /* The input string to do the search on.*/
char    *pat;                   /* The pattern to search for.           */
char    *sub;                   /* The replacement string.              */
{

        int      len;           /* The length of the match.             */
        char    *found;         /* Where in str the match was found.    */
        SR_CPAT *cpat;          /* The compiled pattern.                */
        char     buf[SR_MAX_REP_BUF];   /* Buffer for the changed string.       */
        int      num_found = 0; /* The number of patterns found.        */

        cpat = sr_compile(pat);
        len  = sr_search(cpat, str, &found);
        if (len == -1) {
                sr_free_re(cpat);       /* Free the compiled pattern.   */
                return(0);
                }

        sr_replace(str, found, len, buf, sub);
        while ((len = sr_search(cpat, str, &found)) != -1) {
                if (sr_replace(str, found, len, buf, sub))
                        strcpy(str, buf);

                num_found++;
                }

        sr_free_re(cpat);       /* Free the compiled pattern.           */
        return(num_found);
        }

/*
 * Do a search on the input buffer.
 * Return the next position in str after the matched pattern.
 */
char *
sr_cs(str, cpat)
char    *str;                   /* The input string to do the search on.*/
SR_CPAT *cpat;                  /* The compiled pattern to search for.  */
{

        int      len;           /* The length of the match.             */
        char    *found;         /* Where in str the match was found.    */

        len = sr_search(cpat, str, &found);
        if (len == -1)
                return((char *)NULL);

        return(&found[len]);
        }


/*
 * Do a search and a replace once on the input buffer.
 */
int
sr_csr(str, cpat, sub)
char    *str;                   /* The input string to do the search on.*/
SR_CPAT *cpat;                  /* The compiled pattern to search for.  */
char    *sub;                   /* The replacement string.              */
{

        int      len;           /* The length of the match.             */
        char    *found;         /* Where in str the match was found.    */
        char     buf[SR_MAX_REP_BUF];   /* Buffer for the changed string.       */

        len = sr_search(cpat, str, &found);
        if (len == -1)
                return(FALSE);

        sr_replace(str, found, len, buf, sub);
        strcpy(str, buf);
        return(TRUE);
        }

/*
 * Do a search and a replace globally on the input buffer.
 * Return the number of patterns that were matched in the string.
 */
int
sr_csrg(str, cpat, sub)
char    *str;                   /* The input string to do the search on.*/
SR_CPAT *cpat;                  /* The compiled pattern to search for.  */
char    *sub;                   /* The replacement string.              */
{

        int      len;           /* The length of the match.             */
        char    *found;         /* Where in str the match was found.    */
        char     buf[SR_MAX_REP_BUF];   /* Buffer for the changed string.       */
        int      num_found = 0; /* The number of patterns found.        */

        len = sr_search(cpat, str, &found);
        if (len == -1) {
                return(0);
                }

        sr_replace(str, found, len, buf, sub);
        while ((len = sr_search(cpat, str, &found)) != -1) {
                if (sr_replace(str, found, len, buf, sub))
                        strcpy(str, buf);

                num_found++;
                }

        return(num_found);
        }

/*      sr_search() takes the pattern compiled by sr_compile() and searches
 *      the string str.  If an instance of the pattern is found,
 *      sr_search() returns the length of the match and sets str_st to point
 *      to the first character of the match in str.  The application can use
 *      this information to show the user exactly what was matched in the line.
 *      If the user doesn't wish to do a substitution at that point, sr_search()
 *      can be called again with str set to (str_st + length_of_match) which
 *      will then continue the search from where the last match was found.
 */
int
sr_search(re, str, str_st)
SR_CPAT *re;             /* Compiled pattern from sr_compile().      */
char    *str;            /* The string to search for the pattern.    */
char   **str_st;         /* Returns where the match was found in str.*/
{
        char    *start = str;
        char    *c;
        short    i;
        SR_CPAT *t = re;

        if (re     == NULL ||
            str    == NULL ||
            str_st == NULL)
                return(-1);

        sr_free_assign();

        while (t->type == CAT)
                t = t->left;

        if (t->type == CHARS)
                i = t->ileft;
        else
                i = 0;

        /* This loop moves along the input string str one position at a */
        /* time trying to see if the compiled pattern will match at that*/
        /* position.  scanner() will match a compiled pattern at the    */
        /* beginning of a string.                                       */
        for (;;) {
                if (i)  /* fast loop to find first character            */
                        while (*str != i   &&
                               *str != EOS &&
                               *str != Sr_eol)
                                        str++;

                c = scanner(re, str, start);
                if (c != NULL) {
                        *str_st = str;
                        return((int)(c - str));
                        }

                if (*str == EOS ||
                    *str == Sr_eol)
                        break;

                str++;
                }
        return(-1);
        }

/*
 *      This is a very straightforward routine.  You put one pointer (c)
 *      at the beginning of the old string, and the other pointer (d)
 *      at the beginning of the new string.  You copy from the old string
 *      until you get to where the match began.  You substitute the
 *      new string (with variable referencing, if any).  And then you
 *      copy the rest of the old string over.
 */
int
sr_replace(old_str, old_str_st, len, new_str, sub_str)
char    *old_str;       /* The original string.                         */
char    *old_str_st;    /* Where to start the substitution.             */
int      len;           /* The length of the match in the old_str.      */
char    *new_str;       /* Pointer to a buffer for the new string.      */
char    *sub_str;       /* The substitution string.                     */
{
        char    *c;     /* Pointer to the old string.                   */
        char    *d;     /* Pointer to the new string.                   */
        short    index;

        if (old_str    == NULL ||
            old_str_st == NULL ||
            new_str    == NULL ||
            sub_str    == NULL ||
            len        <  0)
                return(FALSE);

        if (!decipher(sub_str))
                return(FALSE);

        /* The only special character allowed is '<' so                 */
        /* turn off all other special characters and                    */
        /* check syntax first before starting to substitute.            */
        for (index = 0; Nextch[index]; index++)
                if (Flags[index]) {
                        if (Nextch[index] != '<') {
                                Flags[index] = FALSE;
                                continue;
                                }

                        index++;
                        if (isupper(Nextch[index]))
                                Nextch[index] = tolower(Nextch[index]);
                        else if (!islower(Nextch[index]))
                                goto err;
                        index++;
                        if (Nextch[index] != '>')
                                goto err;
                        }

        /* do substitution */
        d = new_str;
        for (c = old_str; c < old_str_st; )
                *d++ = *c++;

        for (index = 0; Nextch[index]; index++) {
                if (Flags[index]) {
                        index++;
                        c = Sr_assign[Nextch[index++] - 'a'];
                        if (c == NULL)
                                continue;

                        while (*c)
                                *d++ = *c++;
                        }

                else
                        *d++ = (char)Nextch[index];
                }

        strcpy(d, old_str_st + len);
        free(Nextch);
        free(Flags);
        return(TRUE);

err:
        free(Nextch);
        free(Flags);
        return(FALSE);
        }

/*
 *      This frees the memory allocated by scanner()
 */
static
void
scan_free(point)
SR_CPAT *point;
{
        SR_CPAT *tree;
        SR_CPAT *t;

        /* Check for NULL value.                                        */
        if (point == NULL)
                return;

        tree = point;
        while (tree != NULL) {
                t = tree->parent;
                freepnode(tree);
                tree = t;
                }

        return;
        }

/*
 *      This frees the memory allocated by sr_compile()
 */
void
sr_free_re(point)
SR_CPAT *point;
{
        SR_CPAT *tree;

        /* Check for NULL value.                                        */
        if (point == NULL)
                return;

        tree = point;
        freepnode(tree);
        switch (tree->type) {
                case ASSIGN:
                        sr_free_re(tree->left);
                        break;

                case ALT:
                case CAT:
                        sr_free_re(tree->left);
                        sr_free_re(tree->right);
                        break;

                case REP1:
                case REP2:
                        sr_free_re(tree->left);
                        freepnode(tree->right);
                        break;

                case CHARS1:
                case CHARS2:
                case ARB1:
                case ARB2:
                        freepnode(tree->right);
                        break;

                case CCLASS1:
                case CCLASS2:
                        freepnode(tree->right);
                        freerange(tree->left);
                        break;

                case CHARS:
                case ARBITRARY:
                case CCLASS:
                case POSITION:
                case VARIABLE:
                        for (;;) {
                                if (tree->type == CCLASS ||
                                    tree->type == POSITION)
                                        freerange(tree->left);

                                tree = tree->right;
                                if (tree == NULL)
                                        break;

                                freepnode(tree);
                                }
                        break;

                default:
                        DEBUGS("sr_free_re", tree->type);
                        break;
                }
        }

/*
 * Free any memory that has been allocated to the Sr_assign array.
 */
void
sr_free_assign() {

        short i;

        for (i = 0; i < 26; i++) {
                if (Sr_assign[i] != NULL) {
                        free(Sr_assign[i]);
                        Sr_assign[i] = (char *)NULL;
                        }
                }

        return;
        }

/*
 *   The parsing scheme uses an operator precedence grammar.
 *
 *   In the following comments the productions, precedence matrix, and
 *   precedence functions are listed.
 *
 *   The scanning scheme is a backtracking one which remembers where
 *   earlier alternatives were chosen. If the match should fail,
 *   the algorithm can retreat back to the last place where a different
 *   alternative could have been selected and try again.
 *
 *   For a description of how backtracking works, the SNOBOL and ICON
 *   programming languages are excellent instructors in the use of
 *   backtracking, particularly in string analysis.
 *
 *   LIST OF PRODUCTIONS
 *
 *      Production               Number             Schematic
 *
 *      OR      OR | AND            0                 N | N
 *              AND
 *      AND     AND & UNARY         1                 N & N
 *              UNARY
 *      UNARY   UNARY *      --
 *              UNARY +        |--  2                 N REP-COUNT-1
 *              UNARY {n,}     |--  3                 N REP-COUNT-2
 *              UNARY {n}      |                      (whether 2 or 3 depends
 *              UNARY {,m}     |                      on n compared to m)
 *              UNARY {n,m}  --
 *              UNARY $ CHARS       4                 N $ CHARS
 *              TERM
 *      TERM    ( OR )              5                 ( N )
 *              CHARS               6                 CHARS
 *              CHAR-CLASS          7                 CHAR-CLASS
 *              ARBITRARY           8                 ARBITRARY
 *              POSITION            9                 POSITION
 *              VARIABLE           10                 VARIABLE
 *
 *      Precedence Matrix
 *          R1 = REP-COUNT-1
 *          R2 = REP-COUNT-2
 *          C  = CHARS
 *          CC = CHAR-CLASS
 *          A  = ARBITRARY
 *          P  = POSITION
 *          V  = VARIABLE
 *
 *               2  4  6  6  6  6  6  6  6  6  6  1
 *               |  &  $  R1 R2 C  CC A  P  V  (  )
 *      3   |    >  <  <  <  <  <  <  <  <  <  <  >
 *      5   &    >  >  <  <  <  <  <  <  <  <  <  >
 *      6   $                   =
 *      7   R1   >  >  >  >  >                    >
 *      7   R2   >  >  >  >  >                    >
 *      7   C    >  >  >  >  >                    >
 *      7   CC   >  >  >  >  >                    >
 *      7   A    >  >  >  >  >                    >
 *      7   P    >  >  >  >  >                    >
 *      7   V    >  >  >  >  >                    >
 *      1   (    <  <  <  <  <  <  <  <  <  <  <  =
 *      7   )    >  >  >  >  >                    >
 *
 */

/* PRECEDENCE FUNCTIONS                                            */
/*                  END |  &  $  R1 R2 C  CC A  P  V  (  )         */
static char ff[] = { 0, 3, 5, 6, 7, 7, 7, 7, 7, 7, 7, 1, 7, } ;
static char gg[] = { 0, 2, 4, 6, 6, 6, 6, 6, 6, 6, 6, 6, 1, } ;

/* ENCODING OF PRODUCTIONS */

/* Productions are reversed because the stack works from the most       */
/* recently pushed at the top.  Comparisons are done from the top of    */
/* the stack down in order to get a reduction.                          */
/*                                                                      */
/* In the N arrays, 0 signifies a non-terminal position.                */
/*                  1 signifies a terminal position.                    */
/* In the M arrays, 0 fills in non-terminal positions.                  */
/*                  the terminal needed for the particular production   */
/*                  is in the terminal position.                        */
/* The last entry in both the N and M arrays is a dummy value that      */
/* facilitates searching for which reduction to apply.                  */
static char N00[] = {0, 1, 0, 1, },        M00[] = {0, ALT, 0, 0, } ;
static char N01[] = {0, 1, 0, 1, },        M01[] = {0, CAT, 0, 0, } ;
static char N02[] = {1, 0, 1, },           M02[] = {REP1, 0, 0, } ;
static char N03[] = {1, 0, 1, },           M03[] = {REP2, 0, 0, } ;
static char N04[] = {1, 1, 0, 1, },        M04[] = {CHARS, ASSIGN, 0, 0, } ;
static char N05[] = {1, 0, 1, 1, },        M05[] = {SR_CLOSE, 0, OPEN, 0, } ;
static char N06[] = {1, 1, },              M06[] = {CHARS, 0, } ;
static char N07[] = {1, 1, },              M07[] = {CCLASS, 0, } ;
static char N08[] = {1, 1, },              M08[] = {ARBITRARY, 0, } ;
static char N09[] = {1, 1, },              M09[] = {POSITION, 0, } ;
static char N10[] = {1, 1, },              M10[] = {VARIABLE, 0, } ;

#define SR_STACK   20              /* size of stack */
#define NPROD   11
static struct {
        char    *terms;
        char    *types;
        } prod[NPROD] = {
        {N00,    M00},            /* N | N => N */
        {N01,    M01},            /* N & N => N */
        {N02,    M02},            /* N R1  => N */
        {N03,    M03},            /* N R2  => N */
        {N04,    M04},            /* N $ C => N */
        {N05,    M05},            /* ( N ) => N */
        {N06,    M06},            /* C     => N */
        {N07,    M07},            /* CC    => N */
        {N08,    M08},            /* A     => N */
        {N09,    M09},            /* P     => N */
        {N10,    M10},            /* V     => N */
        } ;

static SR_CPAT *pfree = NULL;      /* beginning of list of free nodes   */

/*
 *      This routine returns the parsed form of the search patern
 *      that it was passed.
 */
SR_CPAT *
sr_compile(s)
char *s;
{
    struct   pstack stack[SR_STACK];
    SR_CPAT *new;
    SR_CPAT *trace1;
    SR_CPAT *trace2;
    short      nstack;
    short      i;
    short      j;
    short      k;
    short      next;
    short      lastt = END;    /* Last type.                */
    short      nextt = END;    /* Next type.                */

    if (s == NULL)
        return(NULL);

    if (!decipher(s))
        return(NULL);

    Indexsc         = 0;
    stack[0].type   = END;
    stack[0].newval = 0;
    stack[0].term   = TRUE;
    stack[0].val    = NULL;
    nstack          = 1;
    next            = END;
    for (;;) {
        stack[nstack].type = (char)(next = gettoken(&stack[nstack], next));
        if (next < 0)
            goto err;

        for (;;) {
            /*  find top terminal on the stack */
            for (i = nstack - 1; !stack[i].term; i--)
                ;

            /* see if should shift or reduce */
            if (ff[stack[i].type] <= gg[next])
                break;

            /* perform reduction */
            for (i = 0; i < NPROD; i++) {
                j = nstack - 1;
                k = 0;
                for (;;) {
                    if (stack[j].term != prod[i].terms[k])
                        break;

                    if (stack[j].term) {
                        /* Note that you will come through this         */
                        /* place at least twice before having the       */
                        /* prod[i].types[k] == 0 test succeed (which is */
                        /* the only place where lastt is used).         */
                        lastt = nextt;
                        nextt = stack[j].type;
                        if (prod[i].types[k] == 0) {
                            if (ff[nextt] < gg[lastt])
                                goto reduce;

                            break;
                            }

                        if (nextt != prod[i].types[k])
                                break;
                        }

                    j--;
                    k++;
                    }

                continue;
                }

            goto err;

reduce:
            switch(i) {
                case 1:             /* N & N => N */
                    /* The code is a little involved here in the interest */
                    /* of speeding up the scanning time later.            */
                    /* If you have a sequence of characters,              */
                    /* character classes, arbitraries, positions, and     */
                    /* variables, they are put into a linked list.        */
                    /* Scanning is quick since each of these can be       */
                    /* tested without worrying about backtracking.        */
                    trace1 = stack[nstack-3].val;
                    while (trace1->type == CAT)
                        trace1 = trace1->right;
                    if (trace1->type != CHARS     &&
                        trace1->type != CCLASS    &&
                        trace1->type != ARBITRARY &&
                        trace1->type != POSITION  &&
                        trace1->type != VARIABLE)
                                goto cat;

                    while (trace1->right != NULL)
                        trace1 = trace1->right;

                    trace2 = stack[nstack-1].val;
                    if (trace2->type == CHARS     ||
                        trace2->type == CCLASS    ||
                        trace2->type == ARBITRARY ||
                        trace2->type == POSITION  ||
                        trace2->type == VARIABLE)  {
                            trace1->right   = trace2;
                            stack[nstack-2] = stack[nstack];
                            nstack         -= 2;
                            break;
                            }

cat:            /* fall through and handle ordinary case                 */
                case 0:             /* N | N => N */
                    new                         = pnode();
                    new->type                   = stack[nstack-2].type,
                    new->left                   = stack[nstack-3].val;
                    new->right                  = stack[nstack-1].val;
                    stack[nstack-3].val->parent = new;
                    stack[nstack-1].val->parent = new;
                    stack[nstack-3].val         = new;
                    stack[nstack-2]             = stack[nstack];
                    nstack                     -= 2;
                    break;

                /* REPLICATION CODE (Cases 2 and 3)                     */
                /* The code is a little opaque here too since I         */
                /* contract replications of single characters, single   */
                /* character classes and a single arbitrary into a new  */
                /* node with a different type to speed up scanning.     */
                case 2:             /* N R1 => N */
                    trace1 = stack[nstack-2].val;
                    if (trace1->right != NULL)
                        goto rep;

                    if (trace1->type == CHARS) {
                        trace1->type = CHARS1;
                        goto repdone;
                        }

                    if (trace1->type == CCLASS) {
                        trace1->type = CCLASS1;
                        goto repdone;
                        }

                    if (trace1->type == ARBITRARY) {
                        trace1->type = ARB1;
                        goto repdone;
                        }
                    goto rep;

                case 3:             /* N R2 => N */
                    trace1 = stack[nstack-2].val;
                    if (trace1->right != NULL)
                        goto rep;

                    if (trace1->type == CHARS) {
                        trace1->type = CHARS2;
                        goto repdone;
                        }

                    if (trace1->type == CCLASS) {
                        trace1->type = CCLASS2;
                        goto repdone;
                        }

                    if (trace1->type == ARBITRARY) {
                        trace1->type = ARB2;
                        goto repdone;
                        }

                   /* CODE FOR REPLICATION CASES */
rep:
                    new                         = pnode();
                    new->type                   = stack[nstack-1].type,
                    new->left                   = stack[nstack-2].val;
                    new->right                  = stack[nstack-1].val;
                    stack[nstack-2].val->parent = new;
                    stack[nstack-2].val         = new;
                    stack[nstack-1]             = stack[nstack];
                    nstack--;
                    break;

repdone:
                    trace1->right   = stack[nstack-1].val;
                    stack[nstack-1] = stack[nstack];
                    nstack--;
                    break;

                case 4:             /* N $ C => N */
                    if (isupper(stack[nstack-1].newval))
                            stack[nstack-1].newval = tolower(stack[nstack-1].newval);
                    else if (!islower(stack[nstack-1].newval))
                            goto err;

                    new                         = pnode();
                    new->type                   = stack[nstack-2].type,
                    new->left                   = stack[nstack-3].val;
                    new->iright                 = stack[nstack-1].newval;
                    stack[nstack-3].val->parent = new;
                    stack[nstack-3].val         = new;
                    stack[nstack-2]             = stack[nstack];
                    nstack                     -= 2;
                    break;

                case 5:             /* ( N ) => N */
                    stack[nstack-3] = stack[nstack-2];
                    stack[nstack-2] = stack[nstack];
                    nstack         -= 2;
                    break;

                case 6:             /* C  => N */
                case 7:             /* CC => N */
                case 8:             /* A  => N */
                case 9:             /* P  => N */
                case 10:            /* V  => N */
                    new                  = pnode();
                    new->type            = stack[nstack-1].type;
                    new->left            = stack[nstack-1].val;
                    new->ileft           = stack[nstack-1].newval;
                    new->right           = NULL;
                    stack[nstack-1].val  = new;
                    stack[nstack-1].term = FALSE;
                    break;

                default:
                    DEBUGS("sr_compile MAIN SWITCH", i);
                    goto err;
                }
            }

        if (stack[nstack++].type == END)
                break;

        if (nstack == SR_STACK) {
            printf("SEARCH and REPLACE error - stack overflow\n");
            nstack--;
            goto err;
            }
        }

    if (nstack-- != 3)
        goto err;

    free(Nextch);
    free(Flags);
    return(stack[1].val);

err:
    for (i = 1; i <= nstack; i++)
        if (!stack[i].term)
            sr_free_re(stack[i].val);

    free(Nextch);
    free(Flags);
    return(NULL);
    }

/*
 * This routine does the actual scanning of the subject string for the
 * pattern that was compiled by sr_compile().
 *
 * The subject string is the string being scanned.  The pattern is the list of
 * operations to be done in scanning that have been constructed into a parse
 * tree by sr_compile().
 *
 *
 *                              LIST OF OPERATORS:
 *
 *      Generators:
 *
 *      The essence of a generator is that if at some point sufficient
 *      information is pushed onto a stack so that if later in scanning
 *      the pattern matching should fail for some reason, popping the
 *      stack will restore the scanning process to the same state as when
 *      the push was done.  At that time the generator can then take some
 *      alternate course of action.
 *
 *      Current generators include alternation and replication.  Note that
 *      character classes and range of positions are not generators, even
 *      though they imply use of alternatives, because the alternatives
 *      can all be tested without scanning ahead in the subject string.
 *
 *      RESTORE: is a special generator.  When it is pushed, the character
 *          pointer normally associated with the scanning position is instead
 *          a pointer to a node in the tree which needs to have its alter
 *          flag restored to some previous value.  After doing the restoration,
 *          RESTORE fails.
 *
 *      ALT:
 *          First: push; execute left branch
 *          Success: (from either branch) succeed
 *          Failure: execute right branch
 *
 *      REP1: (as many as possible)
 *          First: reset alter counter; goto Success
 *          Success: bump counter;
 *                   if insufficient count then
 *                       push RESTORE; execute left branch
 *                   if upper limit hit then
 *                       push RESTORE; succeed
 *                   push normally
 *                   execute left branch
 *          Failure: push RESTORE
 *                   succeed
 *
 *      REP2: (as few as possible)
 *          First: reset alter counter; goto Success
 *          Success: bump counter;
 *                   if insufficient count then
 *                       push RESTORE; execute left branch
 *                   push normally
 *                   succeed
 *          Failure: if upper limit hit then fail
 *                   push RESTORE
 *                   execute left branch

 *      REP1 and REP2 use as their executable operand (the left branch) any
 *      arbitrary pattern.  In addition, to make scanning faster in typical
 *      cases, there are CHARS1 and CHARS2, ARB1 and ARB2, and CCLASS1 and
 *      CCLASS2.  All of these are special cases of REP1 and REP2.  The code
 *      is simpler and faster since the execution of the left branch can be
 *      skipped and only one push on the stack is required.
 *
 *      Non-Generators:
 *
 *      Note that non-generators will not normally receive either the failure
 *      signal or the success signal.  CAT and ASSIGN are exceptions to this.
 *      Their action is described below.
 *
 *      CAT:
 *          First: set alter flag to indicate left side; execute left branch
 *          Success: if coming from left branch then push RESTORE;
 *                   set the alter flag for right branch; execute right branch
 *                   if coming from right branch, then succeed.
 *
 *      ASSIGN:
 *          First: save starting pos in alter flag; execute left branch
 *          Success: push RESTORE; succeed
 *
 *      CHARS:
 *          Succeed if the characters in the operator string match those
 *          in the subject at current position.  Otherwise fail.
 *
 *      CCLASS:
 *          Succeed if the current character from the subject is any of
 *          those in the CCLASS (or not in the CCLASS if the group is
 *          negated).  Otherwise fail.
 *
 *      ARBITRARY:
 *          Succeed if not at the end of the subject.  Otherwise fail.
 *
 *      POSITION:
 *          Succeed if in the range of positions allowed.  Otherwise fail.
 *
 *      VAR:
 *          Like CHARS, but instead of using an operand to match against,
 *          use the value of a variable.
 */

static
char *
scanner(cpat, subj, st_start) 
SR_CPAT *cpat;
char    *subj; 
char    *st_start;
{
    SR_CPAT     *twork;
    SR_CPAT     *t   = cpat;
    char        *pos = subj;
    char        *d;
    char        *c;
    short        pflag = FIRST;
    short        i;
    short        len;

    Stacksc = NULL;
    for (;;) {
        if (pflag == SUCCESS)
            t = t->parent;

        if (pflag == FAILURE) {
            pos = pop(&twork);
            t   = twork;
            }

        if (t == NULL) {
            if (pflag == SUCCESS) {
                scan_free(Stacksc);
                return(pos);
                }

            /* If FAILURE, then stack is necessarily NULL.              */
            scan_free(Stacksc);
            return(0);
            }

        switch(t->type) {
            case RESTORE:                               /* FAILURE      */
                twork = (SR_CPAT *)pos;
                if (twork->type == ASSIGN) {
                        i = twork->iright - 'a';
                        if (Sr_assign[i]) {
                                free(Sr_assign[i]);
                                Sr_assign[i] = NULL;
                                }

                        if (twork->asgstr) {
                                Sr_assign[i]  = twork->asgstr;
                                twork->asgstr = NULL;
                                }
                        }

                ((SR_CPAT *)pos)->alter = t->alter;
                continue;

            case ALT:
                switch(pflag) {
                case FIRST:
                        if (!push(t, pos))
                            goto err;
                        t = t->left;    /* left branch  */
                        continue;

                case SUCCESS:
                        continue;                       /* we worked    */

                case FAILURE:                           /* TRY AGAIN    */
                        pflag = FIRST;                  /* right branch */
                        t     = t->right;
                        continue;

                default:
                        DEBUGS("scanner ALT", pflag);
                        continue;
                }

            case CAT:
                switch (pflag) {
                case FIRST:
                        t->alter = FALSE;               /* left branch flag */
                        t        = t->left;
                        continue;

                case SUCCESS:
                        if (!pusher(t)) 
                            goto err;                   /* push RESTORE */

                        if (t->alter)
                            continue;                   /* test flag    */

                        t->alter = TRUE;                /* right branch flag */
                        pflag    = FIRST;
                        t        = t->right;
                        continue;

                default:
                        DEBUGS("scanner CAT", pflag);
                        continue;
                }

            case ASSIGN:
                switch (pflag) {
                case FIRST:
                        t->asgpos = pos;
                        t         = t->left;
                        continue;

                case SUCCESS:
                        t->asgstr = NULL;
                        i         = t->iright - 'a';
                        if (Sr_assign[i] != NULL) {
                                t->asgstr    = Sr_assign[i];
                                Sr_assign[i] = NULL;
                                }
                        
                        len = pos - t->asgpos + 1;
                        if (len > 0) {
                                d = Sr_assign[i] = malloc((unsigned)len);
                                if (d == 0)
                                        fatal("out of memory\n");

                                c = t->asgpos;
                                while (c < pos)
                                        *d++ = *c++;

                                *d = EOS;
                                }

                        if (!pusher(t))
                            goto err;                   /* push RESTORE */
                        continue;                       /* succeed      */
                default:
                        DEBUGS("scanner ASSIGN", pflag);
                        continue;
                    }

            case REP1:
                switch (pflag) {
                case FIRST:
                        t->alter = -1;
                        /* Fall through here */
                case SUCCESS:
                        t->alter++;                     /* bump counter */
                        if (t->alter < LOWER(t)) {      /* insufficient count */
                            if (!pusher(t)) 
                                goto err;               /* push RESTORE */

                            pflag = FIRST;              /* execute LEFT */
                            t     = t->left;
                            continue;
                            }

                        if (t->alter > LOWER(t) &&
                            pos == BEGIN(t))     {      /* null string check */
                                pflag = FAILURE;
                                continue;
                                }

                        if (!pusher(t->right))
                            goto err;
                        if (t->alter >= UPPER(t) &&
                            UPPER(t) >= 0)        {     /* upper limit hit */
                            if (!pusher(t)) 
                                goto err;               /* push RESTORE */

                            pflag = SUCCESS;            /* SUCCEED      */
                            continue;
                            }

                        BEGIN(t) = pos;                 /* null check   */
                        if (!push(t, pos)) 
                            goto err;                   /* push         */

                        pflag = FIRST;                  /* execute LEFT */
                        t     = t->left;
                        continue;

                case FAILURE:
                        if (!pusher(t))
                            goto err;                   /* push RESTORE */

                        pflag = SUCCESS;                /* SUCCEED      */
                        continue;

                default:
                        DEBUGS("scanner REP1", pflag);
                        continue;
                }

            case CHARS1:
            case ARB1:
            case CCLASS1:
                switch (pflag) {
                case FIRST:
                        t->alter = 0;
                        if (t->type == CHARS1)
                            while (pos[t->alter] == t->ileft)
                                t->alter++;

                        else if (t->type == ARB1)
                            while (pos[t->alter] != EOS  &&
                                   pos[t->alter] != Sr_eol)
                                        t->alter++;

                        else /* t->type == CCLASS1 */
                            while (inrange(pos[t->alter], t->left))
                                        t->alter++;

                        if (UPPER(t) >= 0      && 
                            UPPER(t) <= t->alter)
                            t->alter = UPPER(t);

                        /* t->alter ends at the maximum number of       */
                        /* successful matches so it needs to be bumped. */
                        t->alter++;
                        pflag = FAILURE;
                        /* fall through here */
                case FAILURE:
                        /* decrement t->alter first */
                        if (--t->alter < LOWER(t))
                            continue;                   /* FAIL         */

                        if (!push(t, pos))
                            goto err;

                        pos  += t->alter;
                        pflag = SUCCESS;
                        continue;

                default:
                        DEBUGS("scanner CCLASS1", pflag);
                        continue;
                    }

            case REP2:
                switch (pflag) {
                case FIRST:
                        t->alter = -1;
                        /* Fall through here */
                case SUCCESS:
                        t->alter++;                     /* bump counter */
                        if (t->alter < LOWER(t)) {      /* insufficient count */
                            if (!pusher(t))
                                goto err;               /* push RESTORE */

                            pflag = FIRST;              /* execute LEFT */
                            t     = t->left;
                            continue;
                            }

                        if (t->alter > LOWER(t)) {
                            if (pos == BEGIN(t)) {
                                /* null string check */
                                pflag = FAILURE;
                                continue;
                                }

                            if (!pusher(t->right))
                                goto err;
                            }

                        if (!push(t, pos)) 
                            goto err;                   /* push         */

                        pflag = SUCCESS;                /* SUCCEED      */
                        continue;

                case FAILURE:
                        if (t->alter == UPPER(t))       /* FAIL         */
                            continue;

                        BEGIN(t) = pos;                 /* save null check */
                        if (!pusher(t)) 
                            goto err;                   /* push RESTORE */

                        pflag = FIRST;                  /* execute LEFT */
                        t     = t->left;
                        continue;

                default:
                        DEBUGS("scanner REP2", pflag);
                        continue;
                }

            case CHARS2: 
            case ARB2:
            case CCLASS2:
                switch (pflag) {
                case FIRST:
                        t->alter = 0;
                        if (t->type == CHARS2)
                            while (pos[t->alter] == t->ileft)
                                t->alter++;

                        else if (t->type == ARB2)
                            while (pos[t->alter] != EOS &&
                                   pos[t->alter] != Sr_eol)
                                        t->alter++;

                        else /* t->type == CCLASS2 */
                            while (inrange(pos[t->alter], t->left))
                                    t->alter++;

                        if (UPPER(t) >= 0      &&
                            UPPER(t) <= t->alter)
                            t->alter = UPPER(t);

                        /* t->alter ends at exactly the maximum       */
                        /* number of successful matches               */
                        if (t->alter < LOWER(t)) {
                            pflag = FAILURE;
                            continue;
                            }

                        pos      += LOWER(t);
                        t->alter -= LOWER(t);
                        /* t-> alter now contains how many different  */
                        /* options we can try                         */
                        if (!push(t, pos))
                            goto err;

                        pflag = SUCCESS;
                        continue;

                case FAILURE:
                        /* t->alter is how many left to try */
                        if (t->alter-- <= 0) 
                                continue;               /* FAIL         */

                        pos++;
                        if (!push(t, pos))
                            goto err;

                        pflag = SUCCESS;
                        continue;

                default:
                        DEBUGS("scanner CLASS2", pflag);
                        continue;
                }

            case CHARS:
            case ARBITRARY:
            case CCLASS:
            case POSITION:
            case VARIABLE:
                twork = t;
                while (twork) {
                    if (twork->type == CHARS) {
                        if ((unsigned char)*pos != twork->ileft)
                            goto cfail;

                        pos++;
                        }

                    else if (twork->type == ARBITRARY) {
                        if (*pos == EOS  || 
                            *pos == Sr_eol)
                            goto cfail;

                        pos++;
                        }

                    else if (twork->type == CCLASS) {
                        if (!inrange((unsigned char)*pos, twork->left))
                            goto cfail;

                        pos++;
                        }

                    else if (twork->type == VARIABLE) {
                        c = Sr_assign[(twork->ileft) - 'a'];
                        if (c != NULL) 
                            for (c = Sr_assign[(twork->ileft) - 'a']; *c; ) 
                                if (*pos++ != *c++)
                                        goto cfail;
                        }

                    else { /* twork->type == POSITION */
                        if (!posscan(twork->left, &pos, st_start))
                                goto cfail;
                        }

                    twork = twork->right;
                    }

                pflag = SUCCESS;
                continue;
cfail:
                pflag = FAILURE;
                continue;

            default:
                DEBUGS("scanner MAIN SWITCH", t->type);
                goto err;
            }
        }
err:
    scan_free(Stacksc);
    return(NULL);
    }

/*
 *      We can normally alloc nodes that have been previously freed.
 *      Previously freed nodes are on the stack pointed to by pfree.
 *      If we don't have any nodes in pfree then we will malloc GETNODE
 *      nodes at once.  This is much faster then having to malloc 
 *      each node separatly.
 */
static
SR_CPAT *
pnode(void) {

        SR_CPAT *t;
        short    i;

        if (pfree == NULL) {
#ifdef SR_DEBUG
                printf("getting nodes\n");
                Kntnode += GETNODE;
#endif
                pfree = (SR_CPAT *)calloc((unsigned)GETNODE, sizeof(SR_CPAT));
                if (pfree == NULL) 
                        fatal("no memory in parsing");

                for (i = 0; i < GETNODE - 1; i++)
                        pfree[i].parent = &pfree[i+1];

                pfree[GETNODE - 1].parent = NULL;
                }

        t         = pfree;
        pfree     = pfree->parent;
        t->parent = NULL;
        t->asgstr = NULL;
#ifdef  SR_DEBUG
        Kntnode--;
#endif
        return(t);
        }

/*
 *      Note that freed nodes are not actually returned to the memory cache.
 *      Note also that the values are not affected except for the parent 
 *      pointer so that in particular, the left and right values may be used.
 *
 *      We free the nodes to a stack of free nodes which pnode() will then
 *      allocate from.  This is considerably faster then calling free()
 *      and then malloc() each time a node is freed and realloced.
 */
static
void
freepnode(p)
SR_CPAT *p;
{
/*      WARNING!!!!  Havoc will result if you free the same              */
/*      node twice.  A simple check helps prevent this.                  */

        SR_CPAT *t;

        if (p == NULL)
                return;

        for (t = pfree; t != NULL; t = t->parent)
                if (t == p) 
                        return;

        p->parent = pfree;
        pfree     = p;
#ifdef  SR_DEBUG
        Kntnode++;
        {       short      i;
                t = pfree;
                for (i = 0; i < Kntnode; i++)
                        t = t->parent;

                if (t != NULL)
                        fatal("the free list is corrupted");
                }
#endif
        }

/* freerange(tree) frees ranges for CCLASS and POSITION nodes */
static
void
freerange(tree)
SR_CPAT *tree; 
{
        while (tree) {
                freepnode(tree);
                tree = tree->right;
                }

        return;
        }

/*
 * This routine translates the pattern into 2 arrays.
 * The first array n contains the translated characters.  All 
 * of the escape characters are removed and any multi character
 * sequences such as octal numbers are converted into there ascii
 * representation.
 * The second array d contains an indictor (TRUE) at any character
 * position for any character that should be interpreted as a 
 * metacharacter.
 */
static
BOOL
decipher(s)
char  *s;
{
        BOOL    *d;     /* TRUE if metacharacter.                       */
        short   *n;     /* The translated pattern character.            */
        short    i;
        short    j;

        Nextch = n = (short *)malloc(sizeof(short) * (strlen(s) + 1));
        if (n == NULL) 
                return(FALSE);

        Flags = d = (BOOL *)malloc(sizeof(BOOL) * (strlen(s) + 1));
        if (d == NULL) {
                free(n);
                return(FALSE);
                }

        while (*s) {
                if (*s != ESCCHAR) {
                        *n++ = (unsigned char)*s;
                        if (special(*s++) &&
                            Sr_metaflag)
                                *d++ = TRUE;
                        else
                                *d++ = FALSE;
                        continue;
                        }

                switch (*++s) {
                        case 't':       *n++ = '\t';    goto gotit;
                        case 'b':       *n++ = '\b';    goto gotit;
                        case 'r':       *n++ = '\r';    goto gotit;
                        case 'n':       *n++ = '\n';    goto gotit;
                        case ESCCHAR:   *n++ = ESCCHAR;
gotit:
                                s++;
                                *d++ = FALSE;
                                break;

                        case '0': 
                        case '1':
                        case '2':
                        case '3':
                                i = 3;
                                goto octal;

                        case '4':
                        case '5':
                        case '6':
                        case '7':
                                i = 2;
octal:
                                j = *s++ - '0';
                                while(*s >= '0' &&
                                      *s <= '7' &&
                                      i--)
                                        j = j * 8 + *s++ - '0';

                                *n++ = j;
                                *d++ = FALSE;
                                break;

                        case EOS:
                                break;

                        default:
                                *n++ = (unsigned char)*s;
                                if (special(*s++) &&
                                    !Sr_metaflag)
                                        *d++ = TRUE;
                                else
                                        *d++ = FALSE;
                                break;
                        }
                }

        *n = 0;
        *d = FALSE;
        return(TRUE);
        }

static
BOOL
special(c)
short   c;
{

        if (strchr(".[*()|$+<{", c))
                return(TRUE);

        return(FALSE);
        }

static
char
gettoken(stack, last)
struct pstack *stack;
short last; 
{
        char     *cc;
        char      onoff;
        short     ind;
        short     c;
        short     n;
        short     m;
        short     k;
        SR_CPAT  *ccval;
        SR_CPAT  *save;

        ind         = Indexsc;
        stack->term = TRUE;
        c           = Nextch[ind];

        /* If any of these were the last token returned,                */
        /* don't put it in CAT operator.                                */
        if (last == END  ||
            last == OPEN ||
            last == ALT  ||
            last == CAT  ||
            last == ASSIGN)
                goto ok;

        /* If any of these are next, don't put in CAT operator          */
        if (c == EOS   ||
            Flags[ind] &&
           (c == '*'   ||
            c == '+'   ||
            c == '{'   ||
            c == '|'   ||
            c == '$'   ||
            c == ')'))
                goto ok;

        /* Put in CAT operator */
        return(CAT);

ok:
        if (c == EOS)
                return(END);

        if (!Flags[ind]) {
                stack->newval = c;
                Indexsc++;
                return(CHARS);
                }

        Indexsc = ++ind;
        switch (c) {
                case '*':
                        if (Sr_shortmatch == TRUE) {
                                n = 1;
                                m = 0;
                                goto REPS;
                                }
                        else {
                                n = -1;
                                m = 0;
                                goto REPS;
                                }

                case '+':
                        if (Sr_shortmatch == TRUE) {
                                n = 1;
                                m = -1;
                                goto REPS;
                                }
                        else {
                                n = -1;
                                m = 1;
                                goto REPS;
                                }

                case '{':
                        n = getint(&ind);
                        if (Nextch[ind] == ',') {
                                ind++;
                                m = getint(&ind);
                                }
                        else 
                                m = n;

                        if (Nextch[ind++] != '}') 
                                return(-1);

                        Indexsc = ind;

REPS:
                        if (n < 0 &&
                            m < 0) 
                                return(-1);

                        stack->val = pnode();
                        if (stack->val == NULL)
                                return(-1);

                        /* lower limit must be on left, upper limit on right */
                        if (n < 0  ||
                           (m >= 0 &&
                            m < n)) {
                                stack->val->ileft  = m;
                                stack->val->iright = n;
                                return(REP1);
                                }

                        stack->val->ileft  = n;
                        stack->val->iright = m;
                        return(REP2);

                case '|':
                        return(ALT);

                case '$':
                        return(ASSIGN);

                case '<':
                        /* The code here is a little complicated since the */
                        /* syntax is a bit involved.  If this is a variable*/
                        /* reference, the allowable characters after the < */
                        /* are a-z or A-Z, so life is easy for that case.  */
                        /* Otherwise, you expect a comma-separated list of */
                        /* possible positions or range of positions.  If   */
                        /* the first character of a position is ~ then the */
                        /* position is to be counted from the end of the   */
                        /* string                                          */
                        n = Nextch[ind++];
                        if (isupper(n))
                                n = tolower(n);

                        /* handle variable reference */
                        if (islower(n)) {
                                if (Nextch[ind++] != '>') 
                                        return(-1);

                                Indexsc       = ind;
                                stack->newval = n;
                                return(VARIABLE);
                                }

                        /* now, for ranges of positions */
                        /* ccval points to the next node to be filled */
                        ccval = pnode();
                        if (ccval == NULL)
                                return(-1);

                        save = ccval;
                        /* restore ind before going into loop */
                        ind = Indexsc;
                        for (;;) {
                                ccval->right = NULL;
                                if (!getpos(&ind, &n)) {
                                        freerange(save);
                                        return(-1);
                                        }

                                k = Nextch[ind++];
                                if (k == '-') {
                                        if (!getpos(&ind, &m)) {
                                                freerange(save);
                                                return(-1);
                                                }

                                        k = Nextch[ind++];
                                        }

                                else
                                        m = n;

                                ccval->ileft = n;
                                ccval->alter = m;
                                if (k == '>')
                                        break;

                                if (k != ',') { 
                                        freerange(save);
                                        return(-1);
                                        }

                                ccval->right = pnode();
                                ccval        = ccval->right;
                                }

                        Indexsc    = ind;
                        stack->val = save;
                        return(POSITION);

                case '[':
                        /* The code here is complicated.  Part of the problem */
                        /* is that the rules for forming a character class are*/
                        /* complicated.  The things to remember are:          */ 
                        /*      1)  a circumflex ^ first indicates that that  */
                        /*          character class is negated.               */ 
                        /*      2)  a close bracket ] first (after a possible */
                        /*          leading ^) is treated as a regular        */
                        /*          character.  Otherwise it ends the         */
                        /*          character class.                          */
                        /*      3)  a minus - either first (after a possible  */
                        /*          leading ^) or last (before the closing ]) */
                        /*          is treated as a regular character.        */
                        /*          Also, a minus after a minus is treated as */
                        /*          a regular character.                      */
                        /*          Otherwise it indicates a range of         */
                        /*          characters.  Normally the first           */
                        /*          character will be less than the second.   */
                        /*                                                    */
                        /* I form a Boolean array 256 long (one for each      */
                        /* character).  I then go through the character class */
                        /* specification and at the end of the pass through,  */
                        /* the array will have ones corresponding to          */
                        /* included characters and otherwise zeros.  Then I   */
                        /* generate nodes for the individual ranges           */
                        /* represented in the array.                          */ 
                        cc = malloc((unsigned)256);
                        if (cc == NULL)
                                return(-1);

                        if (Nextch[ind] == '^') {
                                for (n = 1; n < 256; n++)
                                        cc[n] = TRUE;

                                ind++;
                                onoff = FALSE;
                                }
                        else {
                                for (n = 1; n < 256; n++) 
                                        cc[n] = FALSE;

                                onoff = TRUE;
                                }

                        for (;;) {
                                n = Nextch[ind++];

gotn:
                                if (n == EOS) {
                                        free(cc);
                                        return(-1);
                                        }

                                cc[n] = onoff;

                                /* look ahead one character */
                                m = Nextch[ind++];
                                if (m == ']') 
                                        break;

                                if (m == '-') {
                                        /* look ahead another character */
                                        m = Nextch[ind++];
                                        if (m != ']') {
                                                /* minus sign indicates range */ 
                                                /* of characters              */
                                                while (n < m) 
                                                        cc[++n] = onoff;

                                                while (n > m) 
                                                        cc[--n] = onoff;

                                                /* take care of ] right after */
                                                /* range of characters        */
                                                if (Nextch[ind++] == ']')
                                                        break;

                                                ind--;
                                                }
                                        else {
                                                /* minus sign was last before ] */
                                                cc['-'] = onoff;
                                                break;
                                                }
                                        }
                                else {
                                        /* already looked ahead so have     */
                                        /* next n value                     */
                                        n = m;
                                        goto gotn;
                                        }
                                }

                        Indexsc = ind;
                        ccval   = pnode();
                        /* ccval constantly points to the next node to be  */
                        /* filled.  At the end, ccval must be freed since  */
                        /* it is a useless node.                           */
                        ccval->right = NULL;
                        save = ccval;
                        for (n = 1; n < 256; ) {
                                while (n < 256 && !cc[n]) 
                                        n++;      /* skip over FALSE      */

                                if (n == 256) 
                                        break;

                                for (m = n + 1; m < 256 && cc[m]; m++) 
                                        ;

                                ccval->right         = pnode();
                                ccval->right->parent = ccval;
                                ccval->ileft         = n;
                                ccval->alter         = m - 1;
                                ccval                = ccval->right;
                                ccval->right         = NULL;
                                n                    = m;
                                }

                        if (save->right == NULL)
                                save = NULL;

                        else
                                ccval->parent->right = NULL;

                        freepnode(ccval);
                        stack->val = save;
                        free(cc);
                        return(CCLASS);

                case '(':
                        return(OPEN);

                case ')':
                        return(SR_CLOSE);

                case '.':
                        return(ARBITRARY);

                default:
                        return(-1);
                }
        }

/*
 * getint is similar to atoi but is special purpose and updates indices 
 */
static
short
getint(indpt)
short   *indpt;
{
        short   i;

        if (!isdigit(Nextch[*indpt]))
                return(-1);

        i = 0;
        while (isdigit(Nextch[*indpt]))
                i = 10 * i + Nextch[(*indpt)++] - '0';

        return(i);
        }

/*      getpos gets a position entry.  If ~ is first, it is from the end.
 *      0 value is beginning of string and working forward.
 *      -1 value is end of string working backward.
 */
static
BOOL
getpos(indpt, valpt)
short   *indpt;
short   *valpt; 
{
        short   flag = FALSE;

        if (Nextch[*indpt] == '~') {
                (*indpt)++;
                flag = TRUE;
                }

        *valpt = getint(indpt);
        if (*valpt < 0)
                return(FALSE);

        if (flag)
                *valpt = -1 - *valpt;            /* from the end */

        return(TRUE);
        }

/* normal push */
static
BOOL
push(t, pos) 
SR_CPAT *t;
char    *pos;
{
        SR_CPAT         *new;

        new = pnode();
        if (new == NULL)
                return(FALSE);

        new->parent = Stacksc;
        new->left   = t;
        new->right  = (SR_CPAT *)pos;
        new->alter  = t->alter;
        new->asgstr = t->asgstr;
#ifdef SR_DEBUG
        new->type   = t->type;
        new->ileft  = t->ileft;
        new->iright = t->iright;
        new->asgpos = t->asgpos;
#endif
        Stacksc     = new;
        return(TRUE);
        }

/* push RESTORE */
static
BOOL
pusher(t)
SR_CPAT *t;
{
        SR_CPAT         *new;

        new = pnode();
        if (new == NULL)
                return(FALSE);

        new->parent = Stacksc;
        new->left   = &Restore;
        new->right  = t;
        new->alter  = t->alter;
        new->asgstr = t->asgstr;
#ifdef SR_DEBUG
        new->type   = t->type;
        new->ileft  = t->ileft;
        new->iright = t->iright;
        new->asgpos = t->asgpos;
#endif
        Stacksc     = new;
        return(TRUE);
        }

static
char *
pop(t)
SR_CPAT **t;
{
        SR_CPAT *old;
        char    *c;

        old = Stacksc;
        if (old == NULL) {
                *t = NULL;
                return(NULL);
                }

        *t          = old->left;
        (*t)->alter = old->alter;
        c           = (char *)old->right;
        Stacksc     = old->parent;
        freepnode(old);
        return(c);
        }

/* tests ranges for character classes */
static
BOOL
inrange(c, t)
short    c;
SR_CPAT *t;
{
        /* this requires that ranges are in numerical order             */
        if (c == Sr_eol) 
                return(FALSE);

        while (t) {
                if (c < t->ileft)
                        return(FALSE);                  /* lower limit  */

                if (c <= t->alter)
                        return(TRUE);                   /* upper limit  */

                t = t->right;
                }

        return(FALSE);
        }

static
BOOL
posscan(t, posadr, subject) 
SR_CPAT *t;
char   **posadr;
char    *subject; 
{
        char    *pos = *posadr;
        char    *c;
        short    slen;
        short    bplace;
        short    eplace;
        short    first;
        short    second;
        short    i;

        i      =  0;
        eplace = -1;
        bplace =  0;
        for (c = subject; *c != EOS && *c != Sr_eol; c++) {
                if (c == pos)
                        bplace = i;

                if (*c == '\t') 
                        i = (i + 8) & ~7;                /* tab stop */
                else 
                        i++;

                if (c == pos)
                        eplace = i - 1;
                }

        slen = i;
        if (eplace < 0) {
                /* in case pos was out of range */
                bplace = i;
                eplace = i;
                }

        /* At this point, slen has how long the string is, bplace has   */
        /* the character position from which to begin, and eplace has   */
        /* the character position from which to end.                    */
        while (t) {
                first  = t->ileft;
                second = t->alter;
                if (first < 0) 
                        first = slen + 1 + first;

                if (second < 0)
                        second = slen + 1 + second;

                if (first < second) {
                        if (first  <= eplace && 
                            bplace <= second) {
                                *posadr += (second - first);
                                return(TRUE);
                                }
                        }
                else {
                        if (second <= eplace &&
                            bplace <= first)  {
                                *posadr += (first - second);
                                return(TRUE);
                                }
                        }

                t = t->right;
                }

        return(FALSE);
        }

/* 
 *      This routine is called in case of fatal error,
 *      including out of memory.
 */
static
void
fatal(s)
char *s;
{
        printf("%s\n", s);
        exit(1);
        }

#ifdef  SR_DEBUG
/* the following is the debugging code.                                 */
static char *indexx[TYPES] = {
        "END",
        "ALT",
        "CAT",
        "ASSIGN",
        "REP1",
        "REP2",
        "CHARS",
        "CCLASS",
        "ARBITRARY",
        "POSITION",
        "VAR",
        "OPEN",
        "CLOSE",
        "CHARS1",
        "CHARS2",
        "CCLASS1",
        "CCLASS2",
        "ARB1",
        "ARB2",
        } ;

static
void
stack_prn(call_from)
char    *call_from;
{
        SR_CPAT         *new;

        printf("\n%-10s address type     left     right    il ir al asgpos\n", call_from);

        new = Stacksc;
        while (new != NULL) {
                printf("         %p %-6s %p %p %2d %2d %2d %s\n",
                        new,
                        indexx[new->type],
                        new->left,
                        new->right,
                        new->ileft,
                        new->iright,
                        new->alter,
                        new->asgpos);
                new = new->parent;
                }

        return;
        }


/* the following is parse stack tracing for debugging purposes      */
static
void
stackp(stack, nstack)
struct pstack stack[];
short nstack;
{
        short i;

        for (i = 0; i <= nstack; i++) {
                if (stack[i].term) {
                        if (stack[i].type >= 0 && 
                            stack[i].type <= TYPES)
                                printf("%s\n", indexx[stack[i].type]);
                        else
                                printf("undefined terminal %d\n", stack[i].type);
                        }

                else 
                        printf("non-terminal\n");
                }

        putchar('\n');
        }

/*
 *      This routine will print out the parse tree.
 *      For debugging call this routine with p = 0, indent = 0,
 *      and the regular expression returned from sr_compile()
 *      in tree.
 */
void
sr_treep(tree, p, indent)
SR_CPAT *tree;
SR_CPAT *p;
short    indent; 
{
        short i;

        if (tree == NULL)
                return;

        for (i = 0; i < indent; i++) 
                putchar(' ');

        if (tree->type >= 0   && 
            tree->type <= TYPES)
                printf("%s", indexx[tree->type]);
        else
                printf("unrecognized type: %d", tree->type);

        if (tree->parent != p)
                printf("  parent mismatch");

        switch (tree->type) {
                case ALT: case CAT:
                        putchar('\n');
                        sr_treep(tree->left, tree, indent+4);
                        sr_treep(tree->right, tree, indent+4);
                        break;

                case ASSIGN:
                        printf("   %c\n", tree->iright);
                        sr_treep(tree->left, tree, indent+4);
                        break;

                case CHARS:
                case ARBITRARY:
                case CCLASS:
                case POSITION:
                case VARIABLE:
                        for (;;) {
                                if (tree->type == CHARS) {
                                        printf("   ");
                                        while (tree->type == CHARS) {
                                                printf("%c", tree->ileft);
                                                tree = tree->right;
                                                }

                                        if (tree == NULL)
                                                break;

                                        printf("\n%*s%s", indent, "",
                                                          indexx[tree->type]);
                                        continue;
                                        }

                                printf("   ");
                                if (tree->type == CCLASS)
                                        rangep(tree->left);

                                else if (tree->type == ARBITRARY)
                                        printf(" ARB ");

                                else if (tree->type == POSITION)
                                        for (p = tree->left; ; ) {
                                                printf(" %d %d", p->ileft, p->alter);
                                                p = p->right;
                                                if (p == NULL) 
                                                        break;

                                                printf(" OR ");
                                                }

                                else /* tree->type == VARIABLE */
                                        printf("   %c", tree->ileft);

                                tree = tree->right;
                                if (tree == NULL) 
                                        break;

                                printf("\n%*s%s", indent, "", 
                                                  indexx[tree->type]);
                                }

                        putchar('\n');
                        break;

                case CHARS1:
                case CHARS2:
                        printf("        %c  %d  %d\n",
                                tree->ileft, LOWER(tree), UPPER(tree));
                        break;

                case CCLASS1:
                case CCLASS2:
                        rangep(tree->left);
                        printf("        %d  %d\n", LOWER(tree), UPPER(tree));
                        break;

                case ARB1:
                case ARB2:
                        printf("        %d  %d\n", LOWER(tree), UPPER(tree));
                        break;

                case REP1:
                case REP2:
                        printf("        %d  %d\n", LOWER(tree), UPPER(tree));
                        sr_treep(tree->left, tree, indent+4);
                        break;

                default:
                        putchar('\n');
                        break;
                }
        }

static
void
rangep(t) 
SR_CPAT *t;
{
        while (t) {
                putchar(' ');
                pp(t->ileft);
                putchar('-');
                pp(t->alter);
                t = t->right;
                }

        return;
        }

static
void
pp(i)
short i;
{
        if (i < 0)
                printf("NEGATIVE");

        else if (i < '!')
                printf("\\%03o", i);

        else if (i < 256)
                printf("%c", i);

        else
                printf("%d", i);

        return;
        }

static
void
swerr(cp, val)
char    *cp;
short    val;
{

        printf("switch default error in %s with value <%d>\n", cp, val);
        return;
        }
#endif
