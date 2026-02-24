/** \file
 * \brief WebKitGTK Web Browser Control
 *
 * See Copyright Notice in "iup.h"
 */


#ifndef IUPWEB_USE_DLOPEN
#undef GTK_DISABLE_DEPRECATED
#endif

#include <gtk/gtk.h>

#ifdef IUPWEB_USE_DLOPEN
#include "iupwebgtk_dlopen.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdarg.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_layout.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_webbrowser.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_key.h"
#include "iup_register.h"

/* Include correct GTK driver header based on version */
#if GTK_CHECK_VERSION(4, 0, 0)
  #include "iupgtk4_drv.h"
  /* Create aliases for GTK4 functions to match GTK3 names used below */
  #define iupgtkStrConvertToSystem    iupgtk4StrConvertToSystem
  #define iupgtkStrConvertToFilename  iupgtk4StrConvertToFilename
  #define iupgtkAddToParent           iupgtk4AddToParent
  #define iupgtkShowHelp              iupgtk4ShowHelp
#else
  #include "iupgtk_drv.h"
#endif

#ifndef IUPWEB_USE_DLOPEN
#if GTK_CHECK_VERSION(4, 0, 0)
  /* GTK4 uses WebKit6 */
  #if defined(IUPWEB_USE_WEBKIT6)
    #include <webkit/webkit.h>
    #include <jsc/jsc.h>
    #if !defined(IUPWEB_USE_WEBKIT2)
      #define IUPWEB_USE_WEBKIT2
    #endif
  #else
    /* Default to WebKit6 for GTK4 */
    #include <webkit/webkit.h>
    #include <jsc/jsc.h>
    #if !defined(IUPWEB_USE_WEBKIT2)
      #define IUPWEB_USE_WEBKIT2
    #endif
    #if !defined(IUPWEB_USE_WEBKIT6)
      #define IUPWEB_USE_WEBKIT6
    #endif
  #endif
#elif GTK_CHECK_VERSION(3, 0, 0)
  /* GTK3 uses WebKit2 or WebKit1 */
  #if defined(IUPWEB_USE_WEBKIT2)
    #include <webkit2/webkit2.h>
    #include <JavaScriptCore/JavaScript.h>
  #elif defined(IUPWEB_USE_WEBKIT1)
    #include <webkitgtk/webkitgtk.h>
    #undef IUPWEB_USE_WEBKIT2
  #else
    #include <webkit2/webkit2.h>
    #include <JavaScriptCore/JavaScript.h>
    #if !defined(IUPWEB_USE_WEBKIT2)
      #define IUPWEB_USE_WEBKIT2
    #endif
  #endif
#else
  /* GTK2 uses WebKit1 */
  #include <webkit/webkit.h>
  #if !defined(IUPWEB_USE_WEBKIT1)
    #define IUPWEB_USE_WEBKIT1
  #endif
  #undef IUPWEB_USE_WEBKIT2
#endif
#endif


#ifndef WEBKIT_LOAD_STATUS_FAILED
#define WEBKIT_LOAD_STATUS_FAILED 4
#endif

#ifdef IUPWEB_USE_DLOPEN
static int s_use_webkit2 = -1;
static int s_use_webkit6 = -1;
#else
  #if defined(IUPWEB_USE_WEBKIT6)
    static int s_use_webkit2 = 1;   /* WebKit6 is similar to WebKit2 API */
    static int s_use_webkit6 = 1;
  #elif defined(IUPWEB_USE_WEBKIT2)
    static int s_use_webkit2 = 1;
    static int s_use_webkit6 = 0;
  #elif defined(IUPWEB_USE_WEBKIT1)
    static int s_use_webkit2 = 0;
    static int s_use_webkit6 = 0;
  #else
    static int s_use_webkit2 = 1;
    static int s_use_webkit6 = 0;
  #endif
#endif

#ifdef IUPWEB_USE_DLOPEN
#include <dlfcn.h>

static void* s_webKitLibrary = NULL;

int IupGtkWebBrowserDLOpen()
{
  size_t i;
  const int mode_flags = RTLD_LAZY | RTLD_LOCAL;

  static const char* listOfWebKit2Names[] =
  {
    "libwebkitgtk-6.0.so",        /* WebKitGTK 6.0 for GTK4 */
    "libwebkitgtk-6.0.so.4",      /* WebKitGTK 6.0 for GTK4 (versioned) */
    "libwebkit2gtk-4.1.so",       /* WebKit2GTK 4.1 for GTK3 + libsoup3 */
    "libwebkit2gtk-4.1.so.0",     /* WebKit2GTK 4.1 for GTK3 + libsoup3 (versioned) */
    "libwebkit2gtk-4.0.so",       /* WebKit2GTK 4.0 for GTK3 + libsoup2 */
    "libwebkit2gtk-4.0.so.37"     /* WebKit2GTK 4.0 for GTK3 + libsoup2 (versioned) */
  };
#define WEBKIT2_NAMES_ARRAY_LENGTH (sizeof(listOfWebKit2Names)/sizeof(*listOfWebKit2Names))

  static const char* listOfWebKit1Gtk3Names[] =
  {
    "libwebkitgtk-3.0.so",
    "libwebkitgtk-3.0.so.0"
  };
#define WEBKIT1_GTK3_NAMES_ARRAY_LENGTH (sizeof(listOfWebKit1Gtk3Names)/sizeof(*listOfWebKit1Gtk3Names))

  static const char* listOfWebKit1Gtk2Names[] =
  {
    "libwebkitgtk-1.0.so",
    "libwebkitgtk-1.0.so.0"
  };
#define WEBKIT1_GTK2_NAMES_ARRAY_LENGTH (sizeof(listOfWebKit1Gtk2Names)/sizeof(*listOfWebKit1Gtk2Names))

  if(NULL != s_webKitLibrary)
  {
    return IUP_OPENED;
  }

  s_use_webkit2 = -1;
  s_use_webkit6 = -1;
  iupgtkWebBrowser_ClearDLSymbols();

#if GTK_CHECK_VERSION(4, 0, 0)
  /* GTK4: Only try WebKit6 (libwebkitgtk-6.0) */
  for(i=0; i<2; i++)  /* First two entries are WebKit6 */
  {
    s_webKitLibrary = dlopen(listOfWebKit2Names[i], mode_flags);
    if(NULL != s_webKitLibrary)
    {
      if(iupgtkWebBrowser_SetDLSymbolsWK6(s_webKitLibrary))
      {
        s_use_webkit2 = 1;  /* WebKit6 uses WebKit2-like API */
        s_use_webkit6 = 1;
        break;
      }
      dlclose(s_webKitLibrary);
      s_webKitLibrary = NULL;
    }
  }
#elif GTK_CHECK_VERSION(3, 0, 0)
  /* GTK3: Try WebKit2 (libwebkit2gtk-4.x), skip WebKit6 */
  for(i=2; i<WEBKIT2_NAMES_ARRAY_LENGTH; i++)  /* Start at index 2 to skip WebKit6 entries */
  {
    s_webKitLibrary = dlopen(listOfWebKit2Names[i], mode_flags);
    if(NULL != s_webKitLibrary)
    {
      if(iupgtkWebBrowser_SetDLSymbolsWK2(s_webKitLibrary))
      {
        s_use_webkit2 = 1;
        s_use_webkit6 = 0;
        break;
      }
      else
      {
        dlclose(s_webKitLibrary);
        s_webKitLibrary = NULL;
      }
    }
  }

  /* GTK3: Fallback to WebKit1 if WebKit2 not found */
  if(NULL == s_webKitLibrary)
  {
    for(i=0; i<WEBKIT1_GTK3_NAMES_ARRAY_LENGTH; i++)
    {
      s_webKitLibrary = dlopen(listOfWebKit1Gtk3Names[i], mode_flags);
      if(NULL != s_webKitLibrary)
      {
        if(iupgtkWebBrowser_SetDLSymbolsWK1(s_webKitLibrary))
        {
          s_use_webkit2 = 0;
          s_use_webkit6 = 0;
          break;
        }
        else
        {
          dlclose(s_webKitLibrary);
          s_webKitLibrary = NULL;
        }
      }
    }
  }
#else
  /* GTK2: Only WebKit1 */
  for(i=0; i<WEBKIT1_GTK2_NAMES_ARRAY_LENGTH; i++)
  {
    s_webKitLibrary = dlopen(listOfWebKit1Gtk2Names[i], mode_flags);
    if(NULL != s_webKitLibrary)
    {
      if(iupgtkWebBrowser_SetDLSymbolsWK1(s_webKitLibrary))
      {
        s_use_webkit2 = 0;
        s_use_webkit6 = 0;
        break;
      }
      else
      {
        dlclose(s_webKitLibrary);
        s_webKitLibrary = NULL;
      }
    }
  }
#endif

  if(NULL == s_webKitLibrary)
  {
#if GTK_CHECK_VERSION(4, 0, 0)
    IupSetGlobal("IUP_WEBBROWSER_MISSING_LIB", "libwebkitgtk-6.0.so");
#elif GTK_CHECK_VERSION(3, 0, 0)
    IupSetGlobal("IUP_WEBBROWSER_MISSING_LIB", "libwebkit2gtk-4.1.so, libwebkit2gtk-4.0.so or libwebkitgtk-3.0.so");
#else
    IupSetGlobal("IUP_WEBBROWSER_MISSING_LIB", "libwebkitgtk-1.0.so");
#endif
    iupgtkWebBrowser_ClearDLSymbols();
    s_use_webkit2 = -1;
    s_use_webkit6 = -1;
    return IUP_ERROR;
  }
  else
  {
    return IUP_NOERROR;
  }
}

#endif

struct _IcontrolData
{
  int sb;
};

typedef struct _JavaScriptResult
{
  char* result;
  GMainLoop* loop;
  int completed;
} JavaScriptResult;

static char* gtkWebBrowserEscapeJavaScript(const char* str)
{
  if (!str)
    return iupStrDup("null");

  size_t len = strlen(str);
  size_t escaped_len = len * 6 + 3;
  char* result = (char*)malloc(escaped_len);
  char* p = result;

  *p++ = '"';

  for (const char* s = str; *s; s++)
  {
    switch (*s)
    {
      case '"':  *p++ = '\\'; *p++ = '"'; break;
      case '\\': *p++ = '\\'; *p++ = '\\'; break;
      case '\b': *p++ = '\\'; *p++ = 'b'; break;
      case '\f': *p++ = '\\'; *p++ = 'f'; break;
      case '\n': *p++ = '\\'; *p++ = 'n'; break;
      case '\r': *p++ = '\\'; *p++ = 'r'; break;
      case '\t': *p++ = '\\'; *p++ = 't'; break;
      default:
        if ((unsigned char)*s < 32)
        {
          sprintf(p, "\\u%04x", (unsigned char)*s);
          p += 6;
        }
        else
        {
          *p++ = *s;
        }
        break;
    }
  }

  *p++ = '"';
  *p = '\0';

  return result;
}

static char* gtkWebBrowserGetItemHistoryAttrib(Ihandle* ih, int id)
{
  WebKitBackForwardList* back_forward_list = webkit_web_view_get_back_forward_list((WebKitWebView*)ih->handle);

#ifdef IUPWEB_USE_DLOPEN
  if (s_use_webkit2)
  {
    WebKitBackForwardListItem* item = webkit_back_forward_list_get_nth_item(back_forward_list, id);
    if (item)
      return iupStrReturnStr(webkit_back_forward_list_item_get_uri(item));
  }
  else
  {
    WebKitWebHistoryItem* item = webkit_web_back_forward_list_get_nth_item((WebKitWebBackForwardList*)back_forward_list, id);
    if (item)
      return iupStrReturnStr(webkit_web_history_item_get_uri(item));
  }
#elif defined(IUPWEB_USE_WEBKIT6) || defined(IUPWEB_USE_WEBKIT2)
  WebKitBackForwardListItem* item = webkit_back_forward_list_get_nth_item(back_forward_list, id);
  if (item)
    return iupStrReturnStr(webkit_back_forward_list_item_get_uri(item));
#elif defined(IUPWEB_USE_WEBKIT1)
  WebKitWebHistoryItem* item = webkit_web_back_forward_list_get_nth_item((WebKitWebBackForwardList*)back_forward_list, id);
  if (item)
    return iupStrReturnStr(webkit_web_history_item_get_uri(item));
#endif

  return NULL;
}

