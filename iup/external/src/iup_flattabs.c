/** \file
* \brief iupflattabs control
*
* See Copyright Notice in "iup.h"
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_stdcontrols.h"
#include "iup_layout.h"
#include "iup_image.h"
#include "iup_register.h"
#include "iup_drvdraw.h"
#include "iup_draw.h"
#include "iup_varg.h"


#define ITABS_CLOSE_SIZE 13
#define ITABS_CLOSE_SPACING 12
#define ITABS_CLOSE_BORDER 8
#define ITABS_NONE -1
#define ITABS_SB_TOP -2
#define ITABS_SB_BOTTOM -3
#define ITABS_SB_LEFT -4
#define ITABS_SB_RIGHT -5
#define ITABS_EXTRABUTTON1 -6

#define ITABS_TABID2EXTRABUT(_id) (ITABS_EXTRABUTTON1 - _id + 1)
#define ITABS_EXTRABUT2TABID(_id) (ITABS_EXTRABUTTON1 - _id + 1) /* equal to the above, the conversion is symmetric */

typedef enum
{
  ITABS_TOP, ITABS_BOTTOM, ITABS_LEFT, ITABS_RIGHT
} ItabsType;

static Ihandle* load_image_expand_down(void)
{
  unsigned char imgdata[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0,
    0, 0, 0, 8, 0, 0, 0, 48, 0, 0, 0, 21, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 21, 0, 0, 0, 48, 0, 0, 0, 8,
    0, 0, 0, 45, 0, 0, 0, 109, 0, 0, 0, 93, 0, 0, 0, 24, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 24, 0, 0, 0, 93, 0, 0, 0, 109, 0, 0, 0, 45,
    0, 0, 0, 17, 0, 0, 0, 94, 0, 0, 0, 119, 0, 0, 0, 93, 0, 0, 0, 24, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 24, 0, 0, 0, 93, 0, 0, 0, 119, 0, 0, 0, 93, 0, 0, 0, 16,
    0, 0, 0, 1, 0, 0, 0, 24, 0, 0, 0, 93, 0, 0, 0, 118, 0, 0, 0, 92, 0, 0, 0, 25, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 25, 0, 0, 0, 92, 0, 0, 0, 118, 0, 0, 0, 93, 0, 0, 0, 24, 0, 0, 0, 1,
    0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 24, 0, 0, 0, 92, 0, 0, 0, 118, 0, 0, 0, 92, 0, 0, 0, 25, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 25, 0, 0, 0, 92, 0, 0, 0, 118, 0, 0, 0, 92, 0, 0, 0, 24, 0, 0, 0, 1, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 24, 0, 0, 0, 92, 0, 0, 0, 117, 0, 0, 0, 90, 0, 0, 0, 26, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 26, 0, 0, 0, 90, 0, 0, 0, 117, 0, 0, 0, 92, 0, 0, 0, 24, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 25, 0, 0, 0, 90, 0, 0, 0, 117, 0, 0, 0, 90, 0, 0, 0, 26, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 26, 0, 0, 0, 90, 0, 0, 0, 117, 0, 0, 0, 90, 0, 0, 0, 25, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 25, 0, 0, 0, 92, 0, 0, 0, 116, 0, 0, 0, 90, 0, 0, 0, 28, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 28, 0, 0, 0, 90, 0, 0, 0, 116, 0, 0, 0, 92, 0, 0, 0, 25, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 25, 0, 0, 0, 90, 0, 0, 0, 116, 0, 0, 0, 90, 0, 0, 0, 28, 0, 0, 0, 4, 0, 0, 0, 2, 0, 0, 0, 26, 0, 0, 0, 90, 0, 0, 0, 116, 0, 0, 0, 90, 0, 0, 0, 25, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 25, 0, 0, 0, 92, 0, 0, 0, 116, 0, 0, 0, 90, 0, 0, 0, 30, 0, 0, 0, 29, 0, 0, 0, 90, 0, 0, 0, 116, 0, 0, 0, 92, 0, 0, 0, 25, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 26, 0, 0, 0, 90, 0, 0, 0, 116, 0, 0, 0, 101, 0, 0, 0, 101, 0, 0, 0, 116, 0, 0, 0, 90, 0, 0, 0, 26, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 25, 0, 0, 0, 90, 0, 0, 0, 120, 0, 0, 0, 120, 0, 0, 0, 90, 0, 0, 0, 25, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 26, 0, 0, 0, 89, 0, 0, 0, 89, 0, 0, 0, 26, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 16, 0, 0, 0, 16, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

  Ihandle* image = IupImageRGBA(24, 16, imgdata);
  return image;
}

static Ihandle* load_image_expand_up(void)
{
  unsigned char imgdata[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 16, 0, 0, 0, 16, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 26, 0, 0, 0, 88, 0, 0, 0, 88, 0, 0, 0, 26, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 26, 0, 0, 0, 92, 0, 0, 0, 119, 0, 0, 0, 119, 0, 0, 0, 92, 0, 0, 0, 26, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 26, 0, 0, 0, 90, 0, 0, 0, 116, 0, 0, 0, 100, 0, 0, 0, 100, 0, 0, 0, 114, 0, 0, 0, 90, 0, 0, 0, 26, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 25, 0, 0, 0, 90, 0, 0, 0, 117, 0, 0, 0, 90, 0, 0, 0, 31, 0, 0, 0, 30, 0, 0, 0, 90, 0, 0, 0, 117, 0, 0, 0, 90, 0, 0, 0, 25, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 26, 0, 0, 0, 90, 0, 0, 0, 117, 0, 0, 0, 90, 0, 0, 0, 28, 0, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, 26, 0, 0, 0, 90, 0, 0, 0, 117, 0, 0, 0, 90, 0, 0, 0, 26, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 26, 0, 0, 0, 90, 0, 0, 0, 116, 0, 0, 0, 90, 0, 0, 0, 26, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 26, 0, 0, 0, 90, 0, 0, 0, 116, 0, 0, 0, 90, 0, 0, 0, 26, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 25, 0, 0, 0, 92, 0, 0, 0, 117, 0, 0, 0, 92, 0, 0, 0, 26, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 26, 0, 0, 0, 92, 0, 0, 0, 117, 0, 0, 0, 92, 0, 0, 0, 25, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 24, 0, 0, 0, 92, 0, 0, 0, 117, 0, 0, 0, 92, 0, 0, 0, 26, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 26, 0, 0, 0, 92, 0, 0, 0, 117, 0, 0, 0, 92, 0, 0, 0, 24, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 24, 0, 0, 0, 93, 0, 0, 0, 117, 0, 0, 0, 92, 0, 0, 0, 25, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 25, 0, 0, 0, 92, 0, 0, 0, 117, 0, 0, 0, 93, 0, 0, 0, 24, 0, 0, 0, 1, 0, 0, 0, 0,
    0, 0, 0, 1, 0, 0, 0, 24, 0, 0, 0, 93, 0, 0, 0, 118, 0, 0, 0, 93, 0, 0, 0, 25, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 25, 0, 0, 0, 93, 0, 0, 0, 118, 0, 0, 0, 93, 0, 0, 0, 23, 0, 0, 0, 1,
    0, 0, 0, 17, 0, 0, 0, 94, 0, 0, 0, 119, 0, 0, 0, 93, 0, 0, 0, 24, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 24, 0, 0, 0, 93, 0, 0, 0, 119, 0, 0, 0, 94, 0, 0, 0, 16,
    0, 0, 0, 46, 0, 0, 0, 111, 0, 0, 0, 94, 0, 0, 0, 24, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 24, 0, 0, 0, 94, 0, 0, 0, 111, 0, 0, 0, 46,
    0, 0, 0, 7, 0, 0, 0, 48, 0, 0, 0, 20, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 20, 0, 0, 0, 48, 0, 0, 0, 7,
    0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

  Ihandle* image = IupImageRGBA(24, 16, imgdata);
  return image;
}

static void iFlatTabsInitializeImages(void)
{
  Ihandle *image;

  unsigned char img_close[ITABS_CLOSE_SIZE * ITABS_CLOSE_SIZE] =
  {
    1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
    1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
    0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 0,
    0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0,
    0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0,
    0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0,
    0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0,
    0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0,
    0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 0,
    1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
    1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
  };

  image = IupImage(ITABS_CLOSE_SIZE, ITABS_CLOSE_SIZE, img_close);
  IupSetAttribute(image, "0", "BGCOLOR");
  IupSetAttribute(image, "1", "0 0 0");
  IupSetHandle("IMGFLATCLOSE", image);

  image = IupImage(ITABS_CLOSE_SIZE, ITABS_CLOSE_SIZE, img_close);
  IupSetAttribute(image, "0", "BGCOLOR");
  IupSetAttribute(image, "1", "255 255 255");
  IupSetHandle("IMGFLATCLOSEPRESS", image);

  image = load_image_expand_down();
  IupSetHandle("IupFlatExpandDown", image);

  image = load_image_expand_up();
  IupSetHandle("IupFlatExpandUp", image);
}

static int iFlatTabsGetExtraWidth(Ihandle* ih, int extra_buttons, int img_position, int horiz_padding, int vert_padding, int *extra_height);
static int iFlatTabsGetExtraHeight(Ihandle* ih, int extra_buttons, int img_position, int horiz_padding, int vert_padding, int *extra_width);
static void iFlatTabsGetTabSize(Ihandle* ih, int fixedwidth, int horiz_padding, int vert_padding, int show_close, int pos, int *tab_w, int *tab_h);
static int  iFlatTabsGetTitleSize(Ihandle* ih, int *title_width, int* title_height, int has_scrolled_size);

static void iFlatTabsUpdateScrollPos(Ihandle* ih, Ihandle* child)
{
  int child_pos = IupGetChildPos(ih, child);
  int scroll_pos = iupAttribGetInt(ih, "_IUPFTABS_SCROLLPOS");

  if (child_pos == scroll_pos)
    return; /* already visible */

  if (child_pos < scroll_pos)
  {
    /* if before current scroll simply scroll to child */
    iupAttribSetInt(ih, "_IUPFTABS_SCROLLPOS", child_pos);
    return;
  }

  if (child_pos > scroll_pos)
  {
    int extra_size, horiz_padding, vert_padding;
    int tab_w, tab_h, pos;
    int* visible_width, check_size;
    int img_position = iupFlatGetImagePosition(iupAttribGetStr(ih, "TABSIMAGEPOSITION"));
    int extra_buttons = iupAttribGetInt(ih, "EXTRABUTTONS");
    int fixedwidth = iupAttribGetInt(ih, "FIXEDWIDTH");
    int show_close = iupAttribGetBoolean(ih, "SHOWCLOSE");
    int tabType = iupAttribGetInt(ih, "_IUPTAB_TYPE");
    int count = IupGetChildCount(ih);
    int old_scroll_pos = scroll_pos;
    int size;

    iupAttribGetIntInt(ih, "TABSPADDING", &horiz_padding, &vert_padding, 'x');

    if (tabType == ITABS_TOP || tabType == ITABS_BOTTOM)
      extra_size = iFlatTabsGetExtraWidth(ih, extra_buttons, img_position, horiz_padding, vert_padding, NULL);
    else
      extra_size = iFlatTabsGetExtraHeight(ih, extra_buttons, img_position, horiz_padding, vert_padding, NULL);

    visible_width = calloc(count, sizeof(int));

    check_size = 0;
    for (pos = scroll_pos, child = ih->firstchild; child && pos <= child_pos; child = child->brother, pos++)
    {
      int tabvisible = iupAttribGetBooleanId(ih, "TABVISIBLE", pos);
      if (tabvisible)
      {
        int tab;

        iFlatTabsGetTabSize(ih, fixedwidth, horiz_padding, vert_padding, show_close, pos, &tab_w, &tab_h);

        tab = (tabType == ITABS_TOP || tabType == ITABS_BOTTOM) ? tab_w : tab_h;

        visible_width[pos] = tab;
        check_size += tab;
      }
    }

    size = (tabType == ITABS_TOP || tabType == ITABS_BOTTOM) ? ih->currentwidth - extra_size : ih->currentheight - extra_size;


    while (check_size > size && scroll_pos < child_pos)
    {
      scroll_pos++;

      check_size = 0;
      for (pos = scroll_pos; pos <= child_pos; pos++)
        check_size += visible_width[pos];
    }

    if (old_scroll_pos != scroll_pos)
      iupAttribSetInt(ih, "_IUPFTABS_SCROLLPOS", scroll_pos);

    free(visible_width);
  }
}

static Ihandle* iFlatTabsGetCurrentTab(Ihandle* ih)
{
  return (Ihandle*)iupAttribGet(ih, "_IUPFTABS_VALUE_HANDLE");
}

static void iFlatTabsSetCurrentTab(Ihandle* ih, Ihandle* child)
{
  Ihandle* current_child = iFlatTabsGetCurrentTab(ih);
  if (current_child)
    IupSetAttribute(current_child, "VISIBLE", "No");

  iupAttribSet(ih, "_IUPFTABS_VALUE_HANDLE", (char*)child);
  if (child)
  {
    IupSetAttribute(child, "VISIBLE", "Yes");

    iFlatTabsUpdateScrollPos(ih, child);
  }

  if (!iupAttribGetBoolean(ih, "CHILDSIZEALL"))
    IupRefresh(ih);  /* recalculate layout if CHILDSIZEALL=no */

  if (ih->handle)
    iupdrvPostRedraw(ih);
}

static void iFlatTabsSetTabFont(Ihandle* ih, int pos)
{
  char* font = iupAttribGetId(ih, "TABFONT", pos);
  if (font)
    iupAttribSetStr(ih, "DRAWFONT", font);
  else
  {
    char* tabs_font = iupAttribGet(ih, "TABSFONT");
    iupAttribSetStr(ih, "DRAWFONT", tabs_font);
  }
}

static void iFlatTabsGetTabSize(Ihandle* ih, int fixedwidth, int horiz_padding, int vert_padding, int show_close, int pos, int *tab_w, int *tab_h)
{
  int img_position = iupFlatGetImagePosition(iupAttribGetStr(ih, "TABSIMAGEPOSITION"));
  int spacing = iupAttribGetInt(ih, "TABSIMAGESPACING");
  char* tab_image = iupAttribGetId(ih, "TABIMAGE", pos);
  char* tab_title = iupAttribGetId(ih, "TABTITLE", pos);
  double text_orientation = iupAttribGetDouble(ih, "TABSTEXTORIENTATION");
  int tabType = iupAttribGetInt(ih, "_IUPTAB_TYPE");
  int *tab = (tabType == ITABS_TOP || tabType == ITABS_BOTTOM) ? tab_w : tab_h;
  char num_title[30] = "";

  iFlatTabsSetTabFont(ih, pos);

  if (!tab_image && !tab_title)
  {
    sprintf(num_title, "%d", pos);
    tab_title = num_title;
  }

  iupFlatDrawGetIconSize(ih, img_position, spacing, horiz_padding, vert_padding, tab_image, tab_title, tab_w, tab_h, text_orientation);

  if (fixedwidth)
  {
    *tab = fixedwidth;
    *tab += 2 * horiz_padding;
  }

  if (show_close)
  {
    if (text_orientation > 45.)
      *tab_h += ITABS_CLOSE_SPACING + ITABS_CLOSE_SIZE + ITABS_CLOSE_BORDER;
    else
      *tab_w += ITABS_CLOSE_SPACING + ITABS_CLOSE_SIZE + ITABS_CLOSE_BORDER;
  }
}

static int iFlatTabsGetTitleSize(Ihandle* ih, int *title_width, int* title_height, int has_scrolled_size)
{
  int vert_padding, horiz_padding, extra_buttons;
  int tab_w, tab_h, pos, title_max, scroll_size;
  int fixedwidth = iupAttribGetInt(ih, "FIXEDWIDTH");
  int show_close = iupAttribGetBoolean(ih, "SHOWCLOSE");
  int scroll_pos = iupAttribGetInt(ih, "_IUPFTABS_SCROLLPOS");
  int tabType = iupAttribGetInt(ih, "_IUPTAB_TYPE");
  Ihandle* child;

  iupAttribGetIntInt(ih, "TABSPADDING", &horiz_padding, &vert_padding, 'x');

  *title_width = 0;
  *title_height = 0;
  title_max = 0;

  for (pos = 0, child = ih->firstchild; child; child = child->brother, pos++)
  {
    int tabvisible = iupAttribGetBooleanId(ih, "TABVISIBLE", pos);
    if (tabvisible)
    {
      if ((tabType == ITABS_TOP || tabType == ITABS_BOTTOM) && pos == scroll_pos && has_scrolled_size)
        *title_width = 0;

      if ((tabType == ITABS_LEFT || tabType == ITABS_RIGHT) && pos == scroll_pos && has_scrolled_size)
        *title_height = 0;

      iFlatTabsGetTabSize(ih, fixedwidth, horiz_padding, vert_padding, show_close, pos, &tab_w, &tab_h);

      if (tabType == ITABS_TOP || tabType == ITABS_BOTTOM)
      {
        if (tab_h > *title_height)
          *title_height = tab_h;

        if (tab_w > title_max)
          title_max = tab_w;
      }
      else
      {
        if (tab_w > *title_width)
          *title_width = tab_w;

        if (tab_h > title_max)
          title_max = tab_h;
      }

      if (tabType == ITABS_TOP || tabType == ITABS_BOTTOM)
        *title_width += tab_w;
      else
        *title_height += tab_h;
    }
  }

  if (pos == 0) /* empty tabs, let the application define at least TABTITLE0 */
  {
    iFlatTabsGetTabSize(ih, fixedwidth, horiz_padding, vert_padding, show_close, pos, &tab_w, &tab_h);
    *title_height = tab_h;
    *title_width = tab_w;
  }

  extra_buttons = iupAttribGetInt(ih, "EXTRABUTTONS");
  if (extra_buttons)
  {
    /* must have space for the extra buttons in the title on the opposite direction */
    int img_position = iupFlatGetImagePosition(iupAttribGetStr(ih, "TABSIMAGEPOSITION"));
    if (tabType == ITABS_TOP || tabType == ITABS_BOTTOM)
    {
      int h;
      (void)iFlatTabsGetExtraWidth(ih, extra_buttons, img_position, horiz_padding, vert_padding, &h);
      if (h > *title_height)
        *title_height = h;
    }
    else
    {
      int w;
      (void)iFlatTabsGetExtraHeight(ih, extra_buttons, img_position, horiz_padding, vert_padding, &w);
      if (w > *title_width)
        *title_width = w;
    }
  }

  if (tabType == ITABS_TOP || tabType == ITABS_BOTTOM)
  {
    if (*title_height > title_max)
      scroll_size = title_max / 2;
    else
      scroll_size = *title_height / 2;
  }
  else
  {
    if (*title_width > title_max)
      scroll_size = title_max / 2;
    else
      scroll_size = *title_width / 2;
  }

  return scroll_size;
}

static void iFlatTabsSetExtraFont(Ihandle* ih, int id)
{
  char* font = iupAttribGetId(ih, "EXTRAFONT", id);
  if (font)
    iupAttribSetStr(ih, "DRAWFONT", font);
  else
  {
    char* tabs_font = iupAttribGet(ih, "TABSFONT");
    iupAttribSetStr(ih, "DRAWFONT", tabs_font);
  }
}

static int iFlatTabsGetExtraWidthId(Ihandle* ih, int id, int img_position, int horiz_padding, int vert_padding, int *extra_height)
{
  char* imagename = iupAttribGetId(ih, "EXTRAIMAGE", id);
  char* title = iupAttribGetId(ih, "EXTRATITLE", id);
  int spacing = iupAttribGetInt(ih, "TABSIMAGESPACING");
  double text_orientation = iupAttribGetDouble(ih, "TABSTEXTORIENTATION");
  int w, h;

  iFlatTabsSetExtraFont(ih, id);

  iupFlatDrawGetIconSize(ih, img_position, spacing, horiz_padding, vert_padding, imagename, title, &w, &h, text_orientation);

  if (extra_height)
    *extra_height = h;

  return w;
}

static int iFlatTabsGetExtraWidth(Ihandle* ih, int extra_buttons, int img_position, int horiz_padding, int vert_padding, int *extra_height)
{
  int extra_width = 0, i, h;

  if (extra_buttons == 0)
    return 0;

  if (extra_height)
    *extra_height = 0;

  for (i = 1; i <= extra_buttons; i++)
  {
    int w = iFlatTabsGetExtraWidthId(ih, i, img_position, horiz_padding, vert_padding, &h);
    extra_width += w;

    if (extra_height && h > *extra_height)
      *extra_height = h;
  }

  return extra_width;
}

static int iFlatTabsGetExtraHeightId(Ihandle* ih, int id, int img_position, int horiz_padding, int vert_padding, int *extra_width)
{
  char* imagename = iupAttribGetId(ih, "EXTRAIMAGE", id);
  char* title = iupAttribGetId(ih, "EXTRATITLE", id);
  int spacing = iupAttribGetInt(ih, "TABSIMAGESPACING");
  double text_orientation = iupAttribGetDouble(ih, "TABSTEXTORIENTATION");
  int w, h;

  iFlatTabsSetExtraFont(ih, id);

  iupFlatDrawGetIconSize(ih, img_position, spacing, horiz_padding, vert_padding, imagename, title, &w, &h, text_orientation);

  if (extra_width)
    *extra_width = w;

  return h;
}

static int iFlatTabsGetExtraHeight(Ihandle* ih, int extra_buttons, int img_position, int horiz_padding, int vert_padding, int *extra_width)
{
  int extra_height = 0, i, w;

  if (extra_buttons == 0)
    return 0;

  if (extra_width)
    *extra_width = 0;

  for (i = 1; i <= extra_buttons; i++)
  {
    int h = iFlatTabsGetExtraHeightId(ih, i, img_position, horiz_padding, vert_padding, &w);
    extra_height += h;

    if (extra_width && w > *extra_width)
      *extra_width = w;
  }

  return extra_height;
}

static int iFlatTabsGetExtraActive(Ihandle* ih, int id)
{
  if (iupAttribGetId(ih, "EXTRAACTIVE", id) == NULL) /* not defined */
    return 1; /* default is yes */

  return iupAttribGetBooleanId(ih, "EXTRAACTIVE", id);
}

static void iFlatTabsGetAlignment(const char* alignment, int *horiz_alignment, int *vert_alignment)
{
  char value1[30], value2[30];
  iupStrToStrStr(alignment, value1, value2, ':');
  *horiz_alignment = iupFlatGetHorizontalAlignment(value1);
  *vert_alignment = iupFlatGetVerticalAlignment(value2);
}

static int iFlatTabsArrowSize(int scroll_size)
{
  return (scroll_size + 1) / 2;
}

static void iFlatTabsDrawScrollTopButton(IdrawCanvas* dc, const char *tabs_bgcolor, const char *tabs_forecolor, int active, int title_pos, int title_width, int scroll_size)
{
  int arrow_size = iFlatTabsArrowSize(scroll_size);

  int x = title_pos + (title_width - arrow_size) / 2;
  int y = (scroll_size - arrow_size) / 2;

  iupFlatDrawArrow(dc, x, y, arrow_size, tabs_forecolor, tabs_bgcolor, active, IUPDRAW_ARROW_TOP);
}

static void iFlatTabsDrawScrollBottomButton(IdrawCanvas* dc, const char *tabs_bgcolor, const char *tabs_forecolor, int active, int title_pos, int title_width, int height, int scroll_size)
{
  int arrow_size = iFlatTabsArrowSize(scroll_size);

  int x = title_pos + (title_width - arrow_size) / 2;
  int y = height - 1 - scroll_size + (scroll_size - arrow_size) / 2;

  iupFlatDrawArrow(dc, x, y, arrow_size, tabs_forecolor, tabs_bgcolor, active, IUPDRAW_ARROW_BOTTOM);
}

static void iFlatTabsDrawScrollLeftButton(IdrawCanvas* dc, const char *tabs_bgcolor, const char *tabs_forecolor, int active, int title_pos, int title_height, int scroll_size)
{
  int arrow_size = iFlatTabsArrowSize(scroll_size);

  int x = (scroll_size - arrow_size) / 2;
  int y = title_pos + (title_height - arrow_size) / 2;

  iupFlatDrawArrow(dc, x, y, arrow_size, tabs_forecolor, tabs_bgcolor, active, IUPDRAW_ARROW_LEFT);
}

static void iFlatTabsDrawScrollRightButton(IdrawCanvas* dc, const char *tabs_bgcolor, const char *tabs_forecolor, int active, int title_pos, int title_height, int width, int scroll_size)
{
  int arrow_size = iFlatTabsArrowSize(scroll_size);

  int x = width - 1 - scroll_size + (scroll_size - arrow_size) / 2;
  int y = title_pos + (title_height - arrow_size) / 2;

  iupFlatDrawArrow(dc, x, y, arrow_size, tabs_forecolor, tabs_bgcolor, active, IUPDRAW_ARROW_RIGHT);
}

static void iFlatTabsGetCloseRect(int x, int y, int w, int h, double text_orientation, int *close_x, int *close_y)
{
    if (text_orientation < 45.)
    {
      *close_x = x + w - ITABS_CLOSE_BORDER - ITABS_CLOSE_SIZE;
      *close_y = y + (h - ITABS_CLOSE_SIZE) / 2;
    }
    else
    {
      *close_x = x + (w - ITABS_CLOSE_SIZE) / 2;
      *close_y = y + ITABS_CLOSE_BORDER + (ITABS_CLOSE_SIZE / 2);
    }
}

static int iFlatTabsRedraw_CB(Ihandle* ih)
{
  Ihandle* current_child = iFlatTabsGetCurrentTab(ih);
  char* bgcolor = iupAttribGetStr(ih, "BGCOLOR");
  char* forecolor = iupAttribGetStr(ih, "FORECOLOR");
  char* highcolor = iupAttribGetStr(ih, "HIGHCOLOR");
  char* tabs_bgcolor = iupAttribGet(ih, "TABSBACKCOLOR");  /* don't get with default value, if NULL will use from parent */
  char* tabs_forecolor = iupAttribGetStr(ih, "TABSFORECOLOR");
  char* tabs_highcolor = iupAttribGetStr(ih, "TABSHIGHCOLOR");
  int img_position = iupFlatGetImagePosition(iupAttribGetStr(ih, "TABSIMAGEPOSITION"));
  char* alignment = iupAttribGetStr(ih, "TABSALIGNMENT");
  int text_flags = iupDrawGetTextFlags(ih, "TABSTEXTALIGNMENT", "TABSTEXTWRAP", "TABSTEXTELLIPSIS");
  double text_orientation = iupAttribGetDouble(ih, "TABSTEXTORIENTATION");
  int active = IupGetInt(ih, "ACTIVE");  /* native implementation */
  int spacing = iupAttribGetInt(ih, "TABSIMAGESPACING");
  int horiz_padding, vert_padding;
  int show_lines = iupAttribGetBoolean(ih, "SHOWLINES");
  int title_width, title_height;
  int fixedwidth = iupAttribGetInt(ih, "FIXEDWIDTH");
  Ihandle* child;
  int pos, horiz_alignment, vert_alignment, tab_x = 0, tab_y = 0;
  long line_color = 0;
  int show_close = iupAttribGetBoolean(ih, "SHOWCLOSE");
  int tab_highlighted = iupAttribGetInt(ih, "_IUPFTABS_HIGHLIGHTED");
  int extra_size;
  int extra_buttons = iupAttribGetInt(ih, "EXTRABUTTONS");
  int tabType = iupAttribGetInt(ih, "_IUPTAB_TYPE");
  int child_x_pos, child_y_pos, child_height, child_width, title_x_pos, title_y_pos,
      title_xmax, title_ymax, scroll_pos, scroll_size, button_gap;

  IdrawCanvas* dc = iupdrvDrawCreateCanvas(ih);

  scroll_size = iFlatTabsGetTitleSize(ih, &title_width, &title_height, 1);

  child_x_pos = (tabType == ITABS_LEFT) ? title_width : 0;
  child_y_pos = (tabType == ITABS_TOP) ? title_height : 0;
  child_height = (tabType == ITABS_TOP || tabType == ITABS_BOTTOM) ? ih->currentheight - title_height : ih->currentheight;
  child_width = (tabType == ITABS_TOP || tabType == ITABS_BOTTOM) ? ih->currentwidth : ih->currentwidth - title_width;
  title_x_pos = (tabType == ITABS_RIGHT) ? ih->currentwidth - title_width : 0;
  title_y_pos = (tabType == ITABS_BOTTOM) ? ih->currentheight - title_height : 0;
  title_xmax = (tabType == ITABS_TOP || tabType == ITABS_BOTTOM) ? ih->currentwidth - 1 : title_x_pos + title_width -1;
  title_ymax = (tabType == ITABS_TOP || tabType == ITABS_BOTTOM) ? title_y_pos + title_height - 1 : ih->currentheight - 1;
  scroll_pos = iupAttribGetInt(ih, "_IUPFTABS_SCROLLPOS");
  button_gap = (scroll_pos  > 0) ? scroll_size : 0;


  if (!tabs_bgcolor)
    tabs_bgcolor = iupBaseNativeParentGetBgColorAttrib(ih);

  /* draw child area background */
  iupFlatDrawBox(dc, child_x_pos, child_x_pos + child_width - 1, child_y_pos, child_y_pos + child_height - 1, bgcolor, NULL, 1);

  /* title area background */
  iupFlatDrawBox(dc, title_x_pos, title_xmax, title_y_pos, title_ymax, tabs_bgcolor, NULL, 1);

  iupAttribGetIntInt(ih, "TABSPADDING", &horiz_padding, &vert_padding, 'x');
  iFlatTabsGetAlignment(alignment, &horiz_alignment, &vert_alignment);

  if (tabType == ITABS_TOP || tabType == ITABS_BOTTOM)
    extra_size = iFlatTabsGetExtraWidth(ih, extra_buttons, img_position, horiz_padding, vert_padding, NULL);
  else
    extra_size = iFlatTabsGetExtraHeight(ih, extra_buttons, img_position, horiz_padding, vert_padding, NULL);

  if (show_lines)
  {
    char* title_line_color = iupAttribGetStr(ih, "TABSLINECOLOR");
    line_color = iupDrawStrToColor(title_line_color, line_color);

    if (tabType == ITABS_TOP || tabType == ITABS_BOTTOM)
    {
      /* tab bottom horizontal and top children horizontal */
      int height = (tabType == ITABS_TOP) ? title_height - 1 : child_height;
      iupdrvDrawLine(dc, 0, height, ih->currentwidth - 1, height, line_color, IUP_DRAW_STROKE, 1);
    }
    else
    {
      /* tab bottom horizontal and top children horizontal */
      int width = (tabType == ITABS_LEFT) ? title_xmax - 1 : child_width;
      iupdrvDrawLine(dc, width, 0, width, ih->currentheight - 1, line_color, IUP_DRAW_STROKE, 1);
    }
  }

  if ((tabType == ITABS_TOP || tabType == ITABS_BOTTOM) && scroll_pos > 0)
    tab_x += scroll_size;

  if ((tabType == ITABS_LEFT || tabType == ITABS_RIGHT) && scroll_pos > 0)
    tab_y += scroll_size;

  child = ih->firstchild;
  for (pos = 0; pos < scroll_pos; pos++)
    child = child->brother;

  for (pos = scroll_pos; child; child = child->brother, pos++)
  {
    int tabvisible = iupAttribGetBooleanId(ih, "TABVISIBLE", pos);
    if (tabvisible)
    {
      char* tab_image = iupAttribGetId(ih, "TABIMAGE", pos);
      char* tab_title = iupAttribGetId(ih, "TABTITLE", pos);
      char* tab_backcolor = iupAttribGetId(ih, "TABBACKCOLOR", pos);
      char* tab_forecolor = iupAttribGetId(ih, "TABFORECOLOR", pos);
      char* tab_highcolor = iupAttribGetId(ih, "TABHIGHCOLOR", pos);
      char* background_color = NULL;
      int tab_w, tab_h, tab_active, reset_clip;
      char* foreground_color;
      int icon_x, icon_y, icon_width, icon_height, make_inactive = 0;
      char num_title[30] = "";

      if (!active)
        tab_active = active;
      else
        tab_active = iupAttribGetBooleanId(ih, "TABACTIVE", pos);

      if (!tab_image && !tab_title)
      {
        sprintf(num_title, "%d", pos);
        tab_title = num_title;
      }

      iFlatTabsGetTabSize(ih, fixedwidth, horiz_padding, vert_padding, show_close, pos, &tab_w, &tab_h);  /* this will also set any id based font, tab_title and tab_image are retrieved again inside */

      if (current_child == child)
      {
        /* current tab is always drawn with these colors */
        background_color = bgcolor;
        foreground_color = forecolor;

        iupAttribSet(ih, "SECONDARYTITLE", NULL);
      }
      else
      {
        iupAttribSet(ih, "SECONDARYTITLE", "1");

        /* other tabs are drawn with these colors */
        if (tab_backcolor)
          background_color = tab_backcolor;

        foreground_color = tabs_forecolor;
        if (tab_forecolor)
          foreground_color = tab_forecolor;

        if (pos == tab_highlighted)
        {
          if (highcolor)
            foreground_color = highcolor;
          else
            foreground_color = forecolor;

          if (tabs_highcolor || tab_highcolor)
          {
            if (tab_highcolor)
              background_color = tab_highcolor;
            else
              background_color = tabs_highcolor;
          }
        }
      }

      if (tab_image)
      {
        make_inactive = 0;

        if (!tab_active)
        {
          char* tab_image_inative = iupAttribGetId(ih, "TABIMAGEINACTIVE", pos);
          if (!tab_image_inative)
            make_inactive = 1;
          else
            tab_image = tab_image_inative;
        }
        else if (pos == tab_highlighted)
        {
          char* tab_image_highlight = iupAttribGetId(ih, "TABIMAGEHIGHLIGHT", pos);
          if (tab_image_highlight)
            tab_image = tab_image_highlight;
        }
      }

      reset_clip = 0;
      if ((tabType == ITABS_TOP || tabType == ITABS_BOTTOM) && title_width > ih->currentwidth - extra_size - button_gap) /* has right scroll button */
      {
        if (tab_x + tab_w > ih->currentwidth - extra_size - scroll_size)
        {
          iupdrvDrawSetClipRect(dc, tab_x, title_y_pos, ih->currentwidth - extra_size - scroll_size, title_y_pos + title_height);
          reset_clip = 1;
        }
      }
      else if ((tabType == ITABS_LEFT || tabType == ITABS_RIGHT) && title_height > ih->currentheight - extra_size - button_gap) /* has right scroll button */
      {
        if (tab_y + tab_h > ih->currentheight - extra_size - scroll_size)
        {
          iupdrvDrawSetClipRect(dc, title_x_pos, tab_y, title_x_pos + title_width, ih->currentheight - extra_size - scroll_size);
          reset_clip = 1;
        }
      }

      /* draw tab title background */
      if (background_color)
      {
        int x1, x2, y1, y2;
        if (tabType == ITABS_TOP || tabType == ITABS_BOTTOM)
        {
          x1 = tab_x;
          y1 = title_y_pos;
          x2 = tab_x + tab_w;
          y2 = title_y_pos + title_height - 1;
        }
        else
        {
          x1 = title_x_pos;
          x2 = title_x_pos + title_width - 1;
          y1 = tab_y;
          y2 = tab_y + tab_h;
        }
        iupFlatDrawBox(dc, x1, x2, y1, y2, background_color, NULL, 1);
      }
      else
        background_color = tabs_bgcolor;

      if (show_lines && current_child == child)
      {
        if (tabType == ITABS_TOP || tabType == ITABS_BOTTOM)
        {
          if (tabType == ITABS_TOP)
            iupdrvDrawLine(dc, tab_x, title_y_pos, tab_x + tab_w, title_y_pos, line_color, IUP_DRAW_STROKE, 1); /* tab top horizontal */
          iupdrvDrawLine(dc, tab_x, title_y_pos, tab_x, title_y_pos + title_height - 1, line_color, IUP_DRAW_STROKE, 1); /* tab left vertical */
          iupdrvDrawLine(dc, tab_x + tab_w, title_y_pos, tab_x + tab_w, title_y_pos + title_height - 1, line_color, IUP_DRAW_STROKE, 1); /* tab right vertical */
          if (tabType == ITABS_BOTTOM)
            iupdrvDrawLine(dc, tab_x, title_y_pos + title_height - 1, tab_x + tab_w, title_y_pos + title_height - 1, line_color, IUP_DRAW_STROKE, 1); /* tab bottom horizontal */
        }
        else
        {
          if (tabType == ITABS_LEFT)
            iupdrvDrawLine(dc, title_x_pos, tab_y, title_x_pos, tab_y + tab_h, line_color, IUP_DRAW_STROKE, 1); /* tab left vertical */
          iupdrvDrawLine(dc, title_x_pos, tab_y, title_x_pos + title_width - 1, tab_y, line_color, IUP_DRAW_STROKE, 1); /* tab left vertical */
          iupdrvDrawLine(dc, title_x_pos, tab_y + tab_h, title_x_pos + title_width - 1, tab_y + tab_h, line_color, IUP_DRAW_STROKE, 1); /* tab right vertical */
          if (tabType == ITABS_RIGHT)
            iupdrvDrawLine(dc, title_x_pos + title_width - 1, tab_y, title_x_pos + title_width - 1, tab_y + tab_h, line_color, IUP_DRAW_STROKE, 1); /* tab bottom horizontal */
        }
      }

      icon_width = (tabType == ITABS_TOP || tabType == ITABS_BOTTOM) ? tab_w : title_width;
      icon_height = (tabType == ITABS_TOP || tabType == ITABS_BOTTOM) ? title_height : tab_h;
      icon_x = (tabType == ITABS_TOP || tabType == ITABS_BOTTOM) ? tab_x : title_x_pos;
      icon_y = (tabType == ITABS_TOP || tabType == ITABS_BOTTOM) ? title_y_pos : tab_y;
      if (show_close)
      {
        if (text_orientation > 45.0)
        {
          icon_height -= ITABS_CLOSE_SPACING + ITABS_CLOSE_SIZE + ITABS_CLOSE_BORDER;
          icon_y += ITABS_CLOSE_SPACING + ITABS_CLOSE_SIZE + ITABS_CLOSE_BORDER;
        }
        else
          icon_width -= ITABS_CLOSE_SPACING + ITABS_CLOSE_SIZE + ITABS_CLOSE_BORDER;
      }

      iupFlatDrawIcon(ih, dc, icon_x, icon_y, icon_width, icon_height,
                      img_position, spacing, horiz_alignment, vert_alignment, horiz_padding, vert_padding,
                      tab_image, make_inactive, tab_title, text_flags, text_orientation, foreground_color, background_color, tab_active);

      if (current_child == child && iupAttribGetInt(ih, "HASFOCUS") && iupAttribGetBoolean(ih, "FOCUSFEEDBACK"))
      {
        int x1, y1, x2, y2;

        if (tabType == ITABS_TOP || tabType == ITABS_BOTTOM)
        {
          x1 = tab_x + 3;
          y1 = title_y_pos + 3;
          x2 = tab_x + icon_width - 3;
          y2 = title_y_pos + icon_height - 3;
        }
        else
        {
          x1 = title_x_pos + 3;
          y1 = tab_y + 3;
          x2 = title_x_pos + icon_width - 3;
          y2 = tab_y + icon_height - 3;
        }
        if (show_close && text_orientation > 45.0)
        {
          y1 += ITABS_CLOSE_SPACING + ITABS_CLOSE_SIZE + ITABS_CLOSE_BORDER;
          y2 += ITABS_CLOSE_SPACING + ITABS_CLOSE_SIZE + ITABS_CLOSE_BORDER;
        }
        iupdrvDrawFocusRect(dc, x1, y1, x2, y2);
      }

      if (show_close)
      {
        int close_x, close_y;
        const char* imagename;
        int tab_close_high = iupAttribGetInt(ih, "_IUPFTABS_CLOSEHIGH");
        int tab_close_press = iupAttribGetInt(ih, "_IUPFTABS_CLOSEPRESS");

        if (tabType == ITABS_TOP || tabType == ITABS_BOTTOM)
          iFlatTabsGetCloseRect(tab_x, title_y_pos, tab_w, title_height, text_orientation, &close_x, &close_y);
        else
          iFlatTabsGetCloseRect(title_x_pos, tab_y, title_width, tab_h, text_orientation, &close_x, &close_y);

        if (pos == tab_close_press)
        {
          background_color = iupAttribGetStr(ih, "CLOSEPRESSCOLOR");
          iupFlatDrawBox(dc, close_x - ITABS_CLOSE_BORDER, close_x + ITABS_CLOSE_SIZE + ITABS_CLOSE_BORDER, close_y - ITABS_CLOSE_BORDER, close_y + ITABS_CLOSE_SIZE + ITABS_CLOSE_BORDER, background_color, NULL, 1);
        }
        else if (pos == tab_close_high)
        {
          background_color = iupAttribGetStr(ih, "CLOSEHIGHCOLOR");
          iupFlatDrawBox(dc, close_x - ITABS_CLOSE_BORDER, close_x + ITABS_CLOSE_SIZE + ITABS_CLOSE_BORDER, close_y - ITABS_CLOSE_BORDER, close_y + ITABS_CLOSE_SIZE + ITABS_CLOSE_BORDER, background_color, NULL, 1);
        }

        imagename = iupFlatGetImageName(ih, "CLOSEIMAGE", NULL, pos == tab_close_press, pos == tab_close_high, tab_active, &make_inactive);
        iupdrvDrawImage(dc, imagename, make_inactive, background_color, close_x, close_y, -1, -1);
      }

      /* goto next tab area */
      if (tabType == ITABS_TOP || tabType == ITABS_BOTTOM)
        tab_x += tab_w;
      else
        tab_y += tab_h;

      if (reset_clip)
      {
        iupdrvDrawResetClip(dc);
        break;
      }
    }
  }

  if (scroll_pos > 0)
  {
    char* foreground_color = tabs_forecolor;
    if (tab_highlighted == ITABS_SB_LEFT)
    {
      if (highcolor)
        foreground_color = highcolor;
      else
        foreground_color = forecolor;
    }

    if (tabType == ITABS_TOP || tabType == ITABS_BOTTOM)
      iFlatTabsDrawScrollLeftButton(dc, tabs_bgcolor, foreground_color, active, title_y_pos, title_height, scroll_size);
    else
      iFlatTabsDrawScrollTopButton(dc, tabs_bgcolor, foreground_color, active, title_x_pos, title_width, scroll_size);
  }

  if (((tabType == ITABS_TOP || tabType == ITABS_BOTTOM) && title_width > ih->currentwidth - extra_size - button_gap) ||
      ((tabType == ITABS_LEFT || tabType == ITABS_RIGHT) && title_height > ih->currentheight - extra_size - button_gap))
  {
    char* foreground_color = tabs_forecolor;
    if (tab_highlighted == ITABS_SB_RIGHT)
    {
      if (highcolor)
        foreground_color = highcolor;
      else
        foreground_color = forecolor;
    }

    if (tabType == ITABS_TOP || tabType == ITABS_BOTTOM)
      iFlatTabsDrawScrollRightButton(dc, tabs_bgcolor, foreground_color, active, title_y_pos, title_height, ih->currentwidth - extra_size, scroll_size);
    else
      iFlatTabsDrawScrollBottomButton(dc, tabs_bgcolor, foreground_color, active, title_x_pos, title_width, ih->currentheight - extra_size, scroll_size);
  }

  if (extra_buttons)
  {
    int i, total_extra_size = 0, extra_id;
    int extra_active, make_inactive, extra_x, extra_y, extra_w, extra_h;
    int extra_horiz_alignment, extra_vert_alignment;
    int xmin, ymin, xmax, ymax;

    iupAttribSet(ih, "SECONDARYTITLE", "1");

    for (i = 1; i <= extra_buttons; i++)
    {
      const char* extra_image = iupAttribGetId(ih, "EXTRAIMAGE", i);
      char* extra_title = iupAttribGetId(ih, "EXTRATITLE", i);
      char* extra_alignment = iupAttribGetId(ih, "EXTRAALIGNMENT", i);
      char* extra_forecolor = iupAttribGetId(ih, "EXTRAFORECOLOR", i);
      int extra_press = iupAttribGetInt(ih, "_IUPFTABS_EXTRAPRESS");
      int draw_border = iupAttribGetBooleanId(ih, "EXTRASHOWBORDER", i);
      int border_width = iupAttribGetIntId(ih, "EXTRABORDERWIDTH", i);
      int selected = iupAttribGetIntId(ih, "EXTRAVALUE", i);
      int image_pressed;

      extra_horiz_alignment = horiz_alignment; 
      extra_vert_alignment = vert_alignment;
      iFlatTabsGetAlignment(extra_alignment, &extra_horiz_alignment, &extra_vert_alignment);

      if (!active)
        extra_active = 0;
      else
        extra_active = iFlatTabsGetExtraActive(ih, i);

      if (!extra_forecolor)
        extra_forecolor = tabs_forecolor;

      extra_id = ITABS_EXTRABUT2TABID(i);

      if (tabType == ITABS_TOP || tabType == ITABS_BOTTOM)
      {
        extra_w = iFlatTabsGetExtraWidthId(ih, i, img_position, horiz_padding, vert_padding, NULL);  /* this will also set any id based font */
        extra_h = title_height;
        extra_x = ih->currentwidth - total_extra_size - extra_w;
        extra_y = title_y_pos;
        xmin = extra_x + horiz_padding / 2;
        xmax = extra_x + extra_w - horiz_padding / 2;
        ymin = title_y_pos + vert_padding / 2;
        ymax = title_y_pos + title_height - 1 - vert_padding / 2;
      }
      else
      {
        extra_h = iFlatTabsGetExtraHeightId(ih, i, img_position, horiz_padding, vert_padding, NULL);  /* this will also set any id based font */
        extra_w = title_width;
        extra_y = ih->currentheight - total_extra_size - extra_h;
        extra_x = title_x_pos;
        xmin = title_x_pos + horiz_padding / 2;
        xmax = title_x_pos + title_width - 1 - horiz_padding / 2;
        ymin = extra_y + vert_padding / 2;
        ymax = extra_y + extra_h - vert_padding / 2;
      }

      if (extra_active)
      {
        if (extra_press == extra_id || (selected && tab_highlighted != extra_id))
        {
          char* extra_presscolor = iupAttribGetId(ih, "EXTRAPRESSCOLOR", i);
          if (!extra_presscolor)
            extra_presscolor = IUP_FLAT_PRESSCOLOR;

          iupFlatDrawBox(dc, xmin, xmax, ymin, ymax, extra_presscolor, NULL, 1);
          draw_border = 1;
        }
        else if (tab_highlighted == extra_id)
        {
          char* extra_highcolor = iupAttribGetId(ih, "EXTRAHIGHCOLOR", i);
          if (!extra_highcolor)
            extra_highcolor = IUP_FLAT_HIGHCOLOR;

          iupFlatDrawBox(dc, xmin, xmax, ymin, ymax, extra_highcolor, NULL, 1);
          draw_border = 1;
        }
      }

      if (draw_border && border_width != 0)
      {
        char* bordercolor = iupAttribGetId(ih, "EXTRABORDERCOLOR", i);
        if (!bordercolor)
          bordercolor = IUP_FLAT_BORDERCOLOR;

        iupFlatDrawBorder(dc, xmin, xmax, ymin, ymax, border_width, bordercolor, tabs_bgcolor, extra_active);
      }

      image_pressed = extra_press == extra_id;
      if (selected && !image_pressed && extra_image)
        image_pressed = 1;

      extra_image = iupFlatGetImageNameId(ih, "EXTRAIMAGE", i, extra_image, image_pressed, tab_highlighted == extra_id, extra_active, &make_inactive);

      iupFlatDrawIcon(ih, dc, extra_x, extra_y, extra_w, extra_h,
                      img_position, spacing, extra_horiz_alignment, extra_vert_alignment, horiz_padding, vert_padding,
                      extra_image, make_inactive, extra_title, text_flags, text_orientation, extra_forecolor, tabs_bgcolor, extra_active);

      total_extra_size += (tabType == ITABS_TOP || tabType == ITABS_BOTTOM) ? extra_w : extra_h;
    }
  }

  iupAttribSet(ih, "SECONDARYTITLE", NULL);

  /* lines around children */
  if (show_lines)
  {
    if (tabType != ITABS_LEFT)
      iupdrvDrawLine(dc, child_x_pos, child_y_pos, child_x_pos, child_y_pos + child_height - 1, line_color, IUP_DRAW_STROKE, 1); /* left children vertical */
    if (tabType != ITABS_RIGHT)
      iupdrvDrawLine(dc, child_x_pos + child_width - 1, child_y_pos, child_x_pos + child_width - 1, child_y_pos + child_height - 1, line_color, IUP_DRAW_STROKE, 1); /* right children vertical */
    if (tabType != ITABS_BOTTOM)
      iupdrvDrawLine(dc, child_x_pos, child_y_pos + child_height - 1, child_x_pos + child_width - 1, child_y_pos + child_height - 1, line_color, IUP_DRAW_STROKE, 1); /* bottom children horizontal */
    if (tabType != ITABS_TOP)
      iupdrvDrawLine(dc, child_x_pos, child_y_pos, child_x_pos + child_width - 1, child_y_pos, line_color, IUP_DRAW_STROKE, 1); /* top children horizontal */
  }

  iupdrvDrawFlush(dc);

  iupdrvDrawKillCanvas(dc);

  return IUP_DEFAULT;
}

static int iFlatTabsResize_CB(Ihandle* ih, int width, int height)
{
  int scroll_pos = iupAttribGetInt(ih, "_IUPFTABS_SCROLLPOS");
  int tabType = iupAttribGetInt(ih, "_IUPTAB_TYPE");
  if (scroll_pos)
  {
    int title_width, title_height;
    int title_size, size;
    iFlatTabsGetTitleSize(ih, &title_width, &title_height, 0);

    title_size = (tabType == ITABS_TOP || tabType == ITABS_BOTTOM) ? title_width : title_height;
    size = (tabType == ITABS_TOP || tabType == ITABS_BOTTOM) ? width : height;

    if (title_size > size)
    {
      /* tabs are larger than the element, leave scroll_pos as it is */
      return IUP_DEFAULT;
    }

    /* tabs fit in element area, reset scroll_pos */
    iupAttribSetInt(ih, "_IUPFTABS_SCROLLPOS", 0);
  }

  (void)height;
  return IUP_DEFAULT;
}

static int iFlatTabsCallTabChange(Ihandle* ih, Ihandle* prev_child, int prev_pos, Ihandle* child)
{
  IFnnn cb = (IFnnn)IupGetCallback(ih, "TABCHANGE_CB");
  int ret = IUP_DEFAULT;

  if (cb)
    ret = cb(ih, child, prev_child);
  else
  {
    IFnii cb2 = (IFnii)IupGetCallback(ih, "TABCHANGEPOS_CB");
    if (cb2)
    {
      int pos = IupGetChildPos(ih, child);
      ret = cb2(ih, pos, prev_pos);
    }
  }

  return ret;
}

static void iFlatTabsCheckCurrentTab(Ihandle* ih, Ihandle* check_child, int check_pos, int removed)
{
  Ihandle* current_child = iFlatTabsGetCurrentTab(ih);
  if (current_child == check_child)
  {
    int p;
    Ihandle* child;

    /* if given tab is the current tab,
    then the current tab must be changed to a visible tab */

    /* this function is also called after the child has being removed from the hierarchy,
    but before the system tab being removed. */

    /* look forward first */
    child = IupGetChild(ih, check_pos);
    if (child) /* could be the last */
    {
      p = check_pos;
      if (removed)
        p++;  /* increment to compensate for child already removed, but id based attributes are not updated yet */
      else
      {
        child = child->brother;
        p++; /* increment to get the next child */
      }

      for (; child; child = child->brother, p++)
      {
        if (iupAttribGetBooleanId(ih, "TABVISIBLE", p))
        {
          /* found a new tab to be current */
          if (iupAttribGetBoolean(ih, "TABCHANGEONCHECK"))
            (void)iFlatTabsCallTabChange(ih, check_child, check_pos, child); /* ignore return value */

          iFlatTabsSetCurrentTab(ih, child);
          return;
        }
      }
    }

    /* look backward */
    child = IupGetChild(ih, check_pos - 1);
    if (child) /* could be the first */
    {
      p = check_pos - 1;

      while (p >= 0 && child)
      {
        if (iupAttribGetBooleanId(ih, "TABVISIBLE", p))
        {
          /* found a new tab to be current */
          if (iupAttribGetBoolean(ih, "TABCHANGEONCHECK"))
            (void)iFlatTabsCallTabChange(ih, check_child, check_pos, child); /* ignore return value */

          iFlatTabsSetCurrentTab(ih, child);
          return;
        }

        p--;
        child = IupGetChild(ih, p);
      }
    }

    /* could NOT find a new tab to be current (empty or all invisible) */
    iFlatTabsSetCurrentTab(ih, NULL);
  }
}

static int iFlatTabsFindTab(Ihandle* ih, int cur_x, int cur_y, int show_close, int *inside_close)
{
  int title_width, title_height;
  int tabType = iupAttribGetInt(ih, "_IUPTAB_TYPE");
  int title_pos, title_size, curr, curr2;

  int scroll_size = iFlatTabsGetTitleSize(ih, &title_width, &title_height, 1);

  switch (tabType)
  {
  default: /* ITABS_TOP */
    title_pos = 0;
    title_size = title_height;
    curr = cur_y;
    curr2 = cur_x;
    break;
  case ITABS_BOTTOM:
    title_pos = ih->currentheight - title_height;
    title_size = title_height;
    curr = cur_y;
    curr2 = cur_x;
    break;
  case ITABS_LEFT:
    title_pos = 0;
    title_size = title_width;
    curr = cur_x;
    curr2 = cur_y;
    break;
  case ITABS_RIGHT:
    title_pos = ih->currentwidth - title_width;
    title_size = title_width;
    curr = cur_x;
    curr2 = cur_y;
    break;
  }

  *inside_close = 0;

  if (curr >= title_pos && curr < title_pos + title_size)
  {
    Ihandle* child;
    int pos, horiz_padding, vert_padding, tab_pos = 0;
    int fixedwidth = iupAttribGetInt(ih, "FIXEDWIDTH");
    int img_position = iupFlatGetImagePosition(iupAttribGetStr(ih, "TABSIMAGEPOSITION"));
    double text_orientation = iupAttribGetDouble(ih, "TABSTEXTORIENTATION");
    int tab_space, extra_size;
    int extra_buttons = iupAttribGetInt(ih, "EXTRABUTTONS");
    int size = (tabType == ITABS_TOP || tabType == ITABS_BOTTOM) ? ih->currentwidth : ih->currentheight;
    int scroll_pos = iupAttribGetInt(ih, "_IUPFTABS_SCROLLPOS");
    int button_gap = (scroll_pos  > 0) ? scroll_size : 0;

    iupAttribGetIntInt(ih, "TABSPADDING", &horiz_padding, &vert_padding, 'x');
    if (tabType == ITABS_TOP || tabType == ITABS_BOTTOM)
      extra_size = iFlatTabsGetExtraWidth(ih, extra_buttons, img_position, horiz_padding, vert_padding, NULL);
    else
      extra_size = iFlatTabsGetExtraHeight(ih, extra_buttons, img_position, horiz_padding, vert_padding, NULL);
    tab_space = (tabType == ITABS_TOP || tabType == ITABS_BOTTOM) ? ih->currentwidth - extra_size : ih->currentheight - extra_size;

    if (scroll_pos > 0)
    {
      if (curr2 < scroll_size)
        return ITABS_SB_LEFT;

      tab_pos += scroll_size;
    }

    if (((tabType == ITABS_TOP || tabType == ITABS_BOTTOM) && title_width > ih->currentwidth - extra_size - button_gap) ||
        ((tabType == ITABS_LEFT || tabType == ITABS_RIGHT) && title_height > ih->currentheight - extra_size - button_gap))
    {
      if (curr2 > tab_space - scroll_size && curr2 < tab_space)
        return ITABS_SB_RIGHT;
    }

    if (extra_buttons)
    {
      int i, total_extra_size = 0;
      for (i = 1; i <= extra_buttons; i++)
      {
        if (tabType == ITABS_TOP || tabType == ITABS_BOTTOM)
          extra_size = iFlatTabsGetExtraWidthId(ih, i, img_position, horiz_padding, vert_padding, NULL);
        else
          extra_size = iFlatTabsGetExtraHeightId(ih, i, img_position, horiz_padding, vert_padding, NULL);

        if (curr2 > size - total_extra_size - extra_size && 
            curr2 < size - total_extra_size)
          return ITABS_EXTRABUT2TABID(i);

        total_extra_size += extra_size;
      }
    }

    child = ih->firstchild;
    for (pos = 0; pos < scroll_pos; pos++)
      child = child->brother;

    for (pos = scroll_pos; child; child = child->brother, pos++)
    {
      int tabvisible = iupAttribGetBooleanId(ih, "TABVISIBLE", pos);
      if (tabvisible)
      {
        int tab_w, tab_h, tab;

        iFlatTabsGetTabSize(ih, fixedwidth, horiz_padding, vert_padding, show_close, pos, &tab_w, &tab_h);

        tab = (tabType == ITABS_TOP || tabType == ITABS_BOTTOM) ? tab_w : tab_h;

        if (curr2 > tab_pos && curr2 < tab_pos + tab)
        {
          if (show_close)
          {
            int close_x, close_y;
            if (tabType == ITABS_TOP || tabType == ITABS_BOTTOM)
              iFlatTabsGetCloseRect(tab_pos, title_pos, tab_w, title_height, text_orientation, &close_x, &close_y);
            else
              iFlatTabsGetCloseRect(title_pos, tab_pos, title_width, tab_h, text_orientation, &close_x, &close_y);
            if (cur_x >= close_x && cur_x <= close_x + ITABS_CLOSE_SIZE &&
                cur_y >= close_y && cur_y <= close_y + ITABS_CLOSE_SIZE)
              *inside_close = 1;
          }

          return pos;
        }

        tab_pos += tab;

        if (tab_pos > size)
          break;
      }
    }
  }

  return ITABS_NONE;
}

static void iFlatTabsGetExtraButtonBox(Ihandle* ih, int tabType, int extra_buttons, int img_position, int horiz_padding, int vert_padding,
                                       int title_x_pos, int title_y_pos, int title_height, int title_width,
                                       int id, int *xmin, int *ymin, int *xmax, int *ymax)
{
  int i, total_extra_size = 0;
  int extra_x, extra_y, extra_w, extra_h;

  for (i = 1; i <= extra_buttons; i++)
  {
    if (tabType == ITABS_TOP || tabType == ITABS_BOTTOM)
    {
      extra_w = iFlatTabsGetExtraWidthId(ih, i, img_position, horiz_padding, vert_padding, NULL);  /* this will also set any id based font */
      extra_h = title_height;

      if (i == id)
      {
        extra_x = ih->currentwidth - total_extra_size - extra_w;
        extra_y = title_y_pos;
        *xmin = extra_x + horiz_padding / 2;
        *xmax = extra_x + extra_w - horiz_padding / 2;
        *ymin = title_y_pos + vert_padding / 2;
        *ymax = title_y_pos + title_height - 1 - vert_padding / 2;
        return;
      }
    }
    else
    {
      extra_h = iFlatTabsGetExtraHeightId(ih, i, img_position, horiz_padding, vert_padding, NULL);  /* this will also set any id based font */
      extra_w = title_width;

      if (i == id)
      {
        extra_y = ih->currentheight - total_extra_size - extra_h;
        extra_x = title_x_pos;
        *xmin = title_x_pos + horiz_padding / 2;
        *xmax = title_x_pos + title_width - 1 - horiz_padding / 2;
        *ymin = extra_y + vert_padding / 2;
        *ymax = extra_y + extra_h - vert_padding / 2;
        return;
      }
    }

    total_extra_size += (tabType == ITABS_TOP || tabType == ITABS_BOTTOM) ? extra_w : extra_h;
  }
}


/*****************************************************************************************/

static void iFlatTabsToggleExpand(Ihandle* ih)
{
  int expand_pos = iupAttribGetInt(ih, "EXPANDBUTTONPOS");
  int expand_state = iupAttribGetBoolean(ih, "EXPANDBUTTONSTATE");
  if (expand_state)
  {
    int title_width, title_height;
    iFlatTabsGetTitleSize(ih, &title_width, &title_height, 0);
    iupAttribSetId(ih, "EXTRAIMAGE", expand_pos, "IupFlatExpandDown");
    iupAttribSet(ih, "EXPANDBUTTONSTATE", "No");
    IupSetStrf(ih, "MAXSIZE", "x%d", title_height);
    iupAttribSetInt(ih, "_IUPFTABS_FULLHEIGHT", ih->currentheight);
    IupRefresh(ih);
  }
  else
  {
    iupAttribSetId(ih, "EXTRAIMAGE", expand_pos, "IupFlatExpandUp");
    iupAttribSet(ih, "EXPANDBUTTONSTATE", "Yes");
    IupSetAttribute(ih, "MAXSIZE", NULL);
    iupAttribSet(ih, "_IUPFTABS_FULLHEIGHT", NULL);
    IupRefresh(ih);
  }
}

static int iFlatTabsButton_CB(Ihandle* ih, int button, int pressed, int x, int y, char* status)
{
  IFniiiis button_cb = (IFniiiis)IupGetCallback(ih, "FLAT_BUTTON_CB");
  if (button_cb)
  {
    if (button_cb(ih, button, pressed, x, y, status) == IUP_IGNORE)
      return IUP_DEFAULT;
  }

  if (button == IUP_BUTTON1 && pressed)
  {
    int inside_close;
    int show_close = iupAttribGetBoolean(ih, "SHOWCLOSE");
    int tab_found = iFlatTabsFindTab(ih, x, y, show_close, &inside_close);
    if (tab_found > ITABS_NONE && iupAttribGetBooleanId(ih, "TABACTIVE", tab_found))
    {
      if (show_close && inside_close)
      {
        iupAttribSetInt(ih, "_IUPFTABS_CLOSEPRESS", tab_found);  /* used for press feedback */
        iupdrvPostRedraw(ih);
      }
      else
      {
        Ihandle* child = IupGetChild(ih, tab_found);
        Ihandle* prev_child = iFlatTabsGetCurrentTab(ih);

        iupAttribSetInt(ih, "_IUPFTABS_CLOSEPRESS", ITABS_NONE);

        if (prev_child != child)
        {
          int prev_pos = IupGetChildPos(ih, prev_child);
          int ret = iFlatTabsCallTabChange(ih, prev_child, prev_pos, child);
          if (ret == IUP_DEFAULT)
            iFlatTabsSetCurrentTab(ih, child);   /* iFlatTabsFindTab already returns a visible tab */
        }

        if (iupAttribGetBoolean(ih, "EXPANDBUTTON") && !iupAttribGetBoolean(ih, "EXPANDBUTTONSTATE"))
        {
          int childSizeAll = iupAttribGetBoolean(ih, "CHILDSIZEALL");
          if (!childSizeAll)
          {
            int old_maxsize = IupGetInt2(ih, "MAXSIZE"); /* title_height */
            int old_x = ih->x;
            int old_y = ih->y;
            int old_width = ih->currentwidth;
            IupSetAttribute(ih, "MAXSIZE", NULL);

            iupLayoutCompute(ih);

            ih->currentwidth = old_width;
            ih->x = old_x;
            ih->y = old_y;
            IupSetStrf(ih, "MAXSIZE", "x%d", old_maxsize);
          }
          else
            ih->currentheight = iupAttribGetInt(ih, "_IUPFTABS_FULLHEIGHT");
          IupSetAttribute(ih, "ZORDER", "TOP");
          iupLayoutUpdate(ih);
        }
      }
    }
    else if (tab_found == ITABS_SB_LEFT)
    {
      int pos = iupAttribGetInt(ih, "_IUPFTABS_SCROLLPOS");
      while (pos > 0)
      {
        pos--;
        if (iupAttribGetBooleanId(ih, "TABVISIBLE", pos))
          break;
      }
      iupAttribSetInt(ih, "_IUPFTABS_SCROLLPOS", pos);
      iupdrvPostRedraw(ih);
    }
    else if (tab_found == ITABS_SB_RIGHT)
    {
      int pos = iupAttribGetInt(ih, "_IUPFTABS_SCROLLPOS");
      int count = IupGetChildCount(ih);
      while (pos < count)
      {
        pos++;
        if (iupAttribGetBooleanId(ih, "TABVISIBLE", pos))
          break;
      }
      iupAttribSetInt(ih, "_IUPFTABS_SCROLLPOS", pos);
      iupdrvPostRedraw(ih);
    }
    else if (tab_found <= ITABS_EXTRABUTTON1)
    {
      int extra_active = iFlatTabsGetExtraActive(ih, ITABS_TABID2EXTRABUT(tab_found));
      if (extra_active)
      {
        IFnii cb = (IFnii)IupGetCallback(ih, "EXTRABUTTON_CB");
        if (cb)
          cb(ih, ITABS_TABID2EXTRABUT(tab_found), 1);

        iupAttribSetInt(ih, "_IUPFTABS_EXTRAPRESS", tab_found);
        iupdrvPostRedraw(ih);
      }
    }
  }
  else if (button == IUP_BUTTON1 && !pressed)
  {
    int extra_buttons;
    int tab_found = ITABS_NONE;

    int show_close = iupAttribGetBoolean(ih, "SHOWCLOSE");
    if (show_close)
    {
      int tab_close_press = iupAttribGetInt(ih, "_IUPFTABS_CLOSEPRESS");
      int inside_close;
      tab_found = iFlatTabsFindTab(ih, x, y, show_close, &inside_close);

      iupAttribSetInt(ih, "_IUPFTABS_CLOSEPRESS", ITABS_NONE);

      if (tab_found > ITABS_NONE && iupAttribGetBooleanId(ih, "TABACTIVE", tab_found) && inside_close && tab_close_press == tab_found)
      {
        Ihandle *child = IupGetChild(ih, tab_found);
        if (child)
        {
          int ret = IUP_DEFAULT;
          IFni cb = (IFni)IupGetCallback(ih, "TABCLOSE_CB");
          if (cb)
            ret = cb(ih, tab_found);

          if (ret == IUP_CONTINUE) /* destroy tab and children */
          {
            IupDestroy(child);
            IupRefreshChildren(ih);
          }
          else if (ret == IUP_DEFAULT) /* hide tab and children */
          {
            iupAttribSetId(ih, "TABVISIBLE", tab_found, "No");
            iFlatTabsCheckCurrentTab(ih, child, tab_found, 0);
            iupdrvPostRedraw(ih);
          }
          else
            iupdrvPostRedraw(ih);
        }
      }
    }

    extra_buttons = iupAttribGetInt(ih, "EXTRABUTTONS");
    if (extra_buttons)
    {
      int extra_press = iupAttribGetInt(ih, "_IUPFTABS_EXTRAPRESS");
      int inside_close;
      if (!show_close)
        tab_found = iFlatTabsFindTab(ih, x, y, show_close, &inside_close);

      iupAttribSetInt(ih, "_IUPFTABS_EXTRAPRESS", ITABS_NONE);

      if (tab_found <= ITABS_EXTRABUTTON1 && iFlatTabsGetExtraActive(ih, ITABS_TABID2EXTRABUT(tab_found)) && extra_press == tab_found)
      {
        IFnii cb;

        if (iupAttribGetBooleanId(ih, "EXTRATOGGLE", ITABS_TABID2EXTRABUT(tab_found)))
        {
          int selected = iupAttribGetIntId(ih, "EXTRAVALUE", ITABS_TABID2EXTRABUT(tab_found));
          if (selected)  /* was ON */
            iupAttribSetId(ih, "EXTRAVALUE", ITABS_TABID2EXTRABUT(tab_found), "OFF");
          else  /* was OFF */
            iupAttribSetId(ih, "EXTRAVALUE", ITABS_TABID2EXTRABUT(tab_found), "ON");
        }

        cb = (IFnii)IupGetCallback(ih, "EXTRABUTTON_CB");
        if (cb)
          cb(ih, ITABS_TABID2EXTRABUT(tab_found), 0);

        if (iupAttribGetBoolean(ih, "EXPANDBUTTON") && ITABS_TABID2EXTRABUT(tab_found) == iupAttribGetInt(ih, "EXPANDBUTTONPOS"))
          iFlatTabsToggleExpand(ih);

        iupdrvPostRedraw(ih);
      }
    }
  }
  else if (button == IUP_BUTTON3 && pressed)
  {
    IFni cb = (IFni)IupGetCallback(ih, "RIGHTCLICK_CB");
    if (cb)
    {
      int show_close = iupAttribGetBoolean(ih, "SHOWCLOSE");
      int inside_close;
      int tab_found = iFlatTabsFindTab(ih, x, y, show_close, &inside_close);
      if (tab_found > ITABS_NONE && iupAttribGetBooleanId(ih, "TABACTIVE", tab_found))
        cb(ih, tab_found);
    }
  }

  return IUP_DEFAULT;
}

static int iFlatTabsMotion_CB(Ihandle *ih, int x, int y, char *status)
{
  int tab_found, tab_highlighted, redraw = 0;
  int inside_close, show_close, tab_active;

  IFniis cb = (IFniis)IupGetCallback(ih, "FLAT_MOTION_CB");
  if (cb)
  {
    if (cb(ih, x, y, status) == IUP_IGNORE)
      return IUP_DEFAULT;
  }

  show_close = iupAttribGetBoolean(ih, "SHOWCLOSE");
  tab_highlighted = iupAttribGetInt(ih, "_IUPFTABS_HIGHLIGHTED");
  tab_found = iFlatTabsFindTab(ih, x, y, show_close, &inside_close);

  if (tab_found == ITABS_NONE)
    iupFlatItemResetTip(ih);
  else
  {
    if (tab_found > ITABS_NONE)
    {
      char* tab_tip = iupAttribGetId(ih, "TABTIP", tab_found);
      if (tab_tip)
        iupFlatItemSetTip(ih, tab_tip);
      else
        iupFlatItemResetTip(ih);
    }
    else
    {
      int extra_active = iFlatTabsGetExtraActive(ih, ITABS_TABID2EXTRABUT(tab_found));
      char* extra_tip = iupAttribGetId(ih, "EXTRATIP", ITABS_TABID2EXTRABUT(tab_found));
      if (extra_tip && extra_active)
        iupFlatItemSetTip(ih, extra_tip);
      else
        iupFlatItemResetTip(ih);
    }
  }

  tab_active = 1;
  if (tab_found > ITABS_NONE)
    tab_active = iupAttribGetBooleanId(ih, "TABACTIVE", tab_found);

  if (!tab_active)
    return IUP_DEFAULT;

  if (tab_found != tab_highlighted && !inside_close)
  {
    iupAttribSetInt(ih, "_IUPFTABS_HIGHLIGHTED", tab_found);
    redraw = 1;
  }

  if (show_close && tab_found >= ITABS_NONE)
  {
    int tab_close_high, tab_close_press;

    tab_close_high = iupAttribGetInt(ih, "_IUPFTABS_CLOSEHIGH");
    if (inside_close)
    {
      if (tab_close_high != tab_found)
      {
        iupAttribSetInt(ih, "_IUPFTABS_HIGHLIGHTED", ITABS_NONE);
        iupAttribSetInt(ih, "_IUPFTABS_CLOSEHIGH", tab_found);
        redraw = 1;
      }
    }
    else
    {
      if (tab_close_high != ITABS_NONE)
      {
        iupAttribSetInt(ih, "_IUPFTABS_CLOSEHIGH", ITABS_NONE);
        redraw = 1;
      }
    }

    tab_close_press = iupAttribGetInt(ih, "_IUPFTABS_CLOSEPRESS");
    if (tab_close_press != ITABS_NONE && !inside_close)
    {
      iupAttribSetInt(ih, "_IUPFTABS_CLOSEPRESS", ITABS_NONE);
      redraw = 1;
    }
  }

  if (redraw)
    iupdrvPostRedraw(ih);

  return IUP_DEFAULT;
}

static int iFlatTabsLeaveWindow_CB(Ihandle* ih)
{
  int tab_highlighted, tab_close_high, redraw = 0;

  IFn cb = (IFn)IupGetCallback(ih, "FLAT_LEAVEWINDOW_CB");
  if (cb)
  {
    if (cb(ih) == IUP_IGNORE)
      return IUP_DEFAULT;
  }

  tab_highlighted = iupAttribGetInt(ih, "_IUPFTABS_HIGHLIGHTED");
  if (tab_highlighted != ITABS_NONE)
  {
    iupAttribSetInt(ih, "_IUPFTABS_HIGHLIGHTED", ITABS_NONE);
    redraw = 1;
  }

  tab_close_high = iupAttribGetInt(ih, "_IUPFTABS_CLOSEHIGH");
  if (tab_close_high != ITABS_NONE)
  {
    iupAttribSetInt(ih, "_IUPFTABS_CLOSEHIGH", ITABS_NONE);
    redraw = 1;
  }

  iupFlatItemResetTip(ih);

  if (redraw)
    iupdrvPostRedraw(ih);

  return IUP_DEFAULT;
}

static int iFlatTabsGetFocus_CB(Ihandle* ih)
{
  Icallback cb = IupGetCallback(ih, "FLAT_GETFOCUS_CB");
  if (cb)
  {
    if (cb(ih) == IUP_IGNORE)
      return IUP_DEFAULT;
  }

  iupAttribSet(ih, "HASFOCUS", "Yes");
  iupdrvRedrawNow(ih);

  return IUP_DEFAULT;
}

static int iFlatTabsKillFocus_CB(Ihandle* ih)
{
  Icallback cb = IupGetCallback(ih, "FLAT_KILLFOCUS_CB");
  if (cb)
  {
    if (cb(ih) == IUP_IGNORE)
      return IUP_DEFAULT;
  }

  if (iupAttribGetBoolean(ih, "EXPANDBUTTON") && !iupAttribGetBoolean(ih, "EXPANDBUTTONSTATE"))
  {
    ih->currentheight = IupGetInt2(ih, "MAXSIZE");  /* title_height */
    iupLayoutUpdate(ih);
  }

  iupAttribSet(ih, "HASFOCUS", "No");
  iupdrvRedrawNow(ih);

  return IUP_DEFAULT;
}

static int iFlatTabsKLeft_CB(Ihandle* ih)
{
  if (iupAttribGetInt(ih, "HASFOCUS"))
  {
    Ihandle* child = iFlatTabsGetCurrentTab(ih);
    int pos = IupGetChildPos(ih, child);
    while (pos != -1 && pos > 0) /* found child */
    {
      pos--;
      child = IupGetChild(ih, pos);
      if (child && iupAttribGetBooleanId(ih, "TABVISIBLE", pos))
      {
        iFlatTabsSetCurrentTab(ih, child);
        return IUP_IGNORE;
      }
    }
  }
  return IUP_DEFAULT;
}

static int iFlatTabsKRight_CB(Ihandle* ih)
{
  if (iupAttribGetInt(ih, "HASFOCUS"))
  {
    Ihandle* child = iFlatTabsGetCurrentTab(ih);
    int pos = IupGetChildPos(ih, child);
    int count = IupGetChildCount(ih);
    while (pos != -1 && pos < count-1) /* found child */
    {
      pos++;
      child = IupGetChild(ih, pos);
      if (child && iupAttribGetBooleanId(ih, "TABVISIBLE", pos))
      {
        iFlatTabsSetCurrentTab(ih, child);
        return IUP_IGNORE;
      }
    }
  }
  return IUP_DEFAULT;
}


/*****************************************************************************************/


static int iFlatTabsSetValueHandleAttrib(Ihandle* ih, const char* value)
{
  Ihandle* current_child;
  Ihandle *child = (Ihandle*)value;

  if (!iupObjectCheck(child))
    return 0;

  current_child = iFlatTabsGetCurrentTab(ih);
  if (current_child != child)
  {
    int pos = IupGetChildPos(ih, child);
    if (pos != -1 && iupAttribGetBooleanId(ih, "TABVISIBLE", pos)) /* found child and NOT current and it is visible */
      iFlatTabsSetCurrentTab(ih, child);
  }

  return 0;
}


static char* iFlatTabsGetValueHandleAttrib(Ihandle* ih)
{
  return (char*)iFlatTabsGetCurrentTab(ih);
}

static char* iFlatTabsGetCountAttrib(Ihandle* ih)
{
  return iupStrReturnInt(IupGetChildCount(ih));
}

static int iFlatTabsSetValuePosAttrib(Ihandle* ih, const char* value)
{
  Ihandle* child, *current_child;
  int pos;

  if (!iupStrToInt(value, &pos))
    return 0;

  child = IupGetChild(ih, pos);
  current_child = iFlatTabsGetCurrentTab(ih);
  if (child && current_child != child && iupAttribGetBooleanId(ih, "TABVISIBLE", pos))  /* found child and NOT current and visible */
    iFlatTabsSetCurrentTab(ih, child);

  return 0;
}

static char* iFlatTabsGetValuePosAttrib(Ihandle* ih)
{
  Ihandle* current_child = iFlatTabsGetCurrentTab(ih);
  int pos = IupGetChildPos(ih, current_child);
  if (pos != -1) /* found child */
    return iupStrReturnInt(pos);
  return NULL;
}

static int iFlatTabsSetValueAttrib(Ihandle* ih, const char* value)
{
  Ihandle *child;

  if (!value)
    return 0;

  child = IupGetHandle(value);
  if (!child)
    return 0;

  iFlatTabsSetValueHandleAttrib(ih, (char*)child);

  return 0;
}

static char* iFlatTabsGetValueAttrib(Ihandle* ih)
{
  Ihandle* child = (Ihandle*)iFlatTabsGetValueHandleAttrib(ih);
  return IupGetName(child);  /* Name is guarantied at AddedMethod */
}

static int iFlatTabsSetTabVisibleAttrib(Ihandle* ih, int pos, const char* value)
{
  Ihandle* child = IupGetChild(ih, pos);
  if (child)
  {
    if (!iupStrBoolean(value))
    {
      iupAttribSetId(ih, "TABVISIBLE", pos, "No");
      iFlatTabsCheckCurrentTab(ih, child, pos, 0);
    }
  }

  if (ih->handle)
    iupdrvPostRedraw(ih);

  return 1;
}

static char* iFlatTabsGetClientSizeAttrib(Ihandle* ih)
{
  int width = ih->currentwidth;
  int height = ih->currentheight;
  int title_width, title_height;
  int tabType = iupAttribGetInt(ih, "_IUPTAB_TYPE");

  iFlatTabsGetTitleSize(ih, &title_width, &title_height, 0);

  if (tabType == ITABS_TOP || tabType == ITABS_BOTTOM)
    height -= title_height;
  else
    width -= title_width;

  if (iupAttribGetBoolean(ih, "SHOWLINES"))
  {
    if (tabType == ITABS_TOP || tabType == ITABS_BOTTOM)
    {
      height -= 1;
      width -= 2;
    }
    else
    {
      height -= 2;
      width -= 1;
    }
  }

  if (width < 0) width = 0;
  if (height < 0) height = 0;

  return iupStrReturnIntInt(width, height, 'x');
}

static int iFlatTabsSetBgColorAttrib(Ihandle* ih, const char* value)
{
  Ihandle* child;
  for (child = ih->firstchild; child; child = child->brother)
    IupSetAttribute(child, "BGCOLOR", value);
  return 1;
}

static int iFlatTabsSetAttribPostRedraw(Ihandle* ih, const char* value)
{
  (void)value;
  if (ih->handle)
    iupdrvPostRedraw(ih);
  return 1;
}

static int iFlatTabsSetTabOrientationAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqualNoCase(value, "VERTICAL"))
    iupAttribSet(ih, "TABSTEXTORIENTATION", "90");
  else
    iupAttribSet(ih, "TABSTEXTORIENTATION", NULL);
  if (ih->handle)
    iupdrvPostRedraw(ih);
  return 1;
}

