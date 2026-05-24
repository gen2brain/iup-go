/** \file
 * \brief List Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdint.h>

#include <jni.h>
#include <android/log.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drvfont.h"
#include "iup_mask.h"
#include "iup_image.h"
#include "iup_list.h"

#include "iupandroid_drv.h"
#include "iupandroid_jnimacros.h"


IUPJNI_DECLARE_CLASS_STATIC(IupListHelper);

void* iupAndroidListGetMask(Ihandle* ih)
{
  if (!ih || !ih->data) return NULL;
  return ih->data->mask;
}

static int androidListPreferredRowHeight(void)
{
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupListHelper, jni_env, "io/github/gen2brain/iupgo/IupListHelper");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "getPreferredRowHeightPx", "()I");
  jint h = (*jni_env)->CallStaticIntMethod(jni_env, java_class, method_id);
  iupAndroid_CheckException(jni_env, "IupListHelper.getPreferredRowHeightPx");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return (int)h;
}

static int androidListEditBoxHeight(void)
{
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupListHelper, jni_env, "io/github/gen2brain/iupgo/IupListHelper");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "getEditBoxHeightPx", "()I");
  jint h = (*jni_env)->CallStaticIntMethod(jni_env, java_class, method_id);
  iupAndroid_CheckException(jni_env, "IupListHelper.getEditBoxHeightPx");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return (int)h;
}

void iupdrvListAddItemSpace(Ihandle* ih, int* h)
{
  (void)ih;
  /* Floor to measured row height; VISIBLELINES * row then fits exactly. */
  if (h)
  {
    int pref = androidListPreferredRowHeight();
    if (*h < pref) *h = pref;
  }
}

void iupdrvListAddBorders(Ihandle* ih, int* x, int* y)
{
  int extra_x, extra_y;
  if (ih->data->is_dropdown)
  {
    /* Material3 ExposedDropdown TIL chrome (stroke + content padding + MACTV padding + chevron). */
    JNIEnv* env = iupAndroid_GetEnvThreadSafe();
    jclass cls = IUPJNI_FindClass(IupListHelper, env, "io/github/gen2brain/iupgo/IupListHelper");
    jmethodID mh = (*env)->GetStaticMethodID(env, cls, "getDropdownBorderH", "()I");
    jmethodID mv = (*env)->GetStaticMethodID(env, cls, "getDropdownBorderV", "()I");
    extra_x = (int)(*env)->CallStaticIntMethod(env, cls, mh);
    extra_y = (int)(*env)->CallStaticIntMethod(env, cls, mv);
    (*env)->DeleteLocalRef(env, cls);
  }
  else
  {
    extra_x = iupAndroid_DpToPx(32.0f);
    extra_y = 0;  /* any slack shows a partial next row */
  }
  if (x) *x += extra_x;
  if (y) *y += extra_y;

  /* core uses maximg_w in raw px; we render scaled-to-row-height and need extra slack for the gap */
  if (x && ih->data->show_image && (ih->data->maximg_h > 0 || iupAttribGetBoolean(ih, "DROPTARGET")))
  {
    JNIEnv* env2 = iupAndroid_GetEnvThreadSafe();
    jclass cls2 = IUPJNI_FindClass(IupListHelper, env2, "io/github/gen2brain/iupgo/IupListHelper");
    jmethodID mh = (*env2)->GetStaticMethodID(env2, cls2, "getRowImageHeightPx", "()I");
    jmethodID mp = (*env2)->GetStaticMethodID(env2, cls2, "getRowIconPaddingPx", "()I");
    int target_h = (int)(*env2)->CallStaticIntMethod(env2, cls2, mh);
    int icon_pad = (int)(*env2)->CallStaticIntMethod(env2, cls2, mp);
    (*env2)->DeleteLocalRef(env2, cls2);
    int rendered_w = (ih->data->maximg_h > 0)
      ? (ih->data->fit_image ? ih->data->maximg_w * target_h / ih->data->maximg_h : ih->data->maximg_w)
      : target_h;
    int delta = rendered_w - ih->data->maximg_w;
    if (delta < 0) delta = 0;
    *x += delta + icon_pad + iupAndroid_DpToPx(8.0f);
  }

  /* EDITBOX non-dropdown: VISIBLELINES counts the entry; swap one row for it. */
  if (ih->data->has_editbox && !ih->data->is_dropdown && y)
  {
    int visiblelines = iupAttribGetInt(ih, "VISIBLELINES");
    if (visiblelines > 0)
    {
      int char_w, char_h;
      iupdrvFontGetCharSize(ih, &char_w, &char_h);
      int item_h = char_h;
      iupdrvListAddItemSpace(ih, &item_h);
      *y -= item_h;
    }
    *y += androidListEditBoxHeight();
  }
}

