/** \file
 * \brief String Utilities
 *
 * See Copyright Notice in "iup.h"
 */

 
#ifndef __IUP_STR_H 
#define __IUP_STR_H

#ifdef __cplusplus
extern "C" {
#endif


/** \defgroup str String Utilities
 * \par
 * See \ref iup_str.h
 * \ingroup util */

/** Returns a non zero value if the two strings are equal.
 * str1 or str2 can be NULL.
 * \ingroup str */
IUP_SDK_API int iupStrEqual(const char* str1, const char* str2);

/** Returns a non zero value if the two strings are equal but ignores case.
 * str1 or str2 can be NULL.
 * It will work only for character codes <128.
 * \ingroup str */
IUP_SDK_API int iupStrEqualNoCase(const char* str1, const char* str2);

/** Returns a non zero value if the two strings are equal but ignores case and spaces. \n
 * str1 or str2 can be NULL. \n
 * It will work only for character codes <128.
 * \ingroup str */
IUP_SDK_API int iupStrEqualNoCaseNoSpace(const char* str1, const char* str2);

/** Returns a non zero value if the two strings are equal 
 * up to a number of characters defined by the strlen of the second string. \n
 * str1 or str2 can be NULL.
 * \ingroup str */
IUP_SDK_API int iupStrEqualPartial(const char* str1, const char* str2);

/** Returns a non zero value if the two strings are equal but ignores case 
 * up to a number of characters defined by the strlen of the second string. \n
 * str1 or str2 can be NULL. \n
 * It will work only for character codes <128.
 * \ingroup str */
IUP_SDK_API int iupStrEqualNoCasePartial(const char* str1, const char* str2);



/** Returns 1 if the string is "YES" or "ON". \n
 * Returns 0 otherwise (including NULL or empty).
 * \ingroup str */
IUP_SDK_API int iupStrBoolean(const char* str);

/** Returns 1 if the string is "NO" or "OFF". \n
 * Returns 0 otherwise (including NULL or empty). \n
 * To be used when value can be "False" or others different than "True".
 * \ingroup str */
IUP_SDK_API int iupStrFalse(const char* str);



/** Returns the number of lines in a string.
 * It works for UNIX, DOS and MAC line ends.
 * \ingroup str */
IUP_SDK_API int iupStrLineCount(const char* str, int len);

/** Returns a pointer to the next line and the size of the current line.
 * It works for UNIX, DOS and MAC line ends. The size does not includes the line end.
 * If str is NULL it will return NULL.
 * \ingroup str */
IUP_SDK_API const char* iupStrNextLine(const char* str, int *len);

/** Returns a pointer to the next value and the size of the current value.
 * The size does not includes the separator.
 * If str is NULL it will return NULL.
 * \ingroup str */
IUP_SDK_API const char* iupStrNextValue(const char* str, int str_len, int *len, char sep);

/** Returns the number of repetitions of the character occurs in the string.
 * \ingroup str */
IUP_SDK_API int iupStrCountChar(const char *str, char c);



/** Returns a copy of the given string.
 * If str is NULL it will return NULL.
 * Must free the returned string.
 * \ingroup str */
IUP_SDK_API char* iupStrDup(const char* str);

/** Returns a new string containing a copy of the string up to the character.
 * The string is then incremented to after the position of the character.
 * Must free the returned string.
 * \ingroup str */
IUP_SDK_API char* iupStrDupUntil(const char **str, char c);

/** Copy the string to the buffer, but limited to the max_size of the buffer.
 * buffer is always properly ended.
 * \ingroup str */
IUP_SDK_API void iupStrCopyN(char* dst_str, int dst_max_size, const char* src_str);



/** Returns a buffer with the specified size+1. \n
 * The buffer is resused after 50 calls. It must NOT be freed.
 * Use size=-1 to free all the internal buffers.
 * \ingroup str */
IUP_SDK_API char* iupStrGetMemory(int size);

/** Returns a very large buffer to be used in unknown size string construction.
 * Use snprintf or vsnprintf with the given size.
 * \ingroup str */
IUP_SDK_API char* iupStrGetLargeMem(int *size);



/** Converts a string into lower case. Can be used in-place. \n
 * It will work only for character codes <128.
 * \ingroup str */
IUP_SDK_API void iupStrLower(char* dstr, const char* sstr);

/** Converts a string into upper case. Can be used in-place. \n
 * It will work only for character codes <128.
 * \ingroup str */
IUP_SDK_API void iupStrUpper(char* dstr, const char* sstr);

/** Checks if the string has at least 1 space character.
 * \ingroup str */
IUP_SDK_API int iupStrHasSpace(const char* str);



/** Checks if the character is a digit. 
 * \ingroup str */
#define iup_isdigit(_c) (_c>='0' && _c<='9')

#define iup_isnumber(_c) ((_c>='0' && _c<='9') || \
                          _c == '.'  || _c == ',' || \
                          _c == '-' || _c == '+' || \
                          _c == 'e' || _c == 'E')

