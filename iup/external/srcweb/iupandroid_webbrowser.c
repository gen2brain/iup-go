/** \file
 * \brief Android Web Browser Control (android.webkit.WebView).
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <jni.h>

#include "iup.h"
#include "iupweb.h"
#include "iupcbs.h"

#include "iup_class.h"
#include "iup_object.h"
#include "iup_register.h"
#include "iup_attrib.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_str.h"

#include "iupandroid_drv.h"
#include "iupandroid_jnimacros.h"


IUPJNI_DECLARE_CLASS_STATIC(IupWebBrowserHelper);


static jclass androidWebBrowserClass(JNIEnv* jni_env)
{
  return (jclass)IUPJNI_FindClass(IupWebBrowserHelper, jni_env, "io/github/gen2brain/iupgo/IupWebBrowserHelper");
}


static int androidWebBrowserMapMethod(Ihandle* ih)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupWebBrowserHelper_create);

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = androidWebBrowserClass(jni_env);
  if (!java_class) return IUP_ERROR;

  jmethodID method_id = IUPJNI_GetStaticMethodID(IupWebBrowserHelper_create, jni_env, java_class,
      "create", "(J)Lio/github/gen2brain/iupgo/IupWebBrowserHelper$IupWebView;");

  jobject view = (*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id, (jlong)(intptr_t)ih);
  iupAndroid_CheckException(jni_env, "IupWebBrowserHelper.create");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);

  if (!view)
    return IUP_ERROR;

  ih->handle = (jobject)((*jni_env)->NewGlobalRef(jni_env, view));
  (*jni_env)->DeleteLocalRef(jni_env, view);

  iupAndroid_AddWidgetToParent(jni_env, ih);
  return IUP_NOERROR;
}

static void androidWebBrowserUnMapMethod(Ihandle* ih)
{
  if (!ih || !ih->handle) return;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  iupAndroid_RemoveFromParent(jni_env, ih);
  iupAndroid_ReleaseIhandle(jni_env, ih);
}


static void androidWebBrowserCallVoidStr(Ihandle* ih, jmethodID* cache_slot, const char* method_name, const char* method_sig, const char* value)
{
  if (!ih->handle) return;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = androidWebBrowserClass(jni_env);
  if (!java_class) return;

  if (*cache_slot == 0)
    *cache_slot = (*jni_env)->GetStaticMethodID(jni_env, java_class, method_name, method_sig);

  if (*cache_slot)
  {
    jstring j_value = value ? (*jni_env)->NewStringUTF(jni_env, value) : NULL;
    (*jni_env)->CallStaticVoidMethod(jni_env, java_class, *cache_slot, ih->handle, j_value);
    iupAndroid_CheckException(jni_env, method_name);
    if (j_value) (*jni_env)->DeleteLocalRef(jni_env, j_value);
  }
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
}

static void androidWebBrowserCallVoid(Ihandle* ih, jmethodID* cache_slot, const char* method_name, const char* method_sig)
{
  if (!ih->handle) return;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = androidWebBrowserClass(jni_env);
  if (!java_class) return;

  if (*cache_slot == 0)
    *cache_slot = (*jni_env)->GetStaticMethodID(jni_env, java_class, method_name, method_sig);

  if (*cache_slot)
  {
    (*jni_env)->CallStaticVoidMethod(jni_env, java_class, *cache_slot, ih->handle);
    iupAndroid_CheckException(jni_env, method_name);
  }
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
}

static void androidWebBrowserCallVoidInt(Ihandle* ih, jmethodID* cache_slot, const char* method_name, const char* method_sig, int arg)
{
  if (!ih->handle) return;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = androidWebBrowserClass(jni_env);
  if (!java_class) return;

  if (*cache_slot == 0)
    *cache_slot = (*jni_env)->GetStaticMethodID(jni_env, java_class, method_name, method_sig);

  if (*cache_slot)
  {
    (*jni_env)->CallStaticVoidMethod(jni_env, java_class, *cache_slot, ih->handle, (jint)arg);
    iupAndroid_CheckException(jni_env, method_name);
  }
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
}

static int androidWebBrowserCallInt(Ihandle* ih, jmethodID* cache_slot, const char* method_name, const char* method_sig)
{
  if (!ih->handle) return 0;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = androidWebBrowserClass(jni_env);
  if (!java_class) return 0;

  int result = 0;
  if (*cache_slot == 0)
    *cache_slot = (*jni_env)->GetStaticMethodID(jni_env, java_class, method_name, method_sig);

  if (*cache_slot)
  {
    result = (int)(*jni_env)->CallStaticIntMethod(jni_env, java_class, *cache_slot, ih->handle);
    iupAndroid_CheckException(jni_env, method_name);
  }
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return result;
}

static char* androidWebBrowserCallStringWithInt(Ihandle* ih, jmethodID* cache_slot, const char* method_name, const char* method_sig, int arg)
{
  if (!ih->handle) return NULL;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = androidWebBrowserClass(jni_env);
  if (!java_class) return NULL;

  char* result = NULL;
  if (*cache_slot == 0)
    *cache_slot = (*jni_env)->GetStaticMethodID(jni_env, java_class, method_name, method_sig);

  if (*cache_slot)
  {
    jstring j_str = (jstring)(*jni_env)->CallStaticObjectMethod(jni_env, java_class, *cache_slot, ih->handle, (jint)arg);
    iupAndroid_CheckException(jni_env, method_name);
    result = iupAndroid_JStringToReturnStr(jni_env, j_str);
  }
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return result;
}

static int androidWebBrowserCallBool(Ihandle* ih, jmethodID* cache_slot, const char* method_name, const char* method_sig)
{
  if (!ih->handle) return 0;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = androidWebBrowserClass(jni_env);
  if (!java_class) return 0;

  int result = 0;
  if (*cache_slot == 0)
    *cache_slot = (*jni_env)->GetStaticMethodID(jni_env, java_class, method_name, method_sig);

  if (*cache_slot)
  {
    jboolean rc = (*jni_env)->CallStaticBooleanMethod(jni_env, java_class, *cache_slot, ih->handle);
    iupAndroid_CheckException(jni_env, method_name);
    result = rc ? 1 : 0;
  }
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return result;
}

static char* androidWebBrowserCallString(Ihandle* ih, jmethodID* cache_slot, const char* method_name, const char* method_sig)
{
  if (!ih->handle) return NULL;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = androidWebBrowserClass(jni_env);
  if (!java_class) return NULL;

  char* result = NULL;
  if (*cache_slot == 0)
    *cache_slot = (*jni_env)->GetStaticMethodID(jni_env, java_class, method_name, method_sig);

  if (*cache_slot)
  {
    jstring j_str = (jstring)(*jni_env)->CallStaticObjectMethod(jni_env, java_class, *cache_slot, ih->handle);
    iupAndroid_CheckException(jni_env, method_name);
    result = iupAndroid_JStringToReturnStr(jni_env, j_str);
  }
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return result;
}


static int androidWebBrowserSetValueAttrib(Ihandle* ih, const char* value)
{
  static jmethodID s_loadUrl = 0;
  if (!value) return 0;

  if (iupStrEqualPartial(value, "http://") || iupStrEqualPartial(value, "https://") ||
      iupStrEqualPartial(value, "file://") || iupStrEqualPartial(value, "ftp://"))
  {
    androidWebBrowserCallVoidStr(ih, &s_loadUrl, "loadUrl", "(Landroid/view/View;Ljava/lang/String;)V", value);
  }
  else
  {
    char* url = iupStrFileMakeURL(value);
    if (url)
    {
      androidWebBrowserCallVoidStr(ih, &s_loadUrl, "loadUrl", "(Landroid/view/View;Ljava/lang/String;)V", url);
      free(url);
    }
  }
  return 0;
}

static char* androidWebBrowserGetValueAttrib(Ihandle* ih)
{
  static jmethodID s_getUrl = 0;
  return androidWebBrowserCallString(ih, &s_getUrl, "getUrl", "(Landroid/view/View;)Ljava/lang/String;");
}

static char* androidWebBrowserGetStatusAttrib(Ihandle* ih)
{
  static jmethodID s_getStatus = 0;
  return androidWebBrowserCallString(ih, &s_getStatus, "getStatus", "(Landroid/view/View;)Ljava/lang/String;");
}

static int androidWebBrowserSetReloadAttrib(Ihandle* ih, const char* value)
{
  static jmethodID s_reload = 0;
  (void)value;
  androidWebBrowserCallVoid(ih, &s_reload, "reload", "(Landroid/view/View;)V");
  return 0;
}

static int androidWebBrowserSetStopAttrib(Ihandle* ih, const char* value)
{
  static jmethodID s_stop = 0;
  (void)value;
  androidWebBrowserCallVoid(ih, &s_stop, "stopLoading", "(Landroid/view/View;)V");
  return 0;
}

static int androidWebBrowserSetGoBackAttrib(Ihandle* ih, const char* value)
{
  static jmethodID s_goBack = 0;
  (void)value;
  androidWebBrowserCallVoid(ih, &s_goBack, "goBack", "(Landroid/view/View;)V");
  return 0;
}

static int androidWebBrowserSetGoForwardAttrib(Ihandle* ih, const char* value)
{
  static jmethodID s_goForward = 0;
  (void)value;
  androidWebBrowserCallVoid(ih, &s_goForward, "goForward", "(Landroid/view/View;)V");
  return 0;
}

static int androidWebBrowserSetBackForwardAttrib(Ihandle* ih, const char* value)
{
  static jmethodID s_goBackOrForward = 0;
  int steps = 0;
  if (!iupStrToInt(value, &steps) || steps == 0) return 0;
  androidWebBrowserCallVoidInt(ih, &s_goBackOrForward, "goBackOrForward", "(Landroid/view/View;I)V", steps);
  return 0;
}

static char* androidWebBrowserGetCanGoBackAttrib(Ihandle* ih)
{
  static jmethodID s_canGoBack = 0;
  return iupStrReturnBoolean(androidWebBrowserCallBool(ih, &s_canGoBack, "canGoBack", "(Landroid/view/View;)Z"));
}

static char* androidWebBrowserGetCanGoForwardAttrib(Ihandle* ih)
{
  static jmethodID s_canGoForward = 0;
  return iupStrReturnBoolean(androidWebBrowserCallBool(ih, &s_canGoForward, "canGoForward", "(Landroid/view/View;)Z"));
}


static char* androidWebBrowserEvalJS(Ihandle* ih, const char* js)
{
  static jmethodID s_evalJs = 0;
  if (!ih->handle || !js) return NULL;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = androidWebBrowserClass(jni_env);
  if (!java_class) return NULL;

  char* result = NULL;
  if (s_evalJs == 0)
    s_evalJs = (*jni_env)->GetStaticMethodID(jni_env, java_class, "evalJs", "(Landroid/view/View;Ljava/lang/String;)Ljava/lang/String;");

  if (s_evalJs)
  {
    jstring j_js = (*jni_env)->NewStringUTF(jni_env, js);
    jstring j_result = (jstring)(*jni_env)->CallStaticObjectMethod(jni_env, java_class, s_evalJs, ih->handle, j_js);
    iupAndroid_CheckException(jni_env, "IupWebBrowserHelper.evalJs");
    if (j_js) (*jni_env)->DeleteLocalRef(jni_env, j_js);
    result = iupAndroid_JStringToReturnStr(jni_env, j_result);
  }
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return result;
}

static void androidWebBrowserEvalJSVoid(Ihandle* ih, const char* js)
{
  static jmethodID s_evalJsVoid = 0;
  androidWebBrowserCallVoidStr(ih, &s_evalJsVoid, "evalJsVoid", "(Landroid/view/View;Ljava/lang/String;)V", js);
}

static int androidWebBrowserSetJavascriptAttrib(Ihandle* ih, const char* value)
{
  iupAttribSet(ih, "_IUPWEB_JS_RESULT", NULL);
  if (!value) return 0;
  char* result = androidWebBrowserEvalJS(ih, value);
  if (result) iupAttribSetStr(ih, "_IUPWEB_JS_RESULT", result);
  return 0;
}

static char* androidWebBrowserGetJavascriptAttrib(Ihandle* ih)
{
  return iupAttribGet(ih, "_IUPWEB_JS_RESULT");
}


static char* androidWebBrowserEscapeJS(const char* s)
{
  if (!s) return NULL;
  size_t n = strlen(s);
  char* out = (char*)malloc(n * 6 + 1);
  if (!out) return NULL;
  char* p = out;
  for (size_t i = 0; i < n; i++)
  {
    unsigned char c = (unsigned char)s[i];
    /* U+2028 / U+2029 terminate JS string literals; escape via \uXXXX */
    if (c == 0xE2 && i + 2 < n
        && (unsigned char)s[i+1] == 0x80
        && ((unsigned char)s[i+2] == 0xA8 || (unsigned char)s[i+2] == 0xA9))
    {
      snprintf(p, 7, "\\u202%c", (unsigned char)s[i+2] == 0xA8 ? '8' : '9');
      p += 6;
      i += 2;
      continue;
    }
    switch (c)
    {
      case '\\': *p++ = '\\'; *p++ = '\\'; break;
      case '\'': *p++ = '\\'; *p++ = '\'';  break;
      case '"':  *p++ = '\\'; *p++ = '"';   break;
      case '\n': *p++ = '\\'; *p++ = 'n';   break;
      case '\r': *p++ = '\\'; *p++ = 'r';   break;
      case '\t': *p++ = '\\'; *p++ = 't';   break;
      case '<':  *p++ = '\\'; *p++ = 'x'; *p++ = '3'; *p++ = 'c'; break;
      case '>':  *p++ = '\\'; *p++ = 'x'; *p++ = '3'; *p++ = 'e'; break;
      default:
        if (c < 0x20)
        {
          snprintf(p, 7, "\\u%04x", c);
          p += 6;
        }
        else *p++ = (char)c;
        break;
    }
  }
  *p = '\0';
  return out;
}

