/** \file
 * \brief IupPrintDlg page drawing, implemented by the Win32 and WinUI drivers
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUPWIN_PRINTDLG_H
#define __IUPWIN_PRINTDLG_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _IwinPrintPage {
  float paper_width, paper_height;
  float offset_x, offset_y;
  int width, height;
} IwinPrintPage;

void* iupwinPrintDlgBeginDoc(void* document_target);
int iupwinPrintDlgDrawPage(Ihandle* ih, void* print_control, int page, const IwinPrintPage* page_info);
int iupwinPrintDlgEndDoc(void* print_control);

#ifdef __cplusplus
}
#endif

#endif
