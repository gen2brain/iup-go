/** \file
 * \brief Image Resource.
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <jni.h>
#include <android/bitmap.h>
#include <android/log.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"

#include "iupandroid_drv.h"
#include "iupandroid_jnimacros.h"


IUPJNI_DECLARE_CLASS_STATIC(IupImageHelper);
IUPJNI_DECLARE_METHOD_ID_STATIC(IupImageHelper_createBitmap);
IUPJNI_DECLARE_METHOD_ID_STATIC(IupImageHelper_loadBitmap);
IUPJNI_DECLARE_METHOD_ID_STATIC(IupImageHelper_saveBitmap);
IUPJNI_DECLARE_METHOD_ID_STATIC(IupImageHelper_saveBitmapToBuffer);

/* allocates an android.graphics.Bitmap via Java and locks its pixel buffer; NULL on failure */
static jobject androidImageAllocBitmap(JNIEnv* jni_env, int width, int height, unsigned char** out_pixels, int* out_stride)
{
  jclass java_class;
  jmethodID method_id;
  jobject java_bitmap = NULL;
  AndroidBitmapInfo bitmap_info;
  int ret_val;

  *out_pixels = NULL;
  *out_stride = 0;

  java_class = IUPJNI_FindClass(IupImageHelper, jni_env, "io/github/gen2brain/iupgo/IupImageHelper");
  method_id = IUPJNI_GetStaticMethodID(IupImageHelper_createBitmap, jni_env, java_class, "createBitmap", "(III)Landroid/graphics/Bitmap;");
  java_bitmap = (*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id, (jint)width, (jint)height, (jint)32);
  iupAndroid_CheckException(jni_env, "IupImageHelper.createBitmap");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);

  if (java_bitmap == NULL)
    return NULL;

  ret_val = AndroidBitmap_getInfo(jni_env, java_bitmap, &bitmap_info);
  if (ret_val < 0)
  {
    __android_log_print(ANDROID_LOG_ERROR, "Iup", "AndroidBitmap_getInfo() failed:%d", ret_val);
    (*jni_env)->DeleteLocalRef(jni_env, java_bitmap);
    return NULL;
  }

  ret_val = AndroidBitmap_lockPixels(jni_env, java_bitmap, (void**)out_pixels);
  if (ret_val < 0)
  {
    __android_log_print(ANDROID_LOG_ERROR, "Iup", "AndroidBitmap_lockPixels() failed:%d", ret_val);
    (*jni_env)->DeleteLocalRef(jni_env, java_bitmap);
    return NULL;
  }

  *out_stride = (int)bitmap_info.stride;
  return java_bitmap;
}

/* ARGB_8888 stores premultiplied pixels; IUP image data is straight alpha */
static unsigned char androidImagePremul(unsigned char c, unsigned char a)
{
  return (unsigned char)((c * a + 127) / 255);
}

static jobject androidImageFinalize(JNIEnv* jni_env, jobject java_bitmap)
{
  if (java_bitmap == NULL)
    return NULL;
  AndroidBitmap_unlockPixels(jni_env, java_bitmap);
  jobject return_bitmap = (*jni_env)->NewGlobalRef(jni_env, java_bitmap);
  (*jni_env)->DeleteLocalRef(jni_env, java_bitmap);
  return return_bitmap;
}


/* Expands IUP's RGB/RGBA/indexed buffer into an ARGB_8888 pixel span (RGBA byte order). */
static void androidImageFillPixels(unsigned char* pixels, int stride, int width, int height, int bpp, iupColor* colors, int colors_count, unsigned char* imgdata)
{
  (void)colors_count;

  if (bpp == 32)
  {
    for (int y = 0; y < height; y++)
    {
      unsigned char* dst = pixels + (size_t)y * stride;
      unsigned char* src = imgdata + (size_t)y * width * 4;
      for (int x = 0; x < width; x++)
      {
        *dst++ = *src++;
        *dst++ = *src++;
        *dst++ = *src++;
        *dst++ = *src++;
      }
    }
  }
  else if (bpp == 24)
  {
    for (int y = 0; y < height; y++)
    {
      unsigned char* dst = pixels + (size_t)y * stride;
      unsigned char* src = imgdata + (size_t)y * width * 3;
      for (int x = 0; x < width; x++)
      {
        *dst++ = *src++;
        *dst++ = *src++;
        *dst++ = *src++;
        *dst++ = 255;
      }
    }
  }
  else if (bpp == 8)
  {
    for (int y = 0; y < height; y++)
    {
      unsigned char* dst = pixels + (size_t)y * stride;
      unsigned char* src = imgdata + (size_t)y * width;
      for (int x = 0; x < width; x++)
      {
        iupColor* c = &colors[*src++];
        *dst++ = c->r;
        *dst++ = c->g;
        *dst++ = c->b;
        *dst++ = 255;
      }
    }
  }
}