int iupdrvListGetCount(Ihandle* ih)
{
  if (!ih->handle) return 0;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupListHelper, jni_env, "io/github/gen2brain/iupgo/IupListHelper");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "getCount", "(Landroid/view/View;)I");
  jint count = (*jni_env)->CallStaticIntMethod(jni_env, java_class, method_id, ih->handle);
  iupAndroid_CheckException(jni_env, "IupListHelper.getCount");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return (int)count;
}

static void androidListCallStringAtPos(Ihandle* ih, const char* method, const char* sig, int pos, const char* text)
{
  if (!ih->handle) return;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupListHelper, jni_env, "io/github/gen2brain/iupgo/IupListHelper");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, method, sig);
  jstring j_text = text ? (*jni_env)->NewStringUTF(jni_env, text) : NULL;
  if (pos < 0)
    (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, ih->handle, j_text);
  else
    (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, ih->handle, (jint)pos, j_text);
  iupAndroid_CheckException(jni_env, method);
  if (j_text) (*jni_env)->DeleteLocalRef(jni_env, j_text);
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
}

void iupdrvListAppendItem(Ihandle* ih, const char* value)
{
  androidListCallStringAtPos(ih, "appendItem", "(Landroid/view/View;Ljava/lang/String;)V", -1, value);
}

void iupdrvListInsertItem(Ihandle* ih, int pos, const char* value)
{
  androidListCallStringAtPos(ih, "insertItem", "(Landroid/view/View;ILjava/lang/String;)V", pos, value);
}

void iupdrvListRemoveItem(Ihandle* ih, int pos)
{
  if (!ih->handle) return;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupListHelper, jni_env, "io/github/gen2brain/iupgo/IupListHelper");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "removeItem", "(Landroid/view/View;I)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, ih->handle, (jint)pos);
  iupAndroid_CheckException(jni_env, "IupListHelper.removeItem");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
}

void iupdrvListRemoveAllItems(Ihandle* ih)
{
  if (!ih->handle) return;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupListHelper, jni_env, "io/github/gen2brain/iupgo/IupListHelper");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "removeAll", "(Landroid/view/View;)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, ih->handle);
  iupAndroid_CheckException(jni_env, "IupListHelper.removeAll");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
}

/* JNI local ref, consumed in the same frame by iupdrvListSetImageHandle. */
void* iupdrvListGetImageHandle(Ihandle* ih, int id)
{
  if (!ih->handle) return NULL;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupListHelper, jni_env, "io/github/gen2brain/iupgo/IupListHelper");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "getItemBitmap", "(Landroid/view/View;I)Landroid/graphics/Bitmap;");
  jobject bmp = (*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id, ih->handle, (jint)(id - 1));
  iupAndroid_CheckException(jni_env, "IupListHelper.getItemBitmap");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return (void*)bmp;
}

static char* androidListGetImageNativeHandleAttribId(Ihandle* ih, int id)
{
  return (char*)iupdrvListGetImageHandle(ih, id);
}

