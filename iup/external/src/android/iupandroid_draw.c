/** \file
 * \brief Draw Functions
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdint.h>

#include <jni.h>
#include <android/bitmap.h>
#include <android/log.h>

#include "iup.h"

#include "iup_attrib.h"
#include "iup_class.h"
#include "iup_str.h"
#include "iup_object.h"
#include "iup_image.h"
#include "iup_drvdraw.h"
#include "iup_drvfont.h"
#include "iup_drvinfo.h"
#include "iup_draw.h"

#include "iupandroid_drv.h"
#include "iupandroid_jnimacros.h"


IUPJNI_DECLARE_CLASS_STATIC(IupCanvasHelper);

struct _IdrawCanvas
{
  Ihandle* ih;
  int w, h;
  int clip_x1, clip_y1, clip_x2, clip_y2;
  int clipped;
};

static int androidPackColor(long color)
{
  /* iupDrawAlpha unflips IUP's inverted-alpha; produce 0xAARRGGBB */
  unsigned int a = iupDrawAlpha(color);
  unsigned int r = iupDrawRed(color);
  unsigned int g = iupDrawGreen(color);
  unsigned int b = iupDrawBlue(color);
  return (int)((a << 24) | (r << 16) | (g << 8) | b);
}

static jclass androidDrawFindHelper(JNIEnv* jni_env)
{
  return IUPJNI_FindClass(IupCanvasHelper, jni_env, "io/github/gen2brain/iupgo/IupCanvasHelper");
}

IdrawCanvas* iupdrvDrawCreateCanvas(Ihandle* ih)
{
  IdrawCanvas* dc = calloc(1, sizeof(IdrawCanvas));
  dc->ih = ih;
  dc->w = ih->currentwidth;
  dc->h = ih->currentheight;
  dc->clipped = 0;

  /* Make sure the Java-side Bitmap exists and matches the widget size. */
  if (ih->handle)
  {
    JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
    jclass java_class = androidDrawFindHelper(jni_env);
    jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "ensureBackBuffer", "(Lio/github/gen2brain/iupgo/IupAndroidCanvas;)V");
    (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, ih->handle);
    iupAndroid_CheckException(jni_env, "IupCanvasHelper.ensureBackBuffer");
    (*jni_env)->DeleteLocalRef(jni_env, java_class);
  }

  iupAttribSet(ih, "DRAWDRIVER", "ANDROID");
  return dc;
}

void iupdrvDrawKillCanvas(IdrawCanvas* dc)
{
  free(dc);
}

void iupdrvDrawUpdateSize(IdrawCanvas* dc)
{
  if (!dc || !dc->ih) return;
  dc->w = dc->ih->currentwidth;
  dc->h = dc->ih->currentheight;
  if (dc->ih->handle)
  {
    JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
    jclass java_class = androidDrawFindHelper(jni_env);
    jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "ensureBackBuffer", "(Lio/github/gen2brain/iupgo/IupAndroidCanvas;)V");
    (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, dc->ih->handle);
    iupAndroid_CheckException(jni_env, "IupCanvasHelper.ensureBackBuffer");
    (*jni_env)->DeleteLocalRef(jni_env, java_class);
  }
}

void iupdrvDrawFlush(IdrawCanvas* dc)
{
  if (!dc || !dc->ih || !dc->ih->handle) return;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = androidDrawFindHelper(jni_env);
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "flush", "(Lio/github/gen2brain/iupgo/IupAndroidCanvas;)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, dc->ih->handle);
  iupAndroid_CheckException(jni_env, "IupCanvasHelper.flush");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
}

void iupdrvDrawGetSize(IdrawCanvas* dc, int* w, int* h)
{
  /* HW px -> canvas-coord; float density + ceil for exact round-trip with iupdrvScaleNaturalPx */
  float d = iupAndroid_GetDisplayDensity(); if (d < 1.0f) d = 1.0f;
  if (w) *w = dc ? (int)ceilf((float)dc->w / d) : 0;
  if (h) *h = dc ? (int)ceilf((float)dc->h / d) : 0;
}

static void androidDrawRect(IdrawCanvas* dc, const char* method_name, int x1, int y1, int x2, int y2, long color, int style, int width)
{
  if (!dc || !dc->ih->handle) return;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = androidDrawFindHelper(jni_env);
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, method_name, "(Lio/github/gen2brain/iupgo/IupAndroidCanvas;IIIIIII)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, dc->ih->handle, (jint)x1, (jint)y1, (jint)x2, (jint)y2, (jint)androidPackColor(color), (jint)style, (jint)width);
  iupAndroid_CheckException(jni_env, method_name);
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
}

