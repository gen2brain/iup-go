/** \file
 * \brief iupexpander control
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "iup.h"
#include "iupcbs.h"
#include "iupkey.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_stdcontrols.h"
#include "iup_layout.h"
#include "iup_childtree.h"
#include "iup_image.h"
#include "iup_drvdraw.h"
#include "iup_draw.h"


static Ihandle* load_image_arrowup_highlight(void)
{
  unsigned char imgdata[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 24, 0, 0, 0, 175, 0, 0, 0, 24, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 12, 0, 0, 0, 211, 116, 116, 116, 242, 0, 0, 0, 211, 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 12, 0, 0, 0, 195, 116, 116, 116, 242, 0, 92, 220, 255, 116, 116, 116, 242, 0, 0, 0, 195, 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 171, 116, 116, 116, 242, 0, 92, 220, 255, 0, 92, 220, 255, 0, 92, 220, 255, 116, 116, 116, 242, 0, 0, 0, 171, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 159, 116, 116, 116, 242, 0, 92, 220, 255, 0, 92, 220, 255, 0, 92, 220, 255, 0, 92, 220, 255, 0, 92, 220, 255, 116, 116, 116, 242, 0, 0, 0, 159, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 143, 116, 116, 116, 242, 116, 116, 116, 242, 116, 116, 116, 242, 116, 116, 116, 242, 116, 116, 116, 242, 116, 116, 116, 242, 116, 116, 116, 242, 116, 116, 116, 242, 116, 116, 116, 242, 0, 0, 0, 143, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

  Ihandle* image = IupImageRGBA(15, 15, imgdata);
  return image;
}

static Ihandle* load_image_arrowdown_highlight(void)
{
  unsigned char imgdata[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 143, 116, 116, 116, 242, 116, 116, 116, 242, 116, 116, 116, 242, 116, 116, 116, 242, 116, 116, 116, 242, 116, 116, 116, 242, 116, 116, 116, 242, 116, 116, 116, 242, 116, 116, 116, 242, 0, 0, 0, 143, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 159, 116, 116, 116, 242, 0, 92, 220, 255, 0, 92, 220, 255, 0, 92, 220, 255, 0, 92, 220, 255, 0, 92, 220, 255, 116, 116, 116, 242, 0, 0, 0, 159, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 171, 116, 116, 116, 242, 0, 92, 220, 255, 0, 92, 220, 255, 0, 92, 220, 255, 116, 116, 116, 242, 0, 0, 0, 171, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 12, 0, 0, 0, 195, 116, 116, 116, 242, 0, 92, 220, 255, 116, 116, 116, 242, 0, 0, 0, 195, 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 12, 0, 0, 0, 211, 116, 116, 116, 242, 0, 0, 0, 211, 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 24, 0, 0, 0, 175, 0, 0, 0, 24, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

  Ihandle* image = IupImageRGBA(15, 15, imgdata);
  return image;
}

static Ihandle* load_image_arrowleft_highlight(void)
{
  unsigned char imgdata[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 143, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 159, 116, 116, 116, 242, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 12, 0, 0, 0, 171, 116, 116, 116, 242, 116, 116, 116, 242, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 12, 0, 0, 0, 195, 116, 116, 116, 242, 0, 92, 220, 255, 116, 116, 116, 242, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 24, 0, 0, 0, 211, 116, 116, 116, 242, 0, 92, 220, 255, 0, 92, 220, 255, 116, 116, 116, 242, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 175, 116, 116, 116, 242, 0, 92, 220, 255, 0, 92, 220, 255, 0, 92, 220, 255, 116, 116, 116, 242, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 24, 0, 0, 0, 211, 116, 116, 116, 242, 0, 92, 220, 255, 0, 92, 220, 255, 116, 116, 116, 242, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 12, 0, 0, 0, 195, 116, 116, 116, 242, 0, 92, 220, 255, 116, 116, 116, 242, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 12, 0, 0, 0, 171, 116, 116, 116, 242, 116, 116, 116, 242, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 159, 116, 116, 116, 242, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 143, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

  Ihandle* image = IupImageRGBA(15, 15, imgdata);
  return image;
}

static Ihandle* load_image_arrowright_highlight(void)
{
  unsigned char imgdata[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 143, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 116, 116, 116, 242, 0, 0, 0, 159, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 116, 116, 116, 242, 116, 116, 116, 242, 0, 0, 0, 171, 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 116, 116, 116, 242, 0, 92, 220, 255, 116, 116, 116, 242, 0, 0, 0, 195, 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 116, 116, 116, 242, 0, 92, 220, 255, 0, 92, 220, 255, 116, 116, 116, 242, 0, 0, 0, 211, 0, 0, 0, 24, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 116, 116, 116, 242, 0, 92, 220, 255, 0, 92, 220, 255, 0, 92, 220, 255, 116, 116, 116, 242, 0, 0, 0, 175, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 116, 116, 116, 242, 0, 92, 220, 255, 0, 92, 220, 255, 116, 116, 116, 242, 0, 0, 0, 211, 0, 0, 0, 24, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 116, 116, 116, 242, 0, 92, 220, 255, 116, 116, 116, 242, 0, 0, 0, 195, 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 116, 116, 116, 242, 116, 116, 116, 242, 0, 0, 0, 171, 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 116, 116, 116, 242, 0, 0, 0, 159, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 143, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

  Ihandle* image = IupImageRGBA(15, 15, imgdata);
  return image;
}

static Ihandle* load_image_arrowup(void)
{
  unsigned char imgdata[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 24, 0, 0, 0, 175, 0, 0, 0, 24, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 12, 0, 0, 0, 211, 0, 0, 0, 255, 0, 0, 0, 211, 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 12, 0, 0, 0, 195, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 195, 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 171, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 171, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 159, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 159, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 143, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 143, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

  Ihandle* image = IupImageRGBA(15, 15, imgdata);
  return image;
}

static Ihandle* load_image_arrowleft(void)
{
  unsigned char imgdata[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 143, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 159, 0, 0, 0, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 12, 0, 0, 0, 171, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 12, 0, 0, 0, 195, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 24, 0, 0, 0, 211, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 175, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 24, 0, 0, 0, 211, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 12, 0, 0, 0, 195, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 12, 0, 0, 0, 171, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 159, 0, 0, 0, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 143, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

  Ihandle* image = IupImageRGBA(15, 15, imgdata);
  return image;
}

static Ihandle* load_image_arrowright(void)
{
  unsigned char imgdata[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 143, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 0, 0, 0, 159, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 171, 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 195, 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 211, 0, 0, 0, 24, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 175, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 211, 0, 0, 0, 24, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 195, 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 171, 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 0, 0, 0, 159, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 143, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

  Ihandle* image = IupImageRGBA(15, 15, imgdata);
  return image;
}

static Ihandle* load_image_arrowdown(void)
{
  unsigned char imgdata[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 143, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 143, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 159, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 159, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 171, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 171, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 12, 0, 0, 0, 195, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 195, 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 12, 0, 0, 0, 211, 0, 0, 0, 255, 0, 0, 0, 211, 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 24, 0, 0, 0, 175, 0, 0, 0, 24, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

  Ihandle* image = IupImageRGBA(15, 15, imgdata);
  return image;
}

static void iExpanderLoadImages(void)
{
  IupSetHandle("IupArrowUp", load_image_arrowup());
  IupSetHandle("IupArrowLeft", load_image_arrowleft());
  IupSetHandle("IupArrowRight", load_image_arrowright());
  IupSetHandle("IupArrowDown", load_image_arrowdown());
  IupSetHandle("IupArrowUpHighlight", load_image_arrowup_highlight());
  IupSetHandle("IupArrowDownHighlight", load_image_arrowdown_highlight());
  IupSetHandle("IupArrowLeftHighlight", load_image_arrowleft_highlight());
  IupSetHandle("IupArrowRightHighlight", load_image_arrowright_highlight());
}

enum { IEXPANDER_LEFT, IEXPANDER_RIGHT, IEXPANDER_TOP, IEXPANDER_BOTTOM };
enum { IEXPANDER_CLOSE, IEXPANDER_OPEN, IEXPANDER_OPEN_FLOAT };

struct _IcontrolData
{
  /* attributes */
  int position;
  int state;
  int bar_size;
  int extra_buttons;
  int auto_show;
  int title_expand;
  int animation;
  int state_refresh;

  /* aux */
  Ihandle* auto_show_timer;
  Ihandle* animate_timer;
};

