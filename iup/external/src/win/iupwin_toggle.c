/** \file
 * \brief Toggle Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <windows.h>
#include <commctrl.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdarg.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_toggle.h"
#include "iup_drv.h"
#include "iup_image.h"

#include "iupwin_drv.h"
#include "iupwin_handle.h"
#include "iupwin_draw.h"
#include "iupwin_str.h"

#include "wdl.h"


/* Windows 11 ToggleSwitch dimensions (matching ModernWpf/WinUI 3 specs) */
#define SWITCH_TRACK_WIDTH  40
#define SWITCH_TRACK_HEIGHT 20
#define SWITCH_THUMB_SIZE   12  /* Normal state: 12x12, scales to 14x14 on hover */
#define SWITCH_THUMB_MARGIN 4   /* (20 - 12) / 2 = 4px margin on each side */

/* Animation parameters (matching Windows 11 ModernWpf specs) */
#define SWITCH_ANIMATION_DURATION 367  /* milliseconds (0.367s reposition duration) */
#define SWITCH_ANIMATION_FPS      60   /* frames per second */
#define SWITCH_ANIMATION_INTERVAL (1000 / SWITCH_ANIMATION_FPS)  /* ~16ms */

typedef struct _IupWinSwitchData
{
  double thumb_position;      /* 0.0 = left (off), 1.0 = right (on) */
  double animation_start;     /* Animation start position */
  double animation_end;       /* Animation end position */
  DWORD animation_start_time; /* GetTickCount() when animation started */
  UINT_PTR timer_id;          /* Timer ID for animation */
  int is_animating;           /* 1 if animation in progress */
  int checked_state;          /* Current checked state (BST_CHECKED or BST_UNCHECKED) */
  int is_hovering;            /* 1 if mouse is over the control */
} IupWinSwitchData;

static double winSwitchEaseInOutQuad(double t)
{
  if (t < 0.5)
    return 2.0 * t * t;
  else
    return 1.0 - 2.0 * (1.0 - t) * (1.0 - t);
}

static void winSwitchAnimationStep(Ihandle* ih, IupWinSwitchData* switch_data)
{
  DWORD current_time = GetTickCount();
  DWORD elapsed = current_time - switch_data->animation_start_time;

  if (elapsed >= SWITCH_ANIMATION_DURATION)
  {
    /* Animation complete */
    switch_data->thumb_position = switch_data->animation_end;
    switch_data->is_animating = 0;

    if (switch_data->timer_id != 0)
    {
      KillTimer(ih->handle, switch_data->timer_id);
      switch_data->timer_id = 0;
    }
  }
  else
  {
    /* Interpolate with easing */
    double t = (double)elapsed / (double)SWITCH_ANIMATION_DURATION;
    double eased_t = winSwitchEaseInOutQuad(t);
    switch_data->thumb_position = switch_data->animation_start +
                                   (switch_data->animation_end - switch_data->animation_start) * eased_t;
  }

  /* Trigger repaint */
  InvalidateRect(ih->handle, NULL, FALSE);
}

static VOID CALLBACK winSwitchTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
  Ihandle* ih;
  IupWinSwitchData* switch_data;

  (void)uMsg;
  (void)dwTime;

  ih = iupwinHandleGet(hwnd);
  if (!ih)
    return;

  switch_data = (IupWinSwitchData*)iupAttribGet(ih, "_IUPWIN_SWITCHDATA");
  if (!switch_data || switch_data->timer_id != idEvent)
    return;

  winSwitchAnimationStep(ih, switch_data);
}

static void winSwitchStartAnimation(Ihandle* ih, IupWinSwitchData* switch_data, int checked)
{
  switch_data->animation_start = switch_data->thumb_position;
  switch_data->animation_end = checked ? 1.0 : 0.0;
  switch_data->animation_start_time = GetTickCount();
  switch_data->is_animating = 1;

  if (switch_data->timer_id == 0)
  {
    /* Use a unique timer ID and callback function */
    UINT_PTR timer_id = SetTimer(ih->handle, (UINT_PTR)ih->handle, SWITCH_ANIMATION_INTERVAL, winSwitchTimerProc);
    switch_data->timer_id = timer_id;
  }
  else
  {
    /* Update animation parameters for new click */
    switch_data->animation_start = switch_data->thumb_position;
    switch_data->animation_end = checked ? 1.0 : 0.0;
    switch_data->animation_start_time = GetTickCount();
  }

  /* Trigger immediate repaint for first frame */
  InvalidateRect(ih->handle, NULL, FALSE);
}

