/* ----------------------------------------------------------------------- *\
|
|  File:                STRTOKEN.C
|
|  Description:         This module provides improved string detokenization
|                       compared with strtok().
|
|  Author(s):           Brian Johnson
|
|  History:
|
|  15-Jan-93    bwj     Original
|  17-Aug-97    bwj     Ported to NT
|
\* ----------------------------------------------------------------------- */

#include <windows.h>
#include <string.h>

#include "fwild.h"

/* ----------------------------------------------------------------------- *\
|  Private definitions
\* ----------------------------------------------------------------------- */

#define  BITSINUINT   (8 * sizeof(UINT))
#define  MASK_SIZE    (256 / BITSINUINT)

#define  UC(c)        (unsigned char)(c)
#define  TESTMASK(c)  (_char_mask[UC(c) / BITSINUINT] &  (1 << (UC(c) % BITSINUINT)))
#define  SETMASK(c)   (_char_mask[UC(c) / BITSINUINT] |= (1 << (UC(c) % BITSINUINT)))
#define  CLRMASK()    memset(_char_mask, 0, sizeof(_char_mask))

typedef  UINT  MASKTYPE [MASK_SIZE];
#define  CHARMASK     MASKTYPE  _char_mask

#define  DEFAULT_CONTROL  (" \t\r\n\f,;:|")

static	TOKENCB		defaultsaveblk = {0};	// Default save block

/* ----------------------------------------------------------------------- *\
|
|  Function:            strtoken
|
|  Purpose:             This function returns a pointer to the next token
|                       in the pointed string.
|
|  Parameters:          string - Pointer to the string to be parsed (first
|                       call). Subsequently, NULL is passed to continue
|			parsing tokens from the same string.
|
|			control - Pointer to the control string, which
|			contains all of the characters that are to be
|			treated as delimiters.  If NULL, then a default
|			control string is used.
|
|			saveblk - Pointer to the save block, which contains
|			information that must be preserved between calls.
|			If NULL, then a default save block is used.
|
|  Returns:             This function returns a pointer to the next token
|                       in the pointed string, or NULL if no token is found.
|
|  Comments:            The pointer to the string to be parsed is passed in
|                       the first call.  NULL is passed subsequently.
|
|			strtoken() is nondestructive, provided it is called
|			until it returns NULL.
|
|                       If the control string pointer is NULL, then strtoken()
|                       uses a default control string containing the
|			following delimiters:
|
|			<SPACE TAB CR LF FF , ; : |
|
\* ----------------------------------------------------------------------- */
    char *                      // Returns ptr to the token, or NULL         
strtoken (
    char        *string,        // Pointer to the string to parse, or NULL
    char        *control,       // Pointer to the control string
    TOKENCB     *saveblk)       // Pointer to the token save block

    {
    char         ch;            // Temporary character
    char        *p;             // The returned token pointer
    CHARMASK;                   // The character mask array, macro defined


    /*
    If the string pointer is not NULL, we are starting a new string.
    If the string pointer is NULL, we are continuing a previous string,
    so restore the delimiter and use the saved pointer, unless it is
    also NULL, in which case the old string was finished, so return NULL.
    */

    if (saveblk == NULL)
	saveblk = &defaultsaveblk;

    if (string == NULL)
        {
        if ((string = saveblk->ptr) == NULL)
            return (NULL);
        else
            *string = saveblk->ch;
        }

    /*
    Clear and then build the control mask array.  If the control
    string pointer is NULL, use the default control string.
    */

    CLRMASK();
    if (control == NULL)
        control = DEFAULT_CONTROL;
    while ((ch = *(control++)) != '\0')
        SETMASK(ch);


    // Process the string

    saveblk->ptr = p = NULL;
    while ((ch = *string) != '\0')
        {
        if (TESTMASK(ch))
            {
            if (p != NULL)
                {
                saveblk->ptr = string;
                saveblk->ch  = ch;
                *string      = '\0';
                break;
                }
            }
        else if (p == NULL)
            p = string;
        ++string;
        }

    return (p);
    }

/* ----------------------------------------------------------------------- */
// strtoken.c