static void iExpanderUpdateTitleState(Ihandle* ih)
{
  Ihandle* box = ih->firstchild->firstchild;
  Ihandle* title_label = box->firstchild->brother;

  /* called only when TITLEEXPAND=Yes and BARPOSITION=TOP */

  char* titleimage = iupAttribGet(ih, "TITLEIMAGE");
  if (titleimage)
  {
    if (ih->data->state != IEXPANDER_CLOSE)
    {
      char* imopen = iupAttribGetStr(ih, "TITLEIMAGEOPEN");
      if (imopen) titleimage = imopen;

      if (iupAttribGet(title_label, "HIGHLIGHT"))
      {
        char* imhighlight = iupAttribGetStr(ih, "TITLEIMAGEOPENHIGHLIGHT");
        if (imhighlight) titleimage = imhighlight;
      }
    }
    else
    {
      if (iupAttribGet(title_label, "HIGHLIGHT"))
      {
        char* imhighlight = iupAttribGetStr(ih, "TITLEIMAGEHIGHLIGHT");
        if (imhighlight) titleimage = imhighlight;
      }
    }

    IupSetStrAttribute(title_label, "IMAGE", titleimage);
  }
  else
  {
    char* color = iupAttribGetStr(ih, "FORECOLOR");

    if (iupAttribGet(title_label, "HIGHLIGHT"))
    {
      char* highcolor = iupAttribGet(ih, "HIGHCOLOR");
      if (highcolor) color = highcolor;
    }
    else if (ih->data->state != IEXPANDER_CLOSE)
    {
      char* opencolor = iupAttribGet(ih, "OPENCOLOR");
      if (opencolor) color = opencolor;
    }

    IupSetStrAttribute(title_label, "FGCOLOR", color);
  }
}

static void iExpanderUpdateStateImage(Ihandle* ih)
{
  /* expander -> bar -> box -> (expand_button, ...) */
  Ihandle* box = ih->firstchild->firstchild;
  Ihandle* expand_button = box->firstchild;

  char* image = iupAttribGetStr(ih, "IMAGE");
  if (image)
  {
    if (ih->data->state != IEXPANDER_CLOSE)
    {
      char* imopen = iupAttribGetStr(ih, "IMAGEOPEN");
      if (imopen) image = imopen;

      if (iupAttribGet(expand_button, "HIGHLIGHT"))
      {
        char* imhighlight = iupAttribGetStr(ih, "IMAGEOPENHIGHLIGHT");
        if (imhighlight) image = imhighlight;
      }
    }
    else
    {
      if (iupAttribGet(expand_button, "HIGHLIGHT"))
      {
        char* imhighlight = iupAttribGetStr(ih, "IMAGEHIGHLIGHT");
        if (imhighlight) image = imhighlight;
      }
    }

    IupSetAttribute(expand_button, "IMAGE", image);
  }
  else
  {
    /* the arrow points in the direction of the action */

    switch (ih->data->position)
    {
    case IEXPANDER_LEFT:
      if (ih->data->state == IEXPANDER_CLOSE)
        image = "IupArrowRight";
      else
        image = "IupArrowLeft";
      break;
    case IEXPANDER_RIGHT:
      if (ih->data->state == IEXPANDER_CLOSE)
        image = "IupArrowLeft";
      else
        image = "IupArrowRight";
      break;
    case IEXPANDER_BOTTOM:
      if (ih->data->state == IEXPANDER_CLOSE)
        image = "IupArrowUp";
      else
        image = "IupArrowDown";
      break;
    default: /* IEXPANDER_TOP */
      if (iupAttribGet(ih, "TITLE") || iupAttribGet(ih, "TITLEIMAGE"))
      {
        if (ih->data->state == IEXPANDER_CLOSE)
          image = "IupArrowRight";
        else
          image = "IupArrowDown";
      }
      else
      {
        if (ih->data->state == IEXPANDER_CLOSE)
          image = "IupArrowDown";
        else
          image = "IupArrowUp";
      }
      break;
    }

    if (iupAttribGet(expand_button, "HIGHLIGHT"))
      IupSetfAttribute(expand_button, "IMAGE", "%sHighlight", image);
    else
      IupSetAttribute(expand_button, "IMAGE", image);
  }
}

static int iExpanderAnimateTimer_CB(Ihandle* animate_timer)
{
  Ihandle* ih = (Ihandle*)iupAttribGet(animate_timer, "_IUP_EXPANDER");
  Ihandle* child = (Ihandle*)iupAttribGet(animate_timer, "_IUP_CHILD");
  int final_height = IupGetInt(animate_timer, "_IUP_FINAL_HEIGHT");
  int closing = IupGetInt(animate_timer, "_IUP_CLOSING");
  int width = IupGetInt(animate_timer, "_IUP_WIDTH");
  int frame_time = iupAttribGetInt(ih, "FRAMETIME");
  int num_frames = iupAttribGetInt(ih, "NUMFRAMES");
  int time_delay = iupAttribGetInt(animate_timer, "ELAPSEDTIME");
  int height;
  int current_frame = frame_time != 0 ? time_delay / frame_time : 0;  /* safety check */

  if (num_frames == 0)
    return IUP_DEFAULT;

  if (closing)
    height = (final_height*(num_frames - current_frame)) / num_frames;
  else
    height = (final_height*(current_frame + 1)) / num_frames;

  IupSetfAttribute(child, "MAXSIZE", "%dx%d", width, height);

  if (ih->data->animation == 2) /* slide */
    IupSetfAttribute(child, "CHILDOFFSET", "0x%d", height - final_height);

  if (ih->data->state_refresh)
    IupRefresh(ih);

  if (current_frame == num_frames - 1)
  {
    iupAttribSetStr(child, "MAXSIZE", iupAttribGet(child, "OLD_MAXSIZE"));

    if (closing)
    {
      ih->data->state = IEXPANDER_CLOSE;
      IupSetAttribute(child, "VISIBLE", "NO");

      if (ih->data->state_refresh)
        IupRefresh(ih);
    }

    IupSetAttribute(animate_timer, "RUN", "NO");
  }

  return IUP_DEFAULT;
}