int iupdrvListSetImageHandle(Ihandle* ih, int id, void* hImage)
{
  if (!ih->handle) return 0;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupListHelper, jni_env, "io/github/gen2brain/iupgo/IupListHelper");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "setItemImage", "(Landroid/view/View;ILandroid/graphics/Bitmap;Z)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, ih->handle, (jint)id, (jobject)hImage, (jboolean)(ih->data->fit_image ? JNI_TRUE : JNI_FALSE));
  iupAndroid_CheckException(jni_env, "IupListHelper.setItemImage");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return 1;
}

static int androidListSetImageAttrib(Ihandle* ih, int id, const char* value)
{
  void* h_image = iupImageGetImage(value, ih, 0, NULL);
  int pos = iupListGetPosAttrib(ih, id);
  if (pos < 0) pos = id - 1;
  iupdrvListSetImageHandle(ih, pos, h_image);
  return 1;
}

void iupdrvListSetItemCount(Ihandle* ih, int count)
{
  if (!ih->handle) return;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupListHelper, jni_env, "io/github/gen2brain/iupgo/IupListHelper");
  jmethodID m = (*jni_env)->GetStaticMethodID(jni_env, cls, "setItemCount", "(Landroid/view/View;JI)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, cls, m, ih->handle, (jlong)(intptr_t)ih, (jint)count);
  iupAndroid_CheckException(jni_env, "IupListHelper.setItemCount");
  (*jni_env)->DeleteLocalRef(jni_env, cls);
}

static int androidListSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b)) return 0;
  if (!ih->handle) return 1;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupListHelper, jni_env, "io/github/gen2brain/iupgo/IupListHelper");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "setTextColor", "(Landroid/view/View;III)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, ih->handle, (jint)r, (jint)g, (jint)b);
  iupAndroid_CheckException(jni_env, "IupListHelper.setTextColor");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return 1;
}

static int androidListSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b)) return 0;
  if (!ih->handle) return 1;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupListHelper, jni_env, "io/github/gen2brain/iupgo/IupListHelper");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "setBgColor", "(Landroid/view/View;III)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, ih->handle, (jint)r, (jint)g, (jint)b);
  iupAndroid_CheckException(jni_env, "IupListHelper.setBgColor");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return 1;
}

static char* androidListGetIdValueAttrib(Ihandle* ih, int pos)
{
  if (!ih->handle || pos < 1) return NULL;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupListHelper, jni_env, "io/github/gen2brain/iupgo/IupListHelper");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "getItem", "(Landroid/view/View;I)Ljava/lang/String;");
  jstring j_text = (jstring)(*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id, ih->handle, (jint)(pos - 1));
  iupAndroid_CheckException(jni_env, "IupListHelper.getItem");
  char* result = iupAndroid_JStringToReturnStr(jni_env, j_text);
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return result;
}

static int androidListSetValueAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle) return 1;
  if (!value) return 0;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupListHelper, jni_env, "io/github/gen2brain/iupgo/IupListHelper");

  if (ih->data->has_editbox)
  {
    /* EDITBOX: VALUE is entry text, not index. */
    jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "setEditBoxText", "(Landroid/view/View;Ljava/lang/String;)V");
    jstring j_text = (*jni_env)->NewStringUTF(jni_env, value);
    (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, ih->handle, j_text);
    iupAndroid_CheckException(jni_env, "IupListHelper.setEditBoxText");
    (*jni_env)->DeleteLocalRef(jni_env, j_text);
  }
  else if (ih->data->is_multiple && !ih->data->is_dropdown)
  {
    jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "setMultipleSelection", "(Landroid/view/View;Ljava/lang/String;)V");
    jstring j_mask = (*jni_env)->NewStringUTF(jni_env, value);
    (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, ih->handle, j_mask);
    iupAndroid_CheckException(jni_env, "IupListHelper.setMultipleSelection");
    (*jni_env)->DeleteLocalRef(jni_env, j_mask);
  }
  else
  {
    int pos;
    if (!iupStrToInt(value, &pos)) pos = 0;
    /* Seed OLDVALUE so the async onItemSelected following setSelection is recognised as no-change. */
    if (pos > 0) iupAttribSetInt(ih, "_IUPLIST_OLDVALUE", pos);
    else iupAttribSet(ih, "_IUPLIST_OLDVALUE", NULL);
    jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "setSelection", "(Landroid/view/View;I)V");
    (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, ih->handle, (jint)pos);
    iupAndroid_CheckException(jni_env, "IupListHelper.setSelection");
  }

  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return 0;
}

