#ifndef IUPWEBGTK_DLOPEN_H
#define IUPWEBGTK_DLOPEN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <gtk/gtk.h>

#ifdef IUPWEB_USE_DLOPEN
#include <dlfcn.h>
#endif

/*
  This file provides all necessary definitions for both WebKit1 and WebKit2
  APIs to be loaded dynamically at runtime, without including system webkit headers.
*/

/* --- Common Types --- */
typedef struct _WebKitWebView WebKitWebView;
typedef struct _WebKitNetworkRequest WebKitNetworkRequest;
typedef struct _WebKitPolicyDecision WebKitPolicyDecision;
typedef struct _WebKitPrintOperation WebKitPrintOperation;

/* --- WebKit2 Types --- */
typedef struct _WebKitBackForwardList WebKitBackForwardList;
typedef struct _WebKitBackForwardListItem WebKitBackForwardListItem;
typedef struct _WebKitNavigationAction WebKitNavigationAction;
typedef struct _WebKitWebResource WebKitWebResource;
typedef struct _WebKitNavigationPolicyDecision WebKitNavigationPolicyDecision;
typedef struct _WebKitURIRequest WebKitURIRequest;
typedef struct _WebKitJavascriptResult WebKitJavascriptResult;
typedef struct _WebKitUserContentManager WebKitUserContentManager;
typedef struct _WebKitUserScript WebKitUserScript;

/* --- JSCore Types (used for JavaScript results in WebKit2) --- */
typedef const struct OpaqueJSContext* JSGlobalContextRef;
typedef const struct OpaqueJSValue* JSValueRef;
typedef struct OpaqueJSString* JSStringRef;

/* --- JSCore GObject Types (used in WebKit6) --- */
typedef struct _JSCContext JSCContext;
typedef struct _JSCValue JSCValue;
typedef struct _JSCException JSCException;

/* --- WebKit1 Types --- */
typedef struct _WebKitWebBackForwardList WebKitWebBackForwardList;
typedef struct _WebKitWebHistoryItem WebKitWebHistoryItem;
typedef struct _WebKitWebFrame WebKitWebFrame;
typedef struct _WebKitWebDataSource WebKitWebDataSource;
typedef struct _WebKitWebNavigationAction WebKitWebNavigationAction;
typedef struct _WebKitWebPolicyDecision WebKitWebPolicyDecision;

/* --- Enums --- */

/* WebKitLoadEvent (WK2) */
typedef enum {
    WEBKIT_LOAD_EVENT_STARTED,
    WEBKIT_LOAD_EVENT_REDIRECTED,
    WEBKIT_LOAD_EVENT_COMMITTED,
    WEBKIT_LOAD_EVENT_FINISHED
} WebKitLoadEvent;

/* WebKitLoadStatus (WK1) */
typedef enum {
    WEBKIT_LOAD_STATUS_PROVISIONAL = 0,
    WEBKIT_LOAD_STATUS_COMMITTED   = 1,
    WEBKIT_LOAD_STATUS_FINISHED    = 2,
    WEBKIT_LOAD_STATUS_FIRST_VISUALLY_NON_EMPTY_LAYOUT = 3,
    WEBKIT_LOAD_STATUS_FAILED      = 4
} WebKitLoadStatus;

/* WebKitPolicyDecisionType (WK2) */
typedef enum {
    WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION,
    WEBKIT_POLICY_DECISION_TYPE_NEW_WINDOW_ACTION,
    WEBKIT_POLICY_DECISION_TYPE_RESPONSE
} WebKitPolicyDecisionType;

/* WebKitSaveMode (WK2) */
typedef enum {
    WEBKIT_SAVE_MODE_MHTML
} WebKitSaveMode;

/* WebKitUserScriptInjectionTime (WK2) */
typedef enum {
    WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START,
    WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_END
} WebKitUserScriptInjectionTime;

/* WebKitUserContentInjectedFrames (WK2) */
typedef enum {
    WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
    WEBKIT_USER_CONTENT_INJECT_TOP_FRAME
} WebKitUserContentInjectedFrames;

/* --- WebKit2 Editing Commands (Strings) --- */
#define WEBKIT_EDITING_COMMAND_CUT "Cut"
#define WEBKIT_EDITING_COMMAND_COPY "Copy"
#define WEBKIT_EDITING_COMMAND_PASTE "Paste"
#define WEBKIT_EDITING_COMMAND_UNDO "Undo"
#define WEBKIT_EDITING_COMMAND_REDO "Redo"
#define WEBKIT_EDITING_COMMAND_SELECT_ALL "SelectAll"
#define WEBKIT_EDITING_COMMAND_INSERT_IMAGE "InsertImage"
#define WEBKIT_EDITING_COMMAND_CREATE_LINK "CreateLink"

#ifdef IUPWEB_USE_DLOPEN

/* --- Common Function Pointers --- */
/* These functions have the same name in both APIs */
static GtkWidget* (*webkit_web_view_new)(void);
static const gchar* (*webkit_web_view_get_uri)(WebKitWebView *web_view);
static void (*webkit_web_view_load_uri)(WebKitWebView *web_view, const gchar *uri);
static void (*webkit_web_view_go_back)(WebKitWebView *web_view);
static void (*webkit_web_view_go_forward)(WebKitWebView *web_view);
static gboolean (*webkit_web_view_can_go_back)(WebKitWebView *web_view);
static gboolean (*webkit_web_view_can_go_forward)(WebKitWebView *web_view);
static void (*webkit_web_view_reload)(WebKitWebView *web_view);
static void (*webkit_web_view_stop_loading)(WebKitWebView *web_view);
static void (*webkit_web_view_set_editable)(WebKitWebView *web_view, gboolean editable);
static gdouble (*webkit_web_view_get_zoom_level)(WebKitWebView *web_view);
static void (*webkit_web_view_set_zoom_level)(WebKitWebView *web_view, gdouble zoom_level);
static const gchar* (*webkit_network_request_get_uri)(WebKitNetworkRequest *request);
/* WK1 and WK2 use the same function name but different struct *return* types.
  The common function pointer uses the WK2 (WebKitBackForwardList*) type.
  When using WK1, the result must be cast to (WebKitWebBackForwardList*).
*/
static WebKitBackForwardList* (*webkit_web_view_get_back_forward_list)(WebKitWebView *web_view);


