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
IUP_DRV_API gboolean iupgtkMotionNotifyEvent(GtkWidget *widget, GdkEventMotion *evt, Ihandle *ih);
IUP_DRV_API gboolean iupgtkButtonEvent(GtkWidget *widget, GdkEventButton *evt, Ihandle *ih);
IUP_DRV_API gboolean iupgtkShowHelp(GtkWidget *widget, GtkWidgetHelpType *arg1, Ihandle* ih);

IUP_DRV_API int iupgtkSetMnemonicTitle(Ihandle* ih, GtkLabel* label, const char* value);
IUP_DRV_API void iupgtkUpdateMnemonic(Ihandle* ih);

IUP_DRV_API void iupgdkColorSetRGB(GdkColor* color, unsigned char r, unsigned char g, unsigned char b);
#if GTK_CHECK_VERSION(3, 0, 0)
IUP_DRV_API void iupgdkRGBASet(GdkRGBA* rgba, unsigned char r, unsigned char g, unsigned char b);
#endif
IUP_DRV_API void iupgtkSetBgColor(InativeHandle* handle, unsigned char r, unsigned char g, unsigned char b);
IUP_DRV_API void iupgtkSetFgColor(InativeHandle* handle, unsigned char r, unsigned char g, unsigned char b);

IUP_DRV_API void iupgtkAddToParent(Ihandle* ih);
IUP_DRV_API const char* iupgtkGetWidgetClassName(GtkWidget* widget);
IUP_DRV_API void iupgtkSetPosSize(GtkContainer* parent, GtkWidget* widget, int x, int y, int width, int height);
IUP_DRV_API GdkWindow* iupgtkGetWindow(GtkWidget *widget);
IUP_DRV_API void iupgtkWindowGetPointer(GdkWindow *window, int *x, int *y, GdkModifierType *mask);
IUP_DRV_API int iupgtkIsVisible(GtkWidget* widget);
IUP_DRV_API void iupgtkSetMargin(GtkWidget* widget, int horiz_padding, int vert_padding, int mandatory_gtk3);

IUP_DRV_API GtkWidget* iupgtkNativeContainerNew(int has_window);
IUP_DRV_API void iupgtkNativeContainerSetGLCanvas(int is_gl);
IUP_DRV_API void iupgtkNativeContainerAdd(GtkWidget* container, GtkWidget* widget);
IUP_DRV_API void iupgtkNativeContainerMove(GtkWidget* container, GtkWidget* widget, int x, int y);

/* str */
IUP_DRV_API void  iupgtkStrRelease(void);
IUP_DRV_API char* iupgtkStrConvertToSystem(const char* str);
IUP_DRV_API char* iupgtkStrConvertToSystemLen(const char* str, int *len);
IUP_DRV_API char* iupgtkStrConvertFromSystem(const char* str);
IUP_DRV_API char* iupgtkStrConvertFromFilename(const char* str);
IUP_DRV_API char* iupgtkStrConvertToFilename(const char* str);
IUP_DRV_API void  iupgtkStrSetUTF8Mode(int utf8mode);
IUP_DRV_API int   iupgtkStrGetUTF8Mode(void);


/* focus */
IUP_DRV_API gboolean iupgtkFocusInOutEvent(GtkWidget *widget, GdkEventFocus *evt, Ihandle* ih);
IUP_DRV_API void iupgtkSetCanFocus(GtkWidget *widget, int can);


/* key */
IUP_DRV_API gboolean iupgtkKeyPressEvent(GtkWidget *widget, GdkEventKey *evt, Ihandle* ih);
IUP_DRV_API gboolean iupgtkKeyReleaseEvent(GtkWidget *widget, GdkEventKey *evt, Ihandle* ih);
IUP_DRV_API void iupgtkButtonKeySetStatus(guint state, unsigned int but, char* status, int doubleclick);
IUP_DRV_API int iupgtkKeyDecode(GdkEventKey *evt);


/* font */
IUP_DRV_API PangoFontDescription* iupgtkGetPangoFontDesc(const char* value);
IUP_DRV_API char* iupgtkGetPangoFontDescAttrib(Ihandle *ih);
IUP_DRV_API char* iupgtkGetPangoLayoutAttrib(Ihandle *ih);
IUP_DRV_API char* iupgtkGetFontIdAttrib(Ihandle *ih);
IUP_DRV_API void iupgtkUpdateObjectFont(Ihandle* ih, gpointer object);
IUP_DRV_API void iupgtkUpdateWidgetFont(Ihandle *ih, GtkWidget* widget);
IUP_DRV_API PangoLayout* iupgtkGetPangoLayout(const char* value);

/* There are PANGO_SCALE Pango units in one device unit. 
  For an output backend where a device unit is a pixel, 
  a size value of 10 * PANGO_SCALE gives 10 pixels. */
#define iupGTK_PANGOUNITS2PIXELS(_x) (((_x) + PANGO_SCALE/2) / PANGO_SCALE)
#define iupGTK_PIXELS2PANGOUNITS(_x) ((_x) * PANGO_SCALE)


/* open */
IUP_DRV_API char* iupgtkGetNativeWidgetHandle(GtkWidget *widget);  /* Used only in Canvas, Dialog and FileDlg - for drawing with CD/GDK or OpenGL (not used for IupDraw) */
IUP_DRV_API char* iupgtkGetNativeWindowHandleAttrib(Ihandle* ih);  /* Used only in Canvas and Dialog - for drawing with CD/GDK or OpenGL (not used for IupDraw) */
IUP_DRV_API const char* iupgtkGetNativeWindowHandleName(void);  /* Used only in Canvas, Dialog and FileDlg - for drawing with CD/GDK or OpenGL (not used for IupDraw) */
IUP_DRV_API const char* iupgtkGetNativeFontIdName(void); /* Attribute available for IupGLUseFont - for drawing with OpenGL */
IUP_DRV_API void iupgtkPushVisualAndColormap(void* visual, void* colormap); /* Used in Canvas, for GLCanvas VISUAL attribute (GTK 2 Only) - for drawing with OpenGL */
IUP_DRV_API void* iupgtkGetNativeGraphicsContext(GtkWidget* widget); /* Used in FileDlg PREVIEWDC attribute - for drawing with CD/GDK */
IUP_DRV_API void iupgtkReleaseNativeGraphicsContext(GtkWidget* widget, void* gc); /* Used in FileDlg PREVIEWDC attribute - for drawing with CD/GDK */


/* dialog */
IUP_DRV_API gboolean iupgtkDialogDeleteEvent(GtkWidget *widget, GdkEvent *evt, Ihandle *ih);

IUP_DRV_API int iupgtkIsSystemDarkMode(void);
IUP_DRV_API void iupgtkSetGlobalColors(void);

/* table */
IUP_DRV_API void iupgtkTableDetachVirtualModels(Ihandle* dialog);

#ifdef __cplusplus
}
#endif

#endif
