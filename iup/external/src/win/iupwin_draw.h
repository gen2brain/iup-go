/** \file
 * \brief Draw Functions
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUPWIN_DRAW_H
#define __IUPWIN_DRAW_H

#ifdef __cplusplus
extern "C" {
#endif


IUP_DRV_API void iupwinDrawInit(void);
IUP_DRV_API void iupwinDrawFinish(void);

IUP_DRV_API void iupwinDrawBitmap(HDC hDC, HBITMAP hBitmap, int x, int y, int w, int h, int img_w, int img_h, int bpp);
IUP_DRV_API void iupwinDrawText(HDC hDC, const char* text, int x, int y, int width, int height, HFONT hFont, COLORREF fgcolor, int style);

IUP_DRV_API void iupwinDrawParentBackground(Ihandle* ih, HDC hDC, RECT* rect);
IUP_DRV_API void iupwinDrawButtonBorder(HWND hWnd, HDC hDC, RECT *rect, UINT itemState);
IUP_DRV_API void iupwinDraw3StateButton(HWND hWnd, HDC hDC, RECT *rect);

IUP_DRV_API void iupwinDrawThemeInit(void);
IUP_DRV_API void iupwinDrawThemeFrameBorder(HWND hWnd, HDC hDC, RECT *rect, UINT itemState);
IUP_DRV_API int  iupwinDrawGetThemeTabsBgColor(HWND hWnd, COLORREF *color);
IUP_DRV_API int  iupwinDrawGetThemeButtonBgColor(HWND hWnd, COLORREF *color);
IUP_DRV_API int  iupwinDrawGetThemeFrameFgColor(HWND hWnd, COLORREF *color);
IUP_DRV_API void iupwinDrawRemoveTheme(HWND hWnd);

typedef struct _iupwinBitmapDC
{
  HBITMAP hBitmap, hOldBitmap;
  HDC hBitmapDC, hDC;
  int x, y, w, h;
} iupwinBitmapDC;

IUP_DRV_API HDC iupwinDrawCreateBitmapDC(iupwinBitmapDC *bmpDC, HDC hDC, int x, int y, int w, int h);
IUP_DRV_API void iupwinDrawDestroyBitmapDC(iupwinBitmapDC *bmpDC);

IUP_DRV_API int iupwinCustomDrawToDrawItem(Ihandle* ih, NMHDR* msg_info, int *result, IFdrawItem drawitem_cb);

/* Not defined for MingW and Cygwin */
#ifndef ODS_HOTLIGHT
#define ODS_HOTLIGHT        0x0040
#endif
#ifndef ODS_NOACCEL
#define ODS_NOACCEL   0x0100
#endif
#ifndef DT_HIDEPREFIX
#define DT_HIDEPREFIX   0x00100000
#endif
#ifndef ODS_NOFOCUSRECT
#define ODS_NOFOCUSRECT   0x0200
#endif
#ifndef CDIS_SHOWKEYBOARDCUES
#define CDIS_SHOWKEYBOARDCUES   0x0200
#endif


#ifdef __cplusplus
}
#endif

#endif