/* --- WebKit2 Function Pointers --- */
static WebKitWebView* (*webkit_web_view_new_with_context)(void*); /* WK2 only */
static WebKitBackForwardListItem* (*webkit_back_forward_list_get_nth_item)(WebKitBackForwardList *back_forward_list, gint index);
static const gchar* (*webkit_back_forward_list_item_get_uri)(WebKitBackForwardListItem *item);
static GList* (*webkit_back_forward_list_get_forward_list)(WebKitBackForwardList *back_forward_list);
static GList* (*webkit_back_forward_list_get_back_list)(WebKitBackForwardList *back_forward_list);
static void (*webkit_web_view_load_html)(WebKitWebView *web_view, const gchar *content, const gchar *base_uri);
static WebKitWebResource* (*webkit_web_view_get_main_resource)(WebKitWebView *web_view);
static void (*webkit_web_resource_get_data)(WebKitWebResource *resource, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data);
static guchar* (*webkit_web_resource_get_data_finish)(WebKitWebResource *resource, GAsyncResult *result, gsize *length, GError **error);
static void (*webkit_web_view_save_to_file)(WebKitWebView *web_view, GFile *file, WebKitSaveMode save_mode, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data);
static void (*webkit_web_view_execute_editing_command)(WebKitWebView *web_view, const gchar *command);
static void (*webkit_web_view_execute_editing_command_with_argument)(WebKitWebView *web_view, const gchar *command, const gchar* argument);
static gboolean (*webkit_web_view_is_editable)(WebKitWebView *web_view);
static WebKitPrintOperation* (*webkit_print_operation_new)(WebKitWebView *web_view);
static void (*webkit_print_operation_run_dialog)(WebKitPrintOperation *print_operation, GtkWindow *parent);
static void (*webkit_print_operation_print)(WebKitPrintOperation *print_operation);
static gboolean (*webkit_web_view_is_loading)(WebKitWebView* web_view);
static void (*webkit_web_view_run_javascript)(WebKitWebView *web_view, const gchar *script, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data);
static WebKitJavascriptResult* (*webkit_web_view_run_javascript_finish)(WebKitWebView *web_view, GAsyncResult *result, GError **error);
static JSGlobalContextRef (*webkit_javascript_result_get_global_context)(WebKitJavascriptResult *js_result);
static JSValueRef (*webkit_javascript_result_get_value)(WebKitJavascriptResult *js_result);
static JSCValue* (*webkit_javascript_result_get_js_value)(WebKitJavascriptResult *js_result);
static void (*webkit_javascript_result_unref)(WebKitJavascriptResult *js_result);
static WebKitNavigationAction* (*webkit_navigation_policy_decision_get_navigation_action)(WebKitNavigationPolicyDecision *decision);
static WebKitURIRequest* (*webkit_navigation_action_get_request)(WebKitNavigationAction *navigation);
static const gchar* (*webkit_uri_request_get_uri)(WebKitURIRequest *request);
static WebKitUserContentManager* (*webkit_user_content_manager_new)(void);
static void (*webkit_user_content_manager_add_script)(WebKitUserContentManager *manager, WebKitUserScript *script);
static gboolean (*webkit_user_content_manager_register_script_message_handler)(WebKitUserContentManager *manager, const gchar *name);
static void (*webkit_user_content_manager_unregister_script_message_handler)(WebKitUserContentManager *manager, const gchar *name);
static WebKitUserScript* (*webkit_user_script_new)(const gchar *source, WebKitUserContentInjectedFrames injected_frames, WebKitUserScriptInjectionTime injection_time, const gchar* const *whitelist, const gchar* const *blacklist);
static void (*webkit_user_script_unref)(WebKitUserScript *user_script);
static WebKitWebView* (*webkit_web_view_new_with_user_content_manager)(WebKitUserContentManager *user_content_manager);
static WebKitUserContentManager* (*webkit_web_view_get_user_content_manager)(WebKitWebView *web_view);

/* --- JSCore Function Pointers (for JavaScript result handling in WebKit2) --- */
static JSStringRef (*JSValueToStringCopy)(JSGlobalContextRef ctx, JSValueRef value, JSValueRef* exception);
static size_t (*JSStringGetMaximumUTF8CStringSize)(JSStringRef string);
static size_t (*JSStringGetUTF8CString)(JSStringRef string, char* buffer, size_t bufferSize);
static void (*JSStringRelease)(JSStringRef string);
static int (*JSValueIsNull)(JSGlobalContextRef ctx, JSValueRef value);
static int (*JSValueIsUndefined)(JSGlobalContextRef ctx, JSValueRef value);

