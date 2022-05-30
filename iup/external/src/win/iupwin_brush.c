/** \file
 * \brief Windows Brush Cache
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>

#include <windows.h>

#include "iup.h"
#include "iup_array.h"

#include "iupwin_brush.h"


typedef struct _IwinBrush
{
  HBRUSH hbrush;
  COLORREF color;
} IwinBrush;

static Iarray* win_brushes = NULL;

HBRUSH iupwinBrushGet(COLORREF color)
{
  int i, count = iupArrayCount(win_brushes);

  /* If a brush with the desired color already exists... */
  IwinBrush* brushes = (IwinBrush*)iupArrayGetData(win_brushes);
  for (i = 0; i < count; i++)
  {
    if (brushes[i].color == color)
      return brushes[i].hbrush;
  }

  /* If it doesn't exist, it should be created... */
  brushes = (IwinBrush*)iupArrayInc(win_brushes);

  brushes[i].color = color;
  brushes[i].hbrush = CreateSolidBrush(color);

  return brushes[i].hbrush;
}

void iupwinBrushInit(void)
{
  win_brushes = iupArrayCreate(50, sizeof(IwinBrush));
}

void iupwinBrushFinish(void)
{
  int i, count = iupArrayCount(win_brushes);
  IwinBrush* brushes = (IwinBrush*)iupArrayGetData(win_brushes);
  for (i = 0; i < count; i++)
  {
    DeleteObject(brushes[i].hbrush);
    brushes[i].hbrush = NULL;
  }
  iupArrayDestroy(win_brushes);
}
