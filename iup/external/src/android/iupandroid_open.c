/** \file
 * \brief Android Driver Core
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include <android/log.h>

#include "iup.h"

#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iup_globalattrib.h"
#include "iup_thread.h"

#include "iupandroid_drv.h"
#include "iupandroid_jnimacros.h"
#include "iupandroid_jnicacheglobals.h"


void* iupdrvGetDisplay(void)
{
  return NULL;
}

static void* iupAndroid_open_mutex = NULL;

void iupAndroid_OpenInit(void)
{
  iupAndroid_open_mutex = iupdrvMutexCreate();
}

int iupAndroid_OpenOnce(void)
{
  iupdrvMutexLock(iupAndroid_open_mutex);
  int ret = IupIsOpened() ? IUP_OPENED : IupOpen(0, NULL);
  iupdrvMutexUnlock(iupAndroid_open_mutex);
  return ret;
}

/* go through the JNI_OnLoad cache; FindClass from Go's OS-locked thread can't see app classes */
static int iupAndroidQueryDarkMode(void)
{
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupCommon, env, "io/github/gen2brain/iupgo/IupCommon");
  if (!cls) return 0;
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "isDarkMode", "()Z");
  jboolean dark = (*env)->CallStaticBooleanMethod(env, cls, m);
  iupAndroid_CheckException(env, "IupCommon.isDarkMode");
  (*env)->DeleteLocalRef(env, cls);
  return dark ? 1 : 0;
}

static int iupAndroidParseColorToArgb(const char* rgb)
{
  int r = 0, g = 0, b = 0;
  if (!rgb) return 0xFF000000;
  if (sscanf(rgb, "%d %d %d", &r, &g, &b) != 3) return 0xFF000000;
  return (int)(0xFF000000u | ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (b & 0xFF));
}

/* Resolves Material3 ?attr/<name> at runtime (e.g. "colorPrimary"); ARGB out. */
static int iupAndroidResolveThemeColor(const char* attr_name, int fallback_argb)
{
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass ic = IUPJNI_FindClass(IupCommon, env, "io/github/gen2brain/iupgo/IupCommon");
  if (!ic) return fallback_argb;
  int color = fallback_argb;
  jmethodID m = (*env)->GetStaticMethodID(env, ic, "resolveThemeColorByName", "(Ljava/lang/String;I)I");
  if (m)
  {
    jstring j_name = (*env)->NewStringUTF(env, attr_name);
    color = (*env)->CallStaticIntMethod(env, ic, m, j_name, (jint)fallback_argb);
    iupAndroid_CheckException(env, "IupCommon.resolveThemeColorByName");
    (*env)->DeleteLocalRef(env, j_name);
  }
  (*env)->DeleteLocalRef(env, ic);
  return color;
}

/* drop cached ContextThemeWrapper so getTheme() picks up the new DayNight resources */
static void iupAndroidResetContextTheme(void)
{
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupCommon, env, "io/github/gen2brain/iupgo/IupCommon");
  if (!cls) return;
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "resetContextThemeWrapper", "()V");
  if (m) (*env)->CallStaticVoidMethod(env, cls, m);
  iupAndroid_CheckException(env, "IupCommon.resetContextThemeWrapper");
  (*env)->DeleteLocalRef(env, cls);
}

static void iupAndroidPushPaletteToJava(int dark)
{
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupCommon, env, "io/github/gen2brain/iupgo/IupCommon");
  if (!cls) return;
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "setThemePalette", "(IIIIIIIIIZ)V");
  if (m)
  {
    int dlgBg  = iupAndroidParseColorToArgb(IupGetGlobal("DLGBGCOLOR"));
    int dlgFg  = iupAndroidParseColorToArgb(IupGetGlobal("DLGFGCOLOR"));
    int txtBg  = iupAndroidParseColorToArgb(IupGetGlobal("TXTBGCOLOR"));
    int txtFg  = iupAndroidParseColorToArgb(IupGetGlobal("TXTFGCOLOR"));
    int menuBg = iupAndroidParseColorToArgb(IupGetGlobal("MENUBGCOLOR"));
    int menuFg = iupAndroidParseColorToArgb(IupGetGlobal("MENUFGCOLOR"));
    int txtHlBg = iupAndroidParseColorToArgb(IupGetGlobal("TXTHLCOLOR"));
    int txtHlFg = iupAndroidParseColorToArgb(IupGetGlobal("TXTHLFGCOLOR"));
    int accent  = iupAndroidParseColorToArgb(IupGetGlobal("ACCENTCOLOR"));
    (*env)->CallStaticVoidMethod(env, cls, m,
      (jint)dlgBg, (jint)dlgFg, (jint)txtBg, (jint)txtFg, (jint)menuBg, (jint)menuFg,
      (jint)txtHlBg, (jint)txtHlFg, (jint)accent,
      dark ? JNI_TRUE : JNI_FALSE);
    iupAndroid_CheckException(env, "IupCommon.setThemePalette");
  }
  (*env)->DeleteLocalRef(env, cls);
}