static char* androidWebBrowserGetInnerTextAttrib(Ihandle* ih)
{
  const char* id = iupAttribGet(ih, "ELEMENT_ID");
  if (!id) return NULL;
  char* id_esc = androidWebBrowserEscapeJS(id);
  if (!id_esc) return NULL;
  size_t sz = strlen(id_esc) + 96;
  char* js = (char*)malloc(sz);
  snprintf(js, sz, "(function(){var e=document.getElementById('%s');return e?e.innerText:null;})()", id_esc);
  char* result = androidWebBrowserEvalJS(ih, js);
  free(js);
  free(id_esc);
  return result;
}

static int androidWebBrowserSetInnerTextAttrib(Ihandle* ih, const char* value)
{
  const char* id = iupAttribGet(ih, "ELEMENT_ID");
  if (!id || !value) return 0;
  char* id_esc = androidWebBrowserEscapeJS(id);
  char* val_esc = androidWebBrowserEscapeJS(value);
  if (id_esc && val_esc)
  {
    size_t sz = strlen(id_esc) + strlen(val_esc) + 96;
    char* js = (char*)malloc(sz);
    snprintf(js, sz, "(function(){var e=document.getElementById('%s');if(e)e.innerText='%s';})()", id_esc, val_esc);
    androidWebBrowserEvalJSVoid(ih, js);
    free(js);
  }
  free(id_esc);
  free(val_esc);
  return 0;
}