void iupAndroidListDispatchSelection(Ihandle* ih, int item)
{
  /* Skip the no-change emit Spinner sends after setSelection. */
  int oldpos = 0;
  char* old_str = iupAttribGet(ih, "_IUPLIST_OLDVALUE");
  if (old_str) iupStrToInt(old_str, &oldpos);
  if (oldpos == item) return;

  IFnsii cb = (IFnsii)IupGetCallback(ih, "ACTION");
  if (cb)
    iupListSingleCallActionCb(ih, cb, item);
  else
    iupAttribSetInt(ih, "_IUPLIST_OLDVALUE", item);

  IFn changed_cb = (IFn)IupGetCallback(ih, "VALUECHANGED_CB");
  if (changed_cb && changed_cb(ih) == IUP_CLOSE) IupExitLoop();
}

static char* androidListGetValueAttrib(Ihandle* ih)
{
  if (!ih->handle) return NULL;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupListHelper, jni_env, "io/github/gen2brain/iupgo/IupListHelper");

  char* result = NULL;
  if (ih->data->has_editbox)
  {
    jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "getEditBoxText", "(Landroid/view/View;)Ljava/lang/String;");
    jstring j_text = (jstring)(*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id, ih->handle);
    iupAndroid_CheckException(jni_env, "IupListHelper.getEditBoxText");
    result = iupAndroid_JStringToReturnStr(jni_env, j_text);
  }
  else if (ih->data->is_multiple && !ih->data->is_dropdown)
  {
    jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "getMultipleSelection", "(Landroid/view/View;)Ljava/lang/String;");
    jstring j_mask = (jstring)(*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id, ih->handle);
    iupAndroid_CheckException(jni_env, "IupListHelper.getMultipleSelection");
    result = iupAndroid_JStringToReturnStr(jni_env, j_mask);
  }
  else
  {
    jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "getSelection", "(Landroid/view/View;)I");
    jint pos = (*jni_env)->CallStaticIntMethod(jni_env, java_class, method_id, ih->handle);
    iupAndroid_CheckException(jni_env, "IupListHelper.getSelection");
    result = iupStrReturnInt((int)pos);
  }

  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return result;
}

static int androidListSetFastScrollAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle || ih->data->is_dropdown) return 1;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupListHelper, jni_env, "io/github/gen2brain/iupgo/IupListHelper");
  jmethodID m = (*jni_env)->GetStaticMethodID(jni_env, cls, "setFastScrollEnabled", "(Landroid/view/View;Z)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, cls, m, (jobject)ih->handle, (jboolean)(iupStrBoolean(value) ? JNI_TRUE : JNI_FALSE));
  iupAndroid_CheckException(jni_env, "IupListHelper.setFastScrollEnabled");
  (*jni_env)->DeleteLocalRef(jni_env, cls);
  return 1;
}

static int androidListSetTopItemAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle) return 1;
  if (ih->data->is_dropdown || !value) return 0;
  int pos = 1;
  if (!iupStrToInt(value, &pos) || pos < 1) return 0;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupListHelper, jni_env, "io/github/gen2brain/iupgo/IupListHelper");
  jmethodID m = (*jni_env)->GetStaticMethodID(jni_env, cls, "setTopItem", "(Landroid/view/View;I)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, cls, m, (jobject)ih->handle, (jint)pos);
  iupAndroid_CheckException(jni_env, "IupListHelper.setTopItem");
  (*jni_env)->DeleteLocalRef(jni_env, cls);
  return 0;
}