static int iFlatTabsSetTabFontStyleAttrib(Ihandle* ih, int id, const char* value)
{
  int size = 0;
  int is_bold = 0,
    is_italic = 0,
    is_underline = 0,
    is_strikeout = 0;
  char typeface[1024];
  char* font;

  if (!value)
    return 0;

  font = iupAttribGetId(ih, "TABFONT", id);
  if (!font)
    font = iupAttribGet(ih, "TABSFONT");
  if (!font)
    font = IupGetAttribute(ih, "FONT");

  if (!iupGetFontInfo(font, typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout))
    return 0;

  IupSetfAttributeId(ih, "TABFONT", id, "%s, %s %d", typeface, value, size);

  return 0;
}

static char* iFlatTabsGetTabFontStyleAttrib(Ihandle* ih, int id)
{
  int size = 0;
  int is_bold = 0,
    is_italic = 0,
    is_underline = 0,
    is_strikeout = 0;
  char typeface[1024];

  char* font = iupAttribGetId(ih, "TABFONT", id);
  if (!font)
    font = iupAttribGet(ih, "TABSFONT");
  if (!font)
    font = IupGetAttribute(ih, "FONT");

  if (!iupGetFontInfo(font, typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout))
    return NULL;

  return iupStrReturnStrf("%s%s%s%s", is_bold ? "Bold " : "", is_italic ? "Italic " : "", is_underline ? "Underline " : "", is_strikeout ? "Strikeout " : "");
}