void iupdrvDrawLine(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
  androidDrawRect(dc, "drawLine", x1, y1, x2, y2, color, style, line_width);
}

void iupdrvDrawRectangle(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
  androidDrawRect(dc, "drawRectangle", x1, y1, x2, y2, color, style, line_width);
}

void iupdrvDrawEllipse(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
  androidDrawRect(dc, "drawEllipse", x1, y1, x2, y2, color, style, line_width);
}

void iupdrvDrawArc(IdrawCanvas* dc, int x1, int y1, int x2, int y2, double a1, double a2, long color, int style, int line_width)
{
  if (!dc || !dc->ih->handle) return;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = androidDrawFindHelper(jni_env);
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "drawArc", "(Lio/github/gen2brain/iupgo/IupAndroidCanvas;IIIIFFIII)V");

  /* IUP's arc gives start + end angles; Canvas wants start + sweep. */
  float start = (float)a1;
  float sweep = (float)(a2 - a1);

  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, dc->ih->handle, (jint)x1, (jint)y1, (jint)x2, (jint)y2, (jfloat)start, (jfloat)sweep, (jint)androidPackColor(color), (jint)style, (jint)line_width);
  iupAndroid_CheckException(jni_env, "IupCanvasHelper.drawArc");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
}

void iupdrvDrawPolygon(IdrawCanvas* dc, int* points, int count, long color, int style, int line_width)
{
  if (!dc || !dc->ih->handle || count <= 0) return;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = androidDrawFindHelper(jni_env);
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "drawPolygon", "(Lio/github/gen2brain/iupgo/IupAndroidCanvas;[IIII)V");

  jintArray arr = (*jni_env)->NewIntArray(jni_env, count * 2);
  (*jni_env)->SetIntArrayRegion(jni_env, arr, 0, count * 2, (const jint*)points);
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, dc->ih->handle, arr, (jint)androidPackColor(color), (jint)style, (jint)line_width);
  iupAndroid_CheckException(jni_env, "IupCanvasHelper.drawPolygon");
  (*jni_env)->DeleteLocalRef(jni_env, arr);
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
}

void iupdrvDrawPixel(IdrawCanvas* dc, int x, int y, long color)
{
  if (!dc || !dc->ih->handle) return;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = androidDrawFindHelper(jni_env);
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "drawPixel", "(Lio/github/gen2brain/iupgo/IupAndroidCanvas;III)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, dc->ih->handle, (jint)x, (jint)y, (jint)androidPackColor(color));
  iupAndroid_CheckException(jni_env, "IupCanvasHelper.drawPixel");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
}

void iupdrvDrawRoundedRectangle(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int corner_radius, long color, int style, int line_width)
{
  if (!dc || !dc->ih->handle) return;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = androidDrawFindHelper(jni_env);
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "drawRoundedRectangle", "(Lio/github/gen2brain/iupgo/IupAndroidCanvas;IIIIIIII)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, dc->ih->handle, (jint)x1, (jint)y1, (jint)x2, (jint)y2, (jint)corner_radius, (jint)androidPackColor(color), (jint)style, (jint)line_width);
  iupAndroid_CheckException(jni_env, "IupCanvasHelper.drawRoundedRectangle");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
}

void iupdrvDrawBezier(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4, long color, int style, int line_width)
{
  if (!dc || !dc->ih->handle) return;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = androidDrawFindHelper(jni_env);
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "drawBezier", "(Lio/github/gen2brain/iupgo/IupAndroidCanvas;IIIIIIIIIII)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, dc->ih->handle, (jint)x1, (jint)y1, (jint)x2, (jint)y2, (jint)x3, (jint)y3, (jint)x4, (jint)y4, (jint)androidPackColor(color), (jint)style, (jint)line_width);
  iupAndroid_CheckException(jni_env, "IupCanvasHelper.drawBezier");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
}

void iupdrvDrawQuadraticBezier(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int x3, int y3, long color, int style, int line_width)
{
  if (!dc || !dc->ih->handle) return;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = androidDrawFindHelper(jni_env);
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "drawQuadraticBezier", "(Lio/github/gen2brain/iupgo/IupAndroidCanvas;IIIIIIIII)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, dc->ih->handle, (jint)x1, (jint)y1, (jint)x2, (jint)y2, (jint)x3, (jint)y3, (jint)androidPackColor(color), (jint)style, (jint)line_width);
  iupAndroid_CheckException(jni_env, "IupCanvasHelper.drawQuadraticBezier");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
}