/* --- WebKit6 Function Pointers (GTK4) --- */
/* Note: webkit_web_view_evaluate_javascript replaces webkit_web_view_run_javascript */
static void (*webkit_web_view_evaluate_javascript)(WebKitWebView *web_view, const char *script, gssize length, const char *world_name, const char *source_uri, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data);
static JSCValue* (*webkit_web_view_evaluate_javascript_finish)(WebKitWebView *web_view, GAsyncResult *result, GError **error);
/* Note: WebKit6 changed signature - basic function now takes world_name (3 params vs 2 in WK2) */
static gboolean (*webkit_user_content_manager_register_script_message_handler_wk6)(WebKitUserContentManager *manager, const char *name, const char *world_name);
static void (*webkit_user_content_manager_unregister_script_message_handler_wk6)(WebKitUserContentManager *manager, const char *name, const char *world_name);

/* --- JSCore GObject Function Pointers (for JavaScript in WebKit6) --- */
static char* (*jsc_value_to_string)(JSCValue *value);
static gboolean (*jsc_value_is_null)(JSCValue *value);
static gboolean (*jsc_value_is_undefined)(JSCValue *value);
static gboolean (*jsc_value_is_number)(JSCValue *value);
static gboolean (*jsc_value_is_string)(JSCValue *value);
static gboolean (*jsc_value_is_boolean)(JSCValue *value);
static JSCContext* (*jsc_value_get_context)(JSCValue *value);
static JSCException* (*jsc_context_get_exception)(JSCContext *context);
static char* (*jsc_exception_get_message)(JSCException *exception);

/* --- WebKit1 Function Pointers --- */
static WebKitWebHistoryItem* (*webkit_web_back_forward_list_get_nth_item)(WebKitWebBackForwardList *back_forward_list, gint index);
static const gchar* (*webkit_web_history_item_get_uri)(WebKitWebHistoryItem *item);
static gint (*webkit_web_back_forward_list_get_forward_length)(WebKitWebBackForwardList *back_forward_list);
static gint (*webkit_web_back_forward_list_get_back_length)(WebKitWebBackForwardList *back_forward_list);
static void (*webkit_web_view_load_string)(WebKitWebView *web_view, const gchar *content, const gchar *mime_type, const gchar *encoding, const gchar *base_uri);
static WebKitWebFrame* (*webkit_web_view_get_main_frame)(WebKitWebView *web_view);
static WebKitWebDataSource* (*webkit_web_frame_get_data_source)(WebKitWebFrame *frame);
static GString* (*webkit_web_data_source_get_data)(WebKitWebDataSource* data_source);
static void (*webkit_web_frame_print)(WebKitWebFrame *frame);
static void (*webkit_web_view_cut_clipboard)(WebKitWebView *web_view);
static void (*webkit_web_view_copy_clipboard)(WebKitWebView *web_view);
static void (*webkit_web_view_paste_clipboard)(WebKitWebView *web_view);
static void (*webkit_web_view_undo)(WebKitWebView *web_view);
static void (*webkit_web_view_redo)(WebKitWebView *web_view);
static gboolean (*webkit_web_view_can_paste_clipboard)(WebKitWebView *web_view);
static void (*webkit_web_view_select_all)(WebKitWebView *web_view);
static gboolean (*webkit_web_view_get_editable)(WebKitWebView *web_view);
static WebKitLoadStatus (*webkit_web_view_get_load_status)(WebKitWebView *web_view);
static void (*webkit_web_view_go_back_or_forward)(WebKitWebView *web_view, gint steps);
static void (*webkit_web_view_execute_script)(WebKitWebView *web_view, const gchar *script);
static const gchar* (*webkit_web_frame_get_uri)(WebKitWebFrame *frame);
static WebKitWebNavigationAction* (*webkit_web_policy_decision_get_navigation_action)(WebKitWebPolicyDecision *policy_decision);


