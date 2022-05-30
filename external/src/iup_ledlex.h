/** \file
 * \brief lexical analysis manager for LED (not exported API)
 *
 * See Copyright Notice in "iup.h"
 */
 
#ifndef __IUP_LEX_H 
#define __IUP_LEX_H

#ifdef __cplusplus
extern "C" {
#endif

/* TOKENS */
#define IUPLEX_TK_END         -1  /* end of file */
#define IUPLEX_TK_BEGP         1  /* '(' begin parameters */
#define IUPLEX_TK_ENDP         2  /* ')' end parameters */
#define IUPLEX_TK_ATTR         3  /* '[' begin attributes */
#define IUPLEX_TK_STR          4  /* string value */
#define IUPLEX_TK_NAME         5  /* identifier */
#define IUPLEX_TK_NUMB         6  /* number value */
#define IUPLEX_TK_SET          7  /* '=' assignment */
#define IUPLEX_TK_COMMA        8  /* ',' separator */
#define IUPLEX_TK_FUNC         9  /* function */
#define IUPLEX_TK_ENDATTR     10  /* '[' end attributes */

/* ERRORS */
#define IUPLEX_ERR_FILENOTOPENED   1
#define IUPLEX_ERR_NOTMATCH        2
#define IUPLEX_ERR_NOTENDATTR      3
#define IUPLEX_ERR_PARSE           4

const char* iupLexGetError(void);
const char* iupLexFilename(void);
int     iupLexStart      (const char *filename, const char *buffer);
void    iupLexClose      (void);
int     iupLexLookAhead  (void);
int     iupLexAdvance    (void);
int     iupLexFollowedBy (int t);
int     iupLexMatch      (int t);
int     iupLexSeenMatch  (int t, int *erro);
unsigned char iupLexByte (void);
int     iupLexInt        (void);
float   iupLexFloat      (void);
char*   iupLexGetName    (void);
float   iupLexGetNumber  (void);
int     iupLexError      (int n, ...);
Iclass* iupLexGetClass   (void);
int     iupLexGetLine    (void);

IUP_SDK_API const char* iupLoadLed(const char *filename, const char *buffer, int save_info);


#ifdef __cplusplus
}
#endif

#endif