static void winSwitchCustomDraw(Ihandle* ih, HDC hDC, RECT* rect, UINT itemState)
{
  IupWinSwitchData* switch_data = (IupWinSwitchData*)iupAttribGet(ih, "_IUPWIN_SWITCHDATA");
  if (!switch_data)
    return;

  WD_HCANVAS canvas = wdCreateCanvasWithHDC(hDC, rect, WD_CANVAS_NOGDICOMPAT);
  if (!canvas)
  {
    return;
  }

  wdBeginPaint(canvas);

  int is_checked = (switch_data->checked_state == BST_CHECKED);
  int is_disabled = (itemState & ODS_DISABLED) != 0;

  /* Get theme colors - interpolate during animation for smooth transition */
  COLORREF track_color_ref, thumb_color_ref;
  COLORREF track_off_color = GetSysColor(COLOR_BTNSHADOW);
  COLORREF track_on_color = GetSysColor(COLOR_HIGHLIGHT);

  if (is_disabled)
  {
    /* Disabled: use consistent gray regardless of checked state */
    track_color_ref = GetSysColor(COLOR_BTNSHADOW);
    thumb_color_ref = GetSysColor(COLOR_BTNFACE);
  }
  else
  {
    /* Interpolate track color based on thumb position for smooth transition */
    float pos = (float)switch_data->thumb_position;
    track_color_ref = RGB(
      (int)(GetRValue(track_off_color) * (1.0f - pos) + GetRValue(track_on_color) * pos),
      (int)(GetGValue(track_off_color) * (1.0f - pos) + GetGValue(track_on_color) * pos),
      (int)(GetBValue(track_off_color) * (1.0f - pos) + GetBValue(track_on_color) * pos)
    );
    thumb_color_ref = GetSysColor(COLOR_WINDOW);
  }

  WD_COLOR track_color = WD_COLOR_FROM_GDI(track_color_ref);
  WD_COLOR thumb_color = WD_COLOR_FROM_GDI(thumb_color_ref);

  WD_HBRUSH track_brush = wdCreateSolidBrush(canvas, track_color);
  WD_HBRUSH thumb_brush = wdCreateSolidBrush(canvas, thumb_color);

  /* Draw track using native Direct2D rounded rectangle (optimized, no path allocation) */
  float radius = SWITCH_TRACK_HEIGHT / 2.0f;
  wdFillRoundedRect(canvas, track_brush, 0.0f, 0.0f, (float)SWITCH_TRACK_WIDTH, (float)SWITCH_TRACK_HEIGHT, radius);

  /* WinUI 3: Thumb expands from 12x12 to 14x14 on hover */
  int thumb_size = switch_data->is_hovering ? 14 : SWITCH_THUMB_SIZE;
  float thumb_x_min = (float)SWITCH_THUMB_MARGIN;
  float thumb_x_max = (float)(SWITCH_TRACK_WIDTH - thumb_size - SWITCH_THUMB_MARGIN);
  float thumb_x = thumb_x_min + (thumb_x_max - thumb_x_min) * (float)switch_data->thumb_position;
  float thumb_y = (SWITCH_TRACK_HEIGHT - thumb_size) / 2.0f;
  float thumb_radius = thumb_size / 2.0f;

  wdFillCircle(canvas, thumb_brush, thumb_x + thumb_radius, thumb_y + thumb_radius, thumb_radius);

  wdDestroyBrush(thumb_brush);
  wdDestroyBrush(track_brush);

  wdEndPaint(canvas);
  wdDestroyCanvas(canvas);
}

void iupdrvToggleAddBorders(Ihandle* ih, int *x, int *y)
{
  iupdrvButtonAddBorders(ih, x, y);
}

void iupdrvToggleAddCheckBox(Ihandle* ih, int *x, int *y, const char* str)
{
  /* LAYOUT_DECORATION_ESTIMATE */
  if (iupAttribGetBoolean(ih, "SWITCH"))
  {
    /* Custom switch dimensions */
    int switch_w = SWITCH_TRACK_WIDTH;
    int switch_h = SWITCH_TRACK_HEIGHT;

    (*x) += 2 + switch_w + 2;  /* margins + width + margins */
    if ((*y) < 2 + switch_h + 8)
      (*y) = 2 + switch_h + 8;  /* margins + height + extra vertical space */
    else
      (*y) += 2 + 8;

    if (str && str[0])  /* add spacing between switch and text */
      (*x) += 8;
  }
  else
  {
    /* Standard checkbox */
    int check_box = 16;
    if (iupwinGetScreenRes() > 120)
      check_box = 26;

    (*x) += check_box;
    if ((*y) < check_box) (*y) = check_box; /* minimum height */

    if (str && str[0]) /* add spacing between check box and text */
    {
      (*x) += 8;

      if (!iupwin_comctl32ver6)
        (*x) += 4;
    }
  }
}

static int winToggleGetCheck(Ihandle* ih)
{
  if (ih->data->type==IUP_TOGGLE_IMAGE && !iupwin_comctl32ver6 && ih->data->flat)
    return iupAttribGet(ih, "_IUPWIN_TOGGLE_CHECK")!=NULL? BST_CHECKED: 0;
  else
    return (int)SendMessage(ih->handle, BM_GETCHECK, 0, 0L);
}

static void winToggleSetCheck(Ihandle* ih, int check)
{
  if (ih->data->type==IUP_TOGGLE_IMAGE && !iupwin_comctl32ver6 && ih->data->flat)
  {
    if (check == BST_CHECKED)
      iupAttribSet(ih, "_IUPWIN_TOGGLE_CHECK", "1");
    else
      iupAttribSet(ih, "_IUPWIN_TOGGLE_CHECK", NULL);

    iupdrvRedrawNow(ih);
  }
  else
    SendMessage(ih->handle, BM_SETCHECK, check, 0L);
}

static int winToggleIsActive(Ihandle* ih)
{
  /* called only when (ih->data->type==IUP_TOGGLE_IMAGE && !iupwin_comctl32ver6 && !ih->data->flat) */
  return iupAttribGetInt(ih, "_IUPWIN_ACTIVE");
}