static char* androidWebBrowserGetAttributeAttrib(Ihandle* ih)
{
  const char* id = iupAttribGet(ih, "ELEMENT_ID");
  const char* name = iupAttribGet(ih, "ATTRIBUTE_NAME");
  if (!id || !name) return NULL;
  char* id_esc = androidWebBrowserEscapeJS(id);
  char* name_esc = androidWebBrowserEscapeJS(name);
  char* result = NULL;
  if (id_esc && name_esc)
  {
    size_t sz = strlen(id_esc) + strlen(name_esc) + 96;
    char* js = (char*)malloc(sz);
    snprintf(js, sz, "(function(){var e=document.getElementById('%s');return e?e.getAttribute('%s'):null;})()", id_esc, name_esc);
    result = androidWebBrowserEvalJS(ih, js);
    free(js);
  }
  free(id_esc);
  free(name_esc);
  return result;
}

static int androidWebBrowserSetAttributeAttrib(Ihandle* ih, const char* value)
{
  const char* id = iupAttribGet(ih, "ELEMENT_ID");
  const char* name = iupAttribGet(ih, "ATTRIBUTE_NAME");
  if (!id || !name || !value) return 0;
  char* id_esc = androidWebBrowserEscapeJS(id);
  char* name_esc = androidWebBrowserEscapeJS(name);
  char* val_esc = androidWebBrowserEscapeJS(value);
  if (id_esc && name_esc && val_esc)
  {
    size_t sz = strlen(id_esc) + strlen(name_esc) + strlen(val_esc) + 96;
    char* js = (char*)malloc(sz);
    snprintf(js, sz, "(function(){var e=document.getElementById('%s');if(e)e.setAttribute('%s','%s');})()", id_esc, name_esc, val_esc);
    androidWebBrowserEvalJSVoid(ih, js);
    free(js);
  }
  free(id_esc);
  free(name_esc);
  free(val_esc);
  return 0;
}