static int iFlatTabsSetTabFontSizeAttrib(Ihandle* ih, int id, const char* value)
{
  int size = 0;
  int is_bold = 0,
    is_italic = 0,
    is_underline = 0,
    is_strikeout = 0;
  char typeface[1024];
  char* font;

  if (!value)
    return 0;

  font = iupAttribGetId(ih, "TABFONT", id);
  if (!font)
    font = iupAttribGet(ih, "TABSFONT");
  if (!font)
    font = IupGetAttribute(ih, "FONT");

  if (!iupGetFontInfo(font, typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout))
    return 0;

  IupSetfAttributeId(ih, "TABFONT", id, "%s, %s %d", typeface, is_bold ? "Bold " : "", is_italic ? "Italic " : "", is_underline ? "Underline " : "", is_strikeout ? "Strikeout " : "", value);

  return 0;
}

static char* iFlatTabsGetTabFontSizeAttrib(Ihandle* ih, int id)
{
  int size = 0;
  int is_bold = 0,
    is_italic = 0,
    is_underline = 0,
    is_strikeout = 0;
  char typeface[1024];

  char* font = iupAttribGetId(ih, "TABFONT", id);
  if (!font)
    font = iupAttribGet(ih, "TABSFONT");
  if (!font)
    font = IupGetAttribute(ih, "FONT");

  if (!iupGetFontInfo(font, typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout))
    return NULL;

  return iupStrReturnInt(size);
}