static void winToggleSetBitmap(Ihandle* ih, const char* name, int make_inactive)
{
  /* called only when (ih->data->type==IUP_TOGGLE_IMAGE && !iupwin_comctl32ver6 && !ih->data->flat) */
  if (name)
  {
    HBITMAP bitmap = iupImageGetImage(name, ih, make_inactive, NULL);
    SendMessage(ih->handle, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)bitmap);
  }
  else
    SendMessage(ih->handle, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)NULL);  /* if not defined */
}

static void winToggleUpdateImage(Ihandle* ih, int active, int check)
{
  /* called only when (ih->data->type==IUP_TOGGLE_IMAGE && !iupwin_comctl32ver6 && !ih->data->flat) */
  char* name;

  if (!active)
  {
    name = iupAttribGet(ih, "IMINACTIVE");
    if (name)
      winToggleSetBitmap(ih, name, 0);
    else
    {
      /* if not defined then automatically create one based on IMAGE */
      name = iupAttribGet(ih, "IMAGE");
      winToggleSetBitmap(ih, name, 1); /* make_inactive */
    }
  }
  else
  {
    /* must restore the normal image */
    if (check)
    {
      name = iupAttribGet(ih, "IMPRESS");
      if (name)
        winToggleSetBitmap(ih, name, 0);
      else
      {
        /* if not defined then automatically create one based on IMAGE */
        name = iupAttribGet(ih, "IMAGE");
        winToggleSetBitmap(ih, name, 0);
      }
    }
    else
    {
      name = iupAttribGet(ih, "IMAGE");
      if (name)
        winToggleSetBitmap(ih, name, 0);
    }
  }
}

static void winToggleGetAlignment(Ihandle* ih, int *horiz_alignment, int *vert_alignment)
{
  char value1[30], value2[30];

  iupStrToStrStr(iupAttribGetStr(ih, "ALIGNMENT"), value1, value2, ':');

  if (iupStrEqualNoCase(value1, "ARIGHT"))
    *horiz_alignment = IUP_ALIGN_ARIGHT;
  else if (iupStrEqualNoCase(value1, "ALEFT"))
    *horiz_alignment = IUP_ALIGN_ALEFT;
  else /* "ACENTER" (default) */
    *horiz_alignment = IUP_ALIGN_ACENTER;

  if (iupStrEqualNoCase(value2, "ABOTTOM"))
    *vert_alignment = IUP_ALIGN_ABOTTOM;
  else if (iupStrEqualNoCase(value2, "ATOP"))
    *vert_alignment = IUP_ALIGN_ATOP;
  else /* "ACENTER" (default) */
    *vert_alignment = IUP_ALIGN_ACENTER;
}

static void winToggleDrawImage(Ihandle* ih, HDC hDC, int rect_width, int rect_height, int border, UINT itemState)
{
  /* called only when (ih->data->type==IUP_TOGGLE_IMAGE && (iupwin_comctl32ver6 || ih->data->flat)) */
  int xpad = ih->data->horiz_padding + border, 
      ypad = ih->data->vert_padding + border;
  int horiz_alignment, vert_alignment;
  int x, y, width, height, bpp;
  HBITMAP hBitmap;
  char *name;
  int make_inactive = 0;

  if (itemState & ODS_DISABLED)
  {
    name = iupAttribGet(ih, "IMINACTIVE");
    if (!name)
    {
      name = iupAttribGet(ih, "IMAGE");
      make_inactive = 1;
    }
  }
  else
  {
    name = iupAttribGet(ih, "IMPRESS");
    if (!(itemState & ODS_SELECTED && name))
      name = iupAttribGet(ih, "IMAGE");
  }

  hBitmap = iupImageGetImage(name, ih, make_inactive, NULL);
  if (!hBitmap)
    return;

  /* must use this info, since image can be a driver image loaded from resources */
  iupdrvImageGetInfo(hBitmap, &width, &height, &bpp);

  winToggleGetAlignment(ih, &horiz_alignment, &vert_alignment);
  if (horiz_alignment == IUP_ALIGN_ARIGHT)
    x = rect_width - (width + 2*xpad);
  else if (horiz_alignment == IUP_ALIGN_ACENTER)
    x = (rect_width - (width + 2*xpad))/2;
  else  /* ALEFT */
    x = 0;

  if (vert_alignment == IUP_ALIGN_ABOTTOM)
    y = rect_height - (height + 2*ypad);
  else if (vert_alignment == IUP_ALIGN_ATOP)
    y = 0;
  else  /* ACENTER */
    y = (rect_height - (height + 2*ypad))/2;

  x += xpad;
  y += ypad;

  if (!iupwin_comctl32ver6 && itemState & ODS_SELECTED)
  {
    if (iupAttribGet(ih, "_IUPWINTOG_PRESSED"))
    {
      x++;
      y++;
      iupAttribSet(ih, "_IUPWINTOG_PRESSED", NULL);
    }
  }

  iupwinDrawBitmap(hDC, hBitmap, x, y, width, height, width, height, bpp);
}