/** Converts a character into upper case. \n
 * It will work only for character codes <128.
 * \ingroup str */
#define iup_toupper(_c)  ((_c >= 'a' && _c <= 'z')? (_c - 'a') + 'A': _c)

/** Converts a character into lower case. \n
 * It will work only for character codes <128.
 * \ingroup str */
#define iup_tolower(_c)  ((_c >= 'A' && _c <= 'Z')? (_c - 'A') + 'a': _c)

/** Checks if the string has only ASCII codes.
 * \ingroup str */
IUP_SDK_API int iupStrIsAscii(const char* str);



/** Returns combined values in a formatted string using \ref iupStrGetMemory.
 * This is not supposed to be used for very large strings,
 * just for combinations of numeric data or constant strings.
 * \ingroup str */
IUP_SDK_API char* iupStrReturnStrf(const char* format, ...);

/** Returns a string value in a string using \ref iupStrGetMemory.
 * \ingroup str */
IUP_SDK_API char* iupStrReturnStr(const char* str);

/** Returns a boolean value (as YES or NO) in a string.
 * \ingroup str */
IUP_SDK_API char* iupStrReturnBoolean(int i);

/** Returns a checked value (as ON, OFF or NOTDEF (-1)) in a string.
 * \ingroup str */
IUP_SDK_API char* iupStrReturnChecked(int i);

/** Returns an int value in a string using \ref iupStrGetMemory.
 * \ingroup str */
IUP_SDK_API char* iupStrReturnInt(int i);

/** Returns an unsigned int value in a string using \ref iupStrGetMemory.
* \ingroup str */
IUP_SDK_API char* iupStrReturnUInt(unsigned int i);

/** maximum float precision
* \ingroup str */
#define IUP_FLOAT2STR "%.9f"

/** Returns a float value in a string using \ref iupStrGetMemory.
 * \ingroup str */
IUP_SDK_API char* iupStrReturnFloat(float f);

/** maximum double precision
 * \ingroup str */
#define IUP_DOUBLE2STR "%.18f"

/** Returns a double value in a string using \ref iupStrGetMemory.
 * \ingroup str */
IUP_SDK_API char* iupStrReturnDouble(double d);

/** Returns a RGB value in a string using \ref iupStrGetMemory.
 * \ingroup str */
IUP_SDK_API char* iupStrReturnRGB(unsigned char r, unsigned char g, unsigned char b);

/** Returns a RGBA value in a string using \ref iupStrGetMemory.
 * \ingroup str */
IUP_SDK_API char* iupStrReturnRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a);

/** Returns two string values in a string using \ref iupStrGetMemory.
 * \ingroup str */
IUP_SDK_API char* iupStrReturnStrStr(const char *str1, const char *str2, char sep);

/** Returns two int values in a string using \ref iupStrGetMemory.
 * \ingroup str */
IUP_SDK_API char* iupStrReturnIntInt(int i1, int i2, char sep);


/** Returns the number of decimals in a format string for floating point output.
 * \ingroup str */
IUP_SDK_API int iupStrGetFormatPrecision(const char* format);

/** Prints a double in a string using the given decimal symbol.
 * \ingroup str */
IUP_SDK_API void iupStrPrintfDoubleLocale(char *str, const char *format, double d, const char* decimal_symbol);