static char* gtkWebBrowserGetForwardCountAttrib(Ihandle* ih)
{
  WebKitBackForwardList *back_forward_list = webkit_web_view_get_back_forward_list ((WebKitWebView*)ih->handle);

#ifdef IUPWEB_USE_DLOPEN
  if (s_use_webkit2)
  {
    return iupStrReturnInt(g_list_length(webkit_back_forward_list_get_forward_list(back_forward_list)));
  }
  else
  {
    return iupStrReturnInt(webkit_web_back_forward_list_get_forward_length((WebKitWebBackForwardList*)back_forward_list));
  }
#elif defined(IUPWEB_USE_WEBKIT6) || defined(IUPWEB_USE_WEBKIT2)
  return iupStrReturnInt(g_list_length(webkit_back_forward_list_get_forward_list(back_forward_list)));
#elif defined(IUPWEB_USE_WEBKIT1)
  return iupStrReturnInt(webkit_web_back_forward_list_get_forward_length((WebKitWebBackForwardList*)back_forward_list));
#endif
}

static char* gtkWebBrowserGetBackCountAttrib(Ihandle* ih)
{
  WebKitBackForwardList *back_forward_list = webkit_web_view_get_back_forward_list((WebKitWebView*)ih->handle);

#ifdef IUPWEB_USE_DLOPEN
  if (s_use_webkit2)
  {
    return iupStrReturnInt(g_list_length(webkit_back_forward_list_get_back_list(back_forward_list)));
  }
  else
  {
    return iupStrReturnInt(webkit_web_back_forward_list_get_back_length((WebKitWebBackForwardList*)back_forward_list));
  }
#elif defined(IUPWEB_USE_WEBKIT6) || defined(IUPWEB_USE_WEBKIT2)
  return iupStrReturnInt(g_list_length(webkit_back_forward_list_get_back_list(back_forward_list)));
#elif defined(IUPWEB_USE_WEBKIT1)
  return iupStrReturnInt(webkit_web_back_forward_list_get_back_length((WebKitWebBackForwardList*)back_forward_list));
#endif
}

static void gtkWebBrowserRunJavascript(Ihandle* ih, const char* format, ...);

static int gtkWebBrowserSetHTMLAttrib(Ihandle* ih, const char* value)
{
  if (value)
  {
    iupAttribSet(ih, "_IUPWEB_DIRTY", NULL);
    iupAttribSet(ih, "_IUPWEB_IGNORE_NAVIGATE", "1");

#ifdef IUPWEB_USE_DLOPEN
    if (s_use_webkit2)
      webkit_web_view_load_html((WebKitWebView*)ih->handle, iupgtkStrConvertToSystem(value), NULL);
    else
      webkit_web_view_load_string((WebKitWebView*)ih->handle, iupgtkStrConvertToSystem(value), "text/html", "UTF-8", NULL);
#elif defined(IUPWEB_USE_WEBKIT6) || defined(IUPWEB_USE_WEBKIT2)
    webkit_web_view_load_html((WebKitWebView*)ih->handle, iupgtkStrConvertToSystem(value), NULL);
#elif defined(IUPWEB_USE_WEBKIT1)
    webkit_web_view_load_string((WebKitWebView*)ih->handle, iupgtkStrConvertToSystem(value), "text/html", "UTF-8", NULL);
#endif

    iupAttribSet(ih, "_IUPWEB_IGNORE_NAVIGATE", NULL);
  }
  return 0;
}

#if (defined(IUPWEB_USE_WEBKIT6) || defined(IUPWEB_USE_WEBKIT2) || defined(IUPWEB_USE_DLOPEN))
static void gtkWebBrowserGetResourceData(GObject *source_object, GAsyncResult *res, gpointer user_data)
{
  Ihandle* ih = (Ihandle*)user_data;
  WebKitWebResource *resource = (WebKitWebResource*)source_object;
  GError *error = NULL;
  gsize len = 0;
  guchar* data = webkit_web_resource_get_data_finish(resource, res, &len, &error);

  if (!data)
  {
    if (error)
    {
      iupAttribSetStrf(ih, "_IUP_WEB_HTML_RESULT", "ERROR: %s", error->message);
      g_error_free(error);
    }
    else
      iupAttribSet(ih, "_IUP_WEB_HTML_RESULT", "ERROR: UNKNOWN");
  }
  else
  {
    char* html_str = g_strndup((const gchar*)data, len);
    iupAttribSetStr(ih, "_IUP_WEB_HTML_RESULT", html_str);
    g_free(html_str);
    g_free(data);
  }
}
#endif

static char* gtkWebBrowserExecJavaScriptSync(Ihandle* ih, const char* js);
static char* gtkWebBrowserRunJavaScriptSync(Ihandle* ih, const char* format, ...);

static char* gtkWebBrowserGetHTMLAttrib(Ihandle* ih)
{
  if (iupAttribGet(ih, "_IUPWEB_EDITABLE"))
  {
    /* Use outerHTML to get the complete HTML document, preserving data: URIs and other attributes */
    return gtkWebBrowserRunJavaScriptSync(ih, "document.documentElement.outerHTML;");
  }

#ifdef IUPWEB_USE_DLOPEN
  if (s_use_webkit2)
  {
    char* value = NULL;
    iupAttribSet(ih, "_IUP_WEB_HTML_RESULT", NULL);
    WebKitWebResource* resource = webkit_web_view_get_main_resource((WebKitWebView*)ih->handle);
    if (!resource) return NULL;

    webkit_web_resource_get_data(resource, NULL, gtkWebBrowserGetResourceData, ih);

    int i = 0;
    while (!value && i < 1000)
    {
      IupLoopStep();
      value = iupAttribGet(ih, "_IUP_WEB_HTML_RESULT");
      i++;
    }

    if (value)
    {
      char* result = iupStrReturnStr(value);
      iupAttribSet(ih, "_IUP_WEB_HTML_RESULT", NULL);
      return result;
    }
    return NULL;
  }
  else
  {
    WebKitWebFrame* frame = webkit_web_view_get_main_frame((WebKitWebView*)ih->handle);
    if (frame)
    {
      WebKitWebDataSource* data_source = webkit_web_frame_get_data_source(frame);
      if (data_source)
      {
        GString* data = webkit_web_data_source_get_data(data_source);
        if (data && data->str)
          return iupStrReturnStr(data->str);
      }
    }
    return NULL;
  }
#elif defined(IUPWEB_USE_WEBKIT6) || defined(IUPWEB_USE_WEBKIT2)
  char* value = NULL;
  iupAttribSet(ih, "_IUP_WEB_HTML_RESULT", NULL);
  WebKitWebResource* resource = webkit_web_view_get_main_resource((WebKitWebView*)ih->handle);
  if (!resource) return NULL;

  webkit_web_resource_get_data(resource, NULL, gtkWebBrowserGetResourceData, ih);

  int i = 0;
  while (!value && i < 1000)
  {
    IupLoopStep();
    value = iupAttribGet(ih, "_IUP_WEB_HTML_RESULT");
    i++;
  }

  if (value)
  {
    char* result = iupStrReturnStr(value);
    iupAttribSet(ih, "_IUP_WEB_HTML_RESULT", NULL);
    return result;
  }
  return NULL;
#elif defined(IUPWEB_USE_WEBKIT1)
  WebKitWebFrame* frame = webkit_web_view_get_main_frame((WebKitWebView*)ih->handle);
  if (frame)
  {
    WebKitWebDataSource* data_source = webkit_web_frame_get_data_source(frame);
    if (data_source)
    {
      GString* data = webkit_web_data_source_get_data(data_source);
      if (data && data->str)
        return iupStrReturnStr(data->str);
    }
  }
  return NULL;
#endif
}

static int write_file(const char* filename, const char* str, int count)
{
  FILE* file = fopen(filename, "wb");
  if (!file)
    return 0;

  fwrite(str, 1, count, file);

  fclose(file);
  return 1;
}

static int gtkWebBrowserSetSaveAttrib(Ihandle* ih, const char* value)
{
  if (value)
  {
    int success = 0;
#ifdef IUPWEB_USE_DLOPEN
    if (s_use_webkit2)
    {
      GFile* file = g_file_new_for_path(iupgtkStrConvertToFilename(value));
      if (file)
      {
        webkit_web_view_save_to_file((WebKitWebView*)ih->handle, file, WEBKIT_SAVE_MODE_MHTML, NULL, NULL, NULL);
        g_object_unref(file);
        success = 1;
      }
    }
    else
    {
      WebKitWebFrame* frame = webkit_web_view_get_main_frame((WebKitWebView*)ih->handle);
      if (frame)
      {
        WebKitWebDataSource* data_source = webkit_web_frame_get_data_source(frame);
        if (data_source)
        {
          GString* data = webkit_web_data_source_get_data(data_source);
          if (data && data->str)
            success = write_file(value, data->str, data->len);
        }
      }
    }
#elif defined(IUPWEB_USE_WEBKIT6) || defined(IUPWEB_USE_WEBKIT2)
    GFile* file = g_file_new_for_path(iupgtkStrConvertToFilename(value));
    if (file)
    {
      webkit_web_view_save_to_file((WebKitWebView*)ih->handle, file, WEBKIT_SAVE_MODE_MHTML, NULL, NULL, NULL);
      g_object_unref(file);
      success = 1;
    }
#elif defined(IUPWEB_USE_WEBKIT1)
    WebKitWebFrame* frame = webkit_web_view_get_main_frame((WebKitWebView*)ih->handle);
    if (frame)
    {
      WebKitWebDataSource* data_source = webkit_web_frame_get_data_source(frame);
      if (data_source)
      {
        GString* data = webkit_web_data_source_get_data(data_source);
        if (data && data->str)
          success = write_file(value, data->str, data->len);
      }
    }
#endif
    if (success)
      iupAttribSet(ih, "_IUPWEB_DIRTY", NULL);
  }
  return 0;
}

static int gtkWebBrowserSetValueAttrib(Ihandle* ih, const char* value);
static int gtkWebBrowserSetEditableAttrib(Ihandle* ih, const char* value);
static void gtkWebBrowserExecCommandParam(Ihandle* ih, const char* cmd, const char* param);

static int gtkWebBrowserSetOpenAttrib(Ihandle* ih, const char* value)
{
  if (value)
  {
    char* url = iupStrFileMakeURL(value);
    iupAttribSet(ih, "_IUPWEB_DIRTY", NULL);
    gtkWebBrowserSetValueAttrib(ih, url);
    free(url);
  }
  return 0;
}

static int gtkWebBrowserSetNewAttrib(Ihandle* ih, const char* value)
{
  iupAttribSet(ih, "_IUPWEB_DIRTY", NULL);
  gtkWebBrowserSetHTMLAttrib(ih, "<html><body></body></html>");
  gtkWebBrowserSetEditableAttrib(ih, "Yes");
  (void)value;
  return 0;
}

static int gtkWebBrowserSetCutAttrib(Ihandle* ih, const char* value)
{
#ifdef IUPWEB_USE_DLOPEN
  if (s_use_webkit2)
    webkit_web_view_execute_editing_command((WebKitWebView*)ih->handle, WEBKIT_EDITING_COMMAND_CUT);
  else
    webkit_web_view_cut_clipboard((WebKitWebView*)ih->handle);
#elif defined(IUPWEB_USE_WEBKIT6) || defined(IUPWEB_USE_WEBKIT2)
  webkit_web_view_execute_editing_command((WebKitWebView*)ih->handle, WEBKIT_EDITING_COMMAND_CUT);
#elif defined(IUPWEB_USE_WEBKIT1)
  webkit_web_view_cut_clipboard((WebKitWebView*)ih->handle);
#endif
  (void)value;
  return 0;
}

static int gtkWebBrowserSetCopyAttrib(Ihandle* ih, const char* value)
{
#ifdef IUPWEB_USE_DLOPEN
  if (s_use_webkit2)
    webkit_web_view_execute_editing_command((WebKitWebView*)ih->handle, WEBKIT_EDITING_COMMAND_COPY);
  else
    webkit_web_view_copy_clipboard((WebKitWebView*)ih->handle);
#elif defined(IUPWEB_USE_WEBKIT6) || defined(IUPWEB_USE_WEBKIT2)
  webkit_web_view_execute_editing_command((WebKitWebView*)ih->handle, WEBKIT_EDITING_COMMAND_COPY);
#elif defined(IUPWEB_USE_WEBKIT1)
  webkit_web_view_copy_clipboard((WebKitWebView*)ih->handle);
#endif
  (void)value;
  return 0;
}