static void iExpanderAnimateChild(Ihandle* ih, Ihandle* child)
{
  int width, final_height, closing = 0;
  int frame_time = iupAttribGetInt(ih, "FRAMETIME");

  /* IMPORTANT: child must be a native container or this will not work. */

  if (ih->data->state != IEXPANDER_CLOSE)
  {
    /* was closed */
    IupSetAttribute(child, "VISIBLE", "YES");

    /* calculate the layout but do not update */
    iupLayoutCompute(IupGetDialog(ih));
  }
  else
  {
    /* was open */

    /* pretend it is still open */
    ih->data->state = IEXPANDER_OPEN;
    closing = 1;
  }

  final_height = child->currentheight;
  width = child->currentwidth;

  iupAttribSetStr(child, "OLD_MAXSIZE", iupAttribGet(child, "MAXSIZE"));

  if (!ih->data->animate_timer)
  {
    ih->data->animate_timer = IupTimer();
    IupSetCallback(ih->data->animate_timer, "ACTION_CB", (Icallback)iExpanderAnimateTimer_CB);
    iupAttribSet(ih->data->animate_timer, "_IUP_EXPANDER", (char*)ih);
  }

  IupSetInt(ih->data->animate_timer, "_IUP_FINAL_HEIGHT", final_height);
  IupSetInt(ih->data->animate_timer, "_IUP_CLOSING", closing);
  IupSetInt(ih->data->animate_timer, "_IUP_WIDTH", width);
  iupAttribSet(ih->data->animate_timer, "_IUP_CHILD", (char*)child);

  IupSetInt(ih->data->animate_timer, "TIME", frame_time);
  IupSetAttribute(ih->data->animate_timer, "RUN", "YES");
}

static void iExpanderOpenCloseChild(Ihandle* ih, int refresh, int callcb, int state)
{
  Ihandle* child = ih->firstchild->brother;

  if (ih->data->animate_timer && IupGetInt(ih->data->animate_timer, "RUN"))
    return;

  if (callcb)
  {
    IFni cb = (IFni)IupGetCallback(ih, "OPENCLOSE_CB");
    if (cb)
    {
      int ret = cb(ih, state);
      if (ret == IUP_IGNORE)
        return;
    }
  }

  ih->data->state = state;

  iExpanderUpdateStateImage(ih);
  if (ih->data->position == IEXPANDER_TOP)
    iExpanderUpdateTitleState(ih);

  if (child)
  {
    if (refresh && ih->data->animation && ih->data->position == IEXPANDER_TOP)
      iExpanderAnimateChild(ih, child);
    else
    {
      if (ih->data->state == IEXPANDER_CLOSE)
        IupSetAttribute(child, "VISIBLE", "NO");
      else
        IupSetAttribute(child, "VISIBLE", "YES");

      if (refresh && ih->data->state_refresh)
        IupRefresh(child); /* this will re-compute the layout of the hole dialog */
    }
  }

  if (callcb)
  {
    IFn cb = IupGetCallback(ih, "ACTION");
    if (cb)
      cb(ih);
  }
}

static int iExpanderGetBarSize(Ihandle* ih)
{
  int bar_size;
  if (ih->data->bar_size == -1)
  {
    Ihandle* box = ih->firstchild->firstchild;

    iupBaseComputeNaturalSize(box);

    if (ih->data->position == IEXPANDER_LEFT || ih->data->position == IEXPANDER_RIGHT)
      bar_size = box->naturalwidth;
    else
      bar_size = box->naturalheight;
  }
  else
    bar_size = ih->data->bar_size;

  return bar_size;
}

static void iExpanderUpdateTitle(Ihandle* ih)
{
  Ihandle* box = ih->firstchild->firstchild;
  Ihandle* expand_button = box->firstchild;
  Ihandle* label = expand_button->brother;
  char* title = iupAttribGet(ih, "TITLE");
  char* titleimage = iupAttribGet(ih, "TITLEIMAGE");
  if (title || titleimage)
  {
    if (!ih->handle) /* only update these before map*/
    {
      IupSetAttribute(box, "MARGIN", "0x1");
      IupSetAttribute(box, "GAP", "1");
      IupSetAttribute(expand_button, "EXPAND", "NO");
    }

    IupSetStrAttribute(label, "VISIBLE", "Yes");
    IupSetStrAttribute(label, "TITLE", title);
    IupSetStrAttribute(label, "IMAGE", titleimage);
  }
  else
  {
    if (!ih->handle) /* only update these before map*/
    {
      IupSetAttribute(box, "MARGIN", "2x2");
      IupSetAttribute(box, "GAP", "0");
      IupSetAttribute(expand_button, "EXPAND", "HORIZONTAL");
    }

    IupSetStrAttribute(label, "VISIBLE", "No");
    IupSetAttribute(label, "TITLE", NULL);
    IupSetAttribute(label, "IMAGE", NULL);
  }

  iExpanderUpdateStateImage(ih);

  if (ih->data->position == IEXPANDER_TOP)
    iExpanderUpdateTitleState(ih);
}

static void iExpanderUpdateExtraButtonImage(Ihandle* ih, Ihandle* extra_button, int pressed)
{
  int button = iupAttribGetInt(extra_button, "EXTRABUTTON_NUMBER");
  char* image = iupAttribGetId(ih, "IMAGEEXTRA", button);
  if (!image)
    return;

  if (pressed)
  {
    char* impress = iupAttribGetId(ih, "IMAGEEXTRAPRESS", button);
    if (impress) image = impress;
  }
  else if (iupAttribGet(extra_button, "HIGHLIGHT"))
  {
    char* imhighlight = iupAttribGetId(ih, "IMAGEEXTRAHIGHLIGHT", button);
    if (imhighlight) image = imhighlight;
  }

  IupSetStrAttribute(extra_button, "IMAGE", image);
}

static int iExpanderExtraButtonButton_CB(Ihandle* extra_button, int button, int pressed, int x, int y, char* status);
static int iExpanderExtraButtonEnterWindow_cb(Ihandle* extra_button);
static int iExpanderExtraButtonLeaveWindow_cb(Ihandle* extra_button);

static void iExpanderAddExtraButton(Ihandle* ih, Ihandle* extra_box, int number)
{
  Ihandle* extra_button = IupLabel(NULL);
  IupSetInt(extra_button, "EXTRABUTTON_NUMBER", number);
  IupSetCallback(extra_button, "BUTTON_CB", (Icallback)iExpanderExtraButtonButton_CB);
  IupSetCallback(extra_button, "ENTERWINDOW_CB", (Icallback)iExpanderExtraButtonEnterWindow_cb);
  IupSetCallback(extra_button, "LEAVEWINDOW_CB", (Icallback)iExpanderExtraButtonLeaveWindow_cb);

  iExpanderUpdateExtraButtonImage(ih, extra_button, 0);

  IupAppend(extra_box, extra_button);
}

static void iExpanderUpdateExtraButtons(Ihandle* ih)
{
  Ihandle* box = ih->firstchild->firstchild;
  Ihandle* extra_box = box->firstchild->brother->brother;  /* (expand_button, label, extra_box) */

  /* called only when BARPOSITION=TOP */

  if (extra_box)
    IupDestroy(extra_box);

  if (ih->data->extra_buttons)
  {
    extra_box = IupHbox(NULL);
    IupSetAttribute(box, "MARGIN", "0x0");
    IupSetAttribute(box, "GAP", "2");
    IupAppend(box, extra_box);

    iExpanderAddExtraButton(ih, extra_box, ih->data->extra_buttons);
    if (ih->data->extra_buttons > 1)
      iExpanderAddExtraButton(ih, extra_box, ih->data->extra_buttons-1);
    if (ih->data->extra_buttons > 2)
      iExpanderAddExtraButton(ih, extra_box, ih->data->extra_buttons-2);
  }
}

static int iExpanderTitleButton_CB(Ihandle* title_label, int button, int pressed, int x, int y, char* status);
static int iExpanderTitleEnterWindow_cb(Ihandle* title_label);
static int iExpanderTitleLeaveWindow_cb(Ihandle* title_label);

