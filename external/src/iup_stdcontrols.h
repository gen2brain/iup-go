/** \file
 * \brief Standard Controls Class Initialization functions (not exported API)
 *
 * See Copyright Notice in "iup.h"
 */
 
#ifndef __IUP_STDCONTROLS_H 
#define __IUP_STDCONTROLS_H

#ifdef __cplusplus
extern "C" {
#endif


Iclass* iupDialogNewClass(void);
Iclass* iupMessageDlgNewClass(void);
Iclass* iupColorDlgNewClass(void);
Iclass* iupFontDlgNewClass(void);
Iclass* iupFileDlgNewClass(void);
Iclass* iupProgressDlgNewClass(void);

Iclass* iupLabelNewClass(void);
Iclass* iupButtonNewClass(void);
Iclass* iupToggleNewClass(void);
Iclass* iupRadioNewClass(void);
Iclass* iupCanvasNewClass(void);
Iclass* iupFrameNewClass(void);
Iclass* iupProgressBarNewClass(void);
Iclass* iupTextNewClass(void);
Iclass* iupMultilineNewClass(void);
Iclass* iupValNewClass(void);
Iclass* iupTabsNewClass(void);
Iclass* iupSpinNewClass(void);
Iclass* iupSpinboxNewClass(void);
Iclass* iupListNewClass(void);
Iclass* iupTreeNewClass(void);
Iclass* iupLinkNewClass(void);
Iclass* iupFlatLabelNewClass(void);
Iclass* iupFlatButtonNewClass(void);
Iclass* iupFlatToggleNewClass(void);
Iclass* iupDropButtonNewClass(void);
Iclass* iupFlatSeparatorNewClass(void);
Iclass* iupFlatFrameNewClass(void);
Iclass* iupFlatTabsNewClass(void);
Iclass* iupDialNewClass(void);
Iclass* iupGaugeNewClass(void);
Iclass* iupColorbarNewClass(void);
Iclass* iupColorBrowserNewClass(void);
Iclass* iupSpaceNewClass(void);
Iclass* iupThreadNewClass(void);

Iclass* iupMenuNewClass(void);
Iclass* iupItemNewClass(void);
Iclass* iupSeparatorNewClass(void);
Iclass* iupSubmenuNewClass(void);

Iclass* iupFillNewClass(void);
Iclass* iupHboxNewClass(void);
Iclass* iupVboxNewClass(void);
Iclass* iupZboxNewClass(void);
Iclass* iupCboxNewClass(void);
Iclass* iupSboxNewClass(void);
Iclass* iupNormalizerNewClass(void);
Iclass* iupSplitNewClass(void);
Iclass* iupScrollBoxNewClass(void);
Iclass* iupFlatScrollBoxNewClass(void);
Iclass* iupGridBoxNewClass(void);
Iclass* iupExpanderNewClass(void);
Iclass* iupDetachBoxNewClass(void);
Iclass* iupBackgroundBoxNewClass(void);
Iclass* iupAnimatedLabelNewClass(void);
Iclass* iupCalendarNewClass(void);
Iclass* iupDatePickNewClass(void);
Iclass* iupParamBoxNewClass(void);
Iclass* iupParamNewClass(void);
Iclass* iupMultiBoxNewClass(void);
Iclass* iupFlatListNewClass(void);
Iclass* iupFlatValNewClass(void);
Iclass* iupFlatTreeNewClass(void);

Iclass* iupTimerNewClass(void);
Iclass* iupImageNewClass(void);
Iclass* iupImageRGBNewClass(void);
Iclass* iupImageRGBANewClass(void);
Iclass* iupUserNewClass(void);
Iclass* iupClipboardNewClass(void);

/*************************************************/

void iupdrvMessageDlgInitClass(Iclass* ic);
void iupdrvColorDlgInitClass(Iclass* ic);
void iupdrvFontDlgInitClass(Iclass* ic);
void iupdrvFileDlgInitClass(Iclass* ic);

/************************************************/

IUP_SDK_API Iclass* iupBackgroundBoxNewBaseClass(Iclass* ic_base);

/* Common definition of the canvas class */
typedef struct _iupCanvas {
  int sb;    /* scrollbar configuration, valid only after map, use iupBaseGetScrollbar before map */
  double posx, posy;
  int inside_resize;
} iupCanvas;

#ifdef __cplusplus
}
#endif

#endif