void iupdrvDrawLinearGradient(IdrawCanvas* dc, int x1, int y1, int x2, int y2, float angle, long color1, long color2)
{
  (void)angle;  /* Shader direction is encoded by the endpoints. */
  if (!dc || !dc->ih->handle) return;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = androidDrawFindHelper(jni_env);
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "drawLinearGradient", "(Lio/github/gen2brain/iupgo/IupAndroidCanvas;IIIIII)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, dc->ih->handle, (jint)x1, (jint)y1, (jint)x2, (jint)y2, (jint)androidPackColor(color1), (jint)androidPackColor(color2));
  iupAndroid_CheckException(jni_env, "IupCanvasHelper.drawLinearGradient");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
}

void iupdrvDrawRadialGradient(IdrawCanvas* dc, int cx, int cy, int radius, long colorCenter, long colorEdge)
{
  if (!dc || !dc->ih->handle) return;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = androidDrawFindHelper(jni_env);
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "drawRadialGradient", "(Lio/github/gen2brain/iupgo/IupAndroidCanvas;IIIII)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, dc->ih->handle, (jint)cx, (jint)cy, (jint)radius, (jint)androidPackColor(colorCenter), (jint)androidPackColor(colorEdge));
  iupAndroid_CheckException(jni_env, "IupCanvasHelper.drawRadialGradient");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
}

void iupdrvDrawText(IdrawCanvas* dc, const char* text, int len, int x, int y, int w, int h, long color, const char* font, int flags, double text_orientation)
{
  (void)len;
  if (!dc || !dc->ih->handle || !text) return;

  char typeface[1024] = "";
  int size = 0, is_bold = 0, is_italic = 0, is_underline = 0, is_strikeout = 0;
  if (font && font[0])
    iupGetFontInfo(font, typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout);

  int style = 0;  /* 0=NORMAL, 1=BOLD, 2=ITALIC, 3=BOLD_ITALIC */
  if (is_bold && is_italic) style = 3;
  else if (is_bold)         style = 1;
  else if (is_italic)       style = 2;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = androidDrawFindHelper(jni_env);
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "drawText",
    "(Lio/github/gen2brain/iupgo/IupAndroidCanvas;Ljava/lang/String;IIIIIIFLjava/lang/String;IIZZ)V");

  jstring j_text = (*jni_env)->NewStringUTF(jni_env, text);
  jstring j_family = typeface[0] ? (*jni_env)->NewStringUTF(jni_env, typeface) : NULL;
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id,
    dc->ih->handle, j_text,
    (jint)x, (jint)y, (jint)w, (jint)h,
    (jint)androidPackColor(color), (jint)flags, (jfloat)text_orientation,
    j_family, (jint)style, (jint)size,
    (jboolean)(is_underline ? 1 : 0), (jboolean)(is_strikeout ? 1 : 0));
  iupAndroid_CheckException(jni_env, "IupCanvasHelper.drawText");
  (*jni_env)->DeleteLocalRef(jni_env, j_text);
  if (j_family) (*jni_env)->DeleteLocalRef(jni_env, j_family);
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
}

void iupdrvDrawImage(IdrawCanvas* dc, const char* name, int make_inactive, const char* bgcolor, long tint, int opacity, int x, int y, int w, int h, int sx, int sy, int sw, int sh, int quality)
{
  if (!dc || !dc->ih->handle) return;

  jobject java_bitmap = (jobject)iupImageGetImageTint(name, dc->ih, make_inactive, bgcolor, tint);
  if (!java_bitmap) return;

  int bpp, img_w, img_h;
  iupdrvImageGetInfo(java_bitmap, &img_w, &img_h, &bpp);

  if (sw <= 0 || sh <= 0)
  {
    sx = 0;
    sy = 0;
    sw = img_w;
    sh = img_h;
  }
  if (w <= 0) w = sw;
  if (h <= 0) h = sh;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = androidDrawFindHelper(jni_env);
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "drawBitmap", "(Lio/github/gen2brain/iupgo/IupAndroidCanvas;Landroid/graphics/Bitmap;IIIIIIIIZI)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, dc->ih->handle, java_bitmap, (jint)x, (jint)y, (jint)w, (jint)h,
                                   (jint)sx, (jint)sy, (jint)sw, (jint)sh, (jboolean)(quality != IUP_DRAW_IMAGE_NEAREST), (jint)opacity);
  iupAndroid_CheckException(jni_env, "IupCanvasHelper.drawBitmap");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
}

void iupdrvDrawSetClipRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2)
{
  if (!dc || !dc->ih->handle) return;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = androidDrawFindHelper(jni_env);
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "setClipRect", "(Lio/github/gen2brain/iupgo/IupAndroidCanvas;IIII)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, dc->ih->handle, (jint)x1, (jint)y1, (jint)x2, (jint)y2);
  iupAndroid_CheckException(jni_env, "IupCanvasHelper.setClipRect");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);

  dc->clip_x1 = x1;
  dc->clip_y1 = y1;
  dc->clip_x2 = x2;
  dc->clip_y2 = y2;
  dc->clipped = 1;
}

void iupdrvDrawSetClipRoundedRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int corner_radius)
{
  if (!dc || !dc->ih->handle) return;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = androidDrawFindHelper(jni_env);
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "setClipRoundedRect", "(Lio/github/gen2brain/iupgo/IupAndroidCanvas;IIIII)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, dc->ih->handle, (jint)x1, (jint)y1, (jint)x2, (jint)y2, (jint)corner_radius);
  iupAndroid_CheckException(jni_env, "IupCanvasHelper.setClipRoundedRect");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);

  dc->clip_x1 = x1;
  dc->clip_y1 = y1;
  dc->clip_x2 = x2;
  dc->clip_y2 = y2;
  dc->clipped = 1;
}

void iupdrvDrawResetClip(IdrawCanvas* dc)
{
  if (!dc || !dc->ih->handle || !dc->clipped) return;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = androidDrawFindHelper(jni_env);
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "resetClip", "(Lio/github/gen2brain/iupgo/IupAndroidCanvas;)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, dc->ih->handle);
  iupAndroid_CheckException(jni_env, "IupCanvasHelper.resetClip");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);

  dc->clipped = 0;
}

void iupdrvDrawGetClipRect(IdrawCanvas* dc, int* x1, int* y1, int* x2, int* y2)
{
  if (!dc) return;
  if (x1) *x1 = dc->clip_x1;
  if (y1) *y1 = dc->clip_y1;
  if (x2) *x2 = dc->clip_x2;
  if (y2) *y2 = dc->clip_y2;
}

void iupdrvDrawSelectRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2)
{
  /* Translucent primary-colour fill approximates the native "selected" look. */
  long blue = iupDrawColor(33, 150, 243, 96);
  iupdrvDrawRectangle(dc, x1, y1, x2, y2, blue, IUP_DRAW_FILL, 1);
}

void iupdrvDrawFocusRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2)
{
  long color = iupDrawColor(0, 0, 0, 255);
  iupdrvDrawRectangle(dc, x1, y1, x2, y2, color, IUP_DRAW_STROKE_DOT, 1);
}

/* Logical-sized RGBA snapshot of the canvas back buffer; Java downscales. */
static int androidDrawReadBackBuffer(jobject canvas_view, unsigned char* data, int w, int h)
{
  if (canvas_view == NULL || data == NULL || w <= 0 || h <= 0) return 0;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = androidDrawFindHelper(jni_env);
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "getBackBufferSnapshot", "(Lio/github/gen2brain/iupgo/IupAndroidCanvas;II)Landroid/graphics/Bitmap;");
  jobject bitmap = (*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id, canvas_view, (jint)w, (jint)h);
  iupAndroid_CheckException(jni_env, "IupCanvasHelper.getBackBufferSnapshot");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  if (bitmap == NULL) return 0;

  unsigned char* pixels = NULL;
  if (AndroidBitmap_lockPixels(jni_env, bitmap, (void**)&pixels) < 0)
  {
    (*jni_env)->DeleteLocalRef(jni_env, bitmap);
    return 0;
  }

  AndroidBitmapInfo info;
  AndroidBitmap_getInfo(jni_env, bitmap, &info);
  int line_size = w * 4;
  for (int y = 0; y < h; y++)
    memcpy(data + y * line_size, pixels + y * info.stride, line_size);

  AndroidBitmap_unlockPixels(jni_env, bitmap);
  (*jni_env)->DeleteLocalRef(jni_env, bitmap);
  return 1;
}

int iupdrvDrawGetImageData(IdrawCanvas* dc, unsigned char* data)
{
  if (!dc || !dc->ih || !dc->ih->handle) return 0;
  float d = iupAndroid_GetDisplayDensity(); if (d < 1.0f) d = 1.0f;
  int w = (int)ceilf((float)dc->w / d);
  int h = (int)ceilf((float)dc->h / d);
  return androidDrawReadBackBuffer((jobject)dc->ih->handle, data, w, h);
}

int iupdrvCanvasGetImageData(Ihandle* ih, unsigned char* data, int w, int h)
{
  if (!ih || !ih->handle) return 0;
  return androidDrawReadBackBuffer((jobject)ih->handle, data, w, h);
}