static char* iFlatTabsGetTabsFontSizeAttrib(Ihandle* ih)
{
  int size = 0;
  int is_bold = 0,
    is_italic = 0,
    is_underline = 0,
    is_strikeout = 0;
  char typeface[1024];

  char* font = iupAttribGet(ih, "TABSFONT");
  if (!font)
    font = IupGetAttribute(ih, "FONT");

  if (!iupGetFontInfo(font, typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout))
    return NULL;

  return iupStrReturnInt(size);
}

static int iFlatTabsSetTabsFontSizeAttrib(Ihandle* ih, const char* value)
{
  int size = 0;
  int is_bold = 0,
    is_italic = 0,
    is_underline = 0,
    is_strikeout = 0;
  char typeface[1024];
  char* font;

  if (!value)
    return 0;

  font = iupAttribGet(ih, "TABSFONT");
  if (!font)
    font = IupGetAttribute(ih, "FONT");

  if (!iupGetFontInfo(font, typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout))
    return 0;

  IupSetfAttribute(ih, "TABSFONT", "%s, %s%s%s%s %s", typeface, is_bold ? "Bold " : "", is_italic ? "Italic " : "", is_underline ? "Underline " : "", is_strikeout ? "Strikeout " : "", value);
  return 0;
}

static char* iFlatTabsGetTabsFontStyleAttrib(Ihandle* ih)
{
  int size = 0;
  int is_bold = 0,
    is_italic = 0,
    is_underline = 0,
    is_strikeout = 0;
  char typeface[1024];

  char* font = iupAttribGet(ih, "TABSFONT");
  if (!font)
    font = IupGetAttribute(ih, "FONT");

  if (!iupGetFontInfo(font, typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout))
    return NULL;

  return iupStrReturnStrf("%s%s%s%s", is_bold ? "Bold " : "", is_italic ? "Italic " : "", is_underline ? "Underline " : "", is_strikeout ? "Strikeout " : "");
}

