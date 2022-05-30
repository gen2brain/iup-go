/** \file
* \brief Register the Internal Controls
*
* See Copyright Notice in "iup.h"
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdarg.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_register.h"
#include "iup_stdcontrols.h"


static Itable *iregister_table = NULL;   /* table indexed by name containing Iclass* address */

void iupRegisterInit(void)
{
  iregister_table = iupTableCreate(IUPTABLE_STRINGINDEXED);
}

void iupRegisterFinish(void)
{
  char* name = iupTableFirst(iregister_table);
  while (name)
  {
    Iclass* ic = (Iclass*)iupTableGetCurr(iregister_table);
    iupClassRelease(ic);
    name = iupTableNext(iregister_table);
  }

  iupTableDestroy(iregister_table);
  iregister_table = NULL;
}

IUP_API int IupGetAllClasses(char** list, int n)
{
  int i = 0;
  char* name;

  if (!list || n==0 || n==-1)
    return iupTableCount(iregister_table);

  name = iupTableFirst(iregister_table);
  while (name)
  {
    list[i] = name;
    i++;
    if (i == n)
      break;

    name = iupTableNext(iregister_table);
  }

  return i;
}

IUP_SDK_API Iclass* iupRegisterFindClass(const char* name)
{
  return (Iclass*)iupTableGet(iregister_table, name);
}

static void iupRegisterClassInternal(Iclass* ic)
{
  Iclass* old_ic = (Iclass*)iupTableGet(iregister_table, ic->name);
  if (old_ic)
    iupClassRelease(old_ic);

  ic->is_internal = 1;

  iupTableSet(iregister_table, ic->name, (void*)ic, IUPTABLE_POINTER);
}

IUP_SDK_API void iupRegisterClass(Iclass* ic)
{
  Iclass* old_ic = (Iclass*)iupTableGet(iregister_table, ic->name);
  if (old_ic)
    iupClassRelease(old_ic);

  iupTableSet(iregister_table, ic->name, (void*)ic, IUPTABLE_POINTER);
}

void iupRegisterUpdateClasses(void)
{
  char* name = iupTableFirst(iregister_table);
  while (name)
  {
    Iclass* ic = (Iclass*)iupTableGetCurr(iregister_table);
    iupClassUpdate(ic);
    name = iupTableNext(iregister_table);
  }
}


/***************************************************************/

void iupRegisterInternalClasses(void)
{
  iupRegisterClassInternal(iupDialogNewClass());
  iupRegisterClassInternal(iupMessageDlgNewClass());
  iupRegisterClassInternal(iupColorDlgNewClass());
  iupRegisterClassInternal(iupFontDlgNewClass());
  iupRegisterClassInternal(iupFileDlgNewClass());
  iupRegisterClassInternal(iupProgressDlgNewClass());
  iupRegisterClassInternal(iupParamBoxNewClass());
  iupRegisterClassInternal(iupParamNewClass());

  iupRegisterClassInternal(iupTimerNewClass());
  iupRegisterClassInternal(iupImageNewClass());
  iupRegisterClassInternal(iupImageRGBNewClass());
  iupRegisterClassInternal(iupImageRGBANewClass());
  iupRegisterClassInternal(iupUserNewClass());
  iupRegisterClassInternal(iupClipboardNewClass());
  iupRegisterClassInternal(iupThreadNewClass());

  iupRegisterClassInternal(iupRadioNewClass());
  iupRegisterClassInternal(iupFillNewClass());
  iupRegisterClassInternal(iupHboxNewClass());
  iupRegisterClassInternal(iupVboxNewClass());
  iupRegisterClassInternal(iupZboxNewClass());
  iupRegisterClassInternal(iupCboxNewClass());
  iupRegisterClassInternal(iupSboxNewClass());
  iupRegisterClassInternal(iupNormalizerNewClass());
  iupRegisterClassInternal(iupSplitNewClass());
  iupRegisterClassInternal(iupExpanderNewClass());
  iupRegisterClassInternal(iupDetachBoxNewClass());

  iupRegisterClassInternal(iupMenuNewClass());
  iupRegisterClassInternal(iupItemNewClass());
  iupRegisterClassInternal(iupSeparatorNewClass());
  iupRegisterClassInternal(iupSubmenuNewClass());

  iupRegisterClassInternal(iupLabelNewClass());
  iupRegisterClassInternal(iupButtonNewClass());
  iupRegisterClassInternal(iupToggleNewClass());
  iupRegisterClassInternal(iupCanvasNewClass());
  iupRegisterClassInternal(iupFrameNewClass());
  iupRegisterClassInternal(iupTextNewClass());
  iupRegisterClassInternal(iupMultilineNewClass());
  iupRegisterClassInternal(iupListNewClass());
  iupRegisterClassInternal(iupFlatLabelNewClass());
  iupRegisterClassInternal(iupFlatButtonNewClass());
  iupRegisterClassInternal(iupFlatToggleNewClass());
  iupRegisterClassInternal(iupFlatSeparatorNewClass());
  iupRegisterClassInternal(iupDropButtonNewClass());
  iupRegisterClassInternal(iupCalendarNewClass());
  iupRegisterClassInternal(iupDatePickNewClass());
  iupRegisterClassInternal(iupSpaceNewClass());

  iupRegisterClassInternal(iupProgressBarNewClass());
  iupRegisterClassInternal(iupValNewClass());
  iupRegisterClassInternal(iupTabsNewClass());
  iupRegisterClassInternal(iupSpinNewClass());
  iupRegisterClassInternal(iupSpinboxNewClass());
  iupRegisterClassInternal(iupTreeNewClass());
  iupRegisterClassInternal(iupScrollBoxNewClass());
  iupRegisterClassInternal(iupBackgroundBoxNewClass());
  iupRegisterClassInternal(iupLinkNewClass());
  iupRegisterClassInternal(iupGridBoxNewClass());
  iupRegisterClassInternal(iupAnimatedLabelNewClass());
  iupRegisterClassInternal(iupFlatFrameNewClass());
  iupRegisterClassInternal(iupFlatTabsNewClass());
  iupRegisterClassInternal(iupFlatScrollBoxNewClass());
  iupRegisterClassInternal(iupDialNewClass());
  iupRegisterClassInternal(iupGaugeNewClass());
  iupRegisterClassInternal(iupColorbarNewClass());
  iupRegisterClassInternal(iupColorBrowserNewClass());
  iupRegisterClassInternal(iupMultiBoxNewClass());
  iupRegisterClassInternal(iupFlatListNewClass());
  iupRegisterClassInternal(iupFlatValNewClass());
  iupRegisterClassInternal(iupFlatTreeNewClass());
}