/* Global-ref Bitmap from IUP pixel data; caller owns the ref. */
static jobject androidImageBuildBitmapFromRaw(JNIEnv* jni_env, int width, int height, int bpp, iupColor* colors, int colors_count, unsigned char* imgdata)
{
  unsigned char* pixels = NULL;
  int stride = 0;
  jobject java_bitmap = androidImageAllocBitmap(jni_env, width, height, &pixels, &stride);
  if (java_bitmap == NULL)
    return NULL;
  androidImageFillPixels(pixels, stride, width, height, bpp, colors, colors_count, imgdata);
  return androidImageFinalize(jni_env, java_bitmap);
}


IUP_SDK_API int iupdrvImageGetRawInfo(void* handle, int* w, int* h, int* bpp, iupColor* colors, int* colors_count)
{
  (void)colors;
  (void)colors_count;
  return iupdrvImageGetInfo(handle, w, h, bpp);
}

IUP_SDK_API void* iupdrvImageCreateImage(Ihandle* ih, const char* bgcolor, int make_inactive)
{
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  int bpp = iupAttribGetInt(ih, "BPP");
  int width = ih->currentwidth;
  int height = ih->currentheight;
  unsigned char* imgdata = (unsigned char*)iupAttribGetStr(ih, "WID");

  unsigned char bg_r = 0, bg_g = 0, bg_b = 0;
  iupStrToRGB(bgcolor, &bg_r, &bg_g, &bg_b);

  unsigned char* pixels = NULL;
  int stride = 0;
  jobject java_bitmap = androidImageAllocBitmap(jni_env, width, height, &pixels, &stride);
  if (java_bitmap == NULL)
    return NULL;

  if (bpp == 32)
  {
    for (int y = 0; y < height; y++)
    {
      unsigned char* dst = pixels + (size_t)y * stride;
      unsigned char* src = imgdata + (size_t)y * width * 4;
      for (int x = 0; x < width; x++)
      {
        unsigned char s_r = *src++;
        unsigned char s_g = *src++;
        unsigned char s_b = *src++;
        unsigned char s_a = *src++;
        if (make_inactive)
          iupImageColorMakeInactive(&s_r, &s_g, &s_b, bg_r, bg_g, bg_b);
        *dst++ = androidImagePremul(s_r, s_a);
        *dst++ = androidImagePremul(s_g, s_a);
        *dst++ = androidImagePremul(s_b, s_a);
        *dst++ = s_a;
      }
    }
  }
  else if (bpp == 24)
  {
    for (int y = 0; y < height; y++)
    {
      unsigned char* dst = pixels + (size_t)y * stride;
      unsigned char* src = imgdata + (size_t)y * width * 3;
      for (int x = 0; x < width; x++)
      {
        unsigned char s_r = *src++;
        unsigned char s_g = *src++;
        unsigned char s_b = *src++;
        if (make_inactive)
          iupImageColorMakeInactive(&s_r, &s_g, &s_b, bg_r, bg_g, bg_b);
        *dst++ = s_r;
        *dst++ = s_g;
        *dst++ = s_b;
        *dst++ = 255;
      }
    }
  }
  else if (bpp == 8)
  {
    iupColor colors[256];
    int colors_count = 0;
    int has_alpha = iupImageInitColorTable(ih, colors, &colors_count);
    for (int y = 0; y < height; y++)
    {
      unsigned char* dst = pixels + (size_t)y * stride;
      unsigned char* src = imgdata + (size_t)y * width;
      for (int x = 0; x < width; x++)
      {
        iupColor* c = &colors[*src++];
        unsigned char s_r = c->r;
        unsigned char s_g = c->g;
        unsigned char s_b = c->b;
        unsigned char s_a = has_alpha ? c->a : 255;
        if (make_inactive)
          iupImageColorMakeInactive(&s_r, &s_g, &s_b, bg_r, bg_g, bg_b);
        *dst++ = androidImagePremul(s_r, s_a);
        *dst++ = androidImagePremul(s_g, s_a);
        *dst++ = androidImagePremul(s_b, s_a);
        *dst++ = s_a;
      }
    }
  }

  if (make_inactive)
    iupAttribSetStr(ih, "_IUP_BGCOLOR_DEPEND", "1");

  return androidImageFinalize(jni_env, java_bitmap);
}