static int gtkWebBrowserSetPasteAttrib(Ihandle* ih, const char* value)
{
#ifdef IUPWEB_USE_DLOPEN
  if (s_use_webkit2)
    webkit_web_view_execute_editing_command((WebKitWebView*)ih->handle, WEBKIT_EDITING_COMMAND_PASTE);
  else
    webkit_web_view_paste_clipboard((WebKitWebView*)ih->handle);
#elif defined(IUPWEB_USE_WEBKIT6) || defined(IUPWEB_USE_WEBKIT2)
  webkit_web_view_execute_editing_command((WebKitWebView*)ih->handle, WEBKIT_EDITING_COMMAND_PASTE);
#elif defined(IUPWEB_USE_WEBKIT1)
  webkit_web_view_paste_clipboard((WebKitWebView*)ih->handle);
#endif
  (void)value;
  return 0;
}

static char* gtkWebBrowserExecJavaScriptSync(Ihandle* ih, const char* js);
static char* gtkWebBrowserRunJavaScriptSync(Ihandle* ih, const char* format, ...);

static char* gtkWebBrowserGetPasteAttrib(Ihandle* ih)
{
  char* result = gtkWebBrowserRunJavaScriptSync(ih, "document.queryCommandEnabled('paste');");
  if (result)
  {
    int enabled = strcmp(result, "true") == 0;
    free(result);
    return iupStrReturnBoolean(enabled);
  }
  return iupStrReturnBoolean(0);
}

static int gtkWebBrowserSetUndoAttrib(Ihandle* ih, const char* value)
{
#ifdef IUPWEB_USE_DLOPEN
  if (s_use_webkit2)
    webkit_web_view_execute_editing_command((WebKitWebView*)ih->handle, WEBKIT_EDITING_COMMAND_UNDO);
  else
    webkit_web_view_undo((WebKitWebView*)ih->handle);
#elif defined(IUPWEB_USE_WEBKIT6) || defined(IUPWEB_USE_WEBKIT2)
  webkit_web_view_execute_editing_command((WebKitWebView*)ih->handle, WEBKIT_EDITING_COMMAND_UNDO);
#elif defined(IUPWEB_USE_WEBKIT1)
  webkit_web_view_undo((WebKitWebView*)ih->handle);
#endif
  (void)value;
  return 0;
}

static int gtkWebBrowserSetRedoAttrib(Ihandle* ih, const char* value)
{
#ifdef IUPWEB_USE_DLOPEN
  if (s_use_webkit2)
    webkit_web_view_execute_editing_command((WebKitWebView*)ih->handle, WEBKIT_EDITING_COMMAND_REDO);
  else
    webkit_web_view_redo((WebKitWebView*)ih->handle);
#elif defined(IUPWEB_USE_WEBKIT6) || defined(IUPWEB_USE_WEBKIT2)
  webkit_web_view_execute_editing_command((WebKitWebView*)ih->handle, WEBKIT_EDITING_COMMAND_REDO);
#elif defined(IUPWEB_USE_WEBKIT1)
  webkit_web_view_redo((WebKitWebView*)ih->handle);
#endif
  (void)value;
  return 0;
}

static char* gtkWebBrowserGetEditableAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(iupAttribGet(ih, "_IUPWEB_EDITABLE") != NULL);
}

static int gtkWebBrowserSetSelectAllAttrib(Ihandle* ih, const char* value)
{
#ifdef IUPWEB_USE_DLOPEN
  if (s_use_webkit2)
    webkit_web_view_execute_editing_command((WebKitWebView*)ih->handle, WEBKIT_EDITING_COMMAND_SELECT_ALL);
  else
    webkit_web_view_select_all((WebKitWebView*)ih->handle);
#elif defined(IUPWEB_USE_WEBKIT6) || defined(IUPWEB_USE_WEBKIT2)
  webkit_web_view_execute_editing_command((WebKitWebView*)ih->handle, WEBKIT_EDITING_COMMAND_SELECT_ALL);
#elif defined(IUPWEB_USE_WEBKIT1)
  webkit_web_view_select_all((WebKitWebView*)ih->handle);
#endif
  (void)value;
  return 0;
}

static int gtkWebBrowserSetPrintAttrib(Ihandle* ih, const char* value)
{
#ifdef IUPWEB_USE_DLOPEN
  if (s_use_webkit2)
  {
    WebKitPrintOperation *print_operation = webkit_print_operation_new((WebKitWebView*)ih->handle);
    if (iupStrBoolean(value))
    {
      Ihandle* dlg = IupGetDialog(ih);
      GtkWindow* parent = NULL;

      if (dlg && dlg->handle)
        parent = (GtkWindow*)dlg->handle;

      webkit_print_operation_run_dialog(print_operation, parent);
    }
    else
      webkit_print_operation_print(print_operation);
  }
  else
  {
    WebKitWebFrame* frame = webkit_web_view_get_main_frame((WebKitWebView*)ih->handle);
    if (frame)
      webkit_web_frame_print(frame);

    (void)value;
  }
#elif defined(IUPWEB_USE_WEBKIT6) || defined(IUPWEB_USE_WEBKIT2)
  WebKitPrintOperation *print_operation = webkit_print_operation_new((WebKitWebView*)ih->handle);
  if (iupStrBoolean(value))
  {
    Ihandle* dlg = IupGetDialog(ih);
    GtkWindow* parent = NULL;

    if (dlg && dlg->handle)
      parent = (GtkWindow*)dlg->handle;

    webkit_print_operation_run_dialog(print_operation, parent);
  }
  else
    webkit_print_operation_print(print_operation);
#elif defined(IUPWEB_USE_WEBKIT1)
  WebKitWebFrame* frame = webkit_web_view_get_main_frame((WebKitWebView*)ih->handle);
  if (frame)
    webkit_web_frame_print(frame);

  (void)value;
#endif
  return 0;
}

static int gtkWebBrowserSetPrintPreviewAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  return gtkWebBrowserSetPrintAttrib(ih, NULL);
}

static int gtkWebBrowserSetZoomAttrib(Ihandle* ih, const char* value)
{
  int zoom;
  if (iupStrToInt(value, &zoom))
    webkit_web_view_set_zoom_level((WebKitWebView*)ih->handle, (float)zoom/100.0f);
  return 0;
}

static char* gtkWebBrowserGetZoomAttrib(Ihandle* ih)
{
  int zoom = (int)(webkit_web_view_get_zoom_level((WebKitWebView*)ih->handle) * 100);
  return iupStrReturnInt(zoom);
}

static int gtkWebBrowserSetEditableAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
  {
    gtkWebBrowserRunJavascript(ih, "document.body.contentEditable = 'true';");
    iupAttribSet(ih, "_IUPWEB_EDITABLE", "1");
  }
  else
  {
    gtkWebBrowserRunJavascript(ih, "document.body.contentEditable = 'false';");
    iupAttribSet(ih, "_IUPWEB_EDITABLE", NULL);
  }
  return 0;
}

static void gtkWebBrowserInitSelectionTracking(Ihandle* ih)
{
#ifdef IUPWEB_USE_DLOPEN
  if (s_use_webkit2)
  {
    const char* init_script =
      "(function() {"
      "  if (window.iupSelectionInitialized) return;"
      "  window.iupSelectionInitialized = true;"
      "  var iupSavedRange = null;"
      "  var iupDirtyFlag = false;"
      "  document.addEventListener('selectionchange', function() {"
      "    if(document.body.contentEditable == 'true') {"
      "      var sel = window.getSelection();"
      "      if (sel && sel.rangeCount > 0) {"
      "        iupSavedRange = sel.getRangeAt(0).cloneRange();"
      "        window.webkit.messageHandlers.iupUpdateCb.postMessage(null);"
      "      }"
      "    }"
      "  });"
      "  document.addEventListener('input', function() {"
      "    if(document.body.contentEditable == 'true' && !iupDirtyFlag) {"
      "      iupDirtyFlag = true;"
      "      window.webkit.messageHandlers.iupDirtyFlag.postMessage(null);"
      "    }"
      "  });"
      "  window.iupRestoreSelection = function() {"
      "    if (document.body.contentEditable == 'true' && iupSavedRange) {"
      "      try {"
      "        var sel = window.getSelection();"
      "        sel.removeAllRanges();"
      "        sel.addRange(iupSavedRange);"
      "      } catch (e) {}"
      "    }"
      "  };"
      "  window.iupGetDirtyFlag = function() {"
      "    return iupDirtyFlag;"
      "  };"
      "  window.iupClearDirtyFlag = function() {"
      "    iupDirtyFlag = false;"
      "  };"
      "})();";
    if (s_use_webkit6)
      webkit_web_view_evaluate_javascript((WebKitWebView*)ih->handle, init_script, -1, NULL, NULL, NULL, NULL, NULL);
    else
      webkit_web_view_run_javascript((WebKitWebView*)ih->handle, init_script, NULL, NULL, NULL);
  }
  else
  {
    const char* init_script =
      "(function() {"
      "  if (window.iupSelectionInitialized) return;"
      "  window.iupSelectionInitialized = true;"
      "  var iupSavedRange = null;"
      "  var iupDirtyFlag = false;"
      "  document.addEventListener('selectionchange', function() {"
      "    if(document.body.contentEditable == 'true') {"
      "      var sel = window.getSelection();"
      "      if (sel && sel.rangeCount > 0) {"
      "        iupSavedRange = sel.getRangeAt(0).cloneRange();"
      "      }"
      "    }"
      "  });"
      "  document.addEventListener('input', function() {"
      "    if(document.body.contentEditable == 'true' && !iupDirtyFlag) {"
      "      iupDirtyFlag = true;"
      "    }"
      "  });"
      "  window.iupRestoreSelection = function() {"
      "    if (document.body.contentEditable == 'true' && iupSavedRange) {"
      "      try {"
      "        var sel = window.getSelection();"
      "        sel.removeAllRanges();"
      "        sel.addRange(iupSavedRange);"
      "      } catch (e) {}"
      "    }"
      "  };"
      "  window.iupGetDirtyFlag = function() {"
      "    return iupDirtyFlag;"
      "  };"
      "  window.iupClearDirtyFlag = function() {"
      "    iupDirtyFlag = false;"
      "  };"
      "})();";
    webkit_web_view_execute_script((WebKitWebView*)ih->handle, init_script);
  }
#elif defined(IUPWEB_USE_WEBKIT2)
  const char* init_script =
    "(function() {"
    "  if (window.iupSelectionInitialized) return;"
    "  window.iupSelectionInitialized = true;"
    "  var iupSavedRange = null;"
    "  var iupDirtyFlag = false;"
    "  document.addEventListener('selectionchange', function() {"
    "    if(document.body.contentEditable == 'true') {"
    "      var sel = window.getSelection();"
    "      if (sel && sel.rangeCount > 0) {"
    "        iupSavedRange = sel.getRangeAt(0).cloneRange();"
    "        window.webkit.messageHandlers.iupUpdateCb.postMessage(null);"
    "      }"
    "    }"
    "  });"
    "  document.addEventListener('input', function() {"
    "    if(document.body.contentEditable == 'true' && !iupDirtyFlag) {"
    "      iupDirtyFlag = true;"
    "      window.webkit.messageHandlers.iupDirtyFlag.postMessage(null);"
    "    }"
    "  });"
    "  window.iupRestoreSelection = function() {"
    "    if (document.body.contentEditable == 'true' && iupSavedRange) {"
    "      try {"
    "        var sel = window.getSelection();"
    "        sel.removeAllRanges();"
    "        sel.addRange(iupSavedRange);"
    "      } catch (e) {}"
    "    }"
    "  };"
    "  window.iupGetDirtyFlag = function() {"
    "    return iupDirtyFlag;"
    "  };"
    "  window.iupClearDirtyFlag = function() {"
    "    iupDirtyFlag = false;"
    "  };"
    "})();";
  #if defined(IUPWEB_USE_WEBKIT6)
    webkit_web_view_evaluate_javascript((WebKitWebView*)ih->handle, init_script, -1, NULL, NULL, NULL, NULL, NULL);
  #else
    webkit_web_view_run_javascript((WebKitWebView*)ih->handle, init_script, NULL, NULL, NULL);
  #endif
#elif defined(IUPWEB_USE_WEBKIT1)
  const char* init_script =
    "(function() {"
    "  if (window.iupSelectionInitialized) return;"
    "  window.iupSelectionInitialized = true;"
    "  var iupSavedRange = null;"
    "  var iupDirtyFlag = false;"
    "  document.addEventListener('selectionchange', function() {"
    "    if(document.body.contentEditable == 'true') {"
    "      var sel = window.getSelection();"
    "      if (sel && sel.rangeCount > 0) {"
    "        iupSavedRange = sel.getRangeAt(0).cloneRange();"
    "      }"
    "    }"
    "  });"
    "  document.addEventListener('input', function() {"
    "    if(document.body.contentEditable == 'true' && !iupDirtyFlag) {"
    "      iupDirtyFlag = true;"
    "    }"
    "  });"
    "  window.iupRestoreSelection = function() {"
    "    if (document.body.contentEditable == 'true' && iupSavedRange) {"
    "      try {"
    "        var sel = window.getSelection();"
    "        sel.removeAllRanges();"
    "        sel.addRange(iupSavedRange);"
    "      } catch (e) {}"
    "    }"
    "  };"
    "  window.iupGetDirtyFlag = function() {"
    "    return iupDirtyFlag;"
    "  };"
    "  window.iupClearDirtyFlag = function() {"
    "    iupDirtyFlag = false;"
    "  };"
    "})();";
  webkit_web_view_execute_script((WebKitWebView*)ih->handle, init_script);
#endif
}