static int iExpanderExpandButtonButton_CB(Ihandle* expand_button, int button, int pressed, int x, int y, char* status);
static int iExpanderExpandButtonEnterWindow_cb(Ihandle* expand_button);
static int iExpanderExpandButtonLeaveWindow_cb(Ihandle* expand_button);

static void iExpanderUpdateBox(Ihandle* ih)
{
  Ihandle* bar = ih->firstchild;
  Ihandle* box = ih->firstchild->firstchild;
  Ihandle* expand_button;

  if (box)
    IupDestroy(box);

  expand_button = IupLabel(NULL);
  IupSetAttribute(expand_button, "ALIGNMENT", "ACENTER:ACENTER");
  IupSetCallback(expand_button, "BUTTON_CB", (Icallback)iExpanderExpandButtonButton_CB);
  IupSetCallback(expand_button, "ENTERWINDOW_CB", (Icallback)iExpanderExpandButtonEnterWindow_cb);
  IupSetCallback(expand_button, "LEAVEWINDOW_CB", (Icallback)iExpanderExpandButtonLeaveWindow_cb);

  if (ih->data->position == IEXPANDER_TOP)
  {
    Ihandle* title_label = IupLabel(NULL);
    IupSetAttribute(title_label, "EXPAND", "HORIZONTAL");
    IupSetCallback(title_label, "BUTTON_CB", (Icallback)iExpanderTitleButton_CB);
    IupSetCallback(title_label, "ENTERWINDOW_CB", (Icallback)iExpanderTitleEnterWindow_cb);
    IupSetCallback(title_label, "LEAVEWINDOW_CB", (Icallback)iExpanderTitleLeaveWindow_cb);

    box = IupHbox(expand_button, title_label, NULL);
  }
  else if (ih->data->position == IEXPANDER_BOTTOM)
  {
    box = IupHbox(expand_button, NULL);
    IupSetAttribute(expand_button, "EXPAND", "HORIZONTAL");
  }
  else
  {
    box = IupVbox(expand_button, NULL);
    IupSetAttribute(expand_button, "EXPAND", "VERTICAL");
  }

  IupSetAttribute(box, "MARGIN", "2x2");
  IupSetAttribute(box, "GAP", "0");
  IupSetAttribute(box, "ALIGNMENT", "ACENTER");

  IupAppend(bar, box);

  if (ih->data->position == IEXPANDER_TOP)
    iExpanderUpdateTitle(ih);

  iExpanderUpdateStateImage(ih);
}


/*****************************************************************************\
|* Internal Callbacks                                                        *|
\*****************************************************************************/


static int iExpanderGlobalMotion_cb(int x, int y)
{
  int child_x, child_y;
  Ihandle* ih = (Ihandle*)IupGetGlobal("_IUP_EXPANDER_GLOBAL");
  Ihandle* bar = ih->firstchild;
  Ihandle* child = ih->firstchild->brother;

  if (ih->data->state != IEXPANDER_OPEN_FLOAT)
  {
    IupSetGlobal("_IUP_EXPANDER_GLOBAL", NULL);
    IupSetFunction("GLOBALMOTION_CB", IupGetFunction("_IUP_OLD_GLOBALMOTION_CB"));
    IupSetFunction("_IUP_OLD_GLOBALMOTION_CB", NULL);
    IupSetGlobal("INPUTCALLBACKS", "No");
    return IUP_DEFAULT;
  }

  child_x = 0, child_y = 0;
  iupdrvClientToScreen(bar, &child_x, &child_y);
  if (x > child_x && x < child_x + bar->currentwidth &&
      y > child_y && y < child_y + bar->currentheight)
    return IUP_DEFAULT;  /* ignore if inside the bar */

  child_x = 0, child_y = 0;
  iupdrvClientToScreen(child, &child_x, &child_y);
  if (x < child_x || x > child_x+child->currentwidth ||
      y < child_y || y > child_y+child->currentheight)
  {
    iExpanderOpenCloseChild(ih, 0, 1, IEXPANDER_CLOSE);

    IupSetGlobal("_IUP_EXPANDER_GLOBAL", NULL);
    IupSetFunction("GLOBALMOTION_CB", IupGetFunction("_IUP_OLD_GLOBALMOTION_CB"));
    IupSetFunction("_IUP_OLD_GLOBALMOTION_CB", NULL);
    IupSetGlobal("INPUTCALLBACKS", "No");
  }

  return IUP_DEFAULT;
}

static int iExpanderAutoShowTimer_cb(Ihandle* auto_show_timer)
{
  Ihandle* ih = (Ihandle*)iupAttribGet(auto_show_timer, "_IUP_EXPANDER");
  Ihandle* child = ih->firstchild->brother;

  /* run timer just once each time */
  IupSetAttribute(auto_show_timer, "RUN", "NO");

  /* just show child on top,
     that's why child must be a native container when using autoshow. */
  iExpanderOpenCloseChild(ih, 0, 1, IEXPANDER_OPEN_FLOAT);
  IupRefreshChildren(ih);
  IupSetAttribute(child, "ZORDER", "TOP"); 

  /* now monitor mouse move */
  IupSetGlobal("INPUTCALLBACKS", "Yes");
  IupSetFunction("_IUP_OLD_GLOBALMOTION_CB", IupGetFunction("GLOBALMOTION_CB"));
  IupSetGlobal("_IUP_EXPANDER_GLOBAL", (char*)ih);
  IupSetFunction("GLOBALMOTION_CB", (Icallback)iExpanderGlobalMotion_cb);
  return IUP_DEFAULT;
}

static void iExpanderSetExpandButtonHighlight(Ihandle* ih, Ihandle* expand_button, const char* value)
{
  iupAttribSet(expand_button, "HIGHLIGHT", value);
  iExpanderUpdateStateImage(ih);

  if (ih->data->position == IEXPANDER_TOP && ih->data->title_expand)
  {
    Ihandle* title_label = expand_button->brother;
    iupAttribSet(title_label, "HIGHLIGHT", value);
    iExpanderUpdateTitleState(ih);
  }
}

static int iExpanderExpandButtonLeaveWindow_cb(Ihandle* expand_button)
{
  if (iupAttribGet(expand_button, "HIGHLIGHT"))
  {
    /* expander -> bar -> box -> (expand_button, ...) */
    Ihandle* ih = IupGetParent(IupGetParent(IupGetParent(expand_button)));

    iExpanderSetExpandButtonHighlight(ih, expand_button, NULL);

    if (ih->data->auto_show)
    {
      if (IupGetInt(ih->data->auto_show_timer, "RUN"))
        IupSetAttribute(ih->data->auto_show_timer, "RUN", "NO");
    }
  }
  return IUP_DEFAULT;
}

static int iExpanderExpandButtonEnterWindow_cb(Ihandle* expand_button)
{
  if (!iupAttribGet(expand_button, "HIGHLIGHT"))
  {
    /* expander -> bar -> box -> (expand_button, ...) */
    Ihandle* ih = IupGetParent(IupGetParent(IupGetParent(expand_button)));
    Ihandle* child = ih->firstchild->brother;

    iExpanderSetExpandButtonHighlight(ih, expand_button, "1");

    if (ih->data->auto_show &&
        child &&
        ih->data->state == IEXPANDER_CLOSE)
      IupSetAttribute(ih->data->auto_show_timer, "RUN", "Yes");
  }
  return IUP_DEFAULT;
}