static void winToggleDrawItem(Ihandle* ih, DRAWITEMSTRUCT *drawitem)
{ 
  /* called only when (ih->data->type==IUP_TOGGLE_IMAGE && (iupwin_comctl32ver6 || ih->data->flat)) */
  int width, height, border = 4, check, draw_border;
  HDC hDC;
  iupwinBitmapDC bmpDC;

  width = drawitem->rcItem.right - drawitem->rcItem.left;
  height = drawitem->rcItem.bottom - drawitem->rcItem.top;

  hDC = iupwinDrawCreateBitmapDC(&bmpDC, drawitem->hDC, 0, 0, width, height);

  iupwinDrawParentBackground(ih, hDC, &drawitem->rcItem);

  if (!iupwin_comctl32ver6 && drawitem->itemState & ODS_SELECTED)
    iupAttribSet(ih, "_IUPWINTOG_PRESSED", "1");

  if (iupAttribGet(ih, "_IUPWINTOG_ENTERWIN"))
    drawitem->itemState |= ODS_HOTLIGHT;

  check = winToggleGetCheck(ih);
  if (check)
    drawitem->itemState |= ODS_SELECTED;
  else
    drawitem->itemState |= ODS_DEFAULT;  /* use default mark for NOT checked */

  if (!check && ih->data->flat)
  {
    if (drawitem->itemState & ODS_HOTLIGHT)
      draw_border = 1;
    else
      draw_border = 0;
  }
  else
    draw_border = 1; /* when checked, even if flat the border is drawn */

  if (draw_border)
    iupwinDrawButtonBorder(ih->handle, hDC, &drawitem->rcItem, drawitem->itemState);

  winToggleDrawImage(ih, hDC, width, height, border, drawitem->itemState);

  if (drawitem->itemState & ODS_FOCUS &&
      !(drawitem->itemState & ODS_NOFOCUSRECT) &&
      iupAttribGetBoolean(ih, "CANFOCUS"))
  {
    border--;
    iupwinDrawFocusRect(hDC, border, border, width - 2 * border, height - 2 * border);
  }

  iupwinDrawDestroyBitmapDC(&bmpDC);
}


/***********************************************************************************************/


static int winToggleSetImageAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type==IUP_TOGGLE_IMAGE)
  {
    if (value != iupAttribGet(ih, "IMAGE"))
      iupAttribSet(ih, "IMAGE", (char*)value);

    if (iupwin_comctl32ver6 || ih->data->flat)
      iupdrvRedrawNow(ih);
    else
    {
      int check = (int)SendMessage(ih->handle, BM_GETCHECK, 0L, 0L);
      winToggleUpdateImage(ih, winToggleIsActive(ih), check);
    }
    return 1;
  }
  else
    return 0;
}

static int winToggleSetImInactiveAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type==IUP_TOGGLE_IMAGE)
  {
    if (value != iupAttribGet(ih, "IMINACTIVE"))
      iupAttribSet(ih, "IMINACTIVE", (char*)value);

    if (iupwin_comctl32ver6 || ih->data->flat)
      iupdrvRedrawNow(ih);
    else
    {
      int check = (int)SendMessage(ih->handle, BM_GETCHECK, 0L, 0L);
      winToggleUpdateImage(ih, winToggleIsActive(ih), check);
    }
    return 1;
  }
  else
    return 0;
}

static int winToggleSetImPressAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type==IUP_TOGGLE_IMAGE)
  {
    if (value != iupAttribGet(ih, "IMPRESS"))
      iupAttribSet(ih, "IMPRESS", (char*)value);

    if (iupwin_comctl32ver6 || ih->data->flat)
      iupdrvRedrawNow(ih);
    else
    {
      int check = (int)SendMessage(ih->handle, BM_GETCHECK, 0L, 0L);
      winToggleUpdateImage(ih, winToggleIsActive(ih), check);
    }
    return 1;
  }
  else
    return 0;
}

static int winToggleSetValueAttrib(Ihandle* ih, const char* value)
{
  Ihandle *radio;
  int check;

  if (iupStrEqualNoCase(value,"TOGGLE"))
    check = -1;
  else if (iupStrEqualNoCase(value,"NOTDEF"))
    check = BST_INDETERMINATE;
  else if (iupStrBoolean(value))
    check = BST_CHECKED;
  else
    check = BST_UNCHECKED;

  if (iupAttribGetBoolean(ih, "SWITCH"))
  {
    IupWinSwitchData* switch_data = (IupWinSwitchData*)iupAttribGet(ih, "_IUPWIN_SWITCHDATA");
    int oldcheck = winToggleGetCheck(ih);

    if (check == -1)
    {
      if (oldcheck)
        check = BST_UNCHECKED;
      else
        check = BST_CHECKED;
    }

    winToggleSetCheck(ih, check);

    /* Start animation if value changed */
    if (switch_data && oldcheck != check)
      winSwitchStartAnimation(ih, switch_data, check);

    return 0;
  }

  /* This is necessary because Windows does not handle the radio state
     when a toggle is programmatically changed. */
  radio = iupRadioFindToggleParent(ih);
  if (radio)
  {
    Ihandle* last_tg;

    int oldcheck = winToggleGetCheck(ih);
    if (check == -1)
      check = BST_CHECKED;

    /* After map the first toggle in radio will set value=ON, but last_tg will be NULL */
    last_tg = (Ihandle*)iupAttribGet(radio, "_IUPWIN_LASTTOGGLE");
    if (check)
    {
      if (iupObjectCheck(last_tg) && last_tg != ih)
          winToggleSetCheck(last_tg, BST_UNCHECKED);
      iupAttribSet(radio, "_IUPWIN_LASTTOGGLE", (char*)ih);
    }

    if (last_tg != ih && oldcheck != check)
      winToggleSetCheck(ih, check);
  }
  else
  {
    if (check == -1)
    {
      int oldcheck = winToggleGetCheck(ih);
      if (oldcheck)
        check = BST_UNCHECKED;
      else
        check = BST_CHECKED;
    }

    winToggleSetCheck(ih, check);
  }

  if (ih->data->type==IUP_TOGGLE_IMAGE && !iupwin_comctl32ver6 && !ih->data->flat)
    winToggleUpdateImage(ih, winToggleIsActive(ih), check);

  return 0;
}