/** Extract RGB components from the string. Returns 0 or 1.
 * \ingroup str */
IUP_SDK_API int iupStrToRGB(const char *str, unsigned char *r, unsigned char *g, unsigned char *b);

/** Extract RGBA components from the string. Returns 0 or 1.
 * \ingroup str */
IUP_SDK_API int iupStrToRGBA(const char *str, unsigned char *r, unsigned char *g, unsigned char *b, unsigned char *a);

/** Converts the string to an int. The string must contains only the integer value.
 * Returns a a non zero value if successful.
 * \ingroup str */
IUP_SDK_API int iupStrToInt(const char *str, int *i);

/** Converts the string to an unsigned int. The string must contains only the integer value.
* Returns a a non zero value if successful.
* \ingroup str */
IUP_SDK_API int iupStrToUInt(const char *str, unsigned int *i);

/** Converts the string to two int. The string must contains two integer values in sequence,
 * separated by the given character (usually 'x' or ':').
 * Returns the number of converted values.
 * Values not extracted are not changed.
 * \ingroup str */
IUP_SDK_API int iupStrToIntInt(const char *str, int *i1, int *i2, char sep);

/** Converts the string to a float. The string must contains only the real value.
 * Returns a a non zero value if successful.
 * \ingroup str */
IUP_SDK_API int iupStrToFloat(const char *str, float *f);
IUP_SDK_API int iupStrToFloatDef(const char *str, float *f, float def);

/** Converts the string to a double. The string must contains only the real value.
 * Returns a a non zero value if successful.
 * \ingroup str */
IUP_SDK_API int iupStrToDouble(const char *str, double *d);
IUP_SDK_API int iupStrToDoubleDef(const char *str, double *d, double def);

/** Converts the string to a double using the given decimal symbol. 
 * The string must contains only the real value.
 * Returns a a non zero value if successful. Returns 2 if a locale was set.
 * \ingroup str */
IUP_SDK_API int iupStrToDoubleLocale(const char *str, double *d, const char* decimal_symbol);

/** Converts the string to two float. The string must contains two real values in sequence,
 * separated by the given character (usually 'x' or ':').
 * Returns the number of converted values.
 * Values not extracted are not changed.
 * ATENTION: AVOID DEFINING THIS TYPE OF ATTRIBUTE VALUE.
 * \ingroup str */
IUP_SDK_API int iupStrToFloatFloat(const char *str, float *f1, float *f2, char sep);

/** Converts the string to two double. The string must contains two real values in sequence,
 * separated by the given character (usually 'x' or ':').
 * Returns the number of converted values.
 * Values not extracted are not changed.
 * ATENTION: AVOID DEFINING THIS TYPE OF ATTRIBUTE VALUE.
 * \ingroup str */
IUP_SDK_API int iupStrToDoubleDouble(const char *str, double *f1, double *f2, char sep);

/** Extract two strings from the string.
 * separated by the given character (usually 'x' or ':').
 * Returns the number of converted values.
 * Values not extracted are set to empty strings.
 * \ingroup str */
IUP_SDK_API int iupStrToStrStr(const char *str, char *str1, char *str2, char sep);



/** Returns the file extension of a file name.
 * Supports UNIX and Windows directory separators.
 * Must free the returned string.
 * \ingroup str */
IUP_SDK_API char* iupStrFileGetExt(const char *filename);

/** Returns the file title of a file name.
 * Supports UNIX and Windows directory separators.
 * Must free the returned string.
 * \ingroup str */
IUP_SDK_API char* iupStrFileGetTitle(const char *filename);

/** Returns the file path of a file name.
 * Supports UNIX and Windows directory separators.
 * The returned value includes the last separator.
 * Must free the returned string.
 * \ingroup str */
IUP_SDK_API char* iupStrFileGetPath(const char *filename);

/** Concat path and title addind '/' between if path does not have it.
 * Must free the returned string.
 * \ingroup str */
IUP_SDK_API char* iupStrFileMakeFileName(const char* path, const char* title);