static int iFlatTabsSetTabsFontStyleAttrib(Ihandle* ih, const char* value)
{
  int size = 0;
  int is_bold = 0,
    is_italic = 0,
    is_underline = 0,
    is_strikeout = 0;
  char typeface[1024];
  char* font;

  if (!value)
    return 0;

  font = iupAttribGet(ih, "TABSFONT");
  if (!font)
    font = IupGetAttribute(ih, "FONT");

  if (!iupGetFontInfo(font, typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout))
    return 0;

  IupSetfAttribute(ih, "TABSFONT", "%s, %s %d", typeface, value, size);

  return 0;
}

static int iFlatTabsSetExpandButtonAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value) && !iupAttribGetBoolean(ih, "EXPANDBUTTON"))
  {
    int extra_buttons = iupAttribGetInt(ih, "EXTRABUTTONS");
    extra_buttons++;
    iupAttribSetInt(ih, "EXTRABUTTONS", extra_buttons);
    iupAttribSetInt(ih, "EXPANDBUTTONPOS", extra_buttons);

    iupAttribSetId(ih, "EXTRAIMAGE", extra_buttons, "IupFlatExpandUp");
    iupAttribSetId(ih, "EXTRAALIGNMENT", extra_buttons, "ACENTER:ABOTTOM");
  }

  return 1;
}