static char* winToggleGetValueAttrib(Ihandle* ih)
{
  int check = winToggleGetCheck(ih);
  if (check == BST_INDETERMINATE) check = -1;
  return iupStrReturnChecked(check);
}

static int winToggleSetActiveAttrib(Ihandle* ih, const char* value)
{
  /* update the inactive image if necessary */
  if (ih->data->type==IUP_TOGGLE_IMAGE)
  {
    if (iupwin_comctl32ver6 || ih->data->flat)
    {
      iupBaseSetActiveAttrib(ih, value);
      iupdrvRedrawNow(ih);
      return 0;
    }
    else
    {
      int active = iupStrBoolean(value);
      int check = (int)SendMessage(ih->handle, BM_GETCHECK, 0, 0L);
      if (active)
        iupAttribSet(ih, "_IUPWIN_ACTIVE", "YES");
      else
        iupAttribSet(ih, "_IUPWIN_ACTIVE", "NO");
      winToggleUpdateImage(ih, active, check);
      return 0;
    }
  }

  return iupBaseSetActiveAttrib(ih, value);
}

static char* winToggleGetActiveAttrib(Ihandle* ih)
{
  if (ih->data->type==IUP_TOGGLE_IMAGE && !iupwin_comctl32ver6 && !ih->data->flat)
    return iupAttribGet(ih, "_IUPWIN_ACTIVE");
  else
    return iupBaseGetActiveAttrib(ih);
}

static int winToggleSetTitleAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_TOGGLE_TEXT)
  {
    iupwinSetMnemonicTitle(ih, 0, value);
    return iupwinSetTitleAttrib(ih, value);
  }
  return 0;
}

static int winToggleSetPaddingAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqual(value, "DEFAULTBUTTONPADDING"))
    value = IupGetGlobal("DEFAULTBUTTONPADDING");

  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');

  if (ih->handle && iupwin_comctl32ver6 && ih->data->type==IUP_TOGGLE_IMAGE)
    iupdrvRedrawNow(ih);

  return 0;
}

static int winTogglePostRedrawSetAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  if (ih->handle)
    iupdrvPostRedraw(ih);  /* Post a redraw */
  return 1;
}

static char* winToggleGetBgColorAttrib(Ihandle* ih)
{
  /* the most important use of this is to provide
     the correct background for images */
  if (iupwin_comctl32ver6 && ih->data->type==IUP_TOGGLE_IMAGE)
  {
    COLORREF cr;
    if (iupwinDrawGetThemeButtonBgColor(ih->handle, &cr))
      return iupStrReturnStrf("%d %d %d", (int)GetRValue(cr), (int)GetGValue(cr), (int)GetBValue(cr));
  }

  if (ih->data->type == IUP_TOGGLE_TEXT)
    return iupBaseNativeParentGetBgColorAttrib(ih);
  else
    return IupGetGlobal("DLGBGCOLOR");
}


/****************************************************************************************/


static int winToggleCtlColor(Ihandle* ih, HDC hdc, LRESULT *result)
{
  COLORREF cr;

  SetBkMode(hdc, TRANSPARENT);

  if (iupwinGetColorRef(ih, "FGCOLOR", &cr))
    SetTextColor(hdc, cr);

  if (iupwinGetParentBgColor(ih, &cr))
  {
    SetDCBrushColor(hdc, cr);
    *result = (LRESULT)GetStockObject(DC_BRUSH);
    return 1;
  }
  return 0;
}

static int winToggleImageWmNotify(Ihandle* ih, NMHDR* msg_info, int *result)
{
  /* called only when (ih->data->type==IUP_TOGGLE_IMAGE && iupwin_comctl32ver6) */

  if (msg_info->code == NM_CUSTOMDRAW)
    return iupwinCustomDrawToDrawItem(ih, msg_info, result, (IFdrawItem)winToggleDrawItem);

  return 0; /* result not used */
}

static int winToggleImageFlatMsgProc(Ihandle* ih, UINT msg, WPARAM wp, LPARAM lp, LRESULT *result)
{
  /* Called only when (ih->data->type==IUP_TOGGLE_IMAGE && ih->data->flat) */

  switch (msg)
  {
  case WM_MOUSELEAVE:
    iupAttribSet(ih, "_IUPWINTOG_ENTERWIN", NULL);
    iupdrvRedrawNow(ih);
    break;
  case WM_MOUSEMOVE:
    if (!iupAttribGet(ih, "_IUPWINTOG_ENTERWIN"))
    {
      /* this will not affect the process in iupwinBaseMsgProc*/

      /* must be called so WM_MOUSELEAVE will be called */
      iupwinTrackMouseLeave(ih);

      iupAttribSet(ih, "_IUPWINTOG_ENTERWIN", "1");

      iupdrvRedrawNow(ih);
    }
    break;
  }

  return iupwinBaseMsgProc(ih, msg, wp, lp, result);
}