static int androidWebBrowserSetEditableAttrib(Ihandle* ih, const char* value)
{
  static jmethodID s_setEditable = 0;
  int on = iupStrBoolean(value);
  if (!ih->handle) return 0;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = androidWebBrowserClass(jni_env);
  if (!java_class) return 0;
  if (s_setEditable == 0)
    s_setEditable = (*jni_env)->GetStaticMethodID(jni_env, java_class, "setEditable", "(Landroid/view/View;Z)V");
  if (s_setEditable)
  {
    (*jni_env)->CallStaticVoidMethod(jni_env, java_class, s_setEditable, ih->handle, on ? JNI_TRUE : JNI_FALSE);
    iupAndroid_CheckException(jni_env, "setEditable");
  }
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return 0;
}

static char* androidWebBrowserGetEditableAttrib(Ihandle* ih)
{
  static jmethodID s_getEditable = 0;
  return iupStrReturnBoolean(androidWebBrowserCallBool(ih, &s_getEditable, "getEditable", "(Landroid/view/View;)Z"));
}

static void androidWebBrowserExecCommand(Ihandle* ih, const char* cmd, const char* arg)
{
  char* cmd_esc = androidWebBrowserEscapeJS(cmd);
  char* arg_esc = arg ? androidWebBrowserEscapeJS(arg) : NULL;
  size_t sz = (cmd_esc ? strlen(cmd_esc) : 0) + (arg_esc ? strlen(arg_esc) : 0) + 128;
  char* js = (char*)malloc(sz);
  if (js && cmd_esc)
  {
    if (arg_esc)
      snprintf(js, sz, "document.execCommand('%s',false,'%s');", cmd_esc, arg_esc);
    else
      snprintf(js, sz, "document.execCommand('%s',false,null);", cmd_esc);
    androidWebBrowserEvalJSVoid(ih, js);
  }
  free(js);
  free(cmd_esc);
  free(arg_esc);
}