static int iExpanderExpandButtonButton_CB(Ihandle* expand_button, int button, int pressed, int x, int y, char* status)
{
  if (button == IUP_BUTTON1)
  {
    /* expander -> bar -> box -> (expand_button, ...) */
    Ihandle* ih = IupGetParent(IupGetParent(IupGetParent(expand_button)));

    /* handle when mouse is pressed and moved to/from inside the canvas */
    if (x < 0 || x > expand_button->currentwidth - 1 ||
        y < 0 || y > expand_button->currentheight - 1)
    {
      if (iupAttribGet(expand_button, "HIGHLIGHT"))
        iExpanderSetExpandButtonHighlight(ih, expand_button, NULL);
    }
    else
    {
      if (!iupAttribGet(expand_button, "HIGHLIGHT"))
        iExpanderSetExpandButtonHighlight(ih, expand_button, "1");
    }

    if (pressed && !iup_isdouble(status))
    {
      if (ih->data->auto_show)
      {
        if (IupGetInt(ih->data->auto_show_timer, "RUN"))
          IupSetAttribute(ih->data->auto_show_timer, "RUN", "NO");
      }

      iExpanderOpenCloseChild(ih, 1, 1, ih->data->state == IEXPANDER_OPEN ? IEXPANDER_CLOSE : IEXPANDER_OPEN);
    }
  }

  (void)x;
  (void)y;
  return IUP_DEFAULT;
}

static void iExpanderSetTitleHighlight(Ihandle* ih, Ihandle* title_label, const char* value)
{
  Ihandle* expand_button = IupGetChild(IupGetParent(title_label), 0);
  iupAttribSet(expand_button, "HIGHLIGHT", value);
  iExpanderUpdateStateImage(ih);

  iupAttribSet(title_label, "HIGHLIGHT", value);
  iExpanderUpdateTitleState(ih);
}

static int iExpanderTitleLeaveWindow_cb(Ihandle* title_label)
{
  /* expander -> bar -> box -> (expand_button, title_label, ...) */
  Ihandle* ih = IupGetParent(IupGetParent(IupGetParent(title_label)));

  if (ih->data->title_expand)
  {
    if (iupAttribGet(title_label, "HIGHLIGHT"))
      iExpanderSetTitleHighlight(ih, title_label, NULL);
  }
  return IUP_DEFAULT;
}

static int iExpanderTitleEnterWindow_cb(Ihandle* title_label)
{
  /* expander -> bar -> box -> (expand_button, title_label, ...) */
  Ihandle* ih = IupGetParent(IupGetParent(IupGetParent(title_label)));

  if (ih->data->title_expand)
  {
    if (!iupAttribGet(title_label, "HIGHLIGHT"))
      iExpanderSetTitleHighlight(ih, title_label, "1");
  }
  return IUP_DEFAULT;
}

static int iExpanderTitleButton_CB(Ihandle* title_label, int button, int pressed, int x, int y, char* status)
{
  if (button == IUP_BUTTON1)
  {
    /* expander -> bar -> box -> (expand_button, title_label, ...) */
    Ihandle* ih = IupGetParent(IupGetParent(IupGetParent(title_label)));

    if (ih->data->title_expand)
    {
      /* handle when mouse is pressed and moved to/from inside the canvas */
      if (x < 0 || x > title_label->currentwidth - 1 ||
          y < 0 || y > title_label->currentheight - 1)
      {
        if (iupAttribGet(title_label, "HIGHLIGHT"))
          iExpanderSetTitleHighlight(ih, title_label, NULL);
      }
      else
      {
        if (!iupAttribGet(title_label, "HIGHLIGHT"))
          iExpanderSetTitleHighlight(ih, title_label, "1");
      }

      if (pressed && !iup_isdouble(status))
        iExpanderOpenCloseChild(ih, 1, 1, ih->data->state == IEXPANDER_OPEN ? IEXPANDER_CLOSE : IEXPANDER_OPEN);
    }
  }

  return IUP_DEFAULT;
}

static int iExpanderExtraButtonLeaveWindow_cb(Ihandle* extra_button)
{
  /* expander -> bar -> box -> (expand_button, label, extra_box -> (extra_button) */
  Ihandle* ih = IupGetParent(IupGetParent(IupGetParent(IupGetParent(extra_button))));

  if (iupAttribGet(extra_button, "HIGHLIGHT"))
  {
    iupAttribSet(extra_button, "HIGHLIGHT", NULL);
    iExpanderUpdateExtraButtonImage(ih, extra_button, 0);
  }
  return IUP_DEFAULT;
}

static int iExpanderExtraButtonEnterWindow_cb(Ihandle* extra_button)
{
  /* expander -> bar -> box -> (expand_button, label, extra_box -> (extra_button)) */
  Ihandle* ih = IupGetParent(IupGetParent(IupGetParent(IupGetParent(extra_button))));

  if (!iupAttribGet(extra_button, "HIGHLIGHT"))
  {
    iupAttribSet(extra_button, "HIGHLIGHT", "1");
    iExpanderUpdateExtraButtonImage(ih, extra_button, 0);
  }
  return IUP_DEFAULT;
}

static int iExpanderExtraButtonButton_CB(Ihandle* extra_button, int button, int pressed, int x, int y, char* status)
{
  if (button == IUP_BUTTON1)
  {
    /* expander -> bar -> box -> (expand_button, label, extra_box -> (extra_button)) */
    Ihandle* ih = IupGetParent(IupGetParent(IupGetParent(IupGetParent(extra_button))));

    IFnii cb = (IFnii)IupGetCallback(ih, "EXTRABUTTON_CB");
    if (cb)
    {
      button = IupGetInt(extra_button, "EXTRABUTTON_NUMBER");
      cb(ih, button, pressed);
    }

    iExpanderUpdateExtraButtonImage(ih, extra_button, pressed);
  }

  (void)x;
  (void)y;
  (void)status;
  return IUP_DEFAULT;
}

static int iExpanderBarRedraw_CB(Ihandle* bar)
{
  Ihandle* ih = bar->parent;
  char* backcolor = iupAttribGet(ih, "BACKCOLOR");
  int frame_width = 0;
  int frame = iupAttribGetBoolean(ih, "FRAME");
  IdrawCanvas* dc = iupdrvDrawCreateCanvas(bar);

  if (!backcolor)
    backcolor = iupBaseNativeParentGetBgColorAttrib(ih);

  /* draw border - can still be disabled setting frame_width=0 */
  if (frame)
  {
    char* frame_color = iupAttribGetStr(ih, "FRAMECOLOR");
    int active = IupGetInt(ih, "ACTIVE");
    frame_width = iupAttribGetInt(ih, "FRAMEWIDTH");

    iupFlatDrawBorder(dc, 0, bar->currentwidth - 1,
                          0, bar->currentheight - 1,
                          frame_width, frame_color, NULL, active);
  }

  /* draw child area background */
  iupFlatDrawBox(dc, frame_width, bar->currentwidth  - 1 - frame_width,
                     frame_width, bar->currentheight - 1 - frame_width, backcolor, NULL, 1);  /* background is always active */

  iupdrvDrawFlush(dc);

  iupdrvDrawKillCanvas(dc);

  return IUP_DEFAULT;
}

/*****************************************************************************\
|* Attributes                                                                *|
\*****************************************************************************/


static char* iExpanderGetClientSizeAttrib(Ihandle* ih)
{
  int width = ih->currentwidth;
  int height = ih->currentheight;
  int bar_size = iExpanderGetBarSize(ih);

  if (ih->data->position == IEXPANDER_LEFT || ih->data->position == IEXPANDER_RIGHT)
    width -= bar_size;
  else
    height -= bar_size;

  if (width < 0) width = 0;
  if (height < 0) height = 0;
  return iupStrReturnIntInt(width, height, 'x');
}

