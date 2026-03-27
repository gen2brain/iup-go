/** \file
 * \brief GTK4 Driver
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUPGTK4_DRV_H
#define __IUPGTK4_DRV_H

#ifdef __cplusplus
extern "C" {
#endif

#define iupgtk4ColorFromDouble(_x) ((unsigned char)(_x*255))
#define iupgtk4ColorToDouble(_x) ((double)_x/255.0)

/* Common */
IUP_DRV_API void iupgtk4SetupEnterLeaveEvents(GtkWidget *widget, Ihandle* ih);
IUP_DRV_API void iupgtk4SetupMotionEvents(GtkWidget *widget, Ihandle *ih);
IUP_DRV_API void iupgtk4SetupButtonEvents(GtkWidget *widget, Ihandle *ih);
IUP_DRV_API void iupgtk4ButtonPressed(GtkGestureClick* gesture, int n_press, double x, double y, Ihandle* ih);

IUP_DRV_API int iupgtk4SetMnemonicTitle(Ihandle* ih, GtkLabel* label, const char* value);
IUP_DRV_API void iupgtk4UpdateMnemonic(Ihandle* ih);

IUP_DRV_API void iupgtk4ColorSetRGB(GdkRGBA* color, unsigned char r, unsigned char g, unsigned char b);
IUP_DRV_API void iupgtk4SetBgColor(InativeHandle* handle, unsigned char r, unsigned char g, unsigned char b);
IUP_DRV_API void iupgtk4SetFgColor(InativeHandle* handle, unsigned char r, unsigned char g, unsigned char b);

IUP_DRV_API void iupgtk4AddToParent(Ihandle* ih);
IUP_DRV_API const char* iupgtk4GetWidgetClassName(GtkWidget* widget);
IUP_DRV_API void iupgtk4SetPosSize(GtkWidget* parent, GtkWidget* widget, int x, int y, int width, int height);
IUP_DRV_API GdkSurface* iupgtk4GetSurface(GtkWidget *widget);
IUP_DRV_API void iupgtk4SurfaceGetPointer(GdkSurface *surface, double *x, double *y, GdkModifierType *mask);
IUP_DRV_API int iupgtk4IsVisible(GtkWidget* widget);
IUP_DRV_API void iupgtk4ClearSizeStyleCSS(GtkWidget* widget);
IUP_DRV_API void iupgtk4SetMargin(GtkWidget* widget, int horiz_padding, int vert_padding);

/* Custom iupGtk4Fixed widget */
typedef struct _iupGtk4Fixed iupGtk4Fixed;

IUP_DRV_API GtkWidget* iupgtk4NativeContainerNew(void);
IUP_DRV_API void iupgtk4NativeContainerSetIhandle(GtkWidget* container, Ihandle* ih);
IUP_DRV_API void iupgtk4NativeContainerAdd(GtkWidget* container, GtkWidget* widget);
IUP_DRV_API void iupgtk4NativeContainerMove(GtkWidget* container, GtkWidget* widget, int x, int y);
IUP_DRV_API void iupgtk4NativeContainerSetBounds(GtkWidget* container, GtkWidget* widget, int x, int y, int width, int height);
IUP_DRV_API void iupgtk4FixedMove(iupGtk4Fixed* fixed, GtkWidget* widget, int x, int y);
IUP_DRV_API void iupgtk4NativeContainerSetBorder(GtkWidget* container, int enable);

/* Str */
IUP_DRV_API void  iupgtk4StrRelease(void);
IUP_DRV_API char* iupgtk4StrConvertToSystem(const char* str);
IUP_DRV_API char* iupgtk4StrConvertToSystemLen(const char* str, int *len);
IUP_DRV_API char* iupgtk4StrConvertFromSystem(const char* str);
IUP_DRV_API char* iupgtk4StrConvertFromFilename(const char* str);
IUP_DRV_API char* iupgtk4StrConvertToFilename(const char* str);

/* Focus */
IUP_DRV_API void iupgtk4SetupFocusEvents(GtkWidget *widget, Ihandle* ih);
IUP_DRV_API void iupgtk4SetCanFocus(GtkWidget *widget, int can);

/* Key */
IUP_DRV_API gboolean iupgtk4KeyPressEvent(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, Ihandle *ih);
IUP_DRV_API gboolean iupgtk4KeyReleaseEvent(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, Ihandle *ih);
IUP_DRV_API void iupgtk4SetupKeyEvents(GtkWidget *widget, Ihandle* ih);
IUP_DRV_API void iupgtk4ButtonKeySetStatus(GdkModifierType state, unsigned int but, char* status, int doubleclick);
IUP_DRV_API int iupgtk4KeyDecode(guint keyval, GdkModifierType state);

/* Font */
IUP_DRV_API PangoFontDescription* iupgtk4GetPangoFontDesc(const char* value);
IUP_DRV_API char* iupgtk4GetPangoFontDescAttrib(Ihandle *ih);
IUP_DRV_API char* iupgtk4GetPangoLayoutAttrib(Ihandle *ih);
IUP_DRV_API char* iupgtk4GetFontIdAttrib(Ihandle *ih);
IUP_DRV_API void iupgtk4UpdateWidgetFont(Ihandle *ih, GtkWidget* widget);
IUP_DRV_API PangoLayout* iupgtk4GetPangoLayout(const char* value);