IUP_SDK_API void* iupdrvImageCreateIcon(Ihandle* ih)
{
  return iupdrvImageCreateImage(ih, NULL, 0);
}

IUP_SDK_API void* iupdrvImageCreateCursor(Ihandle* ih)
{
  (void)ih;
  return NULL;
}

IUP_SDK_API void* iupdrvImageLoad(const char* name, int type)
{
  (void)type;

  if (name == NULL)
    return NULL;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupImageHelper, jni_env, "io/github/gen2brain/iupgo/IupImageHelper");
  jmethodID method_id = IUPJNI_GetStaticMethodID(IupImageHelper_loadBitmap, jni_env, java_class, "loadBitmap", "(Ljava/lang/String;)Landroid/graphics/Bitmap;");

  jstring j_string = (*jni_env)->NewStringUTF(jni_env, name);
  jobject java_bitmap = (*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id, j_string);
  iupAndroid_CheckException(jni_env, "IupImageHelper.loadBitmap");
  (*jni_env)->DeleteLocalRef(jni_env, j_string);
  (*jni_env)->DeleteLocalRef(jni_env, java_class);

  if (java_bitmap != NULL)
  {
    jobject return_bitmap = (*jni_env)->NewGlobalRef(jni_env, java_bitmap);
    (*jni_env)->DeleteLocalRef(jni_env, java_bitmap);
    return return_bitmap;
  }
  return NULL;
}

IUP_SDK_API int iupdrvImageGetInfo(void* handle, int* w, int* h, int* bpp)
{
  jobject java_bitmap = (jobject)handle;
  if (java_bitmap == NULL)
    return 0;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  AndroidBitmapInfo bitmap_info;

  int ret_val = AndroidBitmap_getInfo(jni_env, java_bitmap, &bitmap_info);
  if (ret_val < 0)
  {
    __android_log_print(ANDROID_LOG_ERROR, "Iup", "AndroidBitmap_getInfo() failed:%d", ret_val);
    return 0;
  }

  /* Raw pixels; core applies iupdrvScaleNaturalPx for user IupImages. */
  if (w) *w = (int)bitmap_info.width;
  if (h) *h = (int)bitmap_info.height;
  if (bpp) *bpp = 32;

  return 1;
}

IUP_SDK_API void iupdrvImageDestroy(void* handle, int type)
{
  (void)type;
  if (handle == NULL)
    return;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  (*jni_env)->DeleteGlobalRef(jni_env, (jobject)handle);
}

IUP_SDK_API void iupdrvImageGetData(void* handle, unsigned char* imgdata)
{
  jobject java_bitmap = (jobject)handle;
  if (java_bitmap == NULL || imgdata == NULL)
    return;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  AndroidBitmapInfo bitmap_info;
  if (AndroidBitmap_getInfo(jni_env, java_bitmap, &bitmap_info) < 0)
    return;

  unsigned char* pixels = NULL;
  if (AndroidBitmap_lockPixels(jni_env, java_bitmap, (void**)&pixels) < 0)
    return;

  /* Driver reports bpp=32; caller buffer is width*height*4. */
  int width = (int)bitmap_info.width;
  int height = (int)bitmap_info.height;
  for (int y = 0; y < height; y++)
  {
    unsigned char* src = pixels + (size_t)y * bitmap_info.stride;
    unsigned char* dst = imgdata + (size_t)y * width * 4;
    for (int x = 0; x < width; x++)
    {
      unsigned char a = src[3];
      if (a == 0 || a == 255)
      {
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
      }
      else
      {
        dst[0] = (unsigned char)((src[0] * 255 + a / 2) / a);
        dst[1] = (unsigned char)((src[1] * 255 + a / 2) / a);
        dst[2] = (unsigned char)((src[2] * 255 + a / 2) / a);
      }
      dst[3] = a;
      src += 4;
      dst += 4;
    }
  }

  AndroidBitmap_unlockPixels(jni_env, java_bitmap);
}