static int iExpanderSetPositionAttrib(Ihandle* ih, const char* value)
{
  if (ih->handle)
    return 0; /* can be changed only before map */

  if (iupStrEqualNoCase(value, "LEFT"))
    ih->data->position = IEXPANDER_LEFT;
  else if (iupStrEqualNoCase(value, "RIGHT"))
    ih->data->position = IEXPANDER_RIGHT;
  else if (iupStrEqualNoCase(value, "BOTTOM"))
    ih->data->position = IEXPANDER_BOTTOM;
  else  /* Default = TOP */
    ih->data->position = IEXPANDER_TOP;

  iExpanderUpdateBox(ih);
  if (ih->data->position == IEXPANDER_TOP)
    iExpanderUpdateExtraButtons(ih);

  return 0;  /* do not store value in hash table */
}

static char* iExpanderGetPositionAttrib(Ihandle* ih)
{
  if (ih->data->position == IEXPANDER_LEFT)
    return "LEFT";
  else if (ih->data->position == IEXPANDER_RIGHT)
    return "RIGHT";
  else if (ih->data->position == IEXPANDER_BOTTOM)
    return "BOTTOM";
  else  /* Default = TOP */
    return "TOP";
}

static int iExpanderSetBarSizeAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    ih->data->bar_size = -1;
  else
    iupStrToInt(value, &ih->data->bar_size);  /* must manually update layout */
  return 0; /* do not store value in hash table */
}

static char* iExpanderGetBarSizeAttrib(Ihandle* ih)
{
  int bar_size = iExpanderGetBarSize(ih);
  return iupStrReturnInt(bar_size);
}

static int iExpanderSetStateAttrib(Ihandle* ih, const char* value)
{
  int state;
  if (iupStrEqualNoCase(value, "OPEN"))
    state = IEXPANDER_OPEN;
  else
    state = IEXPANDER_CLOSE;

  if (ih->data->state == state)
    return 0;

  iExpanderOpenCloseChild(ih, 1, 0, state);

  return 0; /* do not store value in hash table */
}

static char* iExpanderGetStateAttrib(Ihandle* ih)
{
  if (ih->data->state)
    return "OPEN";
  else
    return "CLOSE";
}

static int iExpanderSetStateRefreshAttrib(Ihandle* ih, const char* value)
{
  ih->data->state_refresh = iupStrBoolean(value);
  return 0; /* do not store value in hash table */
}

static char* iExpanderGetStateRefreshAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->state_refresh);
}

static int iExpanderSetForeColorAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->position == IEXPANDER_TOP)
  {
    if (ih->data->title_expand)
    {
      iupAttribSetStr(ih, "FORECOLOR", value);
      iExpanderUpdateTitleState(ih);
    }
    else 
    {
      Ihandle* box = ih->firstchild->firstchild;
      Ihandle* title_label = box->firstchild->brother;
      IupSetStrAttribute(title_label, "FGCOLOR", value);
    }
  }

  return 1;
}

static int iExpanderSetOpenColorAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->position == IEXPANDER_TOP)
  {
    iupAttribSetStr(ih, "OPENCOLOR", value);
    iExpanderUpdateTitleState(ih);
  }
  return 1;
}

static int iExpanderSetHighColorAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->position == IEXPANDER_TOP)
  {
    iupAttribSetStr(ih, "HIGHCOLOR", value);
    iExpanderUpdateTitleState(ih);
  }
  return 1;
}

static int iExpanderSetBackColorAttrib(Ihandle* ih, const char* value)
{
  Ihandle* bar = ih->firstchild;
  IupSetStrAttribute(bar, "BGCOLOR", value); /* used in children */
  IupUpdate(bar);
  (void)value;
  return 1;
}

static int iExpanderSetImageAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->state == IEXPANDER_CLOSE)
  {
    Ihandle* box = ih->firstchild->firstchild;
    Ihandle* expand_button = box->firstchild;
    IupSetStrAttribute(expand_button, "IMAGE", value);
  }
  return 1;
}

static int iExpanderSetImageOpenAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->state != IEXPANDER_CLOSE)
  {
    Ihandle* box = ih->firstchild->firstchild;
    Ihandle* expand_button = box->firstchild;
    IupSetStrAttribute(expand_button, "IMAGE", value);
  }
  return 1;
}

static int iExpanderSetImageExtra(Ihandle* ih, const char* value, int button)
{
  if (ih->data->extra_buttons > button - 1 && ih->data->position == IEXPANDER_TOP)
  {
    Ihandle* box = ih->firstchild->firstchild;
    Ihandle* extra_box = box->firstchild->brother->brother;  /* (expand_button, label, extra_box) */

    if (extra_box)
    {
      Ihandle* extra_button = IupGetChild(extra_box, ih->data->extra_buttons - button);
      iupAttribSetStrId(ih, "IMAGEEXTRA", button, value);
      iExpanderUpdateExtraButtonImage(ih, extra_button, 0);
    }
  }

  return 1;
}

static int iExpanderSetImageExtraPress(Ihandle* ih, const char* value, int button)
{
  if (ih->data->extra_buttons > button - 1 && ih->data->position == IEXPANDER_TOP)
  {
    Ihandle* box = ih->firstchild->firstchild;
    Ihandle* extra_box = box->firstchild->brother->brother;  /* (expand_button, label, extra_box) */

    if (extra_box)
    {
      Ihandle* extra_button = IupGetChild(extra_box, ih->data->extra_buttons - button);
      iupAttribSetStrId(ih, "IMAGEEXTRAPRESS", button, value);
      iExpanderUpdateExtraButtonImage(ih, extra_button, 0);
    }
  }

  return 1;
}

static int iExpanderSetImageExtraHighlight(Ihandle* ih, const char* value, int button)
{
  if (ih->data->extra_buttons > button - 1 && ih->data->position == IEXPANDER_TOP)
  {
    Ihandle* box = ih->firstchild->firstchild;
    Ihandle* extra_box = box->firstchild->brother->brother;  /* (expand_button, label, extra_box) */

    if (extra_box)
    {
      Ihandle* extra_button = IupGetChild(extra_box, ih->data->extra_buttons - button);
      iupAttribSetStrId(ih, "IMAGEEXTRAHIGHLIGHT", button, value);
      iExpanderUpdateExtraButtonImage(ih, extra_button, 0);
    }
  }

  return 1;
}

static int iExpanderSetImageExtra1Attrib(Ihandle* ih, const char* value)
{
  return iExpanderSetImageExtra(ih, value, 1);
}

static int iExpanderSetImageExtraPress1Attrib(Ihandle* ih, const char* value)
{
  return iExpanderSetImageExtraPress(ih, value, 1);
}

static int iExpanderSetImageExtraHighlight1Attrib(Ihandle* ih, const char* value)
{
  return iExpanderSetImageExtraHighlight(ih, value, 1);
}

static int iExpanderSetImageExtra2Attrib(Ihandle* ih, const char* value)
{
  return iExpanderSetImageExtra(ih, value, 2);
}

static int iExpanderSetImageExtraPress2Attrib(Ihandle* ih, const char* value)
{
  return iExpanderSetImageExtraPress(ih, value, 2);
}

static int iExpanderSetImageExtraHighlight2Attrib(Ihandle* ih, const char* value)
{
  return iExpanderSetImageExtraHighlight(ih, value, 2);
}

static int iExpanderSetImageExtra3Attrib(Ihandle* ih, const char* value)
{
  return iExpanderSetImageExtra(ih, value, 3);
}

static int iExpanderSetImageExtraPress3Attrib(Ihandle* ih, const char* value)
{
  return iExpanderSetImageExtraPress(ih, value, 3);
}