void iupAndroidUpdateGlobalColors(void)
{
  int dark = iupAndroidQueryDarkMode();
  iupAndroidResetContextTheme();

  /* Material3 theme drives the surface/text/menu palette; hardcoded RGBs are fallbacks only. */
  int surf_argb     = iupAndroidResolveThemeColor("colorSurface",          dark ? 0xFF2B2B2B : 0xFFEDEDED);
  int on_surf_argb  = iupAndroidResolveThemeColor("colorOnSurface",        dark ? 0xFFE6E6E6 : 0xFF000000);
  int txt_bg_argb   = surf_argb;
  int menu_bg_argb  = iupAndroidResolveThemeColor("colorSurfaceContainer", dark ? 0xFF2B2B2B : 0xFFB7B7B7);

#define IUP_RGB_FROM_ARGB(name, src) \
  int name##_r = ((src) >> 16) & 0xFF, name##_g = ((src) >> 8) & 0xFF, name##_b = (src) & 0xFF

  IUP_RGB_FROM_ARGB(surf,    surf_argb);
  IUP_RGB_FROM_ARGB(on_surf, on_surf_argb);
  IUP_RGB_FROM_ARGB(txt_bg,  txt_bg_argb);
  IUP_RGB_FROM_ARGB(menu_bg, menu_bg_argb);

  iupGlobalSetDefaultColorAttrib("DLGBGCOLOR",  surf_r,    surf_g,    surf_b);
  iupGlobalSetDefaultColorAttrib("DLGFGCOLOR",  on_surf_r, on_surf_g, on_surf_b);
  iupGlobalSetDefaultColorAttrib("TXTBGCOLOR",  txt_bg_r,  txt_bg_g,  txt_bg_b);
  iupGlobalSetDefaultColorAttrib("TXTFGCOLOR",  on_surf_r, on_surf_g, on_surf_b);
  iupGlobalSetDefaultColorAttrib("MENUBGCOLOR", menu_bg_r, menu_bg_g, menu_bg_b);
  iupGlobalSetDefaultColorAttrib("MENUFGCOLOR", on_surf_r, on_surf_g, on_surf_b);

  int accent_argb = iupAndroidResolveThemeColor("colorAccent",    dark ? 0xFFD0BCFF : 0xFF6750A4);
  int hl_argb     = iupAndroidResolveThemeColor("colorPrimary",   accent_argb);
  int hl_fg_argb  = iupAndroidResolveThemeColor("colorOnPrimary", dark ? 0xFF381E72 : 0xFFFFFFFF);

  IUP_RGB_FROM_ARGB(hl,    hl_argb);
  IUP_RGB_FROM_ARGB(hf,    hl_fg_argb);
  IUP_RGB_FROM_ARGB(ac,    accent_argb);

  iupGlobalSetDefaultColorAttrib("TXTHLCOLOR",   hl_r, hl_g, hl_b);
  iupGlobalSetDefaultColorAttrib("TXTHLFGCOLOR", hf_r, hf_g, hf_b);
  iupGlobalSetDefaultColorAttrib("ACCENTCOLOR",  ac_r, ac_g, ac_b);
  iupGlobalSetDefaultColorAttrib("LINKFGCOLOR",  ac_r, ac_g, ac_b);

#undef IUP_RGB_FROM_ARGB

  iupAndroidPushPaletteToJava(dark);
}

/* c-shared stdout/stderr go to /dev/null on Android; pipe them to logcat */
static void* iupAndroidStdoutPumpThread(void* arg)
{
  int fd = (int)(intptr_t)arg;
  char buf[1024];
  ssize_t n;
  while ((n = read(fd, buf, sizeof(buf) - 1)) > 0)
  {
    if (buf[n - 1] == '\n') n--;
    buf[n] = '\0';
    __android_log_write(ANDROID_LOG_INFO, "IupStdout", buf);
  }
  return NULL;
}

static void* iupAndroidStderrPumpThread(void* arg)
{
  int fd = (int)(intptr_t)arg;
  char buf[1024];
  ssize_t n;
  while ((n = read(fd, buf, sizeof(buf) - 1)) > 0)
  {
    if (buf[n - 1] == '\n') n--;
    buf[n] = '\0';
    __android_log_write(ANDROID_LOG_WARN, "IupStderr", buf);
  }
  return NULL;
}

static void iupAndroidRedirectStdio(void)
{
  static int done = 0;
  if (done) return;
  done = 1;

  setvbuf(stdout, NULL, _IOLBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);

  int out_pipe[2], err_pipe[2];
  if (pipe(out_pipe) == 0)
  {
    dup2(out_pipe[1], STDOUT_FILENO);
    close(out_pipe[1]);
    pthread_t t;
    pthread_create(&t, NULL, iupAndroidStdoutPumpThread, (void*)(intptr_t)out_pipe[0]);
    pthread_detach(t);
  }
  if (pipe(err_pipe) == 0)
  {
    dup2(err_pipe[1], STDERR_FILENO);
    close(err_pipe[1]);
    pthread_t t;
    pthread_create(&t, NULL, iupAndroidStderrPumpThread, (void*)(intptr_t)err_pipe[0]);
    pthread_detach(t);
  }
}

int iupdrvOpen(int *argc, char ***argv)
{
  (void)argc;
  (void)argv;

  iupAndroidRedirectStdio();

  IupSetGlobal("DRIVER", "Android");
  IupSetGlobal("WINDOWING", "ANDROID");

  iupAndroidUpdateGlobalColors();

  IupSetGlobal("_IUP_RESET_GLOBALCOLORS", "YES");

  return IUP_NOERROR;
}

int iupdrvSetGlobalAppIDAttrib(const char* value)
{
  (void)value;
  return 0;
}

int iupdrvSetGlobalAppNameAttrib(const char* value)
{
  (void)value;
  return 0;
}

void iupdrvClose(void)
{
}