static char* androidWebBrowserQueryCommand(Ihandle* ih, const char* kind, const char* cmd)
{
  char* cmd_esc = androidWebBrowserEscapeJS(cmd);
  if (!cmd_esc) return NULL;
  size_t sz = strlen(kind) + strlen(cmd_esc) + 64;
  char* js = (char*)malloc(sz);
  char* result = NULL;
  if (js)
  {
    snprintf(js, sz, "''+document.queryCommand%s('%s')", kind, cmd_esc);
    result = androidWebBrowserEvalJS(ih, js);
  }
  free(js);
  free(cmd_esc);
  return result;
}

static int androidWebBrowserSetExecCommandAttrib(Ihandle* ih, const char* value)
{
  if (!value) return 0;
  androidWebBrowserExecCommand(ih, value, NULL);
  return 0;
}

static int androidWebBrowserSetCopyAttrib(Ihandle* ih, const char* value)
{
  (void)value; androidWebBrowserExecCommand(ih, "copy", NULL); return 0;
}

static int androidWebBrowserSetCutAttrib(Ihandle* ih, const char* value)
{
  (void)value; androidWebBrowserExecCommand(ih, "cut", NULL); return 0;
}

static int androidWebBrowserSetPasteAttrib(Ihandle* ih, const char* value)
{
  (void)value; androidWebBrowserExecCommand(ih, "paste", NULL); return 0;
}

static char* androidWebBrowserGetPasteAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(iupStrBoolean(androidWebBrowserQueryCommand(ih, "Enabled", "paste")));
}

static int androidWebBrowserSetSelectAllAttrib(Ihandle* ih, const char* value)
{
  (void)value; androidWebBrowserExecCommand(ih, "selectAll", NULL); return 0;
}

static int androidWebBrowserSetUndoAttrib(Ihandle* ih, const char* value)
{
  (void)value; androidWebBrowserExecCommand(ih, "undo", NULL); return 0;
}

static int androidWebBrowserSetRedoAttrib(Ihandle* ih, const char* value)
{
  (void)value; androidWebBrowserExecCommand(ih, "redo", NULL); return 0;
}

static int androidWebBrowserSetFontNameAttrib(Ihandle* ih, const char* value)
{
  if (value) androidWebBrowserExecCommand(ih, "fontName", value);
  return 0;
}

static char* androidWebBrowserGetFontNameAttrib(Ihandle* ih)
{
  return androidWebBrowserQueryCommand(ih, "Value", "fontName");
}

static int androidWebBrowserSetFontSizeAttrib(Ihandle* ih, const char* value)
{
  if (value) androidWebBrowserExecCommand(ih, "fontSize", value);
  return 0;
}

static char* androidWebBrowserGetFontSizeAttrib(Ihandle* ih)
{
  return androidWebBrowserQueryCommand(ih, "Value", "fontSize");
}

static int androidWebBrowserSetFormatBlockAttrib(Ihandle* ih, const char* value)
{
  if (value) androidWebBrowserExecCommand(ih, "formatBlock", value);
  return 0;
}

static char* androidWebBrowserGetFormatBlockAttrib(Ihandle* ih)
{
  return androidWebBrowserQueryCommand(ih, "Value", "formatBlock");
}

static int androidWebBrowserSetForeColorAttrib(Ihandle* ih, const char* value)
{
  if (value) androidWebBrowserExecCommand(ih, "foreColor", value);
  return 0;
}

static char* androidWebBrowserGetForeColorAttrib(Ihandle* ih)
{
  return androidWebBrowserQueryCommand(ih, "Value", "foreColor");
}

static int androidWebBrowserSetBackColorAttrib(Ihandle* ih, const char* value)
{
  if (value) androidWebBrowserExecCommand(ih, "backColor", value);
  return 0;
}

static char* androidWebBrowserGetBackColorAttrib(Ihandle* ih)
{
  return androidWebBrowserQueryCommand(ih, "Value", "backColor");
}