static int winToggleImageClassicMsgProc(Ihandle* ih, UINT msg, WPARAM wp, LPARAM lp, LRESULT *result)
{
  /* Called only when (ih->data->type==IUP_TOGGLE_IMAGE && !iupwin_comctl32ver6 && !ih->data->flat) */

  switch (msg)
  {
  case WM_MOUSEACTIVATE:
    if (!winToggleIsActive(ih))
    {
      *result = MA_NOACTIVATEANDEAT;
      return 1;
    }
    break;
  case WM_LBUTTONDOWN:
  case WM_LBUTTONUP:
  case WM_ACTIVATE:
  case WM_SETFOCUS:
    if (!winToggleIsActive(ih)) 
    {
      *result = 0;
      return 1;
    }
    break;
  }

  if (msg == WM_LBUTTONDOWN)
    winToggleUpdateImage(ih, 1, 1);
  else if (msg == WM_LBUTTONUP)
    winToggleUpdateImage(ih, 1, 0);

  return iupwinBaseMsgProc(ih, msg, wp, lp, result);
}

static int winToggleSwitchMsgProc(Ihandle* ih, UINT msg, WPARAM wp, LPARAM lp, LRESULT *result)
{
  IupWinSwitchData* switch_data = (IupWinSwitchData*)iupAttribGet(ih, "_IUPWIN_SWITCHDATA");

  switch (msg)
  {
  case WM_MOUSEMOVE:
    /* Track mouse hover for thumb expansion */
    if (switch_data && !switch_data->is_hovering)
    {
      /* Enable mouse leave tracking */
      iupwinTrackMouseLeave(ih);

      switch_data->is_hovering = 1;
      InvalidateRect(ih->handle, NULL, FALSE);
    }
    break;
  case WM_MOUSELEAVE:
    /* Remove hover state */
    if (switch_data && switch_data->is_hovering)
    {
      switch_data->is_hovering = 0;
      InvalidateRect(ih->handle, NULL, FALSE);
    }
    break;
  case WM_LBUTTONDOWN:
    /* Capture mouse to ensure we get WM_LBUTTONUP */
    SetCapture(ih->handle);
    *result = 0;
    return 0;
  case WM_LBUTTONUP:
    /* Release mouse capture */
    ReleaseCapture();

    {
      IFni cb;
      int check = switch_data->checked_state;
      int new_check = (check == BST_CHECKED) ? BST_UNCHECKED : BST_CHECKED;

      /* Store the new checked state */
      switch_data->checked_state = new_check;

      /* Update the underlying button state (but this doesn't affect custom drawing) */
      SendMessage(ih->handle, BM_SETCHECK, new_check, 0L);

      /* Start animation (this triggers InvalidateRect internally) */
      if (switch_data)
        winSwitchStartAnimation(ih, switch_data, new_check);

      cb = (IFni)IupGetCallback(ih, "ACTION");
      if (cb && cb(ih, new_check) == IUP_CLOSE)
        IupExitLoop();

      if (iupObjectCheck(ih))
        iupBaseCallValueChangedCb(ih);
    }
    *result = 0;
    return 1;
  case WM_THEMECHANGED:
  case WM_SYSCOLORCHANGE:
    /* Redraw on theme change */
    InvalidateRect(ih->handle, NULL, TRUE);
    break;
  }

  return iupwinBaseMsgProc(ih, msg, wp, lp, result);
}

static void winToggleSwitchDrawItem(Ihandle* ih, DRAWITEMSTRUCT* dis)
{
  IupWinSwitchData* switch_data = (IupWinSwitchData*)iupAttribGet(ih, "_IUPWIN_SWITCHDATA");

  /* Only clear background when not animating to prevent flashing */
  /* During animation or rapid state changes, the opaque switch rendering handles everything */
  if (!switch_data || !switch_data->is_animating)
    iupwinDrawParentBackground(ih, dis->hDC, &dis->rcItem);

  /* Draw the switch control */
  winSwitchCustomDraw(ih, dis->hDC, &dis->rcItem, dis->itemState);
}