static void gtkWebBrowserRunJavascript(Ihandle* ih, const char* format, ...)
{
  char js[1024];
  va_list arglist;
  va_start(arglist, format);
  vsnprintf(js, 1024, format, arglist);
  va_end(arglist);

#ifdef IUPWEB_USE_DLOPEN
  if (s_use_webkit6)
    webkit_web_view_evaluate_javascript((WebKitWebView*)ih->handle, js, -1, NULL, NULL, NULL, NULL, NULL);
  else if (s_use_webkit2)
    webkit_web_view_run_javascript((WebKitWebView*)ih->handle, js, NULL, NULL, NULL);
  else
    webkit_web_view_execute_script((WebKitWebView*)ih->handle, js);
#elif defined(IUPWEB_USE_WEBKIT6)
  webkit_web_view_evaluate_javascript((WebKitWebView*)ih->handle, js, -1, NULL, NULL, NULL, NULL, NULL);
#elif defined(IUPWEB_USE_WEBKIT2)
  webkit_web_view_run_javascript((WebKitWebView*)ih->handle, js, NULL, NULL, NULL);
#elif defined(IUPWEB_USE_WEBKIT1)
  webkit_web_view_execute_script((WebKitWebView*)ih->handle, js);
#endif
}

static void gtkWebBrowserUpdateHistory(Ihandle* ih)
{
  gboolean can_go_back = webkit_web_view_can_go_back((WebKitWebView*)ih->handle);
  gboolean can_go_forward = webkit_web_view_can_go_forward((WebKitWebView*)ih->handle);

  iupAttribSet(ih, "CANGOBACK", can_go_back ? "YES" : "NO");
  iupAttribSet(ih, "CANGOFORWARD", can_go_forward ? "YES" : "NO");
}

#if (defined(IUPWEB_USE_WEBKIT2) || defined(IUPWEB_USE_WEBKIT6) || defined(IUPWEB_USE_DLOPEN))
static void gtkWebBrowserJavaScriptFinished(GObject* object, GAsyncResult* result, gpointer user_data)
{
  JavaScriptResult* js_result = (JavaScriptResult*)user_data;
  GError* error = NULL;

  #if defined(IUPWEB_USE_DLOPEN)
  if (s_use_webkit6)
  {
    /* WebKit6: Use JSCore GObject API */
    JSCValue* js_value = webkit_web_view_evaluate_javascript_finish((WebKitWebView*)object, result, &error);

    if (js_value && !jsc_value_is_null(js_value) && !jsc_value_is_undefined(js_value))
    {
      char* str = jsc_value_to_string(js_value);
      if (str)
      {
        js_result->result = iupStrDup(str);
        g_free(str);
      }
    }

    /* Check for JavaScript exceptions */
    if (js_value)
    {
      JSCContext* context = jsc_value_get_context(js_value);
      JSCException* exception = jsc_context_get_exception(context);
      if (exception)
      {
        char* msg = jsc_exception_get_message(exception);
        if (msg)
          g_free(msg);
      }
    }
  }
  else
  #elif defined(IUPWEB_USE_WEBKIT6)
  {
    /* WebKit6: Use JSCore GObject API */
    JSCValue* js_value = webkit_web_view_evaluate_javascript_finish((WebKitWebView*)object, result, &error);

    if (js_value && !jsc_value_is_null(js_value) && !jsc_value_is_undefined(js_value))
    {
      char* str = jsc_value_to_string(js_value);
      if (str)
      {
        js_result->result = iupStrDup(str);
        g_free(str);
      }
    }

    /* Check for JavaScript exceptions */
    if (js_value)
    {
      JSCContext* context = jsc_value_get_context(js_value);
      JSCException* exception = jsc_context_get_exception(context);
      if (exception)
      {
        char* msg = jsc_exception_get_message(exception);
        if (msg)
          g_free(msg);
      }
    }
  }
  #else
  #endif
  #if !defined(IUPWEB_USE_WEBKIT6) || defined(IUPWEB_USE_DLOPEN)
  {
    WebKitJavascriptResult* js_value = webkit_web_view_run_javascript_finish((WebKitWebView*)object, result, &error);

    if (js_value)
    {
#ifdef IUPWEB_USE_DLOPEN
      if (webkit_javascript_result_get_js_value && jsc_value_to_string)
      {
        /* WebKit2GTK 2.22+: Use JSC GObject API */
        JSCValue* jsc_val = webkit_javascript_result_get_js_value(js_value);
        if (jsc_val && !jsc_value_is_null(jsc_val) && !jsc_value_is_undefined(jsc_val))
        {
          char* str = jsc_value_to_string(jsc_val);
          if (str)
          {
            js_result->result = iupStrDup(str);
            g_free(str);
          }
        }
      }
      else
#endif
      {
        /* Old WebKit2: Use JSCore C API */
        JSGlobalContextRef context = webkit_javascript_result_get_global_context(js_value);
        JSValueRef value = webkit_javascript_result_get_value(js_value);

        if (value && !JSValueIsNull(context, value) && !JSValueIsUndefined(context, value))
        {
          JSStringRef js_str = JSValueToStringCopy(context, value, NULL);
          if (js_str)
          {
            size_t max_size = JSStringGetMaximumUTF8CStringSize(js_str);
            char* buffer = malloc(max_size);
            if (buffer)
            {
              JSStringGetUTF8CString(js_str, buffer, max_size);
              js_result->result = iupStrDup(buffer);
              free(buffer);
            }
            JSStringRelease(js_str);
          }
        }
      }
      webkit_javascript_result_unref(js_value);
    }
  }
  #endif

  if (error)
    g_error_free(error);

  js_result->completed = 1;
  if (js_result->loop)
    g_main_loop_quit(js_result->loop);
}

static char* gtkWebBrowserExecJavaScriptSync(Ihandle* ih, const char* js)
{
  #ifdef IUPWEB_USE_DLOPEN
  if (s_use_webkit2)
  #endif
  {
    JavaScriptResult js_result = {NULL, NULL, 0};
    js_result.loop = g_main_loop_new(NULL, FALSE);

    #if defined(IUPWEB_USE_DLOPEN)
    if (s_use_webkit6)
      webkit_web_view_evaluate_javascript((WebKitWebView*)ih->handle, js, -1, NULL, NULL, NULL,
                                          gtkWebBrowserJavaScriptFinished, &js_result);
    else
      webkit_web_view_run_javascript((WebKitWebView*)ih->handle, js, NULL,
                                      gtkWebBrowserJavaScriptFinished, &js_result);
    #elif defined(IUPWEB_USE_WEBKIT6)
    webkit_web_view_evaluate_javascript((WebKitWebView*)ih->handle, js, -1, NULL, NULL, NULL,
                                        gtkWebBrowserJavaScriptFinished, &js_result);
    #else
    webkit_web_view_run_javascript((WebKitWebView*)ih->handle, js, NULL,
                                    gtkWebBrowserJavaScriptFinished, &js_result);
    #endif

    if (!js_result.completed)
    {
      GMainContext* context = g_main_loop_get_context(js_result.loop);
      while (!js_result.completed)
        g_main_context_iteration(context, TRUE);
    }

    g_main_loop_unref(js_result.loop);
    return js_result.result;
  }

  #ifdef IUPWEB_USE_DLOPEN
  return NULL;
  #endif
}

static char* gtkWebBrowserRunJavaScriptSync(Ihandle* ih, const char* format, ...)
{
  char js[1024];
  va_list arglist;
  va_start(arglist, format);
  vsnprintf(js, 1024, format, arglist);
  va_end(arglist);

  return gtkWebBrowserExecJavaScriptSync(ih, js);
}

static char* gtkWebBrowserQueryCommandValue(Ihandle* ih, const char* cmd)
{
  char* escaped_cmd = gtkWebBrowserEscapeJavaScript(cmd);
  char js[256];
  snprintf(js, sizeof(js), "document.queryCommandValue(%s);", escaped_cmd);
  free(escaped_cmd);
  return gtkWebBrowserRunJavaScriptSync(ih, "%s", js);
}

static char* gtkWebBrowserGetFontNameAttrib(Ihandle* ih)
{
  const char* js =
    "(function() {"
    "  var sel = window.getSelection();"
    "  if (!sel.rangeCount) return '';"
    "  var node = sel.getRangeAt(0).startContainer;"
    "  if (node.nodeType === 3) node = node.parentNode;"
    "  return window.getComputedStyle(node).fontFamily;"
    "})();";

  return gtkWebBrowserRunJavaScriptSync(ih, "%s", js);
}

static char* gtkWebBrowserGetFontSizeAttrib(Ihandle* ih)
{
  const char* js =
    "(function() {"
    "  var sel = window.getSelection();"
    "  if (!sel.rangeCount) return '';"
    "  var node = sel.getRangeAt(0).startContainer;"
    "  if (node.nodeType === 3) node = node.parentNode;"
    "  var size = window.getComputedStyle(node).fontSize;"
    "  var px = parseFloat(size);"
    "  if (px <= 10) return '1';"
    "  else if (px <= 13) return '2';"
    "  else if (px <= 16) return '3';"
    "  else if (px <= 18) return '4';"
    "  else if (px <= 24) return '5';"
    "  else if (px <= 32) return '6';"
    "  else return '7';"
    "})();";

  return gtkWebBrowserRunJavaScriptSync(ih, "%s", js);
}

static char* gtkWebBrowserGetFormatBlockAttrib(Ihandle* ih)
{
  const char* js =
    "(function() {"
    "  var sel = window.getSelection();"
    "  if (!sel.rangeCount) return '';"
    "  var node = sel.getRangeAt(0).startContainer;"
    "  while (node && node.nodeType !== 1) node = node.parentNode;"
    "  while (node && node.nodeName !== 'BODY' && node.nodeName !== 'HTML') {"
    "    var tag = node.nodeName.toUpperCase();"
    "    if (tag === 'H1' || tag === 'H2' || tag === 'H3' || tag === 'H4' || "
    "        tag === 'H5' || tag === 'H6' || tag === 'P' || tag === 'DIV' || "
    "        tag === 'PRE' || tag === 'BLOCKQUOTE') {"
    "      return tag;"
    "    }"
    "    node = node.parentNode;"
    "  }"
    "  return '';"
    "})();";

  return gtkWebBrowserRunJavaScriptSync(ih, "%s", js);
}
#endif

static int gtkWebBrowserExecCommandAttrib(Ihandle* ih, const char* value)
{
  if (value)
  {
#ifdef IUPWEB_USE_DLOPEN
    if (s_use_webkit2)
      webkit_web_view_execute_editing_command((WebKitWebView*)ih->handle, value);
    else
    {
      char cmd[256];
      snprintf(cmd, sizeof(cmd), "document.execCommand('%s', false, null);", value);
      webkit_web_view_execute_script((WebKitWebView*)ih->handle, cmd);
    }
#elif defined(IUPWEB_USE_WEBKIT6) || defined(IUPWEB_USE_WEBKIT2)
    webkit_web_view_execute_editing_command((WebKitWebView*)ih->handle, value);
#elif defined(IUPWEB_USE_WEBKIT1)
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "document.execCommand('%s', false, null);", value);
    webkit_web_view_execute_script((WebKitWebView*)ih->handle, cmd);
#endif
  }
  return 0;
}