static int androidWebBrowserSetInsertImageAttrib(Ihandle* ih, const char* value)
{
  if (value) androidWebBrowserExecCommand(ih, "insertImage", value);
  return 0;
}

static int androidWebBrowserSetInsertImageFileAttrib(Ihandle* ih, const char* value)
{
  if (!value || !ih->handle) return 0;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = androidWebBrowserClass(jni_env);
  if (!java_class) return 0;

  static jmethodID s_imageFileToDataUri = 0;
  if (s_imageFileToDataUri == 0)
    s_imageFileToDataUri = (*jni_env)->GetStaticMethodID(jni_env, java_class, "imageFileToDataUri", "(Ljava/lang/String;)Ljava/lang/String;");

  if (s_imageFileToDataUri)
  {
    jstring j_path = (*jni_env)->NewStringUTF(jni_env, value);
    jstring j_uri = (jstring)(*jni_env)->CallStaticObjectMethod(jni_env, java_class, s_imageFileToDataUri, j_path);
    iupAndroid_CheckException(jni_env, "imageFileToDataUri");
    if (j_path) (*jni_env)->DeleteLocalRef(jni_env, j_path);
    if (j_uri)
    {
      const char* data_uri = (*jni_env)->GetStringUTFChars(jni_env, j_uri, NULL);
      androidWebBrowserExecCommand(ih, "insertImage", data_uri);
      (*jni_env)->ReleaseStringUTFChars(jni_env, j_uri, data_uri);
      (*jni_env)->DeleteLocalRef(jni_env, j_uri);
    }
  }
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return 0;
}

static int androidWebBrowserSetCreateLinkAttrib(Ihandle* ih, const char* value)
{
  if (value) androidWebBrowserExecCommand(ih, "createLink", value);
  return 0;
}

static int androidWebBrowserSetInsertTextAttrib(Ihandle* ih, const char* value)
{
  if (value) androidWebBrowserExecCommand(ih, "insertText", value);
  return 0;
}

static int androidWebBrowserSetInsertHtmlAttrib(Ihandle* ih, const char* value)
{
  if (value) androidWebBrowserExecCommand(ih, "insertHTML", value);
  return 0;
}

static char* androidWebBrowserGetCommandStateAttrib(Ihandle* ih)
{
  const char* cmd = iupAttribGet(ih, "COMMAND");
  if (!cmd) return NULL;
  return androidWebBrowserQueryCommand(ih, "State", cmd);
}

static char* androidWebBrowserGetCommandEnabledAttrib(Ihandle* ih)
{
  const char* cmd = iupAttribGet(ih, "COMMAND");
  if (!cmd) return NULL;
  return androidWebBrowserQueryCommand(ih, "Enabled", cmd);
}

static char* androidWebBrowserGetCommandValueAttrib(Ihandle* ih)
{
  const char* cmd = iupAttribGet(ih, "COMMAND");
  if (!cmd) return NULL;
  return androidWebBrowserQueryCommand(ih, "Value", cmd);
}

static char* androidWebBrowserGetCommandTextAttrib(Ihandle* ih)
{
  /* queryCommandText is obsolete in every modern engine; mirror GTK/Cocoa. */
  const char* cmd = iupAttribGet(ih, "COMMAND");
  if (!cmd) return NULL;
  return androidWebBrowserQueryCommand(ih, "Text", cmd);
}

static char* androidWebBrowserGetDirtyAttrib(Ihandle* ih)
{
  char* v = androidWebBrowserEvalJS(ih, "window.__iupDirty?'1':'0'");
  if (!v) return "NO";
  int dirty = (v[0] == '1');
  return iupStrReturnBoolean(dirty);
}

static int androidWebBrowserSetNewAttrib(Ihandle* ih, const char* value)
{
  static jmethodID s_new = 0;
  (void)value;
  androidWebBrowserCallVoid(ih, &s_new, "newDoc", "(Landroid/view/View;)V");
  return 0;
}

static int androidWebBrowserSetOpenFileAttrib(Ihandle* ih, const char* value)
{
  static jmethodID s_open = 0;
  if (!value) return 0;
  androidWebBrowserCallVoidStr(ih, &s_open, "openFile", "(Landroid/view/View;Ljava/lang/String;)V", value);
  return 0;
}