static int androidListGetEditSelectionImpl(Ihandle* ih, int* start, int* end)
{
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupListHelper, jni_env, "io/github/gen2brain/iupgo/IupListHelper");
  jmethodID m = (*jni_env)->GetStaticMethodID(jni_env, cls, "getEditSelection", "(Landroid/view/View;)J");
  jlong packed = (*jni_env)->CallStaticLongMethod(jni_env, cls, m, (jobject)ih->handle);
  iupAndroid_CheckException(jni_env, "IupListHelper.getEditSelection");
  (*jni_env)->DeleteLocalRef(jni_env, cls);
  if (packed == -1LL) return 0;
  *start = (int) ((packed >> 32) & 0xFFFFFFFFLL);
  *end = (int) (packed & 0xFFFFFFFFLL);
  return 1;
}

static void androidListSetEditSelectionImpl(Ihandle* ih, int start, int end)
{
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupListHelper, jni_env, "io/github/gen2brain/iupgo/IupListHelper");
  jmethodID m = (*jni_env)->GetStaticMethodID(jni_env, cls, "setEditSelection", "(Landroid/view/View;II)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, cls, m, (jobject)ih->handle, (jint)start, (jint)end);
  iupAndroid_CheckException(jni_env, "IupListHelper.setEditSelection");
  (*jni_env)->DeleteLocalRef(jni_env, cls);
}

static int androidListSetSelectionAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle) return 1;
  if (!ih->data->has_editbox) return 0;
  int start, end;
  if (!value || iupStrEqualNoCase(value, "NONE")) { start = 0; end = 0; }
  else if (iupStrEqualNoCase(value, "ALL")) { start = 0; end = -1; }
  else
  {
    start = 1; end = 1;
    if (iupStrToIntInt(value, &start, &end, ':') != 2) return 0;
    if (start < 1 || end < 1) return 0;
    start--; end--;
  }
  androidListSetEditSelectionImpl(ih, start, end);
  return 0;
}

static char* androidListGetSelectionAttrib(Ihandle* ih)
{
  if (!ih->handle || !ih->data->has_editbox) return NULL;
  int start = 0, end = 0;
  if (!androidListGetEditSelectionImpl(ih, &start, &end)) return NULL;
  return iupStrReturnIntInt(start + 1, end + 1, ':');
}

static int androidListSetSelectionPosAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle) return 1;
  if (!ih->data->has_editbox) return 0;
  int start, end;
  if (!value || iupStrEqualNoCase(value, "NONE")) { start = 0; end = 0; }
  else if (iupStrEqualNoCase(value, "ALL")) { start = 0; end = -1; }
  else
  {
    start = 0; end = 0;
    if (iupStrToIntInt(value, &start, &end, ':') != 2) return 0;
    if (start < 0 || end < 0) return 0;
  }
  androidListSetEditSelectionImpl(ih, start, end);
  return 0;
}

static char* androidListGetSelectionPosAttrib(Ihandle* ih)
{
  if (!ih->handle || !ih->data->has_editbox) return NULL;
  int start = 0, end = 0;
  if (!androidListGetEditSelectionImpl(ih, &start, &end)) return NULL;
  return iupStrReturnIntInt(start, end, ':');
}

/* _IUP_XY2POS_CB: pixel y -> 1-based item index, -1 if none. */
static int androidListConvertXYToPos(Ihandle* ih, int x, int y)
{
  if (!ih->handle) return -1;
  if (ih->data->is_dropdown) return -1;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupListHelper, jni_env, "io/github/gen2brain/iupgo/IupListHelper");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "pointToPosition", "(Landroid/view/View;II)I");
  jint pos = (*jni_env)->CallStaticIntMethod(jni_env, java_class, method_id, ih->handle, (jint)x, (jint)y);
  iupAndroid_CheckException(jni_env, "IupListHelper.pointToPosition");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  if (pos < 0) return -1;
  return (int)pos + 1;  /* 1-based for IUP */
}