/* There are PANGO_SCALE Pango units in one device unit.
   For an output backend where a device unit is a pixel, a size value of 10 * PANGO_SCALE gives 10 pixels. */
#define iupGTK4_PANGOUNITS2PIXELS(_x) (((_x) + PANGO_SCALE/2) / PANGO_SCALE)
#define iupGTK4_PIXELS2PANGOUNITS(_x) ((_x) * PANGO_SCALE)

/* Open */
IUP_DRV_API char* iupgtk4GetNativeWidgetHandle(GtkWidget *widget);
IUP_DRV_API char* iupgtk4GetNativeWindowHandleAttrib(Ihandle* ih);
IUP_DRV_API const char* iupgtk4GetNativeWindowHandleName(void);
IUP_DRV_API const char* iupgtk4GetNativeFontIdName(void);

/* Dialog */
IUP_DRV_API gboolean iupgtk4DialogCloseRequest(GtkWindow *window, Ihandle *ih);
IUP_DRV_API GtkWindow* iupgtk4GetTransientFor(Ihandle* ih);

/* Button */
IUP_DRV_API void iupgtk4ButtonApplyImagePadding(GtkWidget* widget);

/* Menu */
IUP_DRV_API void iupgtk4DialogSetMenuBar(Ihandle* ih_dialog, Ihandle* ih_menu);

/* Tray */
IUP_DRV_API int iupgtk4SetTrayAttrib(Ihandle *ih, const char *value);
IUP_DRV_API int iupgtk4SetTrayTipAttrib(Ihandle *ih, const char *value);
IUP_DRV_API int iupgtk4SetTrayImageAttrib(Ihandle *ih, const char *value);
IUP_DRV_API int iupgtk4SetTrayMenuAttrib(Ihandle *ih, const char *value);
IUP_DRV_API int iupgtk4TrayCleanup(Ihandle *ih);

/* Loop */
IUP_DRV_API void iupgtk4LoopCleanup(void);

/* CSS Manager */
IUP_DRV_API void iupgtk4CssManagerInit(void);
IUP_DRV_API void iupgtk4CssManagerFinish(void);
IUP_DRV_API void iupgtk4CssSetWidgetBgColor(GtkWidget* widget, unsigned char r, unsigned char g, unsigned char b, int is_text);
IUP_DRV_API void iupgtk4CssSetWidgetFgColor(GtkWidget* widget, unsigned char r, unsigned char g, unsigned char b);
IUP_DRV_API void iupgtk4CssSetWidgetPadding(GtkWidget* widget, int horiz, int vert);
IUP_DRV_API void iupgtk4CssSetWidgetFont(GtkWidget* widget, const char* font_css);
IUP_DRV_API void iupgtk4CssSetWidgetCustom(GtkWidget* widget, const char* css_property, const char* css_value);
IUP_DRV_API void iupgtk4CssClearWidgetStyle(GtkWidget* widget);
IUP_DRV_API void iupgtk4CssResetWidgetPadding(GtkWidget* widget);
IUP_DRV_API void iupgtk4CssResetWidgetCustom(GtkWidget* widget);
IUP_DRV_API void iupgtk4CssAddStaticRule(const char* selector, const char* css_rules);
IUP_DRV_API void iupgtk4CssFlush(void);

IUP_DRV_API int iupgtk4IsSystemDarkMode(void);
IUP_DRV_API void iupgtk4SetGlobalColors(void);

#ifdef GDK_WINDOWING_X11
IUP_DRV_API int iupgtk4X11MoveWindow(void* xdisplay, unsigned long xwindow, int x, int y);
IUP_DRV_API int iupgtk4X11HideFromTaskbar(void* xdisplay, unsigned long xwindow);
IUP_DRV_API int iupgtk4X11Sync(void* xdisplay);
IUP_DRV_API int iupgtk4X11QueryPointer(void* xdisplay, int* x, int* y);
IUP_DRV_API int iupgtk4X11GetWindowPosition(void* xdisplay, unsigned long xwindow, int* x, int* y);
IUP_DRV_API int iupgtk4X11GetDefaultScreen(void* xdisplay);
IUP_DRV_API char* iupgtk4X11GetServerVendor(void* xdisplay);
IUP_DRV_API int iupgtk4X11GetVendorRelease(void* xdisplay);
IUP_DRV_API int iupgtk4X11WarpPointer(void* xdisplay, int x, int y);
IUP_DRV_API void iupgtk4X11Cleanup(void);
#endif

#ifdef GDK_WINDOWING_WIN32
IUP_DRV_API int iupgtk4Win32MoveWindow(void* hwnd, int x, int y);
IUP_DRV_API int iupgtk4Win32HideFromTaskbar(void* hwnd);
#endif

#ifdef GDK_WINDOWING_MACOS
IUP_DRV_API int iupgtk4MacosMoveWindow(void* nswindow, int x, int y);
IUP_DRV_API int iupgtk4MacosHideFromTaskbar(void* nswindow);
#endif

#ifdef __cplusplus
}
#endif

#endif
