/** \file
 * \brief Table Control (Android)
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <jni.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drvfont.h"
#include "iup_image.h"
#include "iup_table.h"

#include "iupandroid_drv.h"
#include "iupandroid_jnimacros.h"


IUPJNI_DECLARE_CLASS_STATIC(IupTableHelper);

#define IUPANDROID_TYPEFACE_NORMAL      0
#define IUPANDROID_TYPEFACE_BOLD        1
#define IUPANDROID_TYPEFACE_ITALIC      2
#define IUPANDROID_TYPEFACE_BOLD_ITALIC 3
#define IUPANDROID_COMPLEX_UNIT_PX      0
#define IUPANDROID_COMPLEX_UNIT_SP      2

static jclass androidTableFindClass(JNIEnv* jni_env)
{
  return IUPJNI_FindClass(IupTableHelper, jni_env, "io/github/gen2brain/iupgo/IupTableHelper");
}

static void androidTableCallVoid(Ihandle* ih, const char* method, const char* sig)
{
  if (!ih->handle) return;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTableFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, method, sig);
  (*env)->CallStaticVoidMethod(env, cls, m, ih->handle);
  iupAndroid_CheckException(env, method);
  (*env)->DeleteLocalRef(env, cls);
}

static int androidTableCallInt(Ihandle* ih, const char* method, const char* sig)
{
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTableFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, method, sig);
  jint r = (*env)->CallStaticIntMethod(env, cls, m, ih->handle);
  iupAndroid_CheckException(env, method);
  (*env)->DeleteLocalRef(env, cls);
  return (int)r;
}

void iupdrvTableSetNumLin(Ihandle* ih, int num_lin)
{
  ih->data->num_lin = num_lin;
  if (!ih->handle) return;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTableFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "setNumLin", "(Landroid/view/View;I)V");
  (*env)->CallStaticVoidMethod(env, cls, m, ih->handle, (jint)num_lin);
  iupAndroid_CheckException(env, "IupTableHelper.setNumLin");
  (*env)->DeleteLocalRef(env, cls);
}

void iupdrvTableSetNumCol(Ihandle* ih, int num_col)
{
  ih->data->num_col = num_col;
  if (!ih->handle) return;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTableFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "setNumCol", "(Landroid/view/View;I)V");
  (*env)->CallStaticVoidMethod(env, cls, m, ih->handle, (jint)num_col);
  iupAndroid_CheckException(env, "IupTableHelper.setNumCol");
  (*env)->DeleteLocalRef(env, cls);
}

void iupdrvTableAddLin(Ihandle* ih, int pos)
{
  if (!ih->handle) return;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTableFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "addLin", "(Landroid/view/View;I)V");
  (*env)->CallStaticVoidMethod(env, cls, m, ih->handle, (jint)pos);
  iupAndroid_CheckException(env, "IupTableHelper.addLin");
  (*env)->DeleteLocalRef(env, cls);
  ih->data->num_lin++;
}

void iupdrvTableDelLin(Ihandle* ih, int pos)
{
  if (!ih->handle) return;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTableFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "delLin", "(Landroid/view/View;I)V");
  (*env)->CallStaticVoidMethod(env, cls, m, ih->handle, (jint)pos);
  iupAndroid_CheckException(env, "IupTableHelper.delLin");
  (*env)->DeleteLocalRef(env, cls);
  if (ih->data->num_lin > 0) ih->data->num_lin--;
}

void iupdrvTableAddCol(Ihandle* ih, int pos)
{
  if (!ih->handle) return;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTableFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "addCol", "(Landroid/view/View;I)V");
  (*env)->CallStaticVoidMethod(env, cls, m, ih->handle, (jint)pos);
  iupAndroid_CheckException(env, "IupTableHelper.addCol");
  (*env)->DeleteLocalRef(env, cls);
  ih->data->num_col++;
}

void iupdrvTableDelCol(Ihandle* ih, int pos)
{
  if (!ih->handle) return;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTableFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "delCol", "(Landroid/view/View;I)V");
  (*env)->CallStaticVoidMethod(env, cls, m, ih->handle, (jint)pos);
  iupAndroid_CheckException(env, "IupTableHelper.delCol");
  (*env)->DeleteLocalRef(env, cls);
  if (ih->data->num_col > 0) ih->data->num_col--;
}

void iupdrvTableSetCellValue(Ihandle* ih, int lin, int col, const char* value)
{
  if (iupAttribGetBoolean(ih, "VIRTUALMODE")) return;
  if (!ih->handle) return;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTableFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "setCellValue", "(Landroid/view/View;IILjava/lang/String;)V");
  jstring j_text = value ? (*env)->NewStringUTF(env, value) : NULL;
  (*env)->CallStaticVoidMethod(env, cls, m, ih->handle, (jint)lin, (jint)col, j_text);
  iupAndroid_CheckException(env, "IupTableHelper.setCellValue");
  if (j_text) (*env)->DeleteLocalRef(env, j_text);
  (*env)->DeleteLocalRef(env, cls);
}

char* iupdrvTableGetCellValue(Ihandle* ih, int lin, int col)
{
  if (iupAttribGetBoolean(ih, "VIRTUALMODE"))
  {
    sIFnii cb = (sIFnii)IupGetCallback(ih, "VALUE_CB");
    if (!cb) return NULL;
    char* v = cb(ih, lin, col);
    return v ? iupStrReturnStr(v) : NULL;
  }
  if (!ih->handle) return NULL;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTableFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "getCellValue", "(Landroid/view/View;II)Ljava/lang/String;");
  jstring j_text = (jstring)(*env)->CallStaticObjectMethod(env, cls, m, ih->handle, (jint)lin, (jint)col);
  iupAndroid_CheckException(env, "IupTableHelper.getCellValue");
  char* result = iupAndroid_JStringToReturnStr(env, j_text);
  (*env)->DeleteLocalRef(env, cls);
  return result;
}

void iupdrvTableSetCellImage(Ihandle* ih, int lin, int col, const char* image)
{
  if (!ih->handle) return;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTableFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "setCellImage", "(Landroid/view/View;IILandroid/graphics/Bitmap;)V");
  jobject bmp = image ? (jobject)iupImageGetImage(image, ih, 0, NULL) : NULL;
  (*env)->CallStaticVoidMethod(env, cls, m, ih->handle, (jint)lin, (jint)col, bmp);
  iupAndroid_CheckException(env, "IupTableHelper.setCellImage");
  (*env)->DeleteLocalRef(env, cls);
}

void iupdrvTableSetColTitle(Ihandle* ih, int col, const char* title)
{
  if (!ih->handle) return;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTableFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "setColTitle", "(Landroid/view/View;ILjava/lang/String;)V");
  jstring j_text = title ? (*env)->NewStringUTF(env, title) : NULL;
  (*env)->CallStaticVoidMethod(env, cls, m, ih->handle, (jint)col, j_text);
  iupAndroid_CheckException(env, "IupTableHelper.setColTitle");
  if (j_text) (*env)->DeleteLocalRef(env, j_text);
  (*env)->DeleteLocalRef(env, cls);
}

char* iupdrvTableGetColTitle(Ihandle* ih, int col)
{
  if (!ih->handle) return NULL;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTableFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "getColTitle", "(Landroid/view/View;I)Ljava/lang/String;");
  jstring j_text = (jstring)(*env)->CallStaticObjectMethod(env, cls, m, ih->handle, (jint)col);
  iupAndroid_CheckException(env, "IupTableHelper.getColTitle");
  char* result = iupAndroid_JStringToReturnStr(env, j_text);
  (*env)->DeleteLocalRef(env, cls);
  return result;
}

void iupdrvTableSetColWidth(Ihandle* ih, int col, int width)
{
  if (!ih->handle) return;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTableFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "setColWidth", "(Landroid/view/View;II)V");
  (*env)->CallStaticVoidMethod(env, cls, m, ih->handle, (jint)col, (jint)width);
  iupAndroid_CheckException(env, "IupTableHelper.setColWidth");
  (*env)->DeleteLocalRef(env, cls);
}

int iupdrvTableGetColWidth(Ihandle* ih, int col)
{
  if (!ih->handle) return 0;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTableFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "getColWidth", "(Landroid/view/View;I)I");
  jint w = (*env)->CallStaticIntMethod(env, cls, m, ih->handle, (jint)col);
  iupAndroid_CheckException(env, "IupTableHelper.getColWidth");
  (*env)->DeleteLocalRef(env, cls);
  return (int)w;
}

void iupdrvTableSetFocusCell(Ihandle* ih, int lin, int col)
{
  if (!ih->handle) return;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTableFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "setFocusCell", "(Landroid/view/View;II)V");
  (*env)->CallStaticVoidMethod(env, cls, m, ih->handle, (jint)lin, (jint)col);
  iupAndroid_CheckException(env, "IupTableHelper.setFocusCell");
  (*env)->DeleteLocalRef(env, cls);
}

void iupdrvTableGetFocusCell(Ihandle* ih, int* lin, int* col)
{
  if (lin) *lin = 1;
  if (col) *col = 1;
  if (!ih->handle) return;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTableFindClass(env);
  jmethodID ml = (*env)->GetStaticMethodID(env, cls, "getFocusCellLin", "(Landroid/view/View;)I");
  jint l = (*env)->CallStaticIntMethod(env, cls, ml, ih->handle);
  jmethodID mc = (*env)->GetStaticMethodID(env, cls, "getFocusCellCol", "(Landroid/view/View;)I");
  jint c = (*env)->CallStaticIntMethod(env, cls, mc, ih->handle);
  iupAndroid_CheckException(env, "IupTableHelper.getFocusCell");
  (*env)->DeleteLocalRef(env, cls);
  if (lin) *lin = (int)l;
  if (col) *col = (int)c;
}

void iupdrvTableScrollToCell(Ihandle* ih, int lin, int col)
{
  if (!ih->handle) return;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTableFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "scrollToCell", "(Landroid/view/View;II)V");
  (*env)->CallStaticVoidMethod(env, cls, m, ih->handle, (jint)lin, (jint)col);
  iupAndroid_CheckException(env, "IupTableHelper.scrollToCell");
  (*env)->DeleteLocalRef(env, cls);
}

void iupdrvTableRedraw(Ihandle* ih)
{
  androidTableCallVoid(ih, "redraw", "(Landroid/view/View;)V");
}

void iupdrvTableSetShowGrid(Ihandle* ih, int show)
{
  if (!ih->handle) return;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTableFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "setShowGrid", "(Landroid/view/View;Z)V");
  (*env)->CallStaticVoidMethod(env, cls, m, ih->handle, (jboolean)(show ? JNI_TRUE : JNI_FALSE));
  iupAndroid_CheckException(env, "IupTableHelper.setShowGrid");
  (*env)->DeleteLocalRef(env, cls);
}

int iupdrvTableGetBorderWidth(Ihandle* ih)
{
  (void)ih;
  return 0;
}

int iupdrvTableGetRowHeight(Ihandle* ih)
{
  if (ih->handle)
    return androidTableCallInt(ih, "getRowHeight", "(Landroid/view/View;)I");

  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTableFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "defaultRowHeightPx", "()I");
  jint r = (*env)->CallStaticIntMethod(env, cls, m);
  iupAndroid_CheckException(env, "IupTableHelper.defaultRowHeightPx");
  (*env)->DeleteLocalRef(env, cls);
  return (int)r;
}

int iupdrvTableGetHeaderHeight(Ihandle* ih)
{
  if (ih->handle)
    return androidTableCallInt(ih, "getHeaderHeight", "(Landroid/view/View;)I");

  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTableFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "defaultHeaderHeightPx", "()I");
  jint r = (*env)->CallStaticIntMethod(env, cls, m);
  iupAndroid_CheckException(env, "IupTableHelper.defaultHeaderHeightPx");
  (*env)->DeleteLocalRef(env, cls);
  return (int)r;
}

void iupdrvTableAddBorders(Ihandle* ih, int* w, int* h)
{
  (void)ih;
  (void)w;
  (void)h;
}

static int androidTableParseRgb(const char* value, int* r, int* g, int* b)
{
  unsigned char rr, gg, bb;
  if (!value || !iupStrToRGB(value, &rr, &gg, &bb))
  {
    *r = -1; *g = -1; *b = -1;
    return 0;
  }
  *r = rr; *g = gg; *b = bb;
  return 1;
}

static void androidTableCallRgb(Ihandle* ih, const char* method, int r, int g, int b)
{
  if (!ih->handle) return;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTableFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, method, "(Landroid/view/View;III)V");
  (*env)->CallStaticVoidMethod(env, cls, m, ih->handle, (jint)r, (jint)g, (jint)b);
  iupAndroid_CheckException(env, method);
  (*env)->DeleteLocalRef(env, cls);
}

static void androidTableCallRgbId2(Ihandle* ih, const char* method, int lin, int col, int r, int g, int b)
{
  if (!ih->handle) return;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTableFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, method, "(Landroid/view/View;IIIII)V");
  (*env)->CallStaticVoidMethod(env, cls, m, ih->handle, (jint)lin, (jint)col, (jint)r, (jint)g, (jint)b);
  iupAndroid_CheckException(env, method);
  (*env)->DeleteLocalRef(env, cls);
}

static int androidTableSetBgColorId2Attrib(Ihandle* ih, int lin, int col, const char* value)
{
  int r, g, b;
  androidTableParseRgb(value, &r, &g, &b);
  androidTableCallRgbId2(ih, "setBgColor", lin, col, r, g, b);
  return 1;
}

static int androidTableSetFgColorId2Attrib(Ihandle* ih, int lin, int col, const char* value)
{
  int r, g, b;
  androidTableParseRgb(value, &r, &g, &b);
  androidTableCallRgbId2(ih, "setFgColor", lin, col, r, g, b);
  return 1;
}

static int androidTableSetFontId2Attrib(Ihandle* ih, int lin, int col, const char* value)
{
  if (!ih->handle) return 1;

  const char* family = NULL;
  int style = IUPANDROID_TYPEFACE_NORMAL;
  int size_unit = IUPANDROID_COMPLEX_UNIT_SP;
  float size_value = 0.0f;

  char typeface[1024] = "";
  int size = 0, is_bold = 0, is_italic = 0, is_underline = 0, is_strikeout = 0;
  int parsed = 0;
  if (value && value[0])
    parsed = iupGetFontInfo(value, typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout);

  if (parsed)
  {
    family = typeface[0] ? typeface : NULL;
    if (is_bold && is_italic) style = IUPANDROID_TYPEFACE_BOLD_ITALIC;
    else if (is_bold)         style = IUPANDROID_TYPEFACE_BOLD;
    else if (is_italic)       style = IUPANDROID_TYPEFACE_ITALIC;
    if (size < 0) { size_unit = IUPANDROID_COMPLEX_UNIT_PX; size_value = (float)(-size); }
    else          { size_unit = IUPANDROID_COMPLEX_UNIT_SP; size_value = (float)size; }
  }

  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTableFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "setCellFont", "(Landroid/view/View;IILjava/lang/String;IIF)V");
  jstring j_family = family ? (*env)->NewStringUTF(env, family) : NULL;
  (*env)->CallStaticVoidMethod(env, cls, m, ih->handle, (jint)lin, (jint)col, j_family, (jint)style, (jint)size_unit, (jfloat)size_value);
  iupAndroid_CheckException(env, "IupTableHelper.setCellFont");
  if (j_family) (*env)->DeleteLocalRef(env, j_family);
  (*env)->DeleteLocalRef(env, cls);
  return 1;
}

static int androidTableSetAlignmentIdAttrib(Ihandle* ih, int col, const char* value)
{
  if (!ih->handle) return 1;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTableFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "setColAlignment", "(Landroid/view/View;ILjava/lang/String;)V");
  jstring j_value = value ? (*env)->NewStringUTF(env, value) : NULL;
  (*env)->CallStaticVoidMethod(env, cls, m, ih->handle, (jint)col, j_value);
  iupAndroid_CheckException(env, "IupTableHelper.setColAlignment");
  if (j_value) (*env)->DeleteLocalRef(env, j_value);
  (*env)->DeleteLocalRef(env, cls);
  return 1;
}

static int androidTableSetAlternateColorAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle) return 1;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTableFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "setAlternateColor", "(Landroid/view/View;Z)V");
  (*env)->CallStaticVoidMethod(env, cls, m, ih->handle, (jboolean)(iupStrBoolean(value) ? JNI_TRUE : JNI_FALSE));
  iupAndroid_CheckException(env, "IupTableHelper.setAlternateColor");
  (*env)->DeleteLocalRef(env, cls);
  return 1;
}

static int androidTableSetEvenRowColorAttrib(Ihandle* ih, const char* value)
{
  int r, g, b;
  if (!androidTableParseRgb(value, &r, &g, &b)) return 1;
  androidTableCallRgb(ih, "setEvenRowColor", r, g, b);
  return 1;
}

static int androidTableSetOddRowColorAttrib(Ihandle* ih, const char* value)
{
  int r, g, b;
  if (!androidTableParseRgb(value, &r, &g, &b)) return 1;
  androidTableCallRgb(ih, "setOddRowColor", r, g, b);
  return 1;
}

static int androidTableSetFgColorAttrib(Ihandle* ih, const char* value)
{
  int r, g, b;
  if (!androidTableParseRgb(value, &r, &g, &b)) return 1;
  androidTableCallRgb(ih, "setDefaultFgColor", r, g, b);
  return 1;
}

static int androidTableSetBgColorAttrib(Ihandle* ih, const char* value)
{
  int r, g, b;
  if (!androidTableParseRgb(value, &r, &g, &b)) return 1;
  androidTableCallRgb(ih, "setDefaultBgColor", r, g, b);
  return 1;
}

static int androidTableSetEditableAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle) return 1;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTableFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "setEditableAll", "(Landroid/view/View;Z)V");
  (*env)->CallStaticVoidMethod(env, cls, m, ih->handle, (jboolean)(iupStrBoolean(value) ? JNI_TRUE : JNI_FALSE));
  iupAndroid_CheckException(env, "IupTableHelper.setEditableAll");
  (*env)->DeleteLocalRef(env, cls);
  return 1;
}

static void androidTableCallBool(Ihandle* ih, const char* method, int v)
{
  if (!ih->handle) return;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTableFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, method, "(Landroid/view/View;Z)V");
  (*env)->CallStaticVoidMethod(env, cls, m, ih->handle, (jboolean)(v ? JNI_TRUE : JNI_FALSE));
  iupAndroid_CheckException(env, method);
  (*env)->DeleteLocalRef(env, cls);
}

static char* androidTableGetSortableAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->sortable);
}

static int androidTableSetSortableAttrib(Ihandle* ih, const char* value)
{
  int on = iupStrBoolean(value);
  ih->data->sortable = on;
  androidTableCallBool(ih, "setSortable", on);
  return 0;
}

static char* androidTableGetUserResizeAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->user_resize);
}

static int androidTableSetUserResizeAttrib(Ihandle* ih, const char* value)
{
  int on = iupStrBoolean(value);
  ih->data->user_resize = on;
  androidTableCallBool(ih, "setUserResize", on);
  return 0;
}

static char* androidTableGetAllowReorderAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->allow_reorder);
}

static int androidTableSetAllowReorderAttrib(Ihandle* ih, const char* value)
{
  int on = iupStrBoolean(value);
  ih->data->allow_reorder = on;
  androidTableCallBool(ih, "setAllowReorder", on);
  return 0;
}

static int androidTableSetVirtualModeAttrib(Ihandle* ih, const char* value)
{
  androidTableCallBool(ih, "setVirtualMode", iupStrBoolean(value));
  return 1;
}

static int androidTableSetFocusRectAttrib(Ihandle* ih, const char* value)
{
  androidTableCallBool(ih, "setFocusRect", iupStrBoolean(value));
  return 1;
}

static int androidTableSetEditableIdAttrib(Ihandle* ih, int col, const char* value)
{
  if (!ih->handle) return 1;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTableFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "setEditableCol", "(Landroid/view/View;IZ)V");
  (*env)->CallStaticVoidMethod(env, cls, m, ih->handle, (jint)col, (jboolean)(iupStrBoolean(value) ? JNI_TRUE : JNI_FALSE));
  iupAndroid_CheckException(env, "IupTableHelper.setEditableCol");
  (*env)->DeleteLocalRef(env, cls);
  return 1;
}

static void androidTableApplyImageFlags(Ihandle* ih)
{
  if (!ih->handle) return;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTableFindClass(env);
  jmethodID ms = (*env)->GetStaticMethodID(env, cls, "setShowImage", "(Landroid/view/View;Z)V");
  (*env)->CallStaticVoidMethod(env, cls, ms, ih->handle, (jboolean)(ih->data->show_image ? JNI_TRUE : JNI_FALSE));
  jmethodID mf = (*env)->GetStaticMethodID(env, cls, "setFitImage", "(Landroid/view/View;Z)V");
  (*env)->CallStaticVoidMethod(env, cls, mf, ih->handle, (jboolean)(ih->data->fit_image ? JNI_TRUE : JNI_FALSE));
  iupAndroid_CheckException(env, "IupTableHelper.setShowImage/setFitImage");
  (*env)->DeleteLocalRef(env, cls);
}

static void androidTableReplayStoredColumns(Ihandle* ih)
{
  int c;
  for (c = 1; c <= ih->data->num_col; c++)
  {
    char name[64];
    char* v;

    snprintf(name, sizeof(name), "TITLE%d", c);
    v = iupAttribGet(ih, name);
    if (v) iupdrvTableSetColTitle(ih, c, v);

    snprintf(name, sizeof(name), "RASTERWIDTH%d", c);
    v = iupAttribGet(ih, name);
    if (!v)
    {
      snprintf(name, sizeof(name), "WIDTH%d", c);
      v = iupAttribGet(ih, name);
    }
    if (v)
    {
      int width;
      if (iupStrToInt(v, &width) && width > 0)
        iupdrvTableSetColWidth(ih, c, width);
    }

    snprintf(name, sizeof(name), "ALIGNMENT%d", c);
    v = iupAttribGet(ih, name);
    if (v) androidTableSetAlignmentIdAttrib(ih, c, v);

    snprintf(name, sizeof(name), "EDITABLE%d", c);
    v = iupAttribGet(ih, name);
    if (v) androidTableSetEditableIdAttrib(ih, c, v);
  }
}

static void androidTableReplayStoredCells(Ihandle* ih)
{
  int l, c;
  char* v;

  for (c = 1; c <= ih->data->num_col; c++)
  {
    v = iupAttribGetId2(ih, "BGCOLOR", 0, c);
    if (v) androidTableSetBgColorId2Attrib(ih, 0, c, v);
    v = iupAttribGetId2(ih, "FGCOLOR", 0, c);
    if (v) androidTableSetFgColorId2Attrib(ih, 0, c, v);
    v = iupAttribGetId2(ih, "FONT", 0, c);
    if (v) androidTableSetFontId2Attrib(ih, 0, c, v);
  }

  for (l = 1; l <= ih->data->num_lin; l++)
  {
    v = iupAttribGetId2(ih, "BGCOLOR", l, 0);
    if (v) androidTableSetBgColorId2Attrib(ih, l, 0, v);
    v = iupAttribGetId2(ih, "FGCOLOR", l, 0);
    if (v) androidTableSetFgColorId2Attrib(ih, l, 0, v);
    v = iupAttribGetId2(ih, "FONT", l, 0);
    if (v) androidTableSetFontId2Attrib(ih, l, 0, v);
  }

  for (l = 1; l <= ih->data->num_lin; l++)
  {
    for (c = 1; c <= ih->data->num_col; c++)
    {
      v = iupAttribGetId2(ih, "", l, c);
      if (v) iupdrvTableSetCellValue(ih, l, c, v);

      v = iupAttribGetId2(ih, "BGCOLOR", l, c);
      if (v) androidTableSetBgColorId2Attrib(ih, l, c, v);

      v = iupAttribGetId2(ih, "FGCOLOR", l, c);
      if (v) androidTableSetFgColorId2Attrib(ih, l, c, v);

      v = iupAttribGetId2(ih, "FONT", l, c);
      if (v) androidTableSetFontId2Attrib(ih, l, c, v);

      if (ih->data->show_image)
      {
        char name[64];
        snprintf(name, sizeof(name), "IMAGE%d:%d", l, c);
        v = iupAttribGet(ih, name);
        if (v) iupdrvTableSetCellImage(ih, l, c, v);
      }
    }
  }
}

static int androidTableMapMethod(Ihandle* ih)
{
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTableFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "createTable", "(J)Landroid/view/View;");
  jobject widget = (*env)->CallStaticObjectMethod(env, cls, m, (jlong)(intptr_t)ih);
  iupAndroid_CheckException(env, "IupTableHelper.createTable");
  (*env)->DeleteLocalRef(env, cls);
  if (!widget) return IUP_ERROR;

  ih->handle = (jobject)((*env)->NewGlobalRef(env, widget));
  (*env)->DeleteLocalRef(env, widget);

  int num_col = ih->data->num_col;
  int num_lin = ih->data->num_lin;
  if (num_col > 0) iupdrvTableSetNumCol(ih, num_col);
  if (num_lin > 0) iupdrvTableSetNumLin(ih, num_lin);
  androidTableReplayStoredColumns(ih);

  char* v = iupAttribGet(ih, "SHOWGRID");
  if (v) iupdrvTableSetShowGrid(ih, iupStrBoolean(v));

  androidTableApplyImageFlags(ih);

  v = iupAttribGet(ih, "EDITABLE");
  if (v) androidTableSetEditableAttrib(ih, v);

  if (ih->data->sortable) androidTableCallBool(ih, "setSortable", 1);
  if (ih->data->user_resize) androidTableCallBool(ih, "setUserResize", 1);
  if (ih->data->allow_reorder) androidTableCallBool(ih, "setAllowReorder", 1);
  if (iupAttribGetBoolean(ih, "VIRTUALMODE")) androidTableCallBool(ih, "setVirtualMode", 1);
  v = iupAttribGet(ih, "FOCUSRECT");
  if (v) androidTableCallBool(ih, "setFocusRect", iupStrBoolean(v));
  androidTableCallBool(ih, "setStretchLast", ih->data->stretch_last);

  v = iupAttribGet(ih, "BGCOLOR");
  if (v) androidTableSetBgColorAttrib(ih, v);
  v = iupAttribGet(ih, "FGCOLOR");
  if (v) androidTableSetFgColorAttrib(ih, v);
  v = iupAttribGet(ih, "ALTERNATECOLOR");
  if (v) androidTableSetAlternateColorAttrib(ih, v);
  v = iupAttribGet(ih, "EVENROWCOLOR");
  if (v) androidTableSetEvenRowColorAttrib(ih, v);
  v = iupAttribGet(ih, "ODDROWCOLOR");
  if (v) androidTableSetOddRowColorAttrib(ih, v);

  androidTableReplayStoredCells(ih);

  v = iupAttribGet(ih, "FOCUSCELL");
  if (v)
  {
    int l, c;
    if (iupStrToIntInt(v, &l, &c, ':') == 2)
      iupdrvTableSetFocusCell(ih, l, c);
  }

  {
    jclass cls2 = androidTableFindClass(env);
    m = (*env)->GetStaticMethodID(env, cls2, "autoSizeColumns", "(Landroid/view/View;)V");
    (*env)->CallStaticVoidMethod(env, cls2, m, ih->handle);
    iupAndroid_CheckException(env, "IupTableHelper.autoSizeColumns");
    (*env)->DeleteLocalRef(env, cls2);
  }

  iupAndroid_AddWidgetToParent(env, ih);
  return IUP_NOERROR;
}

void iupdrvTableInitClass(Iclass* ic)
{
  ic->Map = androidTableMapMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;

  iupClassRegisterAttributeId2(ic, "BGCOLOR", NULL, androidTableSetBgColorId2Attrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId2(ic, "FGCOLOR", NULL, androidTableSetFgColorId2Attrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId2(ic, "FONT", NULL, androidTableSetFontId2Attrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "ALIGNMENT", NULL, androidTableSetAlignmentIdAttrib, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, androidTableSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, androidTableSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "ALTERNATECOLOR", NULL, androidTableSetAlternateColorAttrib, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "EVENROWCOLOR", NULL, androidTableSetEvenRowColorAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ODDROWCOLOR", NULL, androidTableSetOddRowColorAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "EDITABLE", NULL, androidTableSetEditableAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "EDITABLE", NULL, androidTableSetEditableIdAttrib, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "SORTABLE", androidTableGetSortableAttrib, androidTableSetSortableAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "USERRESIZE", androidTableGetUserResizeAttrib, androidTableSetUserResizeAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ALLOWREORDER", androidTableGetAllowReorderAttrib, androidTableSetAllowReorderAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VIRTUALMODE", NULL, androidTableSetVirtualModeAttrib, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FOCUSRECT", NULL, androidTableSetFocusRectAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
}