static int winToggleWmCommand(Ihandle* ih, WPARAM wp, LPARAM lp)
{
  (void)lp;

  switch (HIWORD(wp))
  {
  case BN_DOUBLECLICKED:
    if (iupAttribGetBoolean(ih, "IGNOREDOUBLECLICK"))
      return 0;
    /* continue */
  case BN_CLICKED:
    {
      Ihandle *radio;
      IFni cb;
      int check = winToggleGetCheck(ih);

      if (iupAttribGetBoolean(ih, "SWITCH"))
      {
        IupWinSwitchData* switch_data = (IupWinSwitchData*)iupAttribGet(ih, "_IUPWIN_SWITCHDATA");

        /* Manually toggle the state (BS_OWNERDRAW doesn't auto-toggle) */
        int new_check = (check == BST_CHECKED) ? BST_UNCHECKED : BST_CHECKED;
        winToggleSetCheck(ih, new_check);

        /* Start animation */
        if (switch_data)
          winSwitchStartAnimation(ih, switch_data, new_check);

        cb = (IFni)IupGetCallback(ih, "ACTION");
        if (cb && cb(ih, new_check) == IUP_CLOSE)
          IupExitLoop();

        if (iupObjectCheck(ih))
          iupBaseCallValueChangedCb(ih);

        /* Redraw immediately */
        InvalidateRect(ih->handle, NULL, FALSE);

        return 0;
      }

      if (ih->data->type==IUP_TOGGLE_IMAGE && !iupwin_comctl32ver6 && !ih->data->flat)
      {
        int active = winToggleIsActive(ih);
        winToggleUpdateImage(ih, active, check);
        if (!active)
          return 0;
      }

      radio = iupRadioFindToggleParent(ih);
      if (radio)
      {
        Ihandle* last_tg = (Ihandle*)iupAttribGet(radio, "_IUPWIN_LASTTOGGLE");
        if (iupObjectCheck(last_tg) && last_tg != ih)
        {
          /* uncheck last toggle */
          winToggleSetCheck(last_tg, BST_UNCHECKED);

          cb = (IFni) IupGetCallback(last_tg, "ACTION");
          if (cb && cb(last_tg, 0) == IUP_CLOSE)
              IupExitLoop();

          if (iupObjectCheck(last_tg))
            iupBaseCallValueChangedCb(last_tg);
        }
        iupAttribSet(radio, "_IUPWIN_LASTTOGGLE", (char*)ih);

        if (last_tg != ih)
        {
          /* check new toggle */
          winToggleSetCheck(ih, BST_CHECKED);

          cb = (IFni)IupGetCallback(ih, "ACTION");
          if (cb && cb (ih, 1) == IUP_CLOSE)
              IupExitLoop();

          if (iupObjectCheck(ih))
            iupBaseCallValueChangedCb(ih);
        }
      }
      else
      {
        if ((ih->data->type==IUP_TOGGLE_IMAGE && !iupwin_comctl32ver6 && ih->data->flat) ||
            (HIWORD(wp)==BN_DOUBLECLICKED))
        {
          /* toggle the value manually */
          check = check? 0: BST_CHECKED;
          winToggleSetCheck(ih, check);
        }

        if (check == BST_INDETERMINATE)
          check = -1;

        cb = (IFni)IupGetCallback(ih, "ACTION");
        if (cb && cb (ih, check) == IUP_CLOSE)
            IupExitLoop();

        if (iupObjectCheck(ih))
          iupBaseCallValueChangedCb(ih);
      }
    }
  }

  return 0; /* not used */
}

static void winToggleUnMapMethod(Ihandle* ih)
{
  if (iupAttribGetBoolean(ih, "SWITCH"))
  {
    IupWinSwitchData* switch_data = (IupWinSwitchData*)iupAttribGet(ih, "_IUPWIN_SWITCHDATA");
    if (switch_data)
    {
      if (switch_data->timer_id)
        KillTimer(ih->handle, switch_data->timer_id);

      free(switch_data);
      iupAttribSet(ih, "_IUPWIN_SWITCHDATA", NULL);
    }
  }

  iupdrvBaseUnMapMethod(ih);
}

