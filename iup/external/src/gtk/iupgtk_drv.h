/** \file
 * \brief GTK Driver 
 *
 * See Copyright Notice in "iup.h"
 */
 
#ifndef __IUPGTK_DRV_H 
#define __IUPGTK_DRV_H

#ifdef __cplusplus
extern "C" {
#endif

#define iupgtkColorFromDouble(_x) ((unsigned char)(_x*255))  /* 1.0*255 = 255 */
#define iupgtkColorToDouble(_x) ((double)_x/255.0)


/* common */
IUP_DRV_API gboolean iupgtkEnterLeaveEvent(GtkWidget *widget, GdkEventCrossing *evt, Ihandle* ih);
gboolean iupgtkMotionNotifyEvent(GtkWidget *widget, GdkEventMotion *evt, Ihandle *ih);
IUP_DRV_API gboolean iupgtkButtonEvent(GtkWidget *widget, GdkEventButton *evt, Ihandle *ih);
IUP_DRV_API gboolean iupgtkShowHelp(GtkWidget *widget, GtkWidgetHelpType *arg1, Ihandle* ih);

int iupgtkSetMnemonicTitle(Ihandle* ih, GtkLabel* label, const char* value);
void iupgtkUpdateMnemonic(Ihandle* ih);

void iupgdkColorSetRGB(GdkColor* color, unsigned char r, unsigned char g, unsigned char b);
void iupgtkSetBgColor(InativeHandle* handle, unsigned char r, unsigned char g, unsigned char b);
void iupgtkSetFgColor(InativeHandle* handle, unsigned char r, unsigned char g, unsigned char b);

IUP_DRV_API void iupgtkAddToParent(Ihandle* ih);
const char* iupgtkGetWidgetClassName(GtkWidget* widget);
void iupgtkSetPosSize(GtkContainer* parent, GtkWidget* widget, int x, int y, int width, int height);
GdkWindow* iupgtkGetWindow(GtkWidget *widget);
void iupgtkWindowGetPointer(GdkWindow *window, int *x, int *y, GdkModifierType *mask);
int iupgtkIsVisible(GtkWidget* widget);
void iupgtkClearSizeStyleCSS(GtkWidget* widget);
void iupgtkSetMargin(GtkWidget* widget, int horiz_padding, int vert_padding, int mandatory_gtk3);

GtkWidget* iupgtkNativeContainerNew(int has_window);
void iupgtkNativeContainerAdd(GtkWidget* container, GtkWidget* widget);
void iupgtkNativeContainerMove(GtkWidget* container, GtkWidget* widget, int x, int y);

/* str */
void  iupgtkStrRelease(void);
IUP_DRV_API char* iupgtkStrConvertToSystem(const char* str);
char* iupgtkStrConvertToSystemLen(const char* str, int *len);
char* iupgtkStrConvertFromSystem(const char* str);
char* iupgtkStrConvertFromFilename(const char* str);
char* iupgtkStrConvertToFilename(const char* str);
void  iupgtkStrSetUTF8Mode(int utf8mode);
int   iupgtkStrGetUTF8Mode(void);


/* focus */
IUP_DRV_API gboolean iupgtkFocusInOutEvent(GtkWidget *widget, GdkEventFocus *evt, Ihandle* ih);
IUP_DRV_API void iupgtkSetCanFocus(GtkWidget *widget, int can);


/* key */
IUP_DRV_API gboolean iupgtkKeyPressEvent(GtkWidget *widget, GdkEventKey *evt, Ihandle* ih);
gboolean iupgtkKeyReleaseEvent(GtkWidget *widget, GdkEventKey *evt, Ihandle* ih);
void iupgtkButtonKeySetStatus(guint state, unsigned int but, char* status, int doubleclick);
int iupgtkKeyDecode(GdkEventKey *evt);


/* font */
PangoFontDescription* iupgtkGetPangoFontDesc(const char* value);
char* iupgtkGetPangoFontDescAttrib(Ihandle *ih);
char* iupgtkGetPangoLayoutAttrib(Ihandle *ih);
char* iupgtkGetFontIdAttrib(Ihandle *ih);
void iupgtkUpdateObjectFont(Ihandle* ih, gpointer object);
void iupgtkUpdateWidgetFont(Ihandle *ih, GtkWidget* widget);
PangoLayout* iupgtkGetPangoLayout(const char* value);

/* There are PANGO_SCALE Pango units in one device unit. 
  For an output backend where a device unit is a pixel, 
  a size value of 10 * PANGO_SCALE gives 10 pixels. */
#define iupGTK_PANGOUNITS2PIXELS(_x) (((_x) + PANGO_SCALE/2) / PANGO_SCALE)
#define iupGTK_PIXELS2PANGOUNITS(_x) ((_x) * PANGO_SCALE)


/* open */
char* iupgtkGetNativeWidgetHandle(GtkWidget *widget);  /* Used only in Canvas, Dialog and FileDlg - for drawing with CD/GDK or OpenGL (not used for IupDraw) */
char* iupgtkGetNativeWindowHandleAttrib(Ihandle* ih);  /* Used only in Canvas and Dialog - for drawing with CD/GDK or OpenGL (not used for IupDraw) */
const char* iupgtkGetNativeWindowHandleName(void);  /* Used only in Canvas, Dialog and FileDlg - for drawing with CD/GDK or OpenGL (not used for IupDraw) */
const char* iupgtkGetNativeFontIdName(void); /* Attribute available for IupGLUseFont - for drawing with OpenGL */
void iupgtkPushVisualAndColormap(void* visual, void* colormap); /* Used in Canvas, for GLCanvas VISUAL attribute (GTK 2 Only) - for drawing with OpenGL */
void* iupgtkGetNativeGraphicsContext(GtkWidget* widget); /* Used in FileDlg PREVIEWDC attribute - for drawing with CD/GDK */
void iupgtkReleaseNativeGraphicsContext(GtkWidget* widget, void* gc); /* Used in FileDlg PREVIEWDC attribute - for drawing with CD/GDK */


/* dialog */
gboolean iupgtkDialogDeleteEvent(GtkWidget *widget, GdkEvent *evt, Ihandle *ih);


#ifdef __cplusplus
}
#endif

#endif