/** Creates an URL for the filename by adding "file://" at start
 * and replacing any '\' by '/'.
 * Must free the returned string.
 * \ingroup str */
IUP_SDK_API char* iupStrFileMakeURL(const char* filename);

/** Split the filename in path and title using pre-allocated strings.
 * \ingroup str */
IUP_SDK_API void iupStrFileNameSplit(const char* filename, char* path, char* title);

/** Returns a filename for a temporary file.
 * A file with the result name is created and must be removed after use.
 * \ingroup str */
IUP_SDK_API int iupStrTmpFileName(char* filename, const char* prefix);


/** Replace a character in a string.
 * Returns the number of occurrences.
 * \ingroup str */
IUP_SDK_API int iupStrReplace(char* str, char src, char dst);

/** Replace reserved characters in a string.
 * \ingroup str */
IUP_SDK_API void iupStrReplaceReserved(char* str, char c);


/** Convert line ends to UNIX format in-place (one \n per line).
 * \ingroup str */
IUP_SDK_API void iupStrToUnix(char* str);

/** Convert line ends to MAC format in-place (one \r per line).
 * \ingroup str */
IUP_SDK_API void iupStrToMac(char* str);

/** Convert line ends to DOS/Windows format (the sequence \r\n per line).
 * If returned pointer different the input, it must be freed.
 * \ingroup str */
IUP_SDK_API char* iupStrToDos(const char* str);

/** Convert string to C format. Process \\, \n, \r and \t.
 * If returned pointer different the input, it must be freed.
 * \ingroup str */
IUP_SDK_API char* iupStrConvertToC(const char* str);



/** Remove the interval from the string. Done in-place.
 * \ingroup str */
IUP_SDK_API void iupStrRemove(char* value, int start, int end, int dir, int utf8);

/** Remove the interval from the string and insert the new string at the start.
* If returned pointer different the input, it must be freed.
* \ingroup str */
IUP_SDK_API char* iupStrInsert(const char* value, const char* insert_value, int start, int end, int utf8);



/** Process the mnemonic in the string. If not found returns str.
* If returned pointer different the input, it must be freed.
* If found returns a new string. Action can be:
- 1: replace & by c
- -1: remove & and return in c
- 0: remove &
 * \ingroup str */
IUP_SDK_API char* iupStrProcessMnemonic(const char* str, char *c, int action);

/** Returns the Mnemonic if found. Zero otherwise.
 * \ingroup str */
IUP_SDK_API int iupStrFindMnemonic(const char* str);



/** Compare two strings using strcmp semantics, 
 *  but using the "Alphanum Algorithm" (A1 A2 A11 A30 ...). \n
 *  This means that numbers and text are sorted separately. \n
 *  Also natural alphabetic order is used: 123...aAáÁ...bBcC... \n
 *  Sorting and case insensitive will work only for Latin-1 characters, even when using utf8=1.
 * \ingroup str */
IUP_SDK_API int iupStrCompare(const char* str1, const char* str2, int casesensitive, int utf8);

/** Returns a non zero value if the two strings are equal. \n
 *  If partial=1 the compare up to a number of characters defined by the strlen of the second string. \n
 *  Case insensitive will work only for Latin-1 characters, even when using utf8=1.
 * \ingroup str */
IUP_SDK_API int iupStrCompareEqual(const char *str1, const char *str2, int casesensitive, int utf8, int partial);

/** Returns a non zero value if the second string is found inside the first string.  \n
 *  Uses \ref iupStrCompareEqual.
 * \ingroup str */
IUP_SDK_API int iupStrCompareFind(const char *str1, const char *str2, int casesensitive, int utf8);

/** Case conversion available for \ref iupStrChangeCase.
* \ingroup str */
enum { IUP_CASE_UPPER, IUP_CASE_LOWER, IUP_CASE_TOGGLE, IUP_CASE_TITLE };

/** Converts a string into upper case. Can be used in-place. \n
* It will work only for Latin-1 characters, even when using utf8=1.
* \ingroup str */
IUP_SDK_API void iupStrChangeCase(char* dstr, const char* sstr, int case_flag, int utf8);



#ifdef __cplusplus
}
#endif

#endif