static int winToggleMapMethod(Ihandle* ih)
{
  Ihandle* radio = iupRadioFindToggleParent(ih);
  char* value;
  int ownerdraw = 0;
  DWORD dwStyle = WS_CHILD | WS_CLIPSIBLINGS |
                  BS_NOTIFY; /* necessary because of the base messages */

  if (!ih->parent)
    return IUP_ERROR;

  if (radio)
    ih->data->is_radio = 1;

  if (iupAttribGetBoolean(ih, "SWITCH"))
  {
    IupWinSwitchData* switch_data;

    /* SWITCH must be a checkbox (not radio) */
    if (ih->data->is_radio)
    {
      iupAttribSet(ih, "SWITCH", "NO");
      goto regular_toggle;
    }

    ih->data->type = IUP_TOGGLE_TEXT;
    dwStyle |= BS_OWNERDRAW | BS_CHECKBOX;
    ownerdraw = 1;

    /* Add tabstop if can focus */
    if (iupAttribGetBoolean(ih, "CANFOCUS"))
      dwStyle |= WS_TABSTOP;

    /* Allocate switch data */
    switch_data = (IupWinSwitchData*)malloc(sizeof(IupWinSwitchData));
    memset(switch_data, 0, sizeof(IupWinSwitchData));
    iupAttribSet(ih, "_IUPWIN_SWITCHDATA", (char*)switch_data);

    /* Create the window */
    if (!iupwinCreateWindow(ih, WC_BUTTON, 0, dwStyle, NULL))
    {
      free(switch_data);
      return IUP_ERROR;
    }

    /* Set up switch-specific callbacks */
    IupSetCallback(ih, "_IUPWIN_COMMAND_CB", (Icallback)winToggleWmCommand);
    IupSetCallback(ih, "_IUPWIN_CTLCOLOR_CB", (Icallback)winToggleCtlColor);
    IupSetCallback(ih, "_IUPWIN_DRAWITEM_CB", (Icallback)winToggleSwitchDrawItem);
    IupSetCallback(ih, "_IUPWIN_CTRLMSGPROC_CB", (Icallback)winToggleSwitchMsgProc);

    /* Initialize thumb position and checked state based on initial value */
    value = iupAttribGet(ih, "VALUE");
    if (value && (iupStrEqualNoCase(value, "ON") || iupStrEqualNoCase(value, "YES") || iupStrEqual(value, "1")))
    {
      switch_data->thumb_position = 1.0;
      switch_data->checked_state = BST_CHECKED;
      winToggleSetCheck(ih, BST_CHECKED);
    }
    else
    {
      switch_data->thumb_position = 0.0;
      switch_data->checked_state = BST_UNCHECKED;
      winToggleSetCheck(ih, BST_UNCHECKED);
    }

    /* Force initial paint */
    InvalidateRect(ih->handle, NULL, TRUE);

    return IUP_NOERROR;
  }

regular_toggle:
  value = iupAttribGet(ih, "IMAGE");
  if (value)
  {
    ih->data->type = IUP_TOGGLE_IMAGE;
    if (!iupwin_comctl32ver6 && ih->data->flat)
    {
      dwStyle |= BS_OWNERDRAW;
      ownerdraw = 1;
    }
    else
      dwStyle |= BS_BITMAP|BS_PUSHLIKE;
  }
  else
  {
    ih->data->type = IUP_TOGGLE_TEXT;
    dwStyle |= BS_TEXT|BS_MULTILINE;

    if (iupAttribGetBoolean(ih, "RIGHTBUTTON"))
      dwStyle |= BS_RIGHTBUTTON;
  }

  if (iupAttribGetBoolean(ih, "CANFOCUS"))
    dwStyle |= WS_TABSTOP;

  if (ih->data->is_radio)
  {
    if (!ownerdraw)
      dwStyle |= BS_RADIOBUTTON;

    if (!iupAttribGet(radio, "_IUPWIN_LASTTOGGLE"))
    {
      /* this is the first toggle in the radio, and then set it with VALUE=ON */
      /* After map the attribute value will be processed and _IUPWIN_LASTTOGGLE will be set */
      iupAttribSet(ih, "VALUE","ON");
    }

    /* make sure it has at least one name */
    if (!iupAttribGetHandleName(ih))
      iupAttribSetHandleName(ih);
  }
  else if (!ownerdraw)
  {
    if (ih->data->type == IUP_TOGGLE_TEXT && iupAttribGetBoolean(ih, "3STATE"))
      dwStyle |= BS_AUTO3STATE;
    else
      dwStyle |= BS_AUTOCHECKBOX;
  }

  if (!iupwinCreateWindow(ih, WC_BUTTON, 0, dwStyle, NULL))
    return IUP_ERROR;

  /* Process WM_COMMAND */
  IupSetCallback(ih, "_IUPWIN_COMMAND_CB", (Icallback)winToggleWmCommand);

  /* Process background color */
  IupSetCallback(ih, "_IUPWIN_CTLCOLOR_CB", (Icallback)winToggleCtlColor);

  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    if (iupwin_comctl32ver6)
    {
      IupSetCallback(ih, "_IUPWIN_NOTIFY_CB", (Icallback)winToggleImageWmNotify);  /* Process WM_NOTIFY */
      if (ih->data->flat)
        IupSetCallback(ih, "_IUPWIN_CTRLMSGPROC_CB", (Icallback)winToggleImageFlatMsgProc);
    }
    else
    {
      if (ih->data->flat)
      {
        iupAttribSet(ih, "FLAT_ALPHA", "NO");
        IupSetCallback(ih, "_IUPWIN_DRAWITEM_CB", (Icallback)winToggleDrawItem);  /* Process WM_DRAWITEM */
        IupSetCallback(ih, "_IUPWIN_CTRLMSGPROC_CB", (Icallback)winToggleImageFlatMsgProc);
      }
      else
      {
        IupSetCallback(ih, "_IUPWIN_CTRLMSGPROC_CB", (Icallback)winToggleImageClassicMsgProc);
        iupAttribSet(ih, "_IUPWIN_ACTIVE", "YES");
      }
    }
  }

  return IUP_NOERROR;
}

void iupdrvToggleInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = winToggleMapMethod;
  ic->UnMap = winToggleUnMapMethod;

  /* Driver Dependent Attribute functions */

  /* Overwrite Visual */
  iupClassRegisterAttribute(ic, "ACTIVE", winToggleGetActiveAttrib, winToggleSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);

  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", winToggleGetBgColorAttrib, winTogglePostRedrawSetAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_NO_SAVE);

  /* Special */
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, winTogglePostRedrawSetAttrib, "DLGFGCOLOR", NULL, IUPAF_NOT_MAPPED);  /* force the new default value */  
  iupClassRegisterAttribute(ic, "TITLE", NULL, winToggleSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  /* IupToggle only */
  iupClassRegisterAttribute(ic, "ALIGNMENT", NULL, winTogglePostRedrawSetAttrib, IUPAF_SAMEASSYSTEM, "ACENTER:ACENTER", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, winToggleSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMINACTIVE", NULL, winToggleSetImInactiveAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMPRESS", NULL, winToggleSetImPressAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUE", winToggleGetValueAttrib, winToggleSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PADDING", iupToggleGetPaddingAttrib, winToggleSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);

  /* IupToggle Windows only */
  iupClassRegisterAttribute(ic, "RIGHTBUTTON", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  /* necessary because it uses an old HBITMAP solution when NOT using styles */
  if (!iupwin_comctl32ver6)  /* Used by iupdrvImageCreateImage */
    iupClassRegisterAttribute(ic, "FLAT_ALPHA", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  /* NOT supported */
  iupClassRegisterAttribute(ic, "MARKUP", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED);

  iupClassRegisterAttribute(ic, "CONTROLID", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
}