static void iupgtkWebBrowser_ClearDLSymbols()
{
  /* Clear Common */
  webkit_web_view_new = NULL;
  webkit_web_view_get_uri = NULL;
  webkit_web_view_load_uri = NULL;
  webkit_web_view_go_back = NULL;
  webkit_web_view_go_forward = NULL;
  webkit_web_view_can_go_back = NULL;
  webkit_web_view_can_go_forward = NULL;
  webkit_web_view_reload = NULL;
  webkit_web_view_stop_loading = NULL;
  webkit_web_view_set_editable = NULL;
  webkit_web_view_get_zoom_level = NULL;
  webkit_web_view_set_zoom_level = NULL;
  webkit_network_request_get_uri = NULL;
  webkit_web_view_get_back_forward_list = NULL;

  /* Clear WK2 */
  webkit_web_view_new_with_context = NULL;
  webkit_back_forward_list_get_nth_item = NULL;
  webkit_back_forward_list_item_get_uri = NULL;
  webkit_back_forward_list_get_forward_list = NULL;
  webkit_back_forward_list_get_back_list = NULL;
  webkit_web_view_load_html = NULL;
  webkit_web_view_get_main_resource = NULL;
  webkit_web_resource_get_data = NULL;
  webkit_web_resource_get_data_finish = NULL;
  webkit_web_view_save_to_file = NULL;
  webkit_web_view_execute_editing_command = NULL;
  webkit_web_view_execute_editing_command_with_argument = NULL;
  webkit_web_view_is_editable = NULL;
  webkit_print_operation_new = NULL;
  webkit_print_operation_run_dialog = NULL;
  webkit_print_operation_print = NULL;
  webkit_web_view_is_loading = NULL;
  webkit_web_view_run_javascript = NULL;
  webkit_web_view_run_javascript_finish = NULL;
  webkit_javascript_result_get_global_context = NULL;
  webkit_javascript_result_get_value = NULL;
  webkit_javascript_result_get_js_value = NULL;
  webkit_javascript_result_unref = NULL;
  webkit_navigation_policy_decision_get_navigation_action = NULL;
  webkit_navigation_action_get_request = NULL;
  webkit_uri_request_get_uri = NULL;

  /* Clear JSCore (WebKit2) */
  JSValueToStringCopy = NULL;
  JSStringGetMaximumUTF8CStringSize = NULL;
  JSStringGetUTF8CString = NULL;
  JSStringRelease = NULL;
  JSValueIsNull = NULL;
  JSValueIsUndefined = NULL;

  /* Clear WK6 */
  webkit_web_view_evaluate_javascript = NULL;
  webkit_web_view_evaluate_javascript_finish = NULL;
  webkit_user_content_manager_register_script_message_handler_wk6 = NULL;
  webkit_user_content_manager_unregister_script_message_handler_wk6 = NULL;

  /* Clear JSCore GObject (WebKit6) */
  jsc_value_to_string = NULL;
  jsc_value_is_null = NULL;
  jsc_value_is_undefined = NULL;
  jsc_value_is_number = NULL;
  jsc_value_is_string = NULL;
  jsc_value_is_boolean = NULL;
  jsc_value_get_context = NULL;
  jsc_context_get_exception = NULL;
  jsc_exception_get_message = NULL;

  /* Clear WK1 */
  webkit_web_back_forward_list_get_nth_item = NULL;
  webkit_web_history_item_get_uri = NULL;
  webkit_web_back_forward_list_get_forward_length = NULL;
  webkit_web_back_forward_list_get_back_length = NULL;
  webkit_web_view_load_string = NULL;
  webkit_web_view_get_main_frame = NULL;
  webkit_web_frame_get_data_source = NULL;
  webkit_web_data_source_get_data = NULL;
  webkit_web_frame_print = NULL;
  webkit_web_view_cut_clipboard = NULL;
  webkit_web_view_copy_clipboard = NULL;
  webkit_web_view_paste_clipboard = NULL;
  webkit_web_view_undo = NULL;
  webkit_web_view_redo = NULL;
  webkit_web_view_can_paste_clipboard = NULL;
  webkit_web_view_select_all = NULL;
  webkit_web_view_get_editable = NULL;
  webkit_web_view_get_load_status = NULL;
  webkit_web_view_go_back_or_forward = NULL;
  webkit_web_view_execute_script = NULL;
  webkit_web_frame_get_uri = NULL;
  webkit_web_policy_decision_get_navigation_action = NULL;
}