static int iFlatTabsSetExpandButtonStateAttrib(Ihandle* ih, const char* value)
{
  if (iupAttribGetBoolean(ih, "EXPANDBUTTON"))
  {
    int expand_state = iupAttribGetBoolean(ih, "EXPANDBUTTONSTATE");
    if (iupStrBoolean(value))
    {
      if (!expand_state)
        iFlatTabsToggleExpand(ih);
    }
    else
    {
      if (expand_state)
        iFlatTabsToggleExpand(ih);
    }
  }
  return 1;
}

static int iFlatTabsSetTabTypeAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqualNoCase(value, "BOTTOM"))
    iupAttribSetInt(ih, "_IUPTAB_TYPE", ITABS_BOTTOM);
  else if (iupStrEqualNoCase(value, "LEFT"))
    iupAttribSetInt(ih, "_IUPTAB_TYPE", ITABS_LEFT);
  else if (iupStrEqualNoCase(value, "RIGHT"))
    iupAttribSetInt(ih, "_IUPTAB_TYPE", ITABS_RIGHT);
  else /* "TOP" */
    iupAttribSetInt(ih, "_IUPTAB_TYPE", ITABS_TOP);

  if (ih->handle)
    iupdrvPostRedraw(ih);

  return 1;
}

static char* iFlatTabsGetExtraBoxAttrib(Ihandle* ih, int id)
{
  int extra_buttons = iupAttribGetInt(ih, "EXTRABUTTONS");
  if (extra_buttons && id >= 1 && id <= extra_buttons)
  {
    int xmin, ymin, xmax, ymax;
    int tabType = iupAttribGetInt(ih, "_IUPTAB_TYPE");
    int img_position = iupFlatGetImagePosition(iupAttribGetStr(ih, "TABSIMAGEPOSITION"));
    int horiz_padding, vert_padding;
    int title_x_pos, title_y_pos, title_width, title_height;

    iupAttribGetIntInt(ih, "TABSPADDING", &horiz_padding, &vert_padding, 'x');
    iFlatTabsGetTitleSize(ih, &title_width, &title_height, 1);

    title_x_pos = (tabType == ITABS_RIGHT) ? ih->currentwidth - title_width : 0;
    title_y_pos = (tabType == ITABS_BOTTOM) ? ih->currentheight - title_height : 0;

    iFlatTabsGetExtraButtonBox(ih, tabType, extra_buttons, img_position, horiz_padding, vert_padding,
                               title_x_pos, title_y_pos, title_height, title_width,
                               id, &xmin, &ymin, &xmax, &ymax);

    return iupStrReturnStrf("%d %d %d %d", xmin, ymin, xmax, ymax);
  }

  return NULL;
}