static int iExpanderSetImageExtraHighlight3Attrib(Ihandle* ih, const char* value)
{
  return iExpanderSetImageExtraHighlight(ih, value, 3);
}

static int iExpanderSetTitleAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->position == IEXPANDER_TOP)
  {
    iupAttribSetStr(ih, "TITLE", value);
    iupAttribSet(ih, "TITLEIMAGE", NULL);
    iExpanderUpdateTitle(ih);
  }

  return 1;
}

static int iExpanderSetTitleImageAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->position == IEXPANDER_TOP)
  {
    iupAttribSetStr(ih, "TITLEIMAGE", value);
    iupAttribSet(ih, "TITLE", NULL);
    iExpanderUpdateTitle(ih);
  }

  return 1;
}

static int iExpanderSetTitleImageOpenAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->position == IEXPANDER_TOP)
  {
    iupAttribSetStr(ih, "TITLEIMAGEOPEN", value);
    iExpanderUpdateTitle(ih);
  }

  return 1;
}

static int iExpanderSetAutoShowAttrib(Ihandle* ih, const char* value)
{
  ih->data->auto_show = iupStrBoolean(value);
  if (ih->data->auto_show)
  {
    if (!ih->data->auto_show_timer)
    {
      ih->data->auto_show_timer = IupTimer();
      IupSetAttribute(ih->data->auto_show_timer, "TIME", "1000");  /* 1 second */
      IupSetCallback(ih->data->auto_show_timer, "ACTION_CB", iExpanderAutoShowTimer_cb);
      iupAttribSet(ih->data->auto_show_timer, "_IUP_EXPANDER", (char*)ih);  /* 1 second */
    }
  }
  else
  {
    if (ih->data->auto_show_timer)
      IupSetAttribute(ih->data->auto_show_timer, "RUN", "NO");
  }
  return 0; /* do not store value in hash table */
}

static char* iExpanderGetAutoShowAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->auto_show);
}

static int iExpanderSetAnimationAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqualNoCase(value, "SLIDE"))
    ih->data->animation = 2;
  else if (iupStrEqualNoCase(value, "CURTAIN"))
    ih->data->animation = 1;
  else
    ih->data->animation = 0;

  return 0; /* do not store value in hash table */
}

static char* iExpanderGetAnimationAttrib(Ihandle* ih)
{
  if (ih->data->animation == 2)
    return "SLIDE";
  else if (ih->data->animation)
    return "CURTAIN";
  else
    return "NO";
}

static int iExpanderSetTitleExpandAttrib(Ihandle* ih, const char* value)
{
  ih->data->title_expand = iupStrBoolean(value);
  return 0; /* do not store value in hash table */
}

static char* iExpanderGetTitleExpandAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->title_expand);
}

static int iExpanderSetExtraButtonsAttrib(Ihandle* ih, const char* value)
{
  if (ih->handle)
    return 0; /* can be changed only before map */

  if (!value)
    ih->data->extra_buttons = 0;
  else
  {
    iupStrToInt(value, &(ih->data->extra_buttons));
    if (ih->data->extra_buttons < 0)
      ih->data->extra_buttons = 0;
    else if (ih->data->extra_buttons > 3)
      ih->data->extra_buttons = 3;
  }

  if (ih->data->position == IEXPANDER_TOP)
    iExpanderUpdateExtraButtons(ih);

  return 0; /* do not store value in hash table */
}

static char* iExpanderGetExtraButtonsAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->extra_buttons);
}


/*****************************************************************************\
|* Methods                                                                   *|
\*****************************************************************************/


static void iExpanderComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
  int child_expand = 0,
      natural_w, natural_h;
  Ihandle* child = ih->firstchild->brother;
  Ihandle* box = ih->firstchild->firstchild;
  int bar_size = iExpanderGetBarSize(ih);

  iupBaseComputeNaturalSize(box);

  /* bar */
  if (ih->data->position == IEXPANDER_LEFT || ih->data->position == IEXPANDER_RIGHT)
  {
    natural_w = bar_size;
    natural_h = box->naturalheight;
  }
  else
  {
    natural_w = box->naturalwidth;
    natural_h = bar_size;
  }

  if (child)
  {
    /* update child natural bar_size first */
    iupBaseComputeNaturalSize(child);

    if (ih->data->position == IEXPANDER_LEFT || ih->data->position == IEXPANDER_RIGHT)
    {
      if (ih->data->state == IEXPANDER_OPEN)  /* only open, not float */
        natural_w += child->naturalwidth;
      natural_h = iupMAX(natural_h, child->naturalheight);
    }
    else
    {
      natural_w = iupMAX(natural_w, child->naturalwidth);
      if (ih->data->state == IEXPANDER_OPEN)  /* only open, not float */
        natural_h += child->naturalheight;
    }

    if (ih->data->state == IEXPANDER_OPEN)
      child_expand = child->expand;
    else
    {
      if (ih->data->position == IEXPANDER_LEFT || ih->data->position == IEXPANDER_RIGHT)
        child_expand = child->expand & IUP_EXPAND_HEIGHT;  /* only vertical allowed */
      else
        child_expand = child->expand & IUP_EXPAND_WIDTH;  /* only horizontal allowed */
    }
  }

  *children_expand = child_expand;
  *w = natural_w;
  *h = natural_h;
}

static void iExpanderSetChildrenCurrentSizeMethod(Ihandle* ih, int shrink)
{
  Ihandle* bar = ih->firstchild;
  Ihandle* child = ih->firstchild->brother;
  Ihandle* box = ih->firstchild->firstchild;
  int width = ih->currentwidth;
  int height = ih->currentheight;
  int bar_size = iExpanderGetBarSize(ih);

  if (ih->data->position == IEXPANDER_LEFT || ih->data->position == IEXPANDER_RIGHT)
  {
    bar->currentwidth = bar_size;
    bar->currentheight = ih->currentheight;

    if (ih->currentwidth < bar_size)
      ih->currentwidth = bar_size;

    width = ih->currentwidth - bar_size;
  }
  else /* IEXPANDER_TOP OR IEXPANDER_BOTTOM */
  {
    bar->currentwidth = ih->currentwidth;
    bar->currentheight = bar_size;

    if (ih->currentheight < bar_size)
      ih->currentheight = bar_size;

    height = ih->currentheight - bar_size;
  }

  /* force the box size to be the same size of the bar */
  box->naturalwidth = bar->currentwidth;
  box->naturalheight = bar->currentheight;
  iupBaseSetCurrentSize(box, bar->currentwidth, bar->currentheight, shrink);

  if (child)
  {
    if (ih->data->state == IEXPANDER_OPEN)
      iupBaseSetCurrentSize(child, width, height, shrink);
    else if (ih->data->state == IEXPANDER_OPEN_FLOAT)  /* simply set to the natural size */
      iupBaseSetCurrentSize(child, child->currentwidth, child->currentheight, shrink);
  }
}

static void iExpanderSetChildrenPositionMethod(Ihandle* ih, int x, int y)
{
  Ihandle* bar = ih->firstchild;
  Ihandle* child = ih->firstchild->brother;
  int bar_size = iExpanderGetBarSize(ih);

  /* always position bar */
  if (ih->data->position == IEXPANDER_LEFT)
  {
    iupBaseSetPosition(bar, x, y);
    x += bar_size;
  }
  else if (ih->data->position == IEXPANDER_RIGHT)
    iupBaseSetPosition(bar, x + ih->currentwidth - bar_size, y);
  else if (ih->data->position == IEXPANDER_BOTTOM)
    iupBaseSetPosition(bar, x, y + ih->currentheight - bar_size);
  else /* IEXPANDER_TOP */
  {
    iupBaseSetPosition(bar, x, y);
    y += bar_size;
  }

  if (child)
  {
    if (ih->data->state == IEXPANDER_OPEN)
      iupBaseSetPosition(child, x, y);
    else if (ih->data->state == IEXPANDER_OPEN_FLOAT)
    {
      if (ih->data->position == IEXPANDER_RIGHT)
        x -= child->currentwidth;
      else if (ih->data->position == IEXPANDER_BOTTOM)
        y -= child->currentheight;

      iupBaseSetPosition(child, x, y);
    }
  }
}