static int iupgtkWebBrowser_SetDLSymbolsWK2(void* webkit_library)
{
  /* Load Common Symbols */
  webkit_web_view_new = (GtkWidget* (*)(void))dlsym(webkit_library, "webkit_web_view_new");
  webkit_web_view_get_uri = (const gchar* (*)(WebKitWebView*))dlsym(webkit_library, "webkit_web_view_get_uri");
  webkit_web_view_load_uri = (void (*)(WebKitWebView*, const gchar*))dlsym(webkit_library, "webkit_web_view_load_uri");
  webkit_web_view_go_back = (void (*)(WebKitWebView*))dlsym(webkit_library, "webkit_web_view_go_back");
  webkit_web_view_go_forward = (void (*)(WebKitWebView*))dlsym(webkit_library, "webkit_web_view_go_forward");
  webkit_web_view_can_go_back = (gboolean (*)(WebKitWebView*))dlsym(webkit_library, "webkit_web_view_can_go_back");
  webkit_web_view_can_go_forward = (gboolean (*)(WebKitWebView*))dlsym(webkit_library, "webkit_web_view_can_go_forward");
  webkit_web_view_reload = (void (*)(WebKitWebView*))dlsym(webkit_library, "webkit_web_view_reload");
  webkit_web_view_stop_loading = (void (*)(WebKitWebView*))dlsym(webkit_library, "webkit_web_view_stop_loading");
  webkit_web_view_set_editable = (void (*)(WebKitWebView*, gboolean))dlsym(webkit_library, "webkit_web_view_set_editable");
  webkit_web_view_get_zoom_level = (gdouble (*)(WebKitWebView*))dlsym(webkit_library, "webkit_web_view_get_zoom_level");
  webkit_web_view_set_zoom_level = (void (*)(WebKitWebView*, gdouble))dlsym(webkit_library, "webkit_web_view_set_zoom_level");
  webkit_network_request_get_uri = (const gchar* (*)(WebKitNetworkRequest*))dlsym(webkit_library, "webkit_network_request_get_uri");
  webkit_web_view_get_back_forward_list = (WebKitBackForwardList* (*)(WebKitWebView*))dlsym(webkit_library, "webkit_web_view_get_back_forward_list");

  /* Load WK2 Specific Symbols */
  webkit_web_view_new_with_context = (WebKitWebView* (*)(void*))dlsym(webkit_library, "webkit_web_view_new_with_context");
  webkit_back_forward_list_get_nth_item = (WebKitBackForwardListItem* (*)(WebKitBackForwardList*, gint))dlsym(webkit_library, "webkit_back_forward_list_get_nth_item");
  webkit_back_forward_list_item_get_uri = (const gchar* (*)(WebKitBackForwardListItem*))dlsym(webkit_library, "webkit_back_forward_list_item_get_uri");
  webkit_back_forward_list_get_forward_list = (GList* (*)(WebKitBackForwardList*))dlsym(webkit_library, "webkit_back_forward_list_get_forward_list");
  webkit_back_forward_list_get_back_list = (GList* (*)(WebKitBackForwardList*))dlsym(webkit_library, "webkit_back_forward_list_get_back_list");
  webkit_web_view_load_html = (void (*)(WebKitWebView*, const gchar*, const gchar*))dlsym(webkit_library, "webkit_web_view_load_html");
  webkit_web_view_get_main_resource = (WebKitWebResource* (*)(WebKitWebView*))dlsym(webkit_library, "webkit_web_view_get_main_resource");
  webkit_web_resource_get_data = (void (*)(WebKitWebResource*, GCancellable*, GAsyncReadyCallback, gpointer))dlsym(webkit_library, "webkit_web_resource_get_data");
  webkit_web_resource_get_data_finish = (guchar* (*)(WebKitWebResource*, GAsyncResult*, gsize*, GError**))dlsym(webkit_library, "webkit_web_resource_get_data_finish");
  webkit_web_view_save_to_file = (void (*)(WebKitWebView*, GFile*, WebKitSaveMode, GCancellable*, GAsyncReadyCallback, gpointer))dlsym(webkit_library, "webkit_web_view_save_to_file");
  webkit_web_view_execute_editing_command = (void (*)(WebKitWebView*, const gchar*))dlsym(webkit_library, "webkit_web_view_execute_editing_command");
  webkit_web_view_execute_editing_command_with_argument = (void (*)(WebKitWebView*, const gchar*, const gchar*))dlsym(webkit_library, "webkit_web_view_execute_editing_command_with_argument");
  webkit_web_view_is_editable = (gboolean (*)(WebKitWebView*))dlsym(webkit_library, "webkit_web_view_is_editable");
  webkit_print_operation_new = (WebKitPrintOperation* (*)(WebKitWebView*))dlsym(webkit_library, "webkit_print_operation_new");
  webkit_print_operation_run_dialog = (void (*)(WebKitPrintOperation*, GtkWindow*))dlsym(webkit_library, "webkit_print_operation_run_dialog");
  webkit_print_operation_print = (void (*)(WebKitPrintOperation*))dlsym(webkit_library, "webkit_print_operation_print");
  webkit_web_view_is_loading = (gboolean (*)(WebKitWebView*))dlsym(webkit_library, "webkit_web_view_is_loading");
  webkit_web_view_run_javascript = (void (*)(WebKitWebView*, const gchar*, GCancellable*, GAsyncReadyCallback, gpointer))dlsym(webkit_library, "webkit_web_view_run_javascript");
  webkit_web_view_run_javascript_finish = (WebKitJavascriptResult* (*)(WebKitWebView*, GAsyncResult*, GError**))dlsym(webkit_library, "webkit_web_view_run_javascript_finish");
  webkit_javascript_result_get_global_context = (JSGlobalContextRef (*)(WebKitJavascriptResult*))dlsym(webkit_library, "webkit_javascript_result_get_global_context");
  webkit_javascript_result_get_value = (JSValueRef (*)(WebKitJavascriptResult*))dlsym(webkit_library, "webkit_javascript_result_get_value");
  webkit_javascript_result_get_js_value = (JSCValue* (*)(WebKitJavascriptResult*))dlsym(webkit_library, "webkit_javascript_result_get_js_value");
  webkit_javascript_result_unref = (void (*)(WebKitJavascriptResult*))dlsym(webkit_library, "webkit_javascript_result_unref");
  webkit_navigation_policy_decision_get_navigation_action = (WebKitNavigationAction* (*)(WebKitNavigationPolicyDecision*))dlsym(webkit_library, "webkit_navigation_policy_decision_get_navigation_action");
  webkit_navigation_action_get_request = (WebKitURIRequest* (*)(WebKitNavigationAction*))dlsym(webkit_library, "webkit_navigation_action_get_request");
  webkit_uri_request_get_uri = (const gchar* (*)(WebKitURIRequest*))dlsym(webkit_library, "webkit_uri_request_get_uri");
  webkit_user_content_manager_new = (WebKitUserContentManager* (*)(void))dlsym(webkit_library, "webkit_user_content_manager_new");
  webkit_user_content_manager_add_script = (void (*)(WebKitUserContentManager*, WebKitUserScript*))dlsym(webkit_library, "webkit_user_content_manager_add_script");
  webkit_user_content_manager_register_script_message_handler = (gboolean (*)(WebKitUserContentManager*, const gchar*))dlsym(webkit_library, "webkit_user_content_manager_register_script_message_handler");
  webkit_user_content_manager_unregister_script_message_handler = (void (*)(WebKitUserContentManager*, const gchar*))dlsym(webkit_library, "webkit_user_content_manager_unregister_script_message_handler");
  webkit_user_script_new = (WebKitUserScript* (*)(const gchar*, WebKitUserContentInjectedFrames, WebKitUserScriptInjectionTime, const gchar* const*, const gchar* const*))dlsym(webkit_library, "webkit_user_script_new");
  webkit_user_script_unref = (void (*)(WebKitUserScript*))dlsym(webkit_library, "webkit_user_script_unref");
  webkit_web_view_new_with_user_content_manager = (WebKitWebView* (*)(WebKitUserContentManager*))dlsym(webkit_library, "webkit_web_view_new_with_user_content_manager");
  webkit_web_view_get_user_content_manager = (WebKitUserContentManager* (*)(WebKitWebView*))dlsym(webkit_library, "webkit_web_view_get_user_content_manager");

  /* Load JSCore GObject Symbols (WebKit2GTK 2.22+, preferred over old JSCore C API) */
  jsc_value_to_string = (char* (*)(JSCValue*))dlsym(webkit_library, "jsc_value_to_string");
  jsc_value_is_null = (gboolean (*)(JSCValue*))dlsym(webkit_library, "jsc_value_is_null");
  jsc_value_is_undefined = (gboolean (*)(JSCValue*))dlsym(webkit_library, "jsc_value_is_undefined");
  jsc_value_get_context = (JSCContext* (*)(JSCValue*))dlsym(webkit_library, "jsc_value_get_context");
  jsc_context_get_exception = (JSCException* (*)(JSCContext*))dlsym(webkit_library, "jsc_context_get_exception");
  jsc_exception_get_message = (char* (*)(JSCException*))dlsym(webkit_library, "jsc_exception_get_message");

  /* Load JSCore C API Symbols (fallback for older WebKit2GTK) */
  JSValueToStringCopy = (JSStringRef (*)(JSGlobalContextRef, JSValueRef, JSValueRef*))dlsym(webkit_library, "JSValueToStringCopy");
  JSStringGetMaximumUTF8CStringSize = (size_t (*)(JSStringRef))dlsym(webkit_library, "JSStringGetMaximumUTF8CStringSize");
  JSStringGetUTF8CString = (size_t (*)(JSStringRef, char*, size_t))dlsym(webkit_library, "JSStringGetUTF8CString");
  JSStringRelease = (void (*)(JSStringRef))dlsym(webkit_library, "JSStringRelease");
  JSValueIsNull = (int (*)(JSGlobalContextRef, JSValueRef))dlsym(webkit_library, "JSValueIsNull");
  JSValueIsUndefined = (int (*)(JSGlobalContextRef, JSValueRef))dlsym(webkit_library, "JSValueIsUndefined");

  /* Check for a few critical symbols */
  if (!webkit_web_view_new || !webkit_web_view_load_html ||
      !webkit_navigation_policy_decision_get_navigation_action || !webkit_navigation_action_get_request || !webkit_uri_request_get_uri)
    return 0;

  return 1;
}