/*********************************************************************************/


static int iFlatTabsConvertXYToPos(Ihandle* ih, int x, int y)
{
  int inside_close;
  int show_close = iupAttribGetBoolean(ih, "SHOWCLOSE");
  int tab_found = iFlatTabsFindTab(ih, x, y, show_close, &inside_close);
  if (tab_found > ITABS_NONE)
    return tab_found;
  else
    return -1;
}

#define ATTRIB_ID_COUNT 9
const static char* flattabs_attrib_id[ATTRIB_ID_COUNT] = {
  "TABTITLE",
  "TABIMAGE",
  "TABVISIBLE",
  "TABACTIVE",
  "TABFORECOLOR",
  "TABBACKCOLOR",
  "TABHIGHCOLOR",
  "TABFONT",
  "TABTIP"
};

static void iFlatTabsChildAddedMethod(Ihandle* ih, Ihandle* child)
{
  Ihandle* current_child;
  char* bgcolor;
  int i, p, count, pos;

  pos = IupGetChildPos(ih, child);
  count = IupGetChildCount(ih);  /* already includes the new tab */

  /* if inserted before last tab, must update Id based attributes */
  for (p = count - 1; p > pos; p--)
  {
    for (i = 0; i < ATTRIB_ID_COUNT; i++)
    {
      char* value = iupAttribGetId(ih, flattabs_attrib_id[i], p - 1);
      iupAttribSetStrId(ih, flattabs_attrib_id[i], p, value);
      iupAttribSetId(ih, flattabs_attrib_id[i], p - 1, NULL);
    }
  }

  /* transfer form child to Id based attribute */
  for (i = 0; i < ATTRIB_ID_COUNT; i++)
  {
    if (!iupAttribGetId(ih, flattabs_attrib_id[i], pos)) /* if not already defined at the tabs, use from child */
    {
      char* value = iupAttribGet(child, flattabs_attrib_id[i]);
      if (value)
        iupAttribSetStrId(ih, flattabs_attrib_id[i], pos, value);
      else if (iupStrEqual(flattabs_attrib_id[i], "TABVISIBLE") || iupStrEqual(flattabs_attrib_id[i], "TABACTIVE"))
        iupAttribSetId(ih, flattabs_attrib_id[i], pos, "Yes");  /* ensure a default value */
    }
  }

  /* make sure it has at least one name */
  if (!iupAttribGetHandleName(child))
    iupAttribSetHandleName(child);

  bgcolor = iupAttribGetStr(ih, "BGCOLOR");
  iupAttribSetStr(child, "BGCOLOR", bgcolor);

  current_child = iFlatTabsGetCurrentTab(ih);
  if (!current_child && iupAttribGetBooleanId(ih, "TABVISIBLE", pos))
    iFlatTabsSetCurrentTab(ih, child);
  else
  {
    IupSetAttribute(child, "VISIBLE", "No");
    if (ih->handle)
      iupdrvPostRedraw(ih);
  }
}

static void iFlatTabsChildRemovedMethod(Ihandle* ih, Ihandle* child, int pos)
{
  int p, i, count;

  iFlatTabsCheckCurrentTab(ih, child, pos, 1);

  count = IupGetChildCount(ih);  /* does not include the removed tab */

  /* if removed before last tab, must update Id based attributes */
  for (p = pos; p < count; p++)
  {
    for (i = 0; i < ATTRIB_ID_COUNT; i++)
    {
      char* value = iupAttribGetId(ih, flattabs_attrib_id[i], p + 1);
      iupAttribSetStrId(ih, flattabs_attrib_id[i], p, value);
      iupAttribSetId(ih, flattabs_attrib_id[i], p + 1, NULL);
    }
  }   

  if (pos == count)
  {
    for (i = 0; i < ATTRIB_ID_COUNT; i++)
      iupAttribSetId(ih, flattabs_attrib_id[i], pos, NULL);
  }

  if (ih->handle)
  {
    int scroll_pos = iupAttribGetInt(ih, "_IUPFTABS_SCROLLPOS");
    if (scroll_pos >= count)
      iupAttribSetInt(ih, "_IUPFTABS_SCROLLPOS", 0);

    iupdrvPostRedraw(ih);
  }
}

static void iFlatTabsComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
  Ihandle* child;
  int children_naturalwidth, children_naturalheight;
  int tabType = iupAttribGetInt(ih, "_IUPTAB_TYPE");
  int childSizeAll = iupAttribGetBoolean(ih, "CHILDSIZEALL");
  Ihandle* current_child = (!childSizeAll)? iFlatTabsGetCurrentTab(ih): NULL;
  int height, width;

  /* calculate total children natural size (even for hidden children) */
  children_naturalwidth = 0;
  children_naturalheight = 0;

  for (child = ih->firstchild; child; child = child->brother)
  {
    /* update child natural size first */
    if (!(child->flags & IUP_FLOATING_IGNORE))
      iupBaseComputeNaturalSize(child);

    if (!childSizeAll && child != current_child)
      continue;

    if (!(child->flags & IUP_FLOATING))
    {
      *children_expand |= child->expand;
      children_naturalwidth = iupMAX(children_naturalwidth, child->naturalwidth);
      children_naturalheight = iupMAX(children_naturalheight, child->naturalheight);
    }
  }

  iFlatTabsGetTitleSize(ih, &width, &height, 0);

  *w = children_naturalwidth;
  *h = children_naturalheight;

  if (tabType == ITABS_TOP || tabType == ITABS_BOTTOM)
    *h += height;
  else
    *w += width;

  if (iupAttribGetBoolean(ih, "SHOWLINES"))
  {
    *h += 1;
    *w += 2;
  }
}

static void iFlatTabsSetChildrenCurrentSizeMethod(Ihandle* ih, int shrink)
{
  Ihandle* child;
  int title_width, title_height;
  int tabType = iupAttribGetInt(ih, "_IUPTAB_TYPE");
  int width, height;

  iFlatTabsGetTitleSize(ih, &title_width, &title_height, 0);

  width = (tabType == ITABS_TOP || tabType == ITABS_BOTTOM) ? ih->currentwidth : ih->currentwidth - title_width;
  height = (tabType == ITABS_TOP || tabType == ITABS_BOTTOM) ? ih->currentheight - title_height : ih->currentheight;

  if (iupAttribGetBoolean(ih, "SHOWLINES"))
  {
    width -= (tabType == ITABS_TOP || tabType == ITABS_BOTTOM) ? 2 : 1;
    height -= (tabType == ITABS_TOP || tabType == ITABS_BOTTOM) ? 1 : 2;
  }

  if (width < 0) width = 0;
  if (height < 0) height = 0;

  for (child = ih->firstchild; child; child = child->brother)
  {
    if (!(child->flags & IUP_FLOATING))
    {
      child->currentwidth = width;
      child->currentheight = height;

      if (child->firstchild)
        iupClassObjectSetChildrenCurrentSize(child, shrink);
    }
  }
}

