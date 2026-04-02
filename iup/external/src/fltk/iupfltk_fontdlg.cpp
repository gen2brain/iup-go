/** \file
 * \brief IupFontDlg - FLTK driver
 *
 * Uses the shared custom font dialog from iup_fontdlg.c.
 *
 * See Copyright Notice in "iup.h"
 */

extern "C" {
#include "iup.h"
#include "iup_class.h"
}

extern "C" IUP_SDK_API void iupdrvFontDlgInitClass(Iclass* ic)
{
  (void)ic;
}