static int androidWebBrowserSetSaveFileAttrib(Ihandle* ih, const char* value)
{
  static jmethodID s_save = 0;
  if (!value || !ih->handle) return 0;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = androidWebBrowserClass(jni_env);
  if (!java_class) return 0;
  if (s_save == 0)
    s_save = (*jni_env)->GetStaticMethodID(jni_env, java_class, "saveFile", "(Landroid/view/View;Ljava/lang/String;)Z");
  if (s_save)
  {
    jstring j_path = (*jni_env)->NewStringUTF(jni_env, value);
    (*jni_env)->CallStaticBooleanMethod(jni_env, java_class, s_save, ih->handle, j_path);
    iupAndroid_CheckException(jni_env, "saveFile");
    if (j_path) (*jni_env)->DeleteLocalRef(jni_env, j_path);
  }
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return 0;
}


static int androidWebBrowserSetFindAttrib(Ihandle* ih, const char* value)
{
  static jmethodID s_find = 0;
  androidWebBrowserCallVoidStr(ih, &s_find, "find", "(Landroid/view/View;Ljava/lang/String;)V", value ? value : "");
  return 0;
}

static int androidWebBrowserSetPrintAttrib(Ihandle* ih, const char* value)
{
  static jmethodID s_print = 0;
  const char* title = iupAttribGet(IupGetDialog(ih), "TITLE");
  androidWebBrowserCallVoidStr(ih, &s_print, "print", "(Landroid/view/View;Ljava/lang/String;)V", title ? title : (value ? value : "IupWebBrowser"));
  return 0;
}

static int androidWebBrowserSetPrintPreviewAttrib(Ihandle* ih, const char* value)
{
  return androidWebBrowserSetPrintAttrib(ih, value);
}


static int androidWebBrowserSetHTMLAttrib(Ihandle* ih, const char* value)
{
  static jmethodID s_loadHtml = 0;
  if (!value) return 0;
  androidWebBrowserCallVoidStr(ih, &s_loadHtml, "loadHtml", "(Landroid/view/View;Ljava/lang/String;)V", value);
  return 0;
}

static char* androidWebBrowserGetHTMLAttrib(Ihandle* ih)
{
  return androidWebBrowserEvalJS(ih, "document.documentElement.outerHTML");
}


static int androidWebBrowserSetZoomAttrib(Ihandle* ih, const char* value)
{
  static jmethodID s_setZoom = 0;
  int zoom = 100;
  if (!iupStrToInt(value, &zoom)) return 0;
  androidWebBrowserCallVoidInt(ih, &s_setZoom, "setZoom", "(Landroid/view/View;I)V", zoom);
  return 0;
}

static char* androidWebBrowserGetZoomAttrib(Ihandle* ih)
{
  static jmethodID s_getZoom = 0;
  return iupStrReturnInt(androidWebBrowserCallInt(ih, &s_getZoom, "getZoom", "(Landroid/view/View;)I"));
}


static char* androidWebBrowserGetBackCountAttrib(Ihandle* ih)
{
  static jmethodID s_backCount = 0;
  return iupStrReturnInt(androidWebBrowserCallInt(ih, &s_backCount, "backCount", "(Landroid/view/View;)I"));
}

static char* androidWebBrowserGetForwardCountAttrib(Ihandle* ih)
{
  static jmethodID s_forwardCount = 0;
  return iupStrReturnInt(androidWebBrowserCallInt(ih, &s_forwardCount, "forwardCount", "(Landroid/view/View;)I"));
}

static char* androidWebBrowserGetItemHistoryAttrib(Ihandle* ih, int id)
{
  static jmethodID s_itemHistory = 0;
  return androidWebBrowserCallStringWithInt(ih, &s_itemHistory, "itemHistory", "(Landroid/view/View;I)Ljava/lang/String;", id);
}


static void androidWebBrowserComputeNaturalSizeMethod(Ihandle* ih, int* w, int* h, int* children_expand)
{
  int cw = 0, ch = 0;
  (void)children_expand;
  iupdrvFontGetCharSize(ih, &cw, &ch);
  *w = cw;
  *h = ch;
}

static int androidWebBrowserCreateMethod(Ihandle* ih, void** params)
{
  (void)params;
  ih->expand = IUP_EXPAND_BOTH;
  return IUP_NOERROR;
}