static int gtkWebBrowserSetInsertImageAttrib(Ihandle* ih, const char* value)
{
  if (value)
    gtkWebBrowserExecCommandParam(ih, "insertImage", value);
  return 0;
}

static int gtkWebBrowserSetCreateLinkAttrib(Ihandle* ih, const char* value)
{
  if (value)
    gtkWebBrowserExecCommandParam(ih, "createLink", value);
  return 0;
}

static void gtkWebBrowserExecCommandParam(Ihandle* ih, const char* cmd, const char* param)
{
  char* escaped_param = gtkWebBrowserEscapeJavaScript(param);
  size_t cmd_len = strlen(cmd) + strlen(escaped_param) + 256;
  char* js_cmd = (char*)malloc(cmd_len);

  if (!js_cmd)
  {
    free(escaped_param);
    return;
  }

  gtk_widget_grab_focus(ih->handle);

  snprintf(js_cmd, cmd_len,
           "document.body.focus(); "
           "if (window.iupRestoreSelection) window.iupRestoreSelection(); "
           "document.execCommand('%s', false, %s);", cmd, escaped_param);

#ifdef IUPWEB_USE_DLOPEN
  if (s_use_webkit6)
    webkit_web_view_evaluate_javascript((WebKitWebView*)ih->handle, js_cmd, -1, NULL, NULL, NULL, NULL, NULL);
  else if (s_use_webkit2)
    webkit_web_view_run_javascript((WebKitWebView*)ih->handle, js_cmd, NULL, NULL, NULL);
  else
    webkit_web_view_execute_script((WebKitWebView*)ih->handle, js_cmd);
#elif defined(IUPWEB_USE_WEBKIT6)
  webkit_web_view_evaluate_javascript((WebKitWebView*)ih->handle, js_cmd, -1, NULL, NULL, NULL, NULL, NULL);
#elif defined(IUPWEB_USE_WEBKIT2)
  webkit_web_view_run_javascript((WebKitWebView*)ih->handle, js_cmd, NULL, NULL, NULL);
#elif defined(IUPWEB_USE_WEBKIT1)
  webkit_web_view_execute_script((WebKitWebView*)ih->handle, js_cmd);
#endif

  free(js_cmd);
  free(escaped_param);
}

static int gtkWebBrowserSetInsertTextAttrib(Ihandle* ih, const char* value)
{
  if (value)
    gtkWebBrowserExecCommandParam(ih, "insertText", value);
  return 0;
}

static int gtkWebBrowserSetInsertHtmlAttrib(Ihandle* ih, const char* value)
{
  if (value)
    gtkWebBrowserExecCommandParam(ih, "insertHTML", value);
  return 0;
}

static int gtkWebBrowserSetFontNameAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    return 0;

  char* escaped_font = gtkWebBrowserEscapeJavaScript(value);
  size_t js_len = strlen(escaped_font) + 1024;
  char* js_cmd = (char*)malloc(js_len);

  if (!js_cmd)
  {
    free(escaped_font);
    return 0;
  }

  gtk_widget_grab_focus(ih->handle);

  snprintf(js_cmd, js_len,
           "document.body.focus(); "
           "if (window.iupRestoreSelection) window.iupRestoreSelection(); "
           "(function() {"
           "  try {"
           "    var sel = window.getSelection();"
           "    if (!sel || !sel.rangeCount) return;"
           "    var range = sel.getRangeAt(0);"
           "    if (range.collapsed) return;"
           "    var contents = range.extractContents();"
           "    var span = document.createElement('span');"
           "    span.setAttribute('style', 'font-family: ' + %s);"
           "    span.appendChild(contents);"
           "    range.insertNode(span);"
           "    sel.removeAllRanges();"
           "    var newRange = document.createRange();"
           "    newRange.selectNodeContents(span);"
           "    sel.addRange(newRange);"
           "  } catch(e) {}"
           "})();", escaped_font);

#ifdef IUPWEB_USE_DLOPEN
  if (s_use_webkit6)
    webkit_web_view_evaluate_javascript((WebKitWebView*)ih->handle, js_cmd, -1, NULL, NULL, NULL, NULL, NULL);
  else if (s_use_webkit2)
    webkit_web_view_run_javascript((WebKitWebView*)ih->handle, js_cmd, NULL, NULL, NULL);
  else
    webkit_web_view_execute_script((WebKitWebView*)ih->handle, js_cmd);
#elif defined(IUPWEB_USE_WEBKIT6)
  webkit_web_view_evaluate_javascript((WebKitWebView*)ih->handle, js_cmd, -1, NULL, NULL, NULL, NULL, NULL);
#elif defined(IUPWEB_USE_WEBKIT2)
  webkit_web_view_run_javascript((WebKitWebView*)ih->handle, js_cmd, NULL, NULL, NULL);
#elif defined(IUPWEB_USE_WEBKIT1)
  webkit_web_view_execute_script((WebKitWebView*)ih->handle, js_cmd);
#endif

  free(js_cmd);
  free(escaped_font);
  return 0;
}

static int gtkWebBrowserSetFontSizeAttrib(Ihandle* ih, const char* value)
{
  int param = 0;
  if (iupStrToInt(value, &param) && param > 0 && param < 8)
    gtkWebBrowserExecCommandParam(ih, "fontSize", value);
  return 0;
}

static int gtkWebBrowserSetFormatBlockAttrib(Ihandle* ih, const char* value)
{
  if (value)
    gtkWebBrowserExecCommandParam(ih, "formatBlock", value);
  return 0;
}

static int gtkWebBrowserSetForeColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (iupStrToRGB(value, &r, &g, &b))
  {
    char rgb_color[32];
    sprintf(rgb_color, "rgb(%d,%d,%d)", r, g, b);
    gtkWebBrowserExecCommandParam(ih, "foreColor", rgb_color);
  }
  return 0;
}

static char* gtkWebBrowserGetForeColorAttrib(Ihandle* ih)
{
  char* result = gtkWebBrowserRunJavaScriptSync(ih, "document.queryCommandValue('foreColor');");
  if (result)
  {
    char* ret = iupStrReturnStr(result);
    free(result);
    return ret;
  }
  return NULL;
}

static int gtkWebBrowserSetBackColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (iupStrToRGB(value, &r, &g, &b))
  {
    char rgb_color[32];
    sprintf(rgb_color, "rgb(%d,%d,%d)", r, g, b);
    gtkWebBrowserExecCommandParam(ih, "backColor", rgb_color);
  }
  return 0;
}

static char* gtkWebBrowserGetBackColorAttrib(Ihandle* ih)
{
  char* result = gtkWebBrowserRunJavaScriptSync(ih, "document.queryCommandValue('backColor');");
  if (result)
  {
    char* ret = iupStrReturnStr(result);
    free(result);
    return ret;
  }
  return NULL;
}

static char* gtkWebBrowserGetMimeType(const char* filename)
{
  const char* ext = strrchr(filename, '.');
  if (!ext)
    return iupStrDup("image/png");

  ext++;
  if (iupStrEqualNoCase(ext, "png"))
    return iupStrDup("image/png");
  else if (iupStrEqualNoCase(ext, "jpg") || iupStrEqualNoCase(ext, "jpeg"))
    return iupStrDup("image/jpeg");
  else if (iupStrEqualNoCase(ext, "gif"))
    return iupStrDup("image/gif");
  else if (iupStrEqualNoCase(ext, "svg"))
    return iupStrDup("image/svg+xml");
  else if (iupStrEqualNoCase(ext, "webp"))
    return iupStrDup("image/webp");
  else if (iupStrEqualNoCase(ext, "bmp"))
    return iupStrDup("image/bmp");

  return iupStrDup("image/png");
}

static char* gtkWebBrowserBase64Encode(const unsigned char* data, size_t len)
{
  static const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  size_t encoded_len = 4 * ((len + 2) / 3);
  char* encoded = (char*)malloc(encoded_len + 1);
  if (!encoded)
    return NULL;

  size_t i = 0, j = 0;
  while (i < len)
  {
    unsigned char a = i < len ? data[i++] : 0;
    unsigned char b = i < len ? data[i++] : 0;
    unsigned char c = i < len ? data[i++] : 0;

    unsigned int triple = (a << 16) | (b << 8) | c;

    encoded[j++] = base64_chars[(triple >> 18) & 0x3F];
    encoded[j++] = base64_chars[(triple >> 12) & 0x3F];
    encoded[j++] = (i - 1 < len) ? base64_chars[(triple >> 6) & 0x3F] : '=';
    encoded[j++] = (i < len) ? base64_chars[triple & 0x3F] : '=';
  }

  encoded[j] = '\0';
  return encoded;
}

static char* gtkWebBrowserFileToDataURI(const char* filename)
{
  FILE* file = fopen(filename, "rb");
  if (!file)
    return NULL;

  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  fseek(file, 0, SEEK_SET);

  if (file_size <= 0 || file_size > 10 * 1024 * 1024)
  {
    fclose(file);
    return NULL;
  }

  unsigned char* buffer = (unsigned char*)malloc(file_size);
  if (!buffer)
  {
    fclose(file);
    return NULL;
  }

  size_t bytes_read = fread(buffer, 1, file_size, file);
  fclose(file);

  if (bytes_read != (size_t)file_size)
  {
    free(buffer);
    return NULL;
  }

  char* mime_type = gtkWebBrowserGetMimeType(filename);
  char* base64_data = gtkWebBrowserBase64Encode(buffer, file_size);
  free(buffer);

  if (!base64_data)
  {
    free(mime_type);
    return NULL;
  }

  size_t data_uri_len = strlen(mime_type) + strlen(base64_data) + 50;
  char* data_uri = (char*)malloc(data_uri_len);
  if (!data_uri)
  {
    free(mime_type);
    free(base64_data);
    return NULL;
  }

  snprintf(data_uri, data_uri_len, "data:%s;base64,%s", mime_type, base64_data);

  free(mime_type);
  free(base64_data);

  return data_uri;
}

static int gtkWebBrowserSetInsertImageFileAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    return 0;

  char* data_uri = gtkWebBrowserFileToDataURI(value);
  if (data_uri)
  {
    gtkWebBrowserSetInsertImageAttrib(ih, data_uri);
    free(data_uri);
  }

  return 0;
}

static char* gtkWebBrowserGetStatusAttrib(Ihandle* ih)
{
#ifdef IUPWEB_USE_DLOPEN
  if (s_use_webkit2)
  {
    if (webkit_web_view_is_loading((WebKitWebView*)ih->handle))
      return "LOADING";
    else
      return "COMPLETED";
  }
  else
  {
    switch(webkit_web_view_get_load_status((WebKitWebView*)ih->handle))
    {
    case WEBKIT_LOAD_STATUS_PROVISIONAL:
    case WEBKIT_LOAD_STATUS_COMMITTED:
    case WEBKIT_LOAD_STATUS_FIRST_VISUALLY_NON_EMPTY_LAYOUT:
      return "LOADING";
    case WEBKIT_LOAD_STATUS_FINISHED:
      return "COMPLETED";
    case WEBKIT_LOAD_STATUS_FAILED:
      return "FAILED";
    }
    return "FAILED";
  }
#elif defined(IUPWEB_USE_WEBKIT6) || defined(IUPWEB_USE_WEBKIT2)
  if (webkit_web_view_is_loading((WebKitWebView*)ih->handle))
    return "LOADING";
  else
    return "COMPLETED";
#elif defined(IUPWEB_USE_WEBKIT1)
  switch(webkit_web_view_get_load_status((WebKitWebView*)ih->handle))
  {
  case WEBKIT_LOAD_STATUS_PROVISIONAL:
  case WEBKIT_LOAD_STATUS_COMMITTED:
  case WEBKIT_LOAD_STATUS_FIRST_VISUALLY_NON_EMPTY_LAYOUT:
    return "LOADING";
  case WEBKIT_LOAD_STATUS_FINISHED:
    return "COMPLETED";
  case WEBKIT_LOAD_STATUS_FAILED:
    return "FAILED";
  }
  return "FAILED";
#endif
}

static int gtkWebBrowserSetReloadAttrib(Ihandle* ih, const char* value)
{
  webkit_web_view_reload((WebKitWebView*)ih->handle);
  (void)value;
  return 0;
}

static int gtkWebBrowserSetStopAttrib(Ihandle* ih, const char* value)
{
  webkit_web_view_stop_loading((WebKitWebView*)ih->handle);
  (void)value;
  return 0;
}

