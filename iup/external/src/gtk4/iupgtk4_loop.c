/** \file
 * \brief GTK4 Message Loop
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <gtk/gtk.h>

#include "iup.h"
#include "iupcbs.h"
#include "iup_loop.h"
#include "iup_str.h"


static IFidle gtk_idle_cb = NULL;
static guint gtk_idle_id;

/* Track main loop manually since gtk_main_level() was removed */
/* Use a stack to handle nested loops (e.g., modal dialogs) */
#define MAX_LOOP_DEPTH 10
static GMainLoop *gtk4_loop_stack[MAX_LOOP_DEPTH] = {NULL};
static int gtk4_loop_level = 0;

static gboolean gtkIdleFunc(gpointer data)
{
  (void)data;
  if (gtk_idle_cb)
  {
    int ret = gtk_idle_cb();
    if (ret == IUP_CLOSE)
    {
      gtk_idle_cb = NULL;
      IupExitLoop();
      return FALSE;
    }
    if (ret == IUP_IGNORE)
    {
      gtk_idle_cb = NULL;
      return FALSE;
    }

    return TRUE;
  }

  return FALSE;
}

IUP_SDK_API void iupdrvSetIdleFunction(Icallback f)
{
  if (gtk_idle_cb)
    g_source_remove(gtk_idle_id);

  gtk_idle_cb = (IFidle)f;

  if (gtk_idle_cb)
    gtk_idle_id = g_idle_add(gtkIdleFunc, NULL);
}

IUP_API void IupExitLoop(void)
{
  char* exit_loop = IupGetGlobal("EXITLOOP");
  GMainLoop* current_loop = (gtk4_loop_level > 0) ? gtk4_loop_stack[gtk4_loop_level - 1] : NULL;

  /* Use manual loop level tracking with stack */
  if (gtk4_loop_level > 0 && (gtk4_loop_level > 1 || !exit_loop || iupStrBoolean(exit_loop)))
  {
    if (current_loop && g_main_loop_is_running(current_loop))
      g_main_loop_quit(current_loop);
  }
}

IUP_API int IupMainLoopLevel(void)
{
  /* Return manual loop level */
  return gtk4_loop_level;
}

IUP_API int IupMainLoop(void)
{
  static int has_done_entry = 0;
  if (has_done_entry == 0)
  {
    has_done_entry = 1;
    iupLoopCallEntryCb();
  }

  /* Use GMainLoop instead of deprecated gtk_main() */
  /* Push new loop onto stack to handle nested loops (modal dialogs) */
  if (gtk4_loop_level >= MAX_LOOP_DEPTH)
  {
    fprintf(stderr, "ERROR: Maximum loop nesting depth (%d) exceeded!\n", MAX_LOOP_DEPTH);
    return IUP_ERROR;
  }

  GMainLoop* new_loop = g_main_loop_new(NULL, FALSE);
  gtk4_loop_stack[gtk4_loop_level] = new_loop;
  gtk4_loop_level++;

  g_main_loop_run(new_loop);

  /* Pop loop from stack */
  gtk4_loop_level--;
  gtk4_loop_stack[gtk4_loop_level] = NULL;
  g_main_loop_unref(new_loop);

  if (gtk4_loop_level == 0)
    iupLoopCallExitCb();

  return IUP_NOERROR;
}

IUP_API int IupLoopStepWait(void)
{
  GMainContext *context = g_main_context_default();
  if (g_main_context_iteration(context, TRUE))
    return IUP_CLOSE;
  return IUP_DEFAULT;
}

IUP_API int IupLoopStep(void)
{
  GMainContext *context = g_main_context_default();
  if (g_main_context_iteration(context, FALSE))
    return IUP_CLOSE;
  return IUP_DEFAULT;
}

IUP_API void IupFlush(void)
{
  int count = 0;

  IFidle old_gtk_idle_cb = NULL;
  if (gtk_idle_cb)
  {
    old_gtk_idle_cb = gtk_idle_cb;
    iupdrvSetIdleFunction(NULL);
  }

  GMainContext *context = g_main_context_default();
  while (count<100 && g_main_context_pending(context))
  {
    g_main_context_iteration(context, FALSE);
    count++;
  }

  if (old_gtk_idle_cb)
    iupdrvSetIdleFunction((Icallback)old_gtk_idle_cb);
}


typedef struct {
  Ihandle* ih;
  char* s;
  int i;
  double d;
  char* p;
} gtkPostMessageUserData;

static gint gtkPostMessageCallback(void *cb_data)
{
  gtkPostMessageUserData* user_data = (gtkPostMessageUserData*)cb_data;
  Ihandle* ih = user_data->ih;
  IFnsidv cb = (IFnsidv)IupGetCallback(ih, "POSTMESSAGE_CB");
  if (cb)
    cb(ih, user_data->s, user_data->i, user_data->d, user_data->p);
  if (user_data->s) free(user_data->s);
  free(user_data);
  return FALSE;
}

IUP_API void IupPostMessage(Ihandle* ih, const char* s, int i, double d, void* p)
{
  gtkPostMessageUserData* user_data = (gtkPostMessageUserData*)malloc(sizeof(gtkPostMessageUserData));
  user_data->ih = ih;
  user_data->s = iupStrDup(s);
  user_data->i = i;
  user_data->d = d;
  user_data->p = p;
  g_idle_add(gtkPostMessageCallback, user_data);
}

void iupgtk4LoopCleanup(void)
{
  if (gtk_idle_cb)
  {
    g_source_remove(gtk_idle_id);
    gtk_idle_cb = NULL;
  }

  /* Exit all running loops in the stack. */
  while (gtk4_loop_level > 0)
  {
    GMainLoop* current_loop = gtk4_loop_stack[gtk4_loop_level - 1];
    if (current_loop && g_main_loop_is_running(current_loop))
      g_main_loop_quit(current_loop);
    gtk4_loop_level--;
  }

  /* Process pending events to clean up any stale event sources */
  GMainContext *context = g_main_context_default();
  int count = 0;
  while (count < 100 && g_main_context_pending(context))
  {
    g_main_context_iteration(context, FALSE);
    count++;
  }
}