Iclass* iupWebBrowserNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "webbrowser";
  ic->cons = "WebBrowser";
  ic->format = NULL;
  ic->nativetype = IUP_TYPECONTROL;
  ic->childtype  = IUP_CHILDNONE;
  ic->is_interactive = 1;
  ic->has_attrib_id = 1;

  ic->New    = NULL;
  ic->Create = androidWebBrowserCreateMethod;
  ic->Map    = androidWebBrowserMapMethod;
  ic->UnMap  = androidWebBrowserUnMapMethod;
  ic->ComputeNaturalSize = androidWebBrowserComputeNaturalSizeMethod;
  ic->LayoutUpdate = iupdrvBaseLayoutUpdateMethod;

  iupClassRegisterCallback(ic, "NEWWINDOW_CB",  "s");
  iupClassRegisterCallback(ic, "NAVIGATE_CB",   "s");
  iupClassRegisterCallback(ic, "ERROR_CB",      "s");
  iupClassRegisterCallback(ic, "COMPLETED_CB",  "s");
  iupClassRegisterCallback(ic, "UPDATE_CB",     "");

  iupBaseRegisterCommonAttrib(ic);
  iupBaseRegisterVisualAttrib(ic);

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "VALUE",        androidWebBrowserGetValueAttrib,     androidWebBrowserSetValueAttrib,     NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STATUS",       androidWebBrowserGetStatusAttrib,    NULL,                                NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "RELOAD",       NULL, androidWebBrowserSetReloadAttrib,       NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STOP",         NULL, androidWebBrowserSetStopAttrib,         NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "GOBACK",       NULL, androidWebBrowserSetGoBackAttrib,       NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "GOFORWARD",    NULL, androidWebBrowserSetGoForwardAttrib,    NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKFORWARD",  NULL, androidWebBrowserSetBackForwardAttrib,  NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CANGOBACK",    androidWebBrowserGetCanGoBackAttrib,    NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CANGOFORWARD", androidWebBrowserGetCanGoForwardAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "HTML",         androidWebBrowserGetHTMLAttrib, androidWebBrowserSetHTMLAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ZOOM",         androidWebBrowserGetZoomAttrib, androidWebBrowserSetZoomAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "BACKCOUNT",    androidWebBrowserGetBackCountAttrib,    NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORWARDCOUNT", androidWebBrowserGetForwardCountAttrib, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "ITEMHISTORY", androidWebBrowserGetItemHistoryAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "JAVASCRIPT",     androidWebBrowserGetJavascriptAttrib, androidWebBrowserSetJavascriptAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ELEMENT_ID",     NULL, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INNERTEXT",      androidWebBrowserGetInnerTextAttrib, androidWebBrowserSetInnerTextAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ATTRIBUTE_NAME", NULL, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ATTRIBUTE",      androidWebBrowserGetAttributeAttrib, androidWebBrowserSetAttributeAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "EDITABLE",       androidWebBrowserGetEditableAttrib, androidWebBrowserSetEditableAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NEW",            NULL, androidWebBrowserSetNewAttrib,      NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "OPENFILE",       NULL, androidWebBrowserSetOpenFileAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SAVEFILE",       NULL, androidWebBrowserSetSaveFileAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "UNDO",           NULL, androidWebBrowserSetUndoAttrib,      NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "REDO",           NULL, androidWebBrowserSetRedoAttrib,      NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COPY",           NULL, androidWebBrowserSetCopyAttrib,      NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CUT",            NULL, androidWebBrowserSetCutAttrib,       NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PASTE",          androidWebBrowserGetPasteAttrib, androidWebBrowserSetPasteAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTALL",      NULL, androidWebBrowserSetSelectAllAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "EXECCOMMAND",    NULL, androidWebBrowserSetExecCommandAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERTIMAGE",    NULL, androidWebBrowserSetInsertImageAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERTIMAGEFILE",NULL, androidWebBrowserSetInsertImageFileAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CREATELINK",     NULL, androidWebBrowserSetCreateLinkAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERTTEXT",     NULL, androidWebBrowserSetInsertTextAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERTHTML",     NULL, androidWebBrowserSetInsertHtmlAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "FONTNAME",       androidWebBrowserGetFontNameAttrib, androidWebBrowserSetFontNameAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FONTSIZE",       androidWebBrowserGetFontSizeAttrib, androidWebBrowserSetFontSizeAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATBLOCK",    androidWebBrowserGetFormatBlockAttrib, androidWebBrowserSetFormatBlockAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORECOLOR",      androidWebBrowserGetForeColorAttrib, androidWebBrowserSetForeColorAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKCOLOR",      androidWebBrowserGetBackColorAttrib, androidWebBrowserSetBackColorAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "COMMAND",        NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDSHOWUI",  NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDSTATE",   androidWebBrowserGetCommandStateAttrib,   NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDENABLED", androidWebBrowserGetCommandEnabledAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDVALUE",   androidWebBrowserGetCommandValueAttrib,   NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDTEXT",    androidWebBrowserGetCommandTextAttrib,    NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "DIRTY",          androidWebBrowserGetDirtyAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "FIND",           NULL, androidWebBrowserSetFindAttrib,         NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PRINT",          NULL, androidWebBrowserSetPrintAttrib,        NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PRINTPREVIEW",   NULL, androidWebBrowserSetPrintPreviewAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  return ic;
}