IUP_SDK_API int iupdrvImageSave(unsigned char* imgdata, int width, int height, int bpp, iupColor* colors, int colors_count, const char* filename, const char* format)
{
  if (iupStrEqualNoCase(format, "BMP"))
  {
    int size;
    unsigned char* bmp_data = iupImageWriteBMP(imgdata, width, height, bpp, colors, colors_count, &size);
    if (!bmp_data) return 0;
    FILE* f = fopen(filename, "wb");
    if (!f) { free(bmp_data); return 0; }
    int written = (int)fwrite(bmp_data, 1, size, f);
    fclose(f);
    free(bmp_data);
    return (written == size) ? 1 : 0;
  }

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jobject java_bitmap = androidImageBuildBitmapFromRaw(jni_env, width, height, bpp, colors, colors_count, imgdata);
  if (java_bitmap == NULL) return 0;

  const char* q = IupGetGlobal("IMAGESAVEQUALITY");
  int quality = q ? atoi(q) : 85;

  jclass java_class = IUPJNI_FindClass(IupImageHelper, jni_env, "io/github/gen2brain/iupgo/IupImageHelper");
  jmethodID method_id = IUPJNI_GetStaticMethodID(IupImageHelper_saveBitmap, jni_env, java_class, "saveBitmap", "(Landroid/graphics/Bitmap;Ljava/lang/String;Ljava/lang/String;I)Z");
  jstring j_filename = (*jni_env)->NewStringUTF(jni_env, filename);
  jstring j_format = (*jni_env)->NewStringUTF(jni_env, format);
  jboolean ok = (*jni_env)->CallStaticBooleanMethod(jni_env, java_class, method_id, java_bitmap, j_filename, j_format, (jint)quality);
  iupAndroid_CheckException(jni_env, "IupImageHelper.saveBitmap");
  (*jni_env)->DeleteLocalRef(jni_env, j_filename);
  (*jni_env)->DeleteLocalRef(jni_env, j_format);
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  (*jni_env)->DeleteGlobalRef(jni_env, java_bitmap);

  return ok ? 1 : 0;
}

IUP_SDK_API unsigned char* iupdrvImageSaveToBuffer(unsigned char* imgdata, int width, int height, int bpp, iupColor* colors, int colors_count, const char* format, int* size)
{
  if (iupStrEqualNoCase(format, "BMP"))
    return iupImageWriteBMP(imgdata, width, height, bpp, colors, colors_count, size);

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jobject java_bitmap = androidImageBuildBitmapFromRaw(jni_env, width, height, bpp, colors, colors_count, imgdata);
  if (java_bitmap == NULL) return NULL;

  const char* q = IupGetGlobal("IMAGESAVEQUALITY");
  int quality = q ? atoi(q) : 85;

  jclass java_class = IUPJNI_FindClass(IupImageHelper, jni_env, "io/github/gen2brain/iupgo/IupImageHelper");
  jmethodID method_id = IUPJNI_GetStaticMethodID(IupImageHelper_saveBitmapToBuffer, jni_env, java_class, "saveBitmapToBuffer", "(Landroid/graphics/Bitmap;Ljava/lang/String;I)[B");
  jstring j_format = (*jni_env)->NewStringUTF(jni_env, format);
  jbyteArray j_bytes = (jbyteArray)(*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id, java_bitmap, j_format, (jint)quality);
  iupAndroid_CheckException(jni_env, "IupImageHelper.saveBitmapToBuffer");
  (*jni_env)->DeleteLocalRef(jni_env, j_format);
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  (*jni_env)->DeleteGlobalRef(jni_env, java_bitmap);

  if (j_bytes == NULL) return NULL;

  jsize byte_count = (*jni_env)->GetArrayLength(jni_env, j_bytes);
  unsigned char* buffer = (unsigned char*)malloc((size_t)byte_count);
  if (buffer == NULL) { (*jni_env)->DeleteLocalRef(jni_env, j_bytes); return NULL; }
  (*jni_env)->GetByteArrayRegion(jni_env, j_bytes, 0, byte_count, (jbyte*)buffer);
  (*jni_env)->DeleteLocalRef(jni_env, j_bytes);

  if (size) *size = (int)byte_count;
  return buffer;
}

IUP_SDK_API int iupdrvGetIconPixels(Ihandle* ih, const char* value, int* width, int* height, unsigned char** pixels)
{
  (void)ih;
  (void)value;
  (void)width;
  (void)height;
  (void)pixels;
  return 0;
}
