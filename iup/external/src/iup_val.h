/** \file
 * \brief Valuator Control (not exported API)
 *
 * See Copyright Notice in "iup.h"
 */
 
#ifndef __IUP_VAL_H 
#define __IUP_VAL_H

#ifdef __cplusplus
extern "C" {
#endif

enum {IVAL_VERTICAL, IVAL_HORIZONTAL};

struct _IcontrolData
{
  int orientation;
  int show_ticks;  /* Windows and Motif only - can be used only after map */
  int inverted;
  double val;
  double step;
  double pagestep;
  double vmin;
  double vmax;
};

void  iupValCropValue(Ihandle* ih);
char* iupValGetValueAttrib(Ihandle* ih);
char* iupValGetStepAttrib(Ihandle* ih);
char* iupValGetPageStepAttrib(Ihandle* ih);
char* iupValGetShowTicksAttrib(Ihandle* ih);

void iupdrvValInitClass(Iclass* ic);
void iupdrvValGetMinSize(Ihandle* ih, int *w, int *h);


#ifdef __cplusplus
}
#endif

#endif
