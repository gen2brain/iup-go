/** \file
 * \brief Additional Controls Class Initialization functions.
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUP_CONTROLS_H
#define __IUP_CONTROLS_H

#include "iup_class.h"


#ifdef __cplusplus
extern "C" {
#endif


Iclass* iupCellsNewClass(void);
Iclass* iupMatrixNewClass(void);
Iclass* iupMatrixListNewClass(void);
Iclass* iupMatrixExNewClass(void);

Iclass* iupFlatButtonNewClass(void);
Iclass* iupFlatLabelNewClass(void);
Iclass* iupFlatToggleNewClass(void);
Iclass* iupFlatFrameNewClass(void);
Iclass* iupFlatListNewClass(void);
Iclass* iupFlatTreeNewClass(void);
Iclass* iupFlatValNewClass(void);
Iclass* iupDropButtonNewClass(void);
Iclass* iupFlatTabsNewClass(void);
Iclass* iupFlatScrollBoxNewClass(void);
Iclass* iupGaugeNewClass(void);

char *iupControlBaseGetParentBgColor (Ihandle* ih);
char *iupControlBaseGetBgColorAttrib(Ihandle* ih);


#ifdef __cplusplus
}
#endif

#endif
