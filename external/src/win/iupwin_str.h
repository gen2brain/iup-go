/** \file
 * \brief Windows String Processing
 *
 * See Copyright Notice in "iup.h"
 */
 
#ifndef __IUPWIN_STR_H 
#define __IUPWIN_STR_H

#ifdef __cplusplus
extern "C" {
#endif


void iupwinStrSetUTF8Mode(int utf8mode);
void iupwinStrSetUTF8ModeFile(int utf8mode);
int iupwinStrGetUTF8Mode(void);
int iupwinStrGetUTF8ModeFile(void);

IUP_DRV_API char* iupwinStrFromSystem(const TCHAR* str);
IUP_DRV_API TCHAR* iupwinStrToSystem(const char* str);
IUP_DRV_API TCHAR* iupwinStrToSystemLen(const char* str, int *len);
IUP_DRV_API TCHAR* iupwinStrToSystemFilename(const char* str);
IUP_DRV_API char* iupwinStrFromSystemFilename(const TCHAR* str);

void iupwinStrRelease(void);

IUP_DRV_API WCHAR* iupwinStrChar2Wide(const char* str);
IUP_DRV_API char* iupwinStrWide2Char(const WCHAR* wstr);

void iupwinStrCopy(TCHAR* dst_wstr, const char* src_str, int max_size);


#ifdef __cplusplus
}
#endif

#endif