static void iExpanderChildAddedMethod(Ihandle* ih, Ihandle* child)
{
  if (ih->firstchild == child) /* it was inserted before the bar */
  {
    ih->firstchild = child->brother; /* the actual bar */
    child->brother = ih->firstchild->brother;
    ih->firstchild->brother = child;
  }

  iExpanderOpenCloseChild(ih, 0, 0, ih->data->state);
  (void)child;
}

static int iExpanderCreateMethod(Ihandle* ih, void** params)
{
  Ihandle* bar;

  ih->data = iupALLOCCTRLDATA();

  ih->data->position = IEXPANDER_TOP;
  ih->data->state = IEXPANDER_OPEN;
  ih->data->bar_size = -1;
  ih->data->state_refresh = 1;

  bar = IupBackgroundBox(NULL);
  iupChildTreeAppend(ih, bar);  /* bar will always be the firstchild */
  bar->flags |= IUP_INTERNAL;

  iExpanderUpdateBox(ih);

  IupSetAttribute(bar, "CANFOCUS", "NO");
  IupSetAttribute(bar, "BORDER", "NO");
  IupSetAttribute(bar, "EXPAND", "YES");
  IupSetCallback(bar, "ACTION", (Icallback)iExpanderBarRedraw_CB);

  if (params)
  {
    Ihandle** iparams = (Ihandle**)params;
    if (*iparams)
      IupAppend(ih, *iparams);
  }

  /* Internal Hierarchy:
    Ihandle* bar = ih->firstchild;
    Ihandle* child = bar->brother;
    Ihandle* box = bar->firstchild;
    Ihandle* expand_button = box->firstchild;
    Ihandle* title_label = expand_button->brother;  (only when position==IEXPANDER_TOP)
    Ihandle* extra_box = title_label->brother;  (only when position==IEXPANDER_TOP)
  */

  return IUP_NOERROR;
}

static void iExpanderDestroyMethod(Ihandle* ih)
{
  if (ih->data->auto_show_timer)
    IupDestroy(ih->data->auto_show_timer);
  if (ih->data->animate_timer)
    IupDestroy(ih->data->animate_timer);
}

Iclass* iupExpanderNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name   = "expander";
  ic->format = "h";   /* one Ihandle* */
  ic->nativetype = IUP_TYPEVOID;
  ic->childtype = IUP_CHILDMANY+2;  /* canvas+child */
  ic->is_interactive = 0;

  /* Class functions */
  ic->New     = iupExpanderNewClass;
  ic->Create  = iExpanderCreateMethod;
  ic->Destroy = iExpanderDestroyMethod;
  ic->Map     = iupBaseTypeVoidMapMethod;
  ic->ChildAdded = iExpanderChildAddedMethod;

  ic->ComputeNaturalSize     = iExpanderComputeNaturalSizeMethod;
  ic->SetChildrenCurrentSize = iExpanderSetChildrenCurrentSizeMethod;
  ic->SetChildrenPosition    = iExpanderSetChildrenPositionMethod;

  /* Base Callbacks */
  iupBaseRegisterBaseCallbacks(ic);

  /* Callbacks */
  iupClassRegisterCallback(ic, "ACTION", "");
  iupClassRegisterCallback(ic, "OPENCLOSE_CB", "i");
  iupClassRegisterCallback(ic, "EXTRABUTTON_CB", "ii");

  /* Common */
  iupBaseRegisterCommonAttrib(ic);

  /* Base Container */
  iupClassRegisterAttribute(ic, "EXPAND", iupBaseContainerGetExpandAttrib, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLIENTSIZE", iExpanderGetClientSizeAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLIENTOFFSET", iupBaseGetClientOffsetAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_READONLY | IUPAF_NO_INHERIT);

  /* IupExpander only */
  iupClassRegisterAttribute(ic, "BARPOSITION", iExpanderGetPositionAttrib, iExpanderSetPositionAttrib, IUPAF_SAMEASSYSTEM, "TOP", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BARSIZE", iExpanderGetBarSizeAttrib, iExpanderSetBarSizeAttrib, IUPAF_SAMEASSYSTEM, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STATE", iExpanderGetStateAttrib, iExpanderSetStateAttrib, IUPAF_SAMEASSYSTEM, "OPEN", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORECOLOR", NULL, iExpanderSetForeColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HIGHCOLOR", NULL, iExpanderSetHighColorAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "OPENCOLOR", NULL, iExpanderSetOpenColorAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKCOLOR", NULL, iExpanderSetBackColorAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLE", NULL, iExpanderSetTitleAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLEIMAGE", NULL, iExpanderSetTitleImageAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLEIMAGEOPEN", NULL, iExpanderSetTitleImageOpenAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLEIMAGEHIGHLIGHT", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLEIMAGEOPENHIGHLIGHT", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLEEXPAND", iExpanderGetTitleExpandAttrib, iExpanderSetTitleExpandAttrib, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AUTOSHOW", iExpanderGetAutoShowAttrib, iExpanderSetAutoShowAttrib, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "EXTRABUTTONS", iExpanderGetExtraButtonsAttrib, iExpanderSetExtraButtonsAttrib, IUPAF_SAMEASSYSTEM, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ANIMATION", iExpanderGetAnimationAttrib, iExpanderSetAnimationAttrib, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NUMFRAMES", NULL, NULL, IUPAF_SAMEASSYSTEM, "10", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FRAMETIME", NULL, NULL, IUPAF_SAMEASSYSTEM, "30", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STATEREFRESH", iExpanderGetStateRefreshAttrib, iExpanderSetStateRefreshAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "IMAGE", NULL, iExpanderSetImageAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEHIGHLIGHT", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEOPEN", NULL, iExpanderSetImageOpenAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEOPENHIGHLIGHT", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "IMAGEEXTRA1", NULL, iExpanderSetImageExtra1Attrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEEXTRAPRESS1", NULL, iExpanderSetImageExtraPress1Attrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEEXTRAHIGHLIGHT1", NULL, iExpanderSetImageExtraHighlight1Attrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEEXTRA2", NULL, iExpanderSetImageExtra2Attrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEEXTRAPRESS2", NULL, iExpanderSetImageExtraPress2Attrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEEXTRAHIGHLIGHT2", NULL, iExpanderSetImageExtraHighlight2Attrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEEXTRA3", NULL, iExpanderSetImageExtra3Attrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEEXTRAPRESS3", NULL, iExpanderSetImageExtraPress3Attrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEEXTRAHIGHLIGHT3", NULL, iExpanderSetImageExtraHighlight3Attrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "FRAME", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FRAMECOLOR", NULL, NULL, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FRAMEWIDTH", NULL, NULL, IUPAF_SAMEASSYSTEM, "1", IUPAF_NO_INHERIT);

  if (!IupGetHandle("IupArrowUp") || !IupGetHandle("IupArrowDown"))
    iExpanderLoadImages();

  return ic;
}

IUP_API Ihandle* IupExpander(Ihandle* child)
{
  void *children[2];
  children[0] = (void*)child;
  children[1] = NULL;
  return IupCreatev("expander", children);
}