static int gtkWebBrowserSetBackForwardAttrib(Ihandle* ih, const char* value)
{
  int val;
  if (iupStrToInt(value, &val))
  {
#ifdef IUPWEB_USE_DLOPEN
    if (s_use_webkit2)
    {
      if (val > 0)
      {
        int i;
        for ( i = 0; i < val && webkit_web_view_can_go_forward((WebKitWebView*)ih->handle); i++)
          webkit_web_view_go_forward((WebKitWebView*)ih->handle);
      }
      else if (val < 0)
      {
        int i;
        for (i = 0; i < abs(val) && webkit_web_view_can_go_back((WebKitWebView*)ih->handle); i++)
          webkit_web_view_go_back((WebKitWebView*)ih->handle);
      }
    }
    else
    {
      webkit_web_view_go_back_or_forward((WebKitWebView*)ih->handle, val);
    }
#elif defined(IUPWEB_USE_WEBKIT6) || defined(IUPWEB_USE_WEBKIT2)
    if (val > 0)
    {
      int i;
      for ( i = 0; i < val && webkit_web_view_can_go_forward((WebKitWebView*)ih->handle); i++)
        webkit_web_view_go_forward((WebKitWebView*)ih->handle);
    }
    else if (val < 0)
    {
      int i;
      for (i = 0; i < abs(val) && webkit_web_view_can_go_back((WebKitWebView*)ih->handle); i++)
        webkit_web_view_go_back((WebKitWebView*)ih->handle);
    }
#elif defined(IUPWEB_USE_WEBKIT1)
    webkit_web_view_go_back_or_forward((WebKitWebView*)ih->handle, val);
#endif
  }
  return 0;
}

static int gtkWebBrowserSetGoBackAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  webkit_web_view_go_back((WebKitWebView*)ih->handle);
  return 0;
}

static int gtkWebBrowserSetGoForwardAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  webkit_web_view_go_forward((WebKitWebView*)ih->handle);
  return 0;
}

static char* gtkWebBrowserGetCanGoBackAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(webkit_web_view_can_go_back((WebKitWebView*)ih->handle));
}

static char* gtkWebBrowserGetCanGoForwardAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(webkit_web_view_can_go_forward((WebKitWebView*)ih->handle));
}

static int gtkWebBrowserSetValueAttrib(Ihandle* ih, const char* value)
{
  if (value)
  {
    iupAttribSet(ih, "_IUPWEB_DIRTY", NULL);
    iupAttribSet(ih, "_IUPWEB_IGNORE_NAVIGATE", "1");

    if (iupStrEqualPartial(value, "http://") || iupStrEqualPartial(value, "https://") ||
        iupStrEqualPartial(value, "file://") || iupStrEqualPartial(value, "ftp://"))
    {
      webkit_web_view_load_uri((WebKitWebView*)ih->handle, value);
    }
    else
    {
      char* url = iupStrFileMakeURL(value);
      if (url)
      {
        webkit_web_view_load_uri((WebKitWebView*)ih->handle, url);
        free(url);
      }
    }

    iupAttribSet(ih, "_IUPWEB_IGNORE_NAVIGATE", NULL);
  }
  return 0;
}

static char* gtkWebBrowserGetValueAttrib(Ihandle* ih)
{
  const gchar* value = webkit_web_view_get_uri((WebKitWebView*)ih->handle);
  return iupStrReturnStr(value);
}

static char* gtkWebBrowserQueryCommandState(Ihandle* ih, const char* cmd)
{
  char* escaped_cmd = gtkWebBrowserEscapeJavaScript(cmd);
  char js[256];
  snprintf(js, sizeof(js), "document.queryCommandState(%s);", escaped_cmd);
  free(escaped_cmd);
  return gtkWebBrowserRunJavaScriptSync(ih, "%s", js);
}

static char* gtkWebBrowserQueryCommandEnabled(Ihandle* ih, const char* cmd)
{
  char* escaped_cmd = gtkWebBrowserEscapeJavaScript(cmd);
  char js[256];
  snprintf(js, sizeof(js), "document.queryCommandEnabled(%s);", escaped_cmd);
  free(escaped_cmd);
  return gtkWebBrowserRunJavaScriptSync(ih, "%s", js);
}

static char* gtkWebBrowserGetCommandStateAttrib(Ihandle* ih)
{
  char* cmd = iupAttribGet(ih, "COMMAND");
  if (cmd)
  {
    const char* js_cmd = cmd;

    if (iupStrEqualNoCase(cmd, "FONTNAME"))
      js_cmd = "fontName";
    else if (iupStrEqualNoCase(cmd, "FONTSIZE"))
      js_cmd = "fontSize";
    else if (iupStrEqualNoCase(cmd, "FORMATBLOCK"))
      js_cmd = "formatBlock";
    else if (iupStrEqualNoCase(cmd, "FORECOLOR"))
      js_cmd = "foreColor";
    else if (iupStrEqualNoCase(cmd, "BACKCOLOR"))
      js_cmd = "backColor";

    char* result = gtkWebBrowserQueryCommandState(ih, js_cmd);
    if (result)
    {
      char* ret = iupStrReturnStr(result);
      free(result);
      return ret;
    }
  }
  return iupStrReturnBoolean(0);
}

static char* gtkWebBrowserGetCommandEnabledAttrib(Ihandle* ih)
{
  char* cmd = iupAttribGet(ih, "COMMAND");
  if (cmd)
  {
    const char* js_cmd = cmd;

    if (iupStrEqualNoCase(cmd, "FONTNAME"))
      js_cmd = "fontName";
    else if (iupStrEqualNoCase(cmd, "FONTSIZE"))
      js_cmd = "fontSize";
    else if (iupStrEqualNoCase(cmd, "FORMATBLOCK"))
      js_cmd = "formatBlock";
    else if (iupStrEqualNoCase(cmd, "FORECOLOR"))
      js_cmd = "foreColor";
    else if (iupStrEqualNoCase(cmd, "BACKCOLOR"))
      js_cmd = "backColor";

    char* result = gtkWebBrowserQueryCommandEnabled(ih, js_cmd);
    if (result)
    {
      char* ret = iupStrReturnStr(result);
      free(result);
      return ret;
    }
  }
  return iupStrReturnBoolean(0);
}

static char* gtkWebBrowserGetCommandValueAttrib(Ihandle* ih)
{
  char* cmd = iupAttribGet(ih, "COMMAND");
  if (cmd)
  {
    const char* js_cmd = cmd;

    if (iupStrEqualNoCase(cmd, "FONTNAME"))
      js_cmd = "fontName";
    else if (iupStrEqualNoCase(cmd, "FONTSIZE"))
      js_cmd = "fontSize";
    else if (iupStrEqualNoCase(cmd, "FORMATBLOCK"))
      js_cmd = "formatBlock";
    else if (iupStrEqualNoCase(cmd, "FORECOLOR"))
      js_cmd = "foreColor";
    else if (iupStrEqualNoCase(cmd, "BACKCOLOR"))
      js_cmd = "backColor";

    char* result = gtkWebBrowserQueryCommandValue(ih, js_cmd);
    if (result)
    {
      char* ret = iupStrReturnStr(result);
      free(result);
      return ret;
    }
  }
  return iupStrReturnStr("");
}

static char* gtkWebBrowserGetCommandTextAttrib(Ihandle* ih)
{
  char* cmd = iupAttribGet(ih, "COMMAND");
  if (cmd)
  {
    const char* js_cmd = cmd;

    if (iupStrEqualNoCase(cmd, "FONTNAME"))
      js_cmd = "fontName";
    else if (iupStrEqualNoCase(cmd, "FONTSIZE"))
      js_cmd = "fontSize";
    else if (iupStrEqualNoCase(cmd, "FORMATBLOCK"))
      js_cmd = "formatBlock";
    else if (iupStrEqualNoCase(cmd, "FORECOLOR"))
      js_cmd = "foreColor";
    else if (iupStrEqualNoCase(cmd, "BACKCOLOR"))
      js_cmd = "backColor";

    char* result = gtkWebBrowserQueryCommandValue(ih, js_cmd);
    if (result)
    {
      char* ret = iupStrReturnStr(result);
      free(result);
      return ret;
    }
  }
  return iupStrReturnStr("");
}

static char* gtkWebBrowserGetInnerTextAttrib(Ihandle* ih)
{
  char* element_id = iupAttribGet(ih, "ELEMENT_ID");
  if (!element_id)
    return NULL;

  char* escaped_id = gtkWebBrowserEscapeJavaScript(element_id);
  char js[512];
  snprintf(js, sizeof(js), "document.getElementById(%s)?.innerText || '';", escaped_id);
  free(escaped_id);

  char* result = gtkWebBrowserRunJavaScriptSync(ih, "%s", js);
  if (result)
  {
    char* ret = iupStrReturnStr(result);
    free(result);
    return ret;
  }
  return NULL;
}

static int gtkWebBrowserSetInnerTextAttrib(Ihandle* ih, const char* value)
{
  char* element_id = iupAttribGet(ih, "ELEMENT_ID");
  if (!element_id)
    return 0;

  char* escaped_id = gtkWebBrowserEscapeJavaScript(element_id);
  char* escaped_value = gtkWebBrowserEscapeJavaScript(value ? value : "");
  char js[1024];
  snprintf(js, sizeof(js), "if(document.getElementById(%s)) document.getElementById(%s).innerText = %s;",
           escaped_id, escaped_id, escaped_value);
  free(escaped_id);
  free(escaped_value);

  gtkWebBrowserRunJavascript(ih, "%s", js);
  return 0;
}

static char* gtkWebBrowserGetJavascriptAttrib(Ihandle* ih)
{
  return iupAttribGet(ih, "_IUPWEB_JS_RESULT");
}

static int gtkWebBrowserSetJavascriptAttrib(Ihandle* ih, const char* value)
{
  iupAttribSet(ih, "_IUPWEB_JS_RESULT", NULL);
  if (!value)
    return 0;

  char* result = gtkWebBrowserExecJavaScriptSync(ih, value);
  if (result)
  {
    iupAttribSetStr(ih, "_IUPWEB_JS_RESULT", result);
    free(result);
  }
  return 0;
}

static char* gtkWebBrowserGetAttributeAttrib(Ihandle* ih)
{
  char* element_id = iupAttribGet(ih, "ELEMENT_ID");
  char* attr_name = iupAttribGet(ih, "ATTRIBUTE_NAME");
  if (!element_id || !attr_name)
    return NULL;

  char* escaped_id = gtkWebBrowserEscapeJavaScript(element_id);
  char* escaped_attr = gtkWebBrowserEscapeJavaScript(attr_name);
  char js[512];
  snprintf(js, sizeof(js), "document.getElementById(%s)?.getAttribute(%s) || '';",
           escaped_id, escaped_attr);
  free(escaped_id);
  free(escaped_attr);

  char* result = gtkWebBrowserRunJavaScriptSync(ih, "%s", js);
  if (result)
  {
    char* ret = iupStrReturnStr(result);
    free(result);
    return ret;
  }
  return NULL;
}

static int gtkWebBrowserSetAttributeAttrib(Ihandle* ih, const char* value)
{
  char* element_id = iupAttribGet(ih, "ELEMENT_ID");
  char* attr_name = iupAttribGet(ih, "ATTRIBUTE_NAME");
  if (!element_id || !attr_name)
    return 0;

  char* escaped_id = gtkWebBrowserEscapeJavaScript(element_id);
  char* escaped_attr = gtkWebBrowserEscapeJavaScript(attr_name);
  char* escaped_value = gtkWebBrowserEscapeJavaScript(value ? value : "");
  char js[1024];
  snprintf(js, sizeof(js), "if(document.getElementById(%s)) document.getElementById(%s).setAttribute(%s, %s);",
           escaped_id, escaped_id, escaped_attr, escaped_value);
  free(escaped_id);
  free(escaped_attr);
  free(escaped_value);

  gtkWebBrowserRunJavascript(ih, "%s", js);
  return 0;
}

static char* gtkWebBrowserGetDirtyAttrib(Ihandle* ih)
{
  if (iupAttribGet(ih, "_IUPWEB_DIRTY"))
    return "YES";

  char* result = gtkWebBrowserRunJavaScriptSync(ih, "window.iupGetDirtyFlag ? window.iupGetDirtyFlag() : false;");
  if (result)
  {
    int dirty = strcmp(result, "true") == 0;
    free(result);
    if (dirty)
    {
      iupAttribSet(ih, "_IUPWEB_DIRTY", "1");
      gtkWebBrowserRunJavascript(ih, "if (window.iupClearDirtyFlag) window.iupClearDirtyFlag();");
    }
    return iupStrReturnBoolean(dirty);
  }
  return "NO";
}