static int iupgtkWebBrowser_SetDLSymbolsWK1(void* webkit_library)
{
  /* Load Common Symbols */
  webkit_web_view_new = (GtkWidget* (*)(void))dlsym(webkit_library, "webkit_web_view_new");
  webkit_web_view_get_uri = (const gchar* (*)(WebKitWebView*))dlsym(webkit_library, "webkit_web_view_get_uri");
  webkit_web_view_load_uri = (void (*)(WebKitWebView*, const gchar*))dlsym(webkit_library, "webkit_web_view_load_uri");
  webkit_web_view_go_back = (void (*)(WebKitWebView*))dlsym(webkit_library, "webkit_web_view_go_back");
  webkit_web_view_go_forward = (void (*)(WebKitWebView*))dlsym(webkit_library, "webkit_web_view_go_forward");
  webkit_web_view_can_go_back = (gboolean (*)(WebKitWebView*))dlsym(webkit_library, "webkit_web_view_can_go_back");
  webkit_web_view_can_go_forward = (gboolean (*)(WebKitWebView*))dlsym(webkit_library, "webkit_web_view_can_go_forward");
  webkit_web_view_reload = (void (*)(WebKitWebView*))dlsym(webkit_library, "webkit_web_view_reload");
  webkit_web_view_stop_loading = (void (*)(WebKitWebView*))dlsym(webkit_library, "webkit_web_view_stop_loading");
  webkit_web_view_set_editable = (void (*)(WebKitWebView*, gboolean))dlsym(webkit_library, "webkit_web_view_set_editable");
  webkit_web_view_get_zoom_level = (gdouble (*)(WebKitWebView*))dlsym(webkit_library, "webkit_web_view_get_zoom_level");
  webkit_web_view_set_zoom_level = (void (*)(WebKitWebView*, gdouble))dlsym(webkit_library, "webkit_web_view_set_zoom_level");
  webkit_network_request_get_uri = (const gchar* (*)(WebKitNetworkRequest*))dlsym(webkit_library, "webkit_network_request_get_uri");
  webkit_web_view_get_back_forward_list = (WebKitBackForwardList* (*)(WebKitWebView*))dlsym(webkit_library, "webkit_web_view_get_back_forward_list");

  /* Load WK1 Specific Symbols */
  webkit_web_back_forward_list_get_nth_item = (WebKitWebHistoryItem* (*)(WebKitWebBackForwardList*, gint))dlsym(webkit_library, "webkit_web_back_forward_list_get_nth_item");
  webkit_web_history_item_get_uri = (const gchar* (*)(WebKitWebHistoryItem*))dlsym(webkit_library, "webkit_web_history_item_get_uri");
  webkit_web_back_forward_list_get_forward_length = (gint (*)(WebKitWebBackForwardList*))dlsym(webkit_library, "webkit_web_back_forward_list_get_forward_length");
  webkit_web_back_forward_list_get_back_length = (gint (*)(WebKitWebBackForwardList*))dlsym(webkit_library, "webkit_web_back_forward_list_get_back_length");
  webkit_web_view_load_string = (void (*)(WebKitWebView*, const gchar*, const gchar*, const gchar*, const gchar*))dlsym(webkit_library, "webkit_web_view_load_string");
  webkit_web_view_get_main_frame = (WebKitWebFrame* (*)(WebKitWebView*))dlsym(webkit_library, "webkit_web_view_get_main_frame");
  webkit_web_frame_get_data_source = (WebKitWebDataSource* (*)(WebKitWebFrame*))dlsym(webkit_library, "webkit_web_frame_get_data_source");
  webkit_web_data_source_get_data = (GString* (*)(WebKitWebDataSource*))dlsym(webkit_library, "webkit_web_data_source_get_data");
  webkit_web_frame_print = (void (*)(WebKitWebFrame*))dlsym(webkit_library, "webkit_web_frame_print");
  webkit_web_view_cut_clipboard = (void (*)(WebKitWebView*))dlsym(webkit_library, "webkit_web_view_cut_clipboard");
  webkit_web_view_copy_clipboard = (void (*)(WebKitWebView*))dlsym(webkit_library, "webkit_web_view_copy_clipboard");
  webkit_web_view_paste_clipboard = (void (*)(WebKitWebView*))dlsym(webkit_library, "webkit_web_view_paste_clipboard");
  webkit_web_view_undo = (void (*)(WebKitWebView*))dlsym(webkit_library, "webkit_web_view_undo");
  webkit_web_view_redo = (void (*)(WebKitWebView*))dlsym(webkit_library, "webkit_web_view_redo");
  webkit_web_view_can_paste_clipboard = (gboolean (*)(WebKitWebView*))dlsym(webkit_library, "webkit_web_view_can_paste_clipboard");
  webkit_web_view_select_all = (void (*)(WebKitWebView*))dlsym(webkit_library, "webkit_web_view_select_all");
  webkit_web_view_get_editable = (gboolean (*)(WebKitWebView*))dlsym(webkit_library, "webkit_web_view_get_editable");
  webkit_web_view_get_load_status = (WebKitLoadStatus (*)(WebKitWebView*))dlsym(webkit_library, "webkit_web_view_get_load_status");
  webkit_web_view_go_back_or_forward = (void (*)(WebKitWebView*, gint))dlsym(webkit_library, "webkit_web_view_go_back_or_forward");
  webkit_web_view_execute_script = (void (*)(WebKitWebView*, const gchar*))dlsym(webkit_library, "webkit_web_view_execute_script");
  webkit_web_frame_get_uri = (const gchar* (*)(WebKitWebFrame*))dlsym(webkit_library, "webkit_web_frame_get_uri");
  webkit_web_policy_decision_get_navigation_action = (WebKitWebNavigationAction* (*)(WebKitWebPolicyDecision*))dlsym(webkit_library, "webkit_web_policy_decision_get_navigation_action");

  /* Check for a few critical symbols */
  if (!webkit_web_view_new || !webkit_web_view_load_string || !webkit_web_view_get_main_frame)
    return 0;

  return 1;
}

