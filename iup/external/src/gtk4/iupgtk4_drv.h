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
void iupgtk4SetupEnterLeaveEvents(GtkWidget *widget, Ihandle* ih);
void iupgtk4SetupMotionEvents(GtkWidget *widget, Ihandle *ih);
void iupgtk4SetupButtonEvents(GtkWidget *widget, Ihandle *ih);
IUP_DRV_API void iupgtk4ButtonPressed(GtkGestureClick* gesture, int n_press, double x, double y, Ihandle* ih);
IUP_DRV_API gboolean iupgtk4ShowHelp(GtkWidget *widget, gpointer arg1, Ihandle* ih);

int iupgtk4SetMnemonicTitle(Ihandle* ih, GtkLabel* label, const char* value);
void iupgtk4UpdateMnemonic(Ihandle* ih);

void iupgtk4ColorSetRGB(GdkRGBA* color, unsigned char r, unsigned char g, unsigned char b);
void iupgtk4SetBgColor(InativeHandle* handle, unsigned char r, unsigned char g, unsigned char b);
void iupgtk4SetFgColor(InativeHandle* handle, unsigned char r, unsigned char g, unsigned char b);

IUP_DRV_API void iupgtk4AddToParent(Ihandle* ih);
const char* iupgtk4GetWidgetClassName(GtkWidget* widget);
void iupgtk4SetPosSize(GtkWidget* parent, GtkWidget* widget, int x, int y, int width, int height);
GdkSurface* iupgtk4GetSurface(GtkWidget *widget);
void iupgtk4SurfaceGetPointer(GdkSurface *surface, double *x, double *y, GdkModifierType *mask);
int iupgtk4IsVisible(GtkWidget* widget);
void iupgtk4ClearSizeStyleCSS(GtkWidget* widget);
void iupgtk4SetMargin(GtkWidget* widget, int horiz_padding, int vert_padding);

/* Custom iupGtk4Fixed widget */
typedef struct _iupGtk4Fixed iupGtk4Fixed;

GtkWidget* iupgtk4NativeContainerNew(void);
void iupgtk4NativeContainerSetIhandle(GtkWidget* container, Ihandle* ih);
void iupgtk4NativeContainerAdd(GtkWidget* container, GtkWidget* widget);
void iupgtk4NativeContainerMove(GtkWidget* container, GtkWidget* widget, int x, int y);
void iupgtk4NativeContainerSetBounds(GtkWidget* container, GtkWidget* widget, int x, int y, int width, int height);
void iupgtk4FixedMove(iupGtk4Fixed* fixed, GtkWidget* widget, int x, int y);
void iupgtk4NativeContainerSetBorder(GtkWidget* container, int enable);

/* Str */
void  iupgtk4StrRelease(void);
IUP_DRV_API char* iupgtk4StrConvertToSystem(const char* str);
char* iupgtk4StrConvertToSystemLen(const char* str, int *len);
char* iupgtk4StrConvertFromSystem(const char* str);
char* iupgtk4StrConvertFromFilename(const char* str);
char* iupgtk4StrConvertToFilename(const char* str);

/* Focus */
void iupgtk4SetupFocusEvents(GtkWidget *widget, Ihandle* ih);
IUP_DRV_API void iupgtk4SetCanFocus(GtkWidget *widget, int can);

/* Key */
IUP_DRV_API gboolean iupgtk4KeyPressEvent(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, Ihandle *ih);
gboolean iupgtk4KeyReleaseEvent(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, Ihandle *ih);
void iupgtk4SetupKeyEvents(GtkWidget *widget, Ihandle* ih);
void iupgtk4ButtonKeySetStatus(GdkModifierType state, unsigned int but, char* status, int doubleclick);
int iupgtk4KeyDecode(guint keyval, GdkModifierType state);

/* Font */
PangoFontDescription* iupgtk4GetPangoFontDesc(const char* value);
char* iupgtk4GetPangoFontDescAttrib(Ihandle *ih);
char* iupgtk4GetPangoLayoutAttrib(Ihandle *ih);
char* iupgtk4GetFontIdAttrib(Ihandle *ih);
void iupgtk4UpdateObjectFont(Ihandle* ih, gpointer object);
void iupgtk4UpdateWidgetFont(Ihandle *ih, GtkWidget* widget);
PangoLayout* iupgtk4GetPangoLayout(const char* value);

/* There are PANGO_SCALE Pango units in one device unit.
   For an output backend where a device unit is a pixel, a size value of 10 * PANGO_SCALE gives 10 pixels. */
#define iupGTK4_PANGOUNITS2PIXELS(_x) (((_x) + PANGO_SCALE/2) / PANGO_SCALE)
#define iupGTK4_PIXELS2PANGOUNITS(_x) ((_x) * PANGO_SCALE)

/* Open */
char* iupgtk4GetNativeWidgetHandle(GtkWidget *widget);
char* iupgtk4GetNativeWindowHandleAttrib(Ihandle* ih);
const char* iupgtk4GetNativeWindowHandleName(void);
const char* iupgtk4GetNativeFontIdName(void);

/* Dialog */
gboolean iupgtk4DialogCloseRequest(GtkWindow *window, Ihandle *ih);

/* Menu */
void iupgtk4DialogSetMenuBar(Ihandle* ih_dialog, Ihandle* ih_menu);

/* Tray */
int iupgtk4SetTrayAttrib(Ihandle *ih, const char *value);
int iupgtk4SetTrayTipAttrib(Ihandle *ih, const char *value);
int iupgtk4SetTrayImageAttrib(Ihandle *ih, const char *value);
int iupgtk4SetTrayMenuAttrib(Ihandle *ih, const char *value);
int iupgtk4TrayCleanup(Ihandle *ih);

/* Loop */
void iupgtk4LoopCleanup(void);

int iupgtk4IsSystemDarkMode(void);

#ifdef GDK_WINDOWING_X11
int iupgtk4X11MoveWindow(void* xdisplay, unsigned long xwindow, int x, int y);
int iupgtk4X11HideFromTaskbar(void* xdisplay, unsigned long xwindow);
int iupgtk4X11Sync(void* xdisplay);
int iupgtk4X11GetDefaultScreen(void* xdisplay);
char* iupgtk4X11GetServerVendor(void* xdisplay);
int iupgtk4X11GetVendorRelease(void* xdisplay);
void iupgtk4X11Cleanup(void);
#endif

#ifdef __cplusplus
}
#endif

#endif