static int gtkWebBrowserSetFindAttrib(Ihandle* ih, const char* value)
{
  if (!value || !value[0])
    return 0;

  char* escaped_text = gtkWebBrowserEscapeJavaScript(value);
  char js[512];
  snprintf(js, sizeof(js), "window.find(%s);", escaped_text);
  free(escaped_text);

  gtkWebBrowserRunJavascript(ih, "%s", js);
  return 0;
}

#if (defined(IUPWEB_USE_WEBKIT2) || defined(IUPWEB_USE_DLOPEN))
static void gtkWebBrowserDocumentLoadFinished_WK2(WebKitWebView *web_view, WebKitLoadEvent load_event, Ihandle *ih)
{
#ifdef IUPWEB_USE_DLOPEN
  if (load_event != WEBKIT_LOAD_EVENT_FINISHED)
    return;
#else
  if (load_event != WEBKIT_LOAD_FINISHED)
    return;
#endif

  gtkWebBrowserInitSelectionTracking(ih);

  if (iupAttribGet(ih, "_IUPWEB_EDITABLE"))
    gtkWebBrowserRunJavascript(ih, "document.body.contentEditable = 'true';");
  else
    gtkWebBrowserRunJavascript(ih, "document.body.contentEditable = 'false';");

  gtkWebBrowserUpdateHistory(ih);

  IFns cb = (IFns)IupGetCallback(ih, "COMPLETED_CB");
  if (cb)
    cb(ih, (char*)webkit_web_view_get_uri(web_view));

  IFn cb_update = (IFn)IupGetCallback(ih, "UPDATE_CB");
  if (cb_update)
    cb_update(ih);
}
#endif

#if (defined(IUPWEB_USE_WEBKIT1) || defined(IUPWEB_USE_DLOPEN))
static void gtkWebBrowserDocumentLoadFinished_WK1(WebKitWebView *web_view, WebKitWebFrame *frame, Ihandle *ih)
{
  gtkWebBrowserInitSelectionTracking(ih);

  if (iupAttribGet(ih, "_IUPWEB_EDITABLE"))
    gtkWebBrowserRunJavascript(ih, "document.body.contentEditable = 'true';");
  else
    gtkWebBrowserRunJavascript(ih, "document.body.contentEditable = 'false';");

  gtkWebBrowserUpdateHistory(ih);

  IFns cb = (IFns)IupGetCallback(ih, "COMPLETED_CB");
  if (cb)
    cb(ih, (char*)webkit_web_frame_get_uri(frame));

  IFn cb_update = (IFn)IupGetCallback(ih, "UPDATE_CB");
  if (cb_update)
    cb_update(ih);

  (void)web_view;
}
#endif

#if (defined(IUPWEB_USE_WEBKIT2) || defined(IUPWEB_USE_DLOPEN))
static gboolean gtkWebBrowserLoadError_WK2(WebKitWebView *web_view, WebKitLoadEvent load_event,
                                       gchar *failing_uri, GError *error, Ihandle *ih)
{
  gtkWebBrowserUpdateHistory(ih);

  IFns cb = (IFns)IupGetCallback(ih, "ERROR_CB");
  if (cb)
    cb(ih, failing_uri);

  (void)web_view;
  (void)load_event;
  (void)error;
  return FALSE;
}
#endif

#if (defined(IUPWEB_USE_WEBKIT1) || defined(IUPWEB_USE_DLOPEN))
static gboolean gtkWebBrowserLoadError_WK1(WebKitWebView *web_view, WebKitWebFrame *frame,
                                       gchar *uri, gpointer web_error, Ihandle *ih)
{
  gtkWebBrowserUpdateHistory(ih);

  IFns cb = (IFns)IupGetCallback(ih, "ERROR_CB");
  if (cb)
    cb(ih, uri);

  (void)web_view;
  (void)frame;
  (void)web_error;
  return FALSE;
}
#endif

#if (defined(IUPWEB_USE_WEBKIT2) || defined(IUPWEB_USE_DLOPEN))
static gboolean gtkWebBrowserNavigate_WK2(WebKitWebView *web_view, WebKitPolicyDecision *decision,
                                 WebKitPolicyDecisionType decision_type, Ihandle *ih)
{
  if (decision_type != WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION)
    return FALSE;

  if (iupAttribGet(ih, "_IUPWEB_IGNORE_NAVIGATE"))
    return FALSE;

  IFns cb = (IFns)IupGetCallback(ih, "NAVIGATE_CB");
  if (cb)
  {
    WebKitNavigationAction* nav_action = webkit_navigation_policy_decision_get_navigation_action((WebKitNavigationPolicyDecision*)decision);
    WebKitURIRequest* request = webkit_navigation_action_get_request(nav_action);
    const gchar* uri = webkit_uri_request_get_uri(request);

    if (cb(ih, (char*)uri) == IUP_IGNORE)
      return TRUE;
  }

  (void)web_view;
  return FALSE;
}
#endif

#if (defined(IUPWEB_USE_WEBKIT1) || defined(IUPWEB_USE_DLOPEN))
static gboolean gtkWebBrowserNavigate_WK1(WebKitWebView *web_view, WebKitWebFrame *frame, WebKitNetworkRequest *request,
                                 WebKitWebNavigationAction *navigation_action, WebKitWebPolicyDecision *policy_decision, Ihandle *ih)
{
  if (iupAttribGet(ih, "_IUPWEB_IGNORE_NAVIGATE"))
    return FALSE;

  IFns cb = (IFns)IupGetCallback(ih, "NAVIGATE_CB");
  if (cb)
  {
    if (cb(ih, (char*)webkit_network_request_get_uri(request)) == IUP_IGNORE)
      return TRUE;
  }

  (void)web_view;
  (void)frame;
  (void)navigation_action;
  (void)policy_decision;
  return FALSE;
}
#endif

#if (defined(IUPWEB_USE_WEBKIT2) || defined(IUPWEB_USE_DLOPEN))
static WebKitWebView* gtkWebBrowserNewWindow_WK2(WebKitWebView *web_view, WebKitNavigationAction *navigation_action, Ihandle *ih)
{
  IFns cb = (IFns)IupGetCallback(ih, "NEWWINDOW_CB");
  if (cb)
  {
    WebKitURIRequest* request = webkit_navigation_action_get_request(navigation_action);
    const gchar* uri = webkit_uri_request_get_uri(request);
    cb(ih, (char*)uri);
  }

  (void)web_view;
  return NULL;
}
#endif

#if (defined(IUPWEB_USE_WEBKIT1) || defined(IUPWEB_USE_DLOPEN))
static WebKitWebView* gtkWebBrowserNewWindow_WK1(WebKitWebView *web_view, WebKitWebFrame *frame, Ihandle *ih)
{
  IFns cb = (IFns)IupGetCallback(ih, "NEWWINDOW_CB");
  if (cb)
  {
    cb(ih, (char*)webkit_web_frame_get_uri(frame));
  }

  (void)web_view;
  (void)frame;
  return NULL;
}
#endif

#if (defined(IUPWEB_USE_WEBKIT2) || defined(IUPWEB_USE_WEBKIT6) || defined(IUPWEB_USE_DLOPEN))
#if defined(IUPWEB_USE_WEBKIT6) && !defined(IUPWEB_USE_DLOPEN)
/* WebKit6 uses JSCValue instead of WebKitJavascriptResult */
static void gtkWebBrowserScriptMessageReceived_Update(WebKitUserContentManager *manager, JSCValue *js_value, Ihandle *ih)
{
  IFn cb_update = (IFn)IupGetCallback(ih, "UPDATE_CB");
  if (cb_update)
    cb_update(ih);

  (void)manager;
  (void)js_value;
}

static void gtkWebBrowserScriptMessageReceived_Dirty(WebKitUserContentManager *manager, JSCValue *js_value, Ihandle *ih)
{
  iupAttribSet(ih, "_IUPWEB_DIRTY", "1");

  (void)manager;
  (void)js_value;
}
#else
/* WebKit2 uses WebKitJavascriptResult */
static void gtkWebBrowserScriptMessageReceived_Update(WebKitUserContentManager *manager, WebKitJavascriptResult *js_result, Ihandle *ih)
{
  IFn cb_update = (IFn)IupGetCallback(ih, "UPDATE_CB");
  if (cb_update)
    cb_update(ih);

  (void)manager;
  (void)js_result;
}

static void gtkWebBrowserScriptMessageReceived_Dirty(WebKitUserContentManager *manager, WebKitJavascriptResult *js_result, Ihandle *ih)
{
  iupAttribSet(ih, "_IUPWEB_DIRTY", "1");

  (void)manager;
  (void)js_result;
}
#endif
#endif

static void gtkWebBrowserDummyLogFunc(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data)
{
  (void)log_domain;
  (void)log_level;
  (void)message;
  (void)user_data;
}