static int iupgtkWebBrowser_SetDLSymbolsWK6(void* webkit_library)
{
  /* Load Common Symbols */
  webkit_web_view_new = (GtkWidget* (*)(void))dlsym(webkit_library, "webkit_web_view_new");
  webkit_web_view_get_uri = (const gchar* (*)(WebKitWebView*))dlsym(webkit_library, "webkit_web_view_get_uri");
  webkit_web_view_load_uri = (void (*)(WebKitWebView*, const gchar*))dlsym(webkit_library, "webkit_web_view_load_uri");
  webkit_web_view_go_back = (void (*)(WebKitWebView*))dlsym(webkit_library, "webkit_web_view_go_back");
  webkit_web_view_go_forward = (void (*)(WebKitWebView*))dlsym(webkit_library, "webkit_web_view_go_forward");
  webkit_web_view_can_go_back = (gboolean (*)(WebKitWebView*))dlsym(webkit_library, "webkit_web_view_can_go_back");
  webkit_web_view_can_go_forward = (gboolean (*)(WebKitWebView*))dlsym(webkit_library, "webkit_web_view_can_go_forward");
  webkit_web_view_reload = (void (*)(WebKitWebView*))dlsym(webkit_library, "webkit_web_view_reload");
  webkit_web_view_stop_loading = (void (*)(WebKitWebView*))dlsym(webkit_library, "webkit_web_view_stop_loading");
  webkit_web_view_set_editable = (void (*)(WebKitWebView*, gboolean))dlsym(webkit_library, "webkit_web_view_set_editable");
  webkit_web_view_get_zoom_level = (gdouble (*)(WebKitWebView*))dlsym(webkit_library, "webkit_web_view_get_zoom_level");
  webkit_web_view_set_zoom_level = (void (*)(WebKitWebView*, gdouble))dlsym(webkit_library, "webkit_web_view_set_zoom_level");
  webkit_web_view_get_back_forward_list = (WebKitBackForwardList* (*)(WebKitWebView*))dlsym(webkit_library, "webkit_web_view_get_back_forward_list");

  /* Load WK2 Specific Symbols (many are shared with WK6) */
  webkit_back_forward_list_get_nth_item = (WebKitBackForwardListItem* (*)(WebKitBackForwardList*, gint))dlsym(webkit_library, "webkit_back_forward_list_get_nth_item");
  webkit_back_forward_list_item_get_uri = (const gchar* (*)(WebKitBackForwardListItem*))dlsym(webkit_library, "webkit_back_forward_list_item_get_uri");
  webkit_back_forward_list_get_forward_list = (GList* (*)(WebKitBackForwardList*))dlsym(webkit_library, "webkit_back_forward_list_get_forward_list");
  webkit_back_forward_list_get_back_list = (GList* (*)(WebKitBackForwardList*))dlsym(webkit_library, "webkit_back_forward_list_get_back_list");
  webkit_web_view_load_html = (void (*)(WebKitWebView*, const gchar*, const gchar*))dlsym(webkit_library, "webkit_web_view_load_html");
  webkit_web_view_get_main_resource = (WebKitWebResource* (*)(WebKitWebView*))dlsym(webkit_library, "webkit_web_view_get_main_resource");
  webkit_web_resource_get_data = (void (*)(WebKitWebResource*, GCancellable*, GAsyncReadyCallback, gpointer))dlsym(webkit_library, "webkit_web_resource_get_data");
  webkit_web_resource_get_data_finish = (guchar* (*)(WebKitWebResource*, GAsyncResult*, gsize*, GError**))dlsym(webkit_library, "webkit_web_resource_get_data_finish");
  webkit_web_view_save_to_file = (void (*)(WebKitWebView*, GFile*, WebKitSaveMode, GCancellable*, GAsyncReadyCallback, gpointer))dlsym(webkit_library, "webkit_web_view_save_to_file");
  webkit_web_view_execute_editing_command = (void (*)(WebKitWebView*, const gchar*))dlsym(webkit_library, "webkit_web_view_execute_editing_command");
  webkit_web_view_execute_editing_command_with_argument = (void (*)(WebKitWebView*, const gchar*, const gchar*))dlsym(webkit_library, "webkit_web_view_execute_editing_command_with_argument");
  webkit_web_view_is_editable = (gboolean (*)(WebKitWebView*))dlsym(webkit_library, "webkit_web_view_is_editable");
  webkit_print_operation_new = (WebKitPrintOperation* (*)(WebKitWebView*))dlsym(webkit_library, "webkit_print_operation_new");
  webkit_print_operation_run_dialog = (void (*)(WebKitPrintOperation*, GtkWindow*))dlsym(webkit_library, "webkit_print_operation_run_dialog");
  webkit_print_operation_print = (void (*)(WebKitPrintOperation*))dlsym(webkit_library, "webkit_print_operation_print");
  webkit_web_view_is_loading = (gboolean (*)(WebKitWebView*))dlsym(webkit_library, "webkit_web_view_is_loading");
  webkit_navigation_policy_decision_get_navigation_action = (WebKitNavigationAction* (*)(WebKitNavigationPolicyDecision*))dlsym(webkit_library, "webkit_navigation_policy_decision_get_navigation_action");
  webkit_navigation_action_get_request = (WebKitURIRequest* (*)(WebKitNavigationAction*))dlsym(webkit_library, "webkit_navigation_action_get_request");
  webkit_uri_request_get_uri = (const gchar* (*)(WebKitURIRequest*))dlsym(webkit_library, "webkit_uri_request_get_uri");
  webkit_user_content_manager_new = (WebKitUserContentManager* (*)(void))dlsym(webkit_library, "webkit_user_content_manager_new");
  webkit_user_content_manager_add_script = (void (*)(WebKitUserContentManager*, WebKitUserScript*))dlsym(webkit_library, "webkit_user_content_manager_add_script");
  webkit_user_script_new = (WebKitUserScript* (*)(const gchar*, WebKitUserContentInjectedFrames, WebKitUserScriptInjectionTime, const gchar* const*, const gchar* const*))dlsym(webkit_library, "webkit_user_script_new");
  webkit_user_script_unref = (void (*)(WebKitUserScript*))dlsym(webkit_library, "webkit_user_script_unref");
  webkit_web_view_get_user_content_manager = (WebKitUserContentManager* (*)(WebKitWebView*))dlsym(webkit_library, "webkit_web_view_get_user_content_manager");

  /* Load WK6 Specific Symbols */
  webkit_web_view_evaluate_javascript = (void (*)(WebKitWebView*, const char*, gssize, const char*, const char*, GCancellable*, GAsyncReadyCallback, gpointer))dlsym(webkit_library, "webkit_web_view_evaluate_javascript");
  webkit_web_view_evaluate_javascript_finish = (JSCValue* (*)(WebKitWebView*, GAsyncResult*, GError**))dlsym(webkit_library, "webkit_web_view_evaluate_javascript_finish");
  /* WebKit6 uses 3-parameter version (with world_name) */
  webkit_user_content_manager_register_script_message_handler_wk6 = (gboolean (*)(WebKitUserContentManager*, const char*, const char*))dlsym(webkit_library, "webkit_user_content_manager_register_script_message_handler");
  webkit_user_content_manager_unregister_script_message_handler_wk6 = (void (*)(WebKitUserContentManager*, const char*, const char*))dlsym(webkit_library, "webkit_user_content_manager_unregister_script_message_handler");

  /* Load JSCore GObject Symbols */
  jsc_value_to_string = (char* (*)(JSCValue*))dlsym(webkit_library, "jsc_value_to_string");
  jsc_value_is_null = (gboolean (*)(JSCValue*))dlsym(webkit_library, "jsc_value_is_null");
  jsc_value_is_undefined = (gboolean (*)(JSCValue*))dlsym(webkit_library, "jsc_value_is_undefined");
  jsc_value_is_number = (gboolean (*)(JSCValue*))dlsym(webkit_library, "jsc_value_is_number");
  jsc_value_is_string = (gboolean (*)(JSCValue*))dlsym(webkit_library, "jsc_value_is_string");
  jsc_value_is_boolean = (gboolean (*)(JSCValue*))dlsym(webkit_library, "jsc_value_is_boolean");
  jsc_value_get_context = (JSCContext* (*)(JSCValue*))dlsym(webkit_library, "jsc_value_get_context");
  jsc_context_get_exception = (JSCException* (*)(JSCContext*))dlsym(webkit_library, "jsc_context_get_exception");
  jsc_exception_get_message = (char* (*)(JSCException*))dlsym(webkit_library, "jsc_exception_get_message");

  /* Check for a few critical WK6 symbols */
  if (!webkit_web_view_new || !webkit_web_view_load_html ||
      !webkit_web_view_evaluate_javascript || !jsc_value_to_string ||
      !webkit_navigation_policy_decision_get_navigation_action || !webkit_navigation_action_get_request || !webkit_uri_request_get_uri)
    return 0;

  return 1;
}

#endif /* IUPWEB_USE_DLOPEN */


#ifdef __cplusplus
}
#endif

#endif