static int androidListMapMethod(Ihandle* ih)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupListHelper_createListView);
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupListHelper_createDropdownView);

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupListHelper, jni_env, "io/github/gen2brain/iupgo/IupListHelper");

  jobject widget;
  if (ih->data->is_dropdown)
  {
    jmethodID method_id = IUPJNI_GetStaticMethodID(IupListHelper_createDropdownView, jni_env, java_class, "createDropdownView", "(JZ)Landroid/view/View;");
    widget = (*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id, (jlong)(intptr_t)ih,
      (jboolean)(ih->data->has_editbox ? JNI_TRUE : JNI_FALSE));
  }
  else
  {
    jmethodID method_id = IUPJNI_GetStaticMethodID(IupListHelper_createListView, jni_env, java_class, "createListView", "(JZZZ)Landroid/view/View;");
    widget = (*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id, (jlong)(intptr_t)ih,
      (jboolean)(ih->data->is_multiple ? JNI_TRUE : JNI_FALSE),
      (jboolean)(ih->data->has_editbox ? JNI_TRUE : JNI_FALSE),
      (jboolean)(ih->data->sb ? JNI_TRUE : JNI_FALSE));
  }
  iupAndroid_CheckException(jni_env, "IupListHelper.createX");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);

  if (!widget) return IUP_ERROR;

  ih->handle = (jobject)((*jni_env)->NewGlobalRef(jni_env, widget));
  (*jni_env)->DeleteLocalRef(jni_env, widget);

  iupAndroid_AddWidgetToParent(jni_env, ih);

  IupSetCallback(ih, "_IUP_XY2POS_CB", (Icallback)androidListConvertXYToPos);

  /* Replay pre-map items stored in "" attribs. */
  iupListSetInitialItems(ih);

  /* VIRTUALMODE: ITEMCOUNT setter is a no-op pre-map; apply it now. */
  if (ih->data->is_virtual && ih->data->item_count > 0)
    iupdrvListSetItemCount(ih, ih->data->item_count);

  /* VIRTUALMODE auto-promotes FASTSCROLL to YES; explicit values flow through the setter via attribute replay. */
  if (!ih->data->is_dropdown && ih->data->is_virtual && !iupAttribGet(ih, "FASTSCROLL"))
    androidListSetFastScrollAttrib(ih, "YES");

  return IUP_NOERROR;
}

void iupdrvListInitClass(Iclass* ic)
{
  ic->Map = androidListMapMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;

  iupClassRegisterAttribute(ic, "VALUE", androidListGetValueAttrib, androidListSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  iupClassRegisterAttributeId(ic, "IDVALUE", androidListGetIdValueAttrib, iupListSetIdValueAttrib, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, androidListSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, androidListSetFgColorAttrib, "DLGFGCOLOR", NULL, IUPAF_DEFAULT);

  iupClassRegisterAttributeId(ic, "IMAGE", NULL, androidListSetImageAttrib, IUPAF_IHANDLENAME|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "IMAGENATIVEHANDLE", androidListGetImageNativeHandleAttribId, NULL, IUPAF_NO_STRING|IUPAF_READONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "TOPITEM", NULL, androidListSetTopItemAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FASTSCROLL", NULL, androidListSetFastScrollAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTION", androidListGetSelectionAttrib, androidListSetSelectionAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTIONPOS", androidListGetSelectionPosAttrib, androidListSetSelectionPosAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "SHOWDRAGDROP", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPACING", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VISIBLEITEMS", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  /* ListView scrollbars are transient overlays; no always-visible mode */
  iupClassRegisterAttribute(ic, "AUTOHIDE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
}