static int gtkWebBrowserMapMethod(Ihandle* ih)
{
  GtkScrolledWindow* scrolled_window = NULL;

  if (s_use_webkit2 == -1)
    return IUP_ERROR;

#if (defined(IUPWEB_USE_WEBKIT2) || defined(IUPWEB_USE_DLOPEN))
  #ifdef IUPWEB_USE_DLOPEN
  if (s_use_webkit2)
  #endif
  {
    #if defined(IUPWEB_USE_WEBKIT6) || defined(IUPWEB_USE_DLOPEN)
    #ifdef IUPWEB_USE_DLOPEN
    if (s_use_webkit6)
    #endif
    {
      /* WebKit6: Create WebView first, then get manager */
      ih->handle = (GtkWidget*)webkit_web_view_new();
      if (ih->handle)
      {
        WebKitUserContentManager* manager = webkit_web_view_get_user_content_manager((WebKitWebView*)ih->handle);
        /* WebKit6: Basic function takes world_name parameter (use NULL for default world) */
        #ifdef IUPWEB_USE_DLOPEN
        webkit_user_content_manager_register_script_message_handler_wk6(manager, "iupUpdateCb", NULL);
        webkit_user_content_manager_register_script_message_handler_wk6(manager, "iupDirtyFlag", NULL);
        #else
        webkit_user_content_manager_register_script_message_handler(manager, "iupUpdateCb", NULL);
        webkit_user_content_manager_register_script_message_handler(manager, "iupDirtyFlag", NULL);
        #endif
        g_signal_connect(G_OBJECT(manager), "script-message-received::iupUpdateCb", G_CALLBACK(gtkWebBrowserScriptMessageReceived_Update), ih);
        g_signal_connect(G_OBJECT(manager), "script-message-received::iupDirtyFlag", G_CALLBACK(gtkWebBrowserScriptMessageReceived_Dirty), ih);
      }
    }
    #ifdef IUPWEB_USE_DLOPEN
    else
    #endif
    #endif
    #if !defined(IUPWEB_USE_WEBKIT6) || defined(IUPWEB_USE_DLOPEN)
    {
      /* WebKit2: Create manager first, then WebView */
      WebKitUserContentManager* manager = webkit_user_content_manager_new();
      webkit_user_content_manager_register_script_message_handler(manager, "iupUpdateCb");
      webkit_user_content_manager_register_script_message_handler(manager, "iupDirtyFlag");
      g_signal_connect(G_OBJECT(manager), "script-message-received::iupUpdateCb", G_CALLBACK(gtkWebBrowserScriptMessageReceived_Update), ih);
      g_signal_connect(G_OBJECT(manager), "script-message-received::iupDirtyFlag", G_CALLBACK(gtkWebBrowserScriptMessageReceived_Dirty), ih);

      ih->handle = (GtkWidget*)webkit_web_view_new_with_user_content_manager(manager);
      g_object_unref(manager);
    }
    #endif
  }
  #ifdef IUPWEB_USE_DLOPEN
  else
  {
    ih->handle = (GtkWidget*)webkit_web_view_new();
  }
  #endif
#endif
#if defined(IUPWEB_USE_WEBKIT1) && !defined(IUPWEB_USE_DLOPEN)
  ih->handle = (GtkWidget*)webkit_web_view_new();
#endif

  if (!ih->handle)
    return IUP_ERROR;

  if (s_use_webkit2 == 0)
  {
#if GTK_CHECK_VERSION(4, 0, 0)
    scrolled_window = (GtkScrolledWindow*)gtk_scrolled_window_new();
#else
    scrolled_window = (GtkScrolledWindow*)gtk_scrolled_window_new(NULL, NULL);
#endif
    if (!scrolled_window)
      return IUP_ERROR;

    {
#if GTK_CHECK_VERSION(2, 6, 0) && !GTK_CHECK_VERSION(4, 0, 0)
      GLogFunc def_func = g_log_set_default_handler(gtkWebBrowserDummyLogFunc, NULL);
#endif
#if GTK_CHECK_VERSION(4, 0, 0)
      gtk_scrolled_window_set_child(scrolled_window, ih->handle);
#else
      gtk_container_add((GtkContainer*)scrolled_window, ih->handle);
#endif
#if GTK_CHECK_VERSION(2, 6, 0) && !GTK_CHECK_VERSION(4, 0, 0)
      g_log_set_default_handler(def_func, NULL);
#endif
    }

    if (ih->data->sb)
    {
      GtkPolicyType hscrollbar_policy = GTK_POLICY_NEVER, vscrollbar_policy = GTK_POLICY_NEVER;
      if (ih->data->sb & IUP_SB_HORIZ)
        hscrollbar_policy = GTK_POLICY_AUTOMATIC;
      if (ih->data->sb & IUP_SB_VERT)
        vscrollbar_policy = GTK_POLICY_AUTOMATIC;
      gtk_scrolled_window_set_policy(scrolled_window, hscrollbar_policy, vscrollbar_policy);
    }
    else
      gtk_scrolled_window_set_policy(scrolled_window, GTK_POLICY_NEVER, GTK_POLICY_NEVER);

#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_widget_set_visible((GtkWidget*)scrolled_window, TRUE);
#else
    gtk_widget_show((GtkWidget*)scrolled_window);
#endif

    iupAttribSet(ih, "_IUP_EXTRAPARENT", (char*)scrolled_window);
  }

  iupgtkAddToParent(ih);

#if GTK_CHECK_VERSION(4, 0, 0)
  /* GTK4 uses event controllers instead of signals for these events */
  iupgtk4SetupEnterLeaveEvents(ih->handle, ih);
  iupgtk4SetupFocusEvents(ih->handle, ih);
  /* Note: GTK4 doesn't have "show-help" signal, and query-tooltip has incompatible signature */
#else
  /* GTK3 uses signals for events */
  g_signal_connect(G_OBJECT(ih->handle), "enter-notify-event", G_CALLBACK(iupgtkEnterLeaveEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "leave-notify-event", G_CALLBACK(iupgtkEnterLeaveEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "focus-in-event",     G_CALLBACK(iupgtkFocusInOutEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "focus-out-event",    G_CALLBACK(iupgtkFocusInOutEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "show-help",          G_CALLBACK(iupgtkShowHelp),        ih);
#endif

#if (defined(IUPWEB_USE_WEBKIT2) || defined(IUPWEB_USE_DLOPEN))
  #ifdef IUPWEB_USE_DLOPEN
  if (s_use_webkit2)
  #endif
  {
    g_signal_connect(G_OBJECT(ih->handle), "create", G_CALLBACK(gtkWebBrowserNewWindow_WK2), ih);
    g_signal_connect(G_OBJECT(ih->handle), "decide-policy", G_CALLBACK(gtkWebBrowserNavigate_WK2), ih);
    g_signal_connect(G_OBJECT(ih->handle), "load-failed", G_CALLBACK(gtkWebBrowserLoadError_WK2), ih);
    g_signal_connect(G_OBJECT(ih->handle), "load-changed", G_CALLBACK(gtkWebBrowserDocumentLoadFinished_WK2), ih);
  }
#endif
#if (defined(IUPWEB_USE_WEBKIT1) || defined(IUPWEB_USE_DLOPEN))
  #ifdef IUPWEB_USE_DLOPEN
  if (s_use_webkit2 == 0)
  #endif
  {
    g_signal_connect(G_OBJECT(ih->handle), "create-web-view", G_CALLBACK(gtkWebBrowserNewWindow_WK1), ih);
    g_signal_connect(G_OBJECT(ih->handle), "navigation-policy-decision-requested", G_CALLBACK(gtkWebBrowserNavigate_WK1), ih);
    g_signal_connect(G_OBJECT(ih->handle), "load-error", G_CALLBACK(gtkWebBrowserLoadError_WK1), ih);
    g_signal_connect(G_OBJECT(ih->handle), "document-load-finished", G_CALLBACK(gtkWebBrowserDocumentLoadFinished_WK1), ih);
  }
#endif

  if (scrolled_window)
    gtk_widget_realize((GtkWidget*)scrolled_window);

  gtk_widget_realize(ih->handle);

  return IUP_NOERROR;
}

static void gtkWebBrowserComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
  int natural_w = 0, natural_h = 0;
  (void)children_expand;

  iupdrvFontGetCharSize(ih, &natural_w, &natural_h);

  *w = natural_w;
  *h = natural_h;
}

static void gtkWebBrowserUnMapMethod(Ihandle* ih)
{
#if (defined(IUPWEB_USE_WEBKIT2) || defined(IUPWEB_USE_WEBKIT6) || defined(IUPWEB_USE_DLOPEN))
  #ifdef IUPWEB_USE_DLOPEN
  if (s_use_webkit2 && ih->handle)
  #else
  if (ih->handle)
  #endif
  {
    WebKitUserContentManager* manager = webkit_web_view_get_user_content_manager((WebKitWebView*)ih->handle);
    if (manager)
    {
      /* Unregister script message handlers to release references */
      #if defined(IUPWEB_USE_DLOPEN)
      if (s_use_webkit6)
      {
        webkit_user_content_manager_unregister_script_message_handler_wk6(manager, "iupUpdateCb", NULL);
        webkit_user_content_manager_unregister_script_message_handler_wk6(manager, "iupDirtyFlag", NULL);
      }
      else
      #elif defined(IUPWEB_USE_WEBKIT6)
      {
        webkit_user_content_manager_unregister_script_message_handler(manager, "iupUpdateCb", NULL);
        webkit_user_content_manager_unregister_script_message_handler(manager, "iupDirtyFlag", NULL);
      }
      #else
      #endif
      #if !defined(IUPWEB_USE_WEBKIT6) || defined(IUPWEB_USE_DLOPEN)
      {
        webkit_user_content_manager_unregister_script_message_handler(manager, "iupUpdateCb");
        webkit_user_content_manager_unregister_script_message_handler(manager, "iupDirtyFlag");
      }
      #endif
    }
  }
#endif

  /* Call base unmap to destroy the widget */
  iupdrvBaseUnMapMethod(ih);
}

static int gtkWebBrowserCreateMethod(Ihandle* ih, void **params)
{
  (void)params;

  ih->data = iupALLOCCTRLDATA();

  ih->expand = IUP_EXPAND_BOTH;
  ih->data->sb = IUP_SB_HORIZ | IUP_SB_VERT;

  return IUP_NOERROR;
}

Iclass* iupWebBrowserNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "webbrowser";
  ic->cons = "WebBrowser";
  ic->format = NULL;
  ic->nativetype  = IUP_TYPECONTROL;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 1;
  ic->has_attrib_id = 1;

  ic->New = NULL;
  ic->Create = gtkWebBrowserCreateMethod;
  ic->Map = gtkWebBrowserMapMethod;
  ic->UnMap = gtkWebBrowserUnMapMethod;
  ic->ComputeNaturalSize = gtkWebBrowserComputeNaturalSizeMethod;
  ic->LayoutUpdate = iupdrvBaseLayoutUpdateMethod;

  iupClassRegisterCallback(ic, "NEWWINDOW_CB", "s");
  iupClassRegisterCallback(ic, "NAVIGATE_CB", "s");
  iupClassRegisterCallback(ic, "ERROR_CB", "s");
  iupClassRegisterCallback(ic, "COMPLETED_CB", "s");
  iupClassRegisterCallback(ic, "UPDATE_CB", "");

  iupBaseRegisterCommonAttrib(ic);

  iupBaseRegisterVisualAttrib(ic);

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "VALUE", gtkWebBrowserGetValueAttrib, gtkWebBrowserSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKFORWARD", NULL, gtkWebBrowserSetBackForwardAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "GOBACK", NULL, gtkWebBrowserSetGoBackAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "GOFORWARD", NULL, gtkWebBrowserSetGoForwardAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STOP", NULL, gtkWebBrowserSetStopAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "RELOAD", NULL, gtkWebBrowserSetReloadAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HTML", gtkWebBrowserGetHTMLAttrib, gtkWebBrowserSetHTMLAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STATUS", gtkWebBrowserGetStatusAttrib, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ZOOM", gtkWebBrowserGetZoomAttrib, gtkWebBrowserSetZoomAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PRINT", NULL, gtkWebBrowserSetPrintAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CANGOBACK", gtkWebBrowserGetCanGoBackAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CANGOFORWARD", gtkWebBrowserGetCanGoForwardAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "EDITABLE", gtkWebBrowserGetEditableAttrib, gtkWebBrowserSetEditableAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NEW", NULL, gtkWebBrowserSetNewAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "OPENFILE", NULL, gtkWebBrowserSetOpenAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SAVEFILE", NULL, gtkWebBrowserSetSaveAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "UNDO", NULL, gtkWebBrowserSetUndoAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "REDO", NULL, gtkWebBrowserSetRedoAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COPY", NULL, gtkWebBrowserSetCopyAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CUT", NULL, gtkWebBrowserSetCutAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PASTE", gtkWebBrowserGetPasteAttrib, gtkWebBrowserSetPasteAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTALL", NULL, gtkWebBrowserSetSelectAllAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "EXECCOMMAND", NULL, gtkWebBrowserExecCommandAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERTIMAGE", NULL, gtkWebBrowserSetInsertImageAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERTIMAGEFILE", NULL, gtkWebBrowserSetInsertImageFileAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CREATELINK", NULL, gtkWebBrowserSetCreateLinkAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERTTEXT", NULL, gtkWebBrowserSetInsertTextAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERTHTML", NULL, gtkWebBrowserSetInsertHtmlAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
#if defined(IUPWEB_USE_WEBKIT2) || defined(IUPWEB_USE_DLOPEN)
  iupClassRegisterAttribute(ic, "FONTNAME", gtkWebBrowserGetFontNameAttrib, gtkWebBrowserSetFontNameAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FONTSIZE", gtkWebBrowserGetFontSizeAttrib, gtkWebBrowserSetFontSizeAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATBLOCK", gtkWebBrowserGetFormatBlockAttrib, gtkWebBrowserSetFormatBlockAttrib, NULL, NULL, IUPAF_NO_INHERIT);
#else
  iupClassRegisterAttribute(ic, "FONTNAME", NULL, gtkWebBrowserSetFontNameAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FONTSIZE", NULL, gtkWebBrowserSetFontSizeAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATBLOCK", NULL, gtkWebBrowserSetFormatBlockAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
#endif
  iupClassRegisterAttribute(ic, "FORECOLOR", gtkWebBrowserGetForeColorAttrib, gtkWebBrowserSetForeColorAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKCOLOR", gtkWebBrowserGetBackColorAttrib, gtkWebBrowserSetBackColorAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "COMMAND", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDSHOWUI", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDSTATE", gtkWebBrowserGetCommandStateAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDENABLED", gtkWebBrowserGetCommandEnabledAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDTEXT", gtkWebBrowserGetCommandTextAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDVALUE", gtkWebBrowserGetCommandValueAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "ELEMENT_ID", NULL, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INNERTEXT", gtkWebBrowserGetInnerTextAttrib, gtkWebBrowserSetInnerTextAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ATTRIBUTE_NAME", NULL, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ATTRIBUTE", gtkWebBrowserGetAttributeAttrib, gtkWebBrowserSetAttributeAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "JAVASCRIPT", gtkWebBrowserGetJavascriptAttrib, gtkWebBrowserSetJavascriptAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "BACKCOUNT", gtkWebBrowserGetBackCountAttrib, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORWARDCOUNT", gtkWebBrowserGetForwardCountAttrib, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "ITEMHISTORY",  gtkWebBrowserGetItemHistoryAttrib,  NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "DIRTY", gtkWebBrowserGetDirtyAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FIND", NULL, gtkWebBrowserSetFindAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PRINTPREVIEW", NULL, gtkWebBrowserSetPrintPreviewAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  return ic;
}
