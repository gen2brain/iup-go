/** \file
 * \brief Scrollbar Control (not exported API)
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUP_SCROLLBAR_H
#define __IUP_SCROLLBAR_H

#ifdef __cplusplus
extern "C" {
#endif

enum {ISCROLLBAR_VERTICAL, ISCROLLBAR_HORIZONTAL};

struct _IcontrolData
{
  int orientation;
  int inverted;
  double val;
  double vmin;
  double vmax;
  double pagesize;
  double linestep;
  double pagestep;
};

void  iupScrollbarCropValue(Ihandle* ih);
char* iupScrollbarGetValueAttrib(Ihandle* ih);
char* iupScrollbarGetLineStepAttrib(Ihandle* ih);
char* iupScrollbarGetPageStepAttrib(Ihandle* ih);
char* iupScrollbarGetPageSizeAttrib(Ihandle* ih);

void iupdrvScrollbarInitClass(Iclass* ic);
void iupdrvScrollbarGetMinSize(Ihandle* ih, int *w, int *h);


#ifdef __cplusplus
}
#endif

#endif