static void iFlatTabsSetChildrenPositionMethod(Ihandle* ih, int x, int y)
{
  /* In all systems, each tab is a native window covering the client area.
     Child coordinates are relative to client left-top corner of the tab page. */
  Ihandle* child;
  char* offset = iupAttribGet(ih, "CHILDOFFSET");
  int title_width, title_height;
  int tabType = iupAttribGetInt(ih, "_IUPTAB_TYPE");

  /* Native container, position is reset */
  x = 0;
  y = 0;

  if (offset) iupStrToIntInt(offset, &x, &y, 'x');

  iFlatTabsGetTitleSize(ih, &title_width, &title_height, 0);

  if (tabType == ITABS_TOP)
    y += title_height;

  if (tabType == ITABS_LEFT)
    x += title_width;

  if ((tabType == ITABS_RIGHT || tabType == ITABS_TOP || tabType == ITABS_BOTTOM) && iupAttribGetBoolean(ih, "SHOWLINES"))
    x += 1;

  if ((tabType == ITABS_BOTTOM || tabType == ITABS_LEFT || tabType == ITABS_RIGHT) && iupAttribGetBoolean(ih, "SHOWLINES"))
    y += 1;

  for (child = ih->firstchild; child; child = child->brother)
  {
    if (!(child->flags & IUP_FLOATING))
      iupBaseSetPosition(child, x, y);
  }
}

static int iFlatTabsCreateMethod(Ihandle* ih, void **params)
{
  /* add children */
  if (params)
  {
    Ihandle** iparams = (Ihandle**)params;
    while (*iparams)
    {
      IupAppend(ih, *iparams);
      iparams++;
    }
  }

  IupSetCallback(ih, "_IUP_XY2POS_CB", (Icallback)iFlatTabsConvertXYToPos);

  iupAttribSetInt(ih, "_IUPFTABS_HIGHLIGHTED", ITABS_NONE);
  iupAttribSetInt(ih, "_IUPFTABS_CLOSEHIGH", ITABS_NONE);
  iupAttribSetInt(ih, "_IUPFTABS_CLOSEPRESS", ITABS_NONE);
  iupAttribSetInt(ih, "_IUPFTABS_EXTRAPRESS", ITABS_NONE);

  IupSetCallback(ih, "ACTION", (Icallback)iFlatTabsRedraw_CB);
  IupSetCallback(ih, "BUTTON_CB", (Icallback)iFlatTabsButton_CB);
  IupSetCallback(ih, "MOTION_CB", (Icallback)iFlatTabsMotion_CB);
  IupSetCallback(ih, "LEAVEWINDOW_CB", (Icallback)iFlatTabsLeaveWindow_CB);
  IupSetCallback(ih, "GETFOCUS_CB", (Icallback)iFlatTabsGetFocus_CB);  /* can NOT use FOCUS_CB because tabs is a container and will affect PROPAGATEFOCUS */
  IupSetCallback(ih, "KILLFOCUS_CB", (Icallback)iFlatTabsKillFocus_CB);
  IupSetCallback(ih, "RESIZE_CB", (Icallback)iFlatTabsResize_CB);
  IupSetCallback(ih, "K_LEFT", (Icallback)iFlatTabsKLeft_CB);
  IupSetCallback(ih, "K_RIGHT", (Icallback)iFlatTabsKRight_CB);

  return IUP_NOERROR;
}

Iclass* iupFlatTabsNewClass(void)
{
  Iclass* ic = iupClassNew(iupRegisterFindClass("canvas"));

  ic->name = "flattabs";
  ic->cons = "FlatTabs";
  ic->format = "g"; /* array of Ihandle */
  ic->nativetype = IUP_TYPECANVAS;
  ic->childtype = IUP_CHILDMANY;  /* can have children */
  ic->is_interactive = 1;
  ic->has_attrib_id = 1;

  /* Class functions */
  ic->New = iupFlatTabsNewClass;
  ic->Create = iFlatTabsCreateMethod;

  ic->ChildAdded = iFlatTabsChildAddedMethod;
  ic->ChildRemoved = iFlatTabsChildRemovedMethod;

  ic->ComputeNaturalSize = iFlatTabsComputeNaturalSizeMethod;
  ic->SetChildrenCurrentSize = iFlatTabsSetChildrenCurrentSizeMethod;
  ic->SetChildrenPosition = iFlatTabsSetChildrenPositionMethod;

  /* IupFlatTabs Callbacks */
  iupClassRegisterCallback(ic, "TABCHANGE_CB", "nn");
  iupClassRegisterCallback(ic, "TABCHANGEPOS_CB", "ii");
  iupClassRegisterCallback(ic, "RIGHTCLICK_CB", "i");
  iupClassRegisterCallback(ic, "TABCLOSE_CB", "i");
  iupClassRegisterCallback(ic, "EXTRABUTTON_CB", "ii");

  iupClassRegisterCallback(ic, "FLAT_BUTTON_CB", "iiiis");
  iupClassRegisterCallback(ic, "FLAT_MOTION_CB", "iis");
  iupClassRegisterCallback(ic, "FLAT_LEAVEWINDOW_CB", "");
  iupClassRegisterCallback(ic, "FLAT_GETFOCUS_CB", "");
  iupClassRegisterCallback(ic, "FLAT_KILLFOCUS_CB", "");

  /* Base Container */
  iupClassRegisterAttribute(ic, "EXPAND", iupBaseContainerGetExpandAttrib, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLIENTOFFSET", iupBaseGetClientOffsetAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLIENTSIZE", iFlatTabsGetClientSizeAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  /* Native Container */
  iupClassRegisterAttribute(ic, "CHILDOFFSET", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  /* replace IupCanvas behavior */
  iupClassRegisterReplaceAttribDef(ic, "BORDER", "NO", NULL);
  iupClassRegisterReplaceAttribFlags(ic, "BORDER", IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterReplaceAttribFlags(ic, "SCROLLBAR", IUPAF_READONLY | IUPAF_NO_INHERIT);

  iupClassRegisterReplaceAttribFunc(ic, "ACTIVE", NULL, iupFlatSetActiveAttrib);
  iupClassRegisterAttribute(ic, "TIP", NULL, iupFlatItemSetTipAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  /* IupFlatTabs only */
  iupClassRegisterAttribute(ic, "VALUE", iFlatTabsGetValueAttrib, iFlatTabsSetValueAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUEPOS", iFlatTabsGetValuePosAttrib, iFlatTabsSetValuePosAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_SAVE | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUE_HANDLE", iFlatTabsGetValueHandleAttrib, iFlatTabsSetValueHandleAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT | IUPAF_IHANDLE | IUPAF_NO_STRING);
  iupClassRegisterAttribute(ic, "COUNT", iFlatTabsGetCountAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FIXEDWIDTH", NULL, iFlatTabsSetAttribPostRedraw, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABCHANGEONCHECK", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HASFOCUS", NULL, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABTYPE", NULL, iFlatTabsSetTabTypeAttrib, IUPAF_SAMEASSYSTEM, "TOP", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CHILDSIZEALL", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FOCUSFEEDBACK", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);

  /* IupFlatTabs Child only */
  iupClassRegisterAttributeId(ic, "TABTITLE", NULL, (IattribSetIdFunc)iFlatTabsSetAttribPostRedraw, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TABIMAGE", NULL, (IattribSetIdFunc)iFlatTabsSetAttribPostRedraw, IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TABVISIBLE", NULL, iFlatTabsSetTabVisibleAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TABACTIVE", NULL, (IattribSetIdFunc)iFlatTabsSetAttribPostRedraw, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TABFORECOLOR", NULL, (IattribSetIdFunc)iFlatTabsSetAttribPostRedraw, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TABBACKCOLOR", NULL, (IattribSetIdFunc)iFlatTabsSetAttribPostRedraw, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TABHIGHCOLOR", NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TABFONT", NULL, (IattribSetIdFunc)iFlatTabsSetAttribPostRedraw, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TABTIP", NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttributeId(ic, "TABFONTSTYLE", iFlatTabsGetTabFontStyleAttrib, iFlatTabsSetTabFontStyleAttrib, IUPAF_NO_SAVE | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TABFONTSIZE", iFlatTabsGetTabFontSizeAttrib, iFlatTabsSetTabFontSizeAttrib, IUPAF_NO_SAVE | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  /* Visual for current TAB */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iFlatTabsSetBgColorAttrib, IUPAF_SAMEASSYSTEM, IUP_FLAT_BACKCOLOR, IUPAF_NO_INHERIT);   /* NO inherited - exception for BGCOLOR */
  iupClassRegisterAttribute(ic, "FORECOLOR", NULL, iFlatTabsSetAttribPostRedraw, IUPAF_SAMEASSYSTEM, IUP_FLAT_BORDERCOLOR, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HIGHCOLOR", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  /* Visual for the other TABS */
  iupClassRegisterAttribute(ic, "TABSFORECOLOR", NULL, iFlatTabsSetAttribPostRedraw, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABSBACKCOLOR", NULL, iFlatTabsSetAttribPostRedraw, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABSHIGHCOLOR", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  /* Visual for all TABS */
  iupClassRegisterAttribute(ic, "TABSFONT", NULL, iupdrvSetFontAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABSFONTSTYLE", iFlatTabsGetTabsFontStyleAttrib, iFlatTabsSetTabsFontStyleAttrib, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABSFONTSIZE", iFlatTabsGetTabsFontSizeAttrib, iFlatTabsSetTabsFontSizeAttrib, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "SHOWLINES", NULL, iFlatTabsSetAttribPostRedraw, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABSLINECOLOR", NULL, iFlatTabsSetAttribPostRedraw, IUPAF_SAMEASSYSTEM, "160 160 160", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABSIMAGEPOSITION", NULL, iFlatTabsSetAttribPostRedraw, IUPAF_SAMEASSYSTEM, "LEFT", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABSIMAGESPACING", NULL, iFlatTabsSetAttribPostRedraw, IUPAF_SAMEASSYSTEM, "2", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABSALIGNMENT", NULL, iFlatTabsSetAttribPostRedraw, "ACENTER:ACENTER", NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABSPADDING", NULL, iFlatTabsSetAttribPostRedraw, IUPAF_SAMEASSYSTEM, "6x4", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABSTEXTALIGNMENT", NULL, iFlatTabsSetAttribPostRedraw, IUPAF_SAMEASSYSTEM, "ALEFT", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABSTEXTWRAP", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABSTEXTELLIPSIS", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABSTEXTCLIP", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABSTEXTORIENTATION", NULL, iFlatTabsSetAttribPostRedraw, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABORIENTATION", NULL, iFlatTabsSetTabOrientationAttrib, IUPAF_SAMEASSYSTEM, "HORIZONTAL", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "SHOWCLOSE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLOSEIMAGE", NULL, iFlatTabsSetAttribPostRedraw, IUPAF_SAMEASSYSTEM, "IMGFLATCLOSE", IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLOSEIMAGEPRESS", NULL, iFlatTabsSetAttribPostRedraw, IUPAF_SAMEASSYSTEM, "IMGFLATCLOSEPRESS", IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLOSEIMAGEHIGHLIGHT", NULL, NULL, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLOSEIMAGEINACTIVE", NULL, iFlatTabsSetAttribPostRedraw, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLOSEPRESSCOLOR", NULL, NULL, IUPAF_SAMEASSYSTEM, IUP_FLAT_PRESSCOLOR, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLOSEHIGHCOLOR", NULL, NULL, IUPAF_SAMEASSYSTEM, IUP_FLAT_HIGHCOLOR, IUPAF_NO_INHERIT);

  /* Extra Buttons */
  iupClassRegisterAttribute(ic, "EXTRABUTTONS", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "EXTRATITLE", NULL, (IattribSetIdFunc)iFlatTabsSetAttribPostRedraw, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "EXTRAACTIVE", NULL, (IattribSetIdFunc)iFlatTabsSetAttribPostRedraw, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "EXTRAFORECOLOR", NULL, (IattribSetIdFunc)iFlatTabsSetAttribPostRedraw, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "EXTRAPRESSCOLOR", NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "EXTRAHIGHCOLOR", NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "EXTRAFONT", NULL, (IattribSetIdFunc)iFlatTabsSetAttribPostRedraw, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "EXTRAALIGNMENT", NULL, (IattribSetIdFunc)iFlatTabsSetAttribPostRedraw, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "EXTRAIMAGE", NULL, (IattribSetIdFunc)iFlatTabsSetAttribPostRedraw, IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "EXTRAIMAGEPRESS", NULL, (IattribSetIdFunc)iFlatTabsSetAttribPostRedraw, IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "EXTRAIMAGEHIGHLIGHT", NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "EXTRAIMAGEINACTIVE", NULL, (IattribSetIdFunc)iFlatTabsSetAttribPostRedraw, IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "EXTRATIP", NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "EXTRASHOWBORDER", NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "EXTRABORDERCOLOR", NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "EXTRABORDERWIDTH", NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "EXTRAVALUE", NULL, (IattribSetIdFunc)iFlatTabsSetAttribPostRedraw, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "EXTRATOGGLE", NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "EXTRABOX", iFlatTabsGetExtraBoxAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "EXPANDBUTTON", NULL, iFlatTabsSetExpandButtonAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "EXPANDBUTTONPOS", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "EXPANDBUTTONSTATE", NULL, iFlatTabsSetExpandButtonStateAttrib, IUPAF_SAMEASSYSTEM, "Yes", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  /* Default node images */
  if (!IupGetHandle("IMGFLATCLOSE"))
    iFlatTabsInitializeImages();

  return ic;
}

IUP_API Ihandle* IupFlatTabsv(Ihandle** params)
{
  return IupCreatev("flattabs", (void**)params);
}

IUP_API Ihandle* IupFlatTabsV(Ihandle* child, va_list arglist)
{
  return IupCreateV("flattabs", child, arglist);
}

IUP_API Ihandle* IupFlatTabs(Ihandle* child, ...)
{
  Ihandle *ih;

  va_list arglist;
  va_start(arglist, child);
  ih = IupCreateV("flattabs", child, arglist);
  va_end(arglist);

  return ih;
}
