/** \file
 * \brief Text Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <Xm/Xm.h>
#include <Xm/TextF.h>
#include <Xm/Text.h>
#include <Xm/ScrolledW.h>
#include <Xm/SpinB.h>
#include <X11/keysym.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdarg.h>
#include <limits.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_childtree.h"
#include "iup_attrib.h"
#include "iup_dialog.h"
#include "iup_str.h"
#include "iup_mask.h"
#include "iup_drv.h"
#include "iup_array.h"
#include "iup_text.h"

#include "iupmot_drv.h"
#include "iupmot_color.h"


#ifndef XmIsSpinBox
#define XmIsSpinBox(w)	XtIsSubclass(w, xmSpinBoxWidgetClass)
#endif

#ifndef XmNwrap
#define XmNwrap "Nwrap"
#endif

void* iupdrvTextAddFormatTagStartBulk(Ihandle* ih)
{
  (void)ih;
  return NULL;
}

void iupdrvTextAddFormatTagStopBulk(Ihandle* ih, void* state)
{
  (void)ih;
  (void)state;
}

void iupdrvTextAddFormatTag(Ihandle* ih, Ihandle* formattag, int bulk)
 {
   (void)ih;
   (void)formattag;
   (void)bulk;
 }

void iupdrvTextAddSpin(Ihandle* ih, int *w, int h)
{
  *w += h/2;
  (void)ih;
}

void iupdrvTextAddBorders(Ihandle* ih, int *w, int *h)
{
  int border_size = 2*5;
  (*w) += border_size;
  (*h) += border_size;
  (void)ih;
}

static void motTextGetLinColFromPosition(const char *str, int pos, int *lin, int *col )
{
  int i;

  *lin = 0;
  *col = 0;

  for (i=0; i<pos; i++)
  {
    if (*str == '\n')
    {
      (*lin)++;
      *col = 0;
    }
    else
      (*col)++;

    str++;
  }

  (*lin)++; /* IUP starts at 1 */
  (*col)++;
}

static int motTextSetLinColToPosition(const char *str, int lin, int col)
{
  int pos=0, cur_lin, cur_col;

  lin--; /* IUP starts at 1 */
  col--;

  /* find the line */
  cur_lin=0;
  while (*str && cur_lin<lin)
  {
    if (*str == '\n')
      cur_lin++;

    str++;
    pos++;
  }
  
  /* find the column */
  cur_col=0;
  while (*str && cur_col<col)
  {
    if (*str == '\n')
      break;

    cur_col++;

    str++;
    pos++;
  }

  return pos;
}

void iupdrvTextConvertLinColToPos(Ihandle* ih, int lin, int col, int *pos)
{
  char* str = XmTextGetString(ih->handle);
  *pos = motTextSetLinColToPosition(str, lin, col);
  XtFree(str);
}

void iupdrvTextConvertPosToLinCol(Ihandle* ih, int pos, int *lin, int *col)
{
  char *str = XmTextGetString(ih->handle);
  motTextGetLinColFromPosition(str, pos, lin, col);
  XtFree(str);
}

static int motTextConvertXYToPos(Ihandle* ih, int x, int y)
{
  return XmTextXYToPos(ih->handle, x, y);
}


/*******************************************************************************************/


static int motTextSetPaddingAttrib(Ihandle* ih, const char* value)
{
  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');
  if (ih->handle)
  {
    XtVaSetValues(ih->handle, XmNmarginHeight, ih->data->vert_padding,
                              XmNmarginWidth, ih->data->horiz_padding, NULL);
    return 0;
  }
  else
    return 1; /* store until not mapped, when mapped will be set again */
}

static int motTextSetReadOnlyAttrib(Ihandle* ih, const char* value)
{
  XtVaSetValues(ih->handle, XmNeditable, iupStrBoolean(value)? False: True, NULL);
  return 0;
}

static char* motTextGetReadOnlyAttrib(Ihandle* ih)
{
  Boolean editable;
  XtVaGetValues(ih->handle, XmNeditable, &editable, NULL);
  return iupStrReturnBoolean (!editable); 
}

static int motTextSetInsertAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle)  /* do not do the action before map */
    return 0;
  if (!value)
    return 0;

  /* disable callbacks */
  ih->data->disable_callbacks = 1;
  XmTextRemove(ih->handle);
  XmTextInsert(ih->handle, XmTextGetInsertionPosition(ih->handle), (char*)value);
  ih->data->disable_callbacks = 0;

  return 0;
}

static int motTextSetSelectedTextAttrib(Ihandle* ih, const char* value)
{
  XmTextPosition start, end;

  if (!value)
    return 0;

  if (XmTextGetSelectionPosition(ih->handle, &start, &end) && start!=end)
  {
    /* disable callbacks */
    ih->data->disable_callbacks = 1;
    XmTextReplace(ih->handle, start, end, (char*)value);
    ih->data->disable_callbacks = 0;
  }

  return 0;
}

static char* motTextGetCountAttrib(Ihandle* ih)
{
  int count = XmTextGetLastPosition(ih->handle);
  return iupStrReturnInt(count);
}

static char* motTextGetLineCountAttrib(Ihandle* ih)
{
  if (ih->data->is_multiline)
  {
    int linecount;
    XtVaGetValues(ih->handle, XmNtotalLines, &linecount, NULL);
    return iupStrReturnInt(linecount);
  }
  else
    return "1";
}

static char* motTextGetSelectedTextAttrib(Ihandle* ih)
{
  char* selectedtext = XmTextGetSelection(ih->handle);
  char* str = iupStrReturnStr(selectedtext);
  XtFree(selectedtext);
  return str;
}

static int motTextSetAppendAttrib(Ihandle* ih, const char* value)
{
  XmTextPosition pos;
  if (!ih->handle)  /* do not do the action before map */
    return 0;
  pos = XmTextGetLastPosition(ih->handle);
  /* disable callbacks */
  ih->data->disable_callbacks = 1;
  if (ih->data->is_multiline && ih->data->append_newline && pos!=0)
  {
    XmTextInsert(ih->handle, pos, "\n");
    pos++;
  }
	if (value)
    XmTextInsert(ih->handle, pos, (char*)value);
  ih->data->disable_callbacks = 0;
  return 0;
}

static int motTextSetSelectionAttrib(Ihandle* ih, const char* value)
{
  int start=1, end=1;

  if (!value || iupStrEqualNoCase(value, "NONE"))
  {
    XmTextClearSelection(ih->handle, CurrentTime);
    return 0;
  }

  if (iupStrEqualNoCase(value, "ALL"))
  {
    XmTextSetSelection(ih->handle, (XmTextPosition)0, (XmTextPosition)XmTextGetLastPosition(ih->handle), CurrentTime);
    return 0;
  }

  if (ih->data->is_multiline)
  {
    int lin_start=1, col_start=1, lin_end=1, col_end=1;
    char *str;

    if (sscanf(value, "%d,%d:%d,%d", &lin_start, &col_start, &lin_end, &col_end)!=4) return 0;
    if (lin_start<1 || col_start<1 || lin_end<1 || col_end<1) return 0;

    str = XmTextGetString(ih->handle);
    start = motTextSetLinColToPosition(str, lin_start, col_start);
    end = motTextSetLinColToPosition(str, lin_end, col_end);
    XtFree(str);
  }
  else
  {
    if (iupStrToIntInt(value, &start, &end, ':')!=2) 
      return 0;

    if(start<1 || end<1) 
      return 0;

    start--; /* IUP starts at 1 */
    end--;
  }

  XmTextSetSelection(ih->handle, (XmTextPosition)start, (XmTextPosition)end, CurrentTime);

  return 0;
}

static char* motTextGetSelectionAttrib(Ihandle* ih)
{
  XmTextPosition start = 0, end = 0;

  if (!XmTextGetSelectionPosition(ih->handle, &start, &end) || start==end)
    return NULL;

  if (ih->data->is_multiline)
  {
    int start_col, start_lin, end_col, end_lin;

    char *value = XmTextGetString(ih->handle);
    motTextGetLinColFromPosition(value, start, &start_lin, &start_col);
    motTextGetLinColFromPosition(value, end,   &end_lin,   &end_col);
    XtFree(value);

    return iupStrReturnStrf("%d,%d:%d,%d", start_lin, start_col, end_lin, end_col);
  }
  else
  {
    start++; /* IUP starts at 1 */
    end++;
    return iupStrReturnIntInt((int)start, (int)end, ':');
  }
}

static int motTextSetSelectionPosAttrib(Ihandle* ih, const char* value)
{
  int start=0, end=0;

  if (!value || iupStrEqualNoCase(value, "NONE"))
  {
    XmTextClearSelection(ih->handle, CurrentTime);
    return 0;
  }

  if (iupStrEqualNoCase(value, "ALL"))
  {
    XmTextSetSelection(ih->handle, (XmTextPosition)0, (XmTextPosition)XmTextGetLastPosition(ih->handle), CurrentTime);
    return 0;
  }

  if (iupStrToIntInt(value, &start, &end, ':')!=2) 
    return 0;

  if(start<0 || end<0) 
    return 0;

  XmTextSetSelection(ih->handle, (XmTextPosition)start, (XmTextPosition)end, CurrentTime);

  return 0;
}

static char* motTextGetSelectionPosAttrib(Ihandle* ih)
{
  XmTextPosition start = 0, end = 0;

  if (!XmTextGetSelectionPosition(ih->handle, &start, &end) || start==end)
    return NULL;

  return iupStrReturnIntInt((int)start, (int)end, ':');
}

static int motTextSetCaretAttrib(Ihandle* ih, const char* value)
{
  int pos = 1;

  if (!value)
    return 0;

  if (ih->data->is_multiline)
  {
    int lin = 1, col = 1;
    char *str;

    iupStrToIntInt(value, &lin, &col, ',');

    str = XmTextGetString(ih->handle);
    pos = motTextSetLinColToPosition(str, lin, col);
    XtFree(str);
  }
  else
  {
    iupStrToInt(value, &pos);
    pos--; /* IUP starts at 1 */
  }

  XmTextSetInsertionPosition(ih->handle, (XmTextPosition)pos);
  XmTextShowPosition(ih->handle, (XmTextPosition)pos);

  return 0;
}

static char* motTextGetCaretAttrib(Ihandle* ih)
{
  XmTextPosition pos = XmTextGetInsertionPosition(ih->handle);

  if (ih->data->is_multiline)
  {
    int col, lin;

    char *value = XmTextGetString(ih->handle);
    motTextGetLinColFromPosition(value, pos, &lin, &col);
    XtFree(value);

    return iupStrReturnIntInt(lin, col, ',');
  }
  else
  {
    pos++; /* IUP starts at 1 */
    return iupStrReturnInt((int)pos);
  }
}

static int motTextSetCaretPosAttrib(Ihandle* ih, const char* value)
{
  int pos = 0;

  if (!value)
    return 0;

  iupStrToInt(value, &pos);
  if (pos < 0) pos = 0;

  XmTextSetInsertionPosition(ih->handle, (XmTextPosition)pos);
  XmTextShowPosition(ih->handle, (XmTextPosition)pos);

  return 0;
}

static char* motTextGetCaretPosAttrib(Ihandle* ih)
{
  XmTextPosition pos = XmTextGetInsertionPosition(ih->handle);
  return iupStrReturnInt((int)pos);
}

static int motTextSetScrollToAttrib(Ihandle* ih, const char* value)
{
  int pos = 1;

  if (!value)
    return 0;

  if (ih->data->is_multiline)
  {
    int lin = 1, col = 1;
    char* str;

    iupStrToIntInt(value, &lin, &col, ',');
    if (lin < 1) lin = 1;
    if (col < 1) col = 1;

    str = XmTextGetString(ih->handle);
    pos = motTextSetLinColToPosition(str, lin, col);
    XtFree(str);
  }
  else
  {
    iupStrToInt(value, &pos);
    if (pos < 1) pos = 1;
    pos--;  /* return to Motif referece */
  }

  XmTextShowPosition(ih->handle, (XmTextPosition)pos);

  return 0;
}

static int motTextSetScrollToPosAttrib(Ihandle* ih, const char* value)
{
  int pos = 0;

  if (!value)
    return 0;

  iupStrToInt(value, &pos);
  if (pos < 0) pos = 0;

  XmTextShowPosition(ih->handle, (XmTextPosition)pos);

  return 0;
}

static int motTextSetNCAttrib(Ihandle* ih, const char* value)
{
  if (!iupStrToInt(value, &ih->data->nc))
    ih->data->nc = INT_MAX;
  if (ih->handle)
  {
    XtVaSetValues(ih->handle, XmNmaxLength, ih->data->nc, NULL);
    return 0;
  }
  else
    return 1; /* store until not mapped, when mapped will be set again */
}

static int motTextSetClipboardAttrib(Ihandle *ih, const char *value)
{
  Boolean editable;
  XtVaGetValues(ih->handle, XmNeditable, &editable, NULL);
  
  /* NOTE: the functions XmTextCopy, XmTextPaste and XmTextCut did not work as expected.
    But using IupClipboard does not catch selections made in a terminal. */

  if (iupStrEqualNoCase(value, "COPY"))
  {
    Ihandle* clipboard;
    char *str = XmTextGetSelection(ih->handle);
    if (!str) 
      return 0;

    clipboard = IupClipboard();
    IupSetAttribute(clipboard, "TEXT", str);
    IupDestroy(clipboard);

    XtFree(str);
  }
  else if (iupStrEqualNoCase(value, "CUT"))
  {
    Ihandle* clipboard;
    char *str;

    if (!editable)
      return 0;

    str = XmTextGetSelection(ih->handle);
    if (!str) 
      return 0;

    clipboard = IupClipboard();
    IupSetAttribute(clipboard, "TEXT", str);
    IupDestroy(clipboard);

    XtFree(str);

    /* disable callbacks if not interactive */
    if (ih->data->disable_callbacks == -1)
      ih->data->disable_callbacks = 0;
    else
      ih->data->disable_callbacks = 1;
    XmTextRemove(ih->handle);
    ih->data->disable_callbacks = 0;
  }
  else if (iupStrEqualNoCase(value, "PASTE"))
  {
    Ihandle* clipboard;
    char *str;

    if (!editable)
      return 0;

    clipboard = IupClipboard();
    str = IupGetAttribute(clipboard, "TEXT");

    /* disable callbacks */
    /* disable callbacks if not interactive */
    if (ih->data->disable_callbacks == -1)
      ih->data->disable_callbacks = 0;
    else
      ih->data->disable_callbacks = 1;
    XmTextRemove(ih->handle);
    XmTextInsert(ih->handle, XmTextGetInsertionPosition(ih->handle), str);
    ih->data->disable_callbacks = 0;
  }
  else if (iupStrEqualNoCase(value, "CLEAR"))
  {
    if (!editable)
      return 0;

    /* disable callbacks if not interactive */
    if (ih->data->disable_callbacks == -1)
      ih->data->disable_callbacks = 0;
    else
      ih->data->disable_callbacks = 1;
    XmTextRemove(ih->handle);
    ih->data->disable_callbacks = 0;
  }
  return 0;
}

static int motTextSetAutoRedrawAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    XmTextEnableRedisplay(ih->handle);
  else
    XmTextDisableRedisplay(ih->handle);
  return 0;
}

static int motTextSetBgColorAttrib(Ihandle* ih, const char* value)
{
  Widget sb_win = (Widget)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  if (sb_win)
  {
    Pixel color;

    /* ignore given value, must use only from parent for the scrollbars */
    char* parent_value = iupBaseNativeParentGetBgColor(ih);

    color = iupmotColorGetPixelStr(parent_value);
    if (color != (Pixel)-1 && iupStrBoolean(IupGetGlobal("SB_BGCOLOR")))
    {
      Widget sb = NULL;

      iupmotSetBgColor(sb_win, color);

      XtVaGetValues(sb_win, XmNverticalScrollBar, &sb, NULL);
      if (sb) iupmotSetBgColor(sb, color);

      XtVaGetValues(sb_win, XmNhorizontalScrollBar, &sb, NULL);
      if (sb) iupmotSetBgColor(sb, color);
    }
  }

  return iupdrvBaseSetBgColorAttrib(ih, value);   /* use given value for contents */
}

static int motTextSetSpinMinAttrib(Ihandle* ih, const char* value)
{
  Widget spinbox = (Widget)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  if (spinbox && XmIsSpinBox(spinbox))
  {
    int min;
    if (iupStrToInt(value, &min))
    {
      ih->data->disable_callbacks = 1;
      XtVaSetValues(ih->handle, XmNminimumValue, min, NULL);
      ih->data->disable_callbacks = 0;
    }
  }
  return 1;
}

static int motTextSetSpinMaxAttrib(Ihandle* ih, const char* value)
{
  Widget spinbox = (Widget)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  if (spinbox && XmIsSpinBox(spinbox))
  {
    int max;
    if (iupStrToInt(value, &max))
    {
      ih->data->disable_callbacks = 1;
      XtVaSetValues(ih->handle, XmNmaximumValue, max, NULL);
      ih->data->disable_callbacks = 0;
    }
  }
  return 1;
}

static int motTextSetSpinIncAttrib(Ihandle* ih, const char* value)
{
  Widget spinbox = (Widget)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  if (spinbox && XmIsSpinBox(spinbox))
  {
    int inc;
    if (iupStrToInt(value, &inc))
    {
      ih->data->disable_callbacks = 1;
      XtVaSetValues(ih->handle, XmNincrementValue, inc, NULL);
      ih->data->disable_callbacks = 0;
    }
  }
  return 1;
}

static int motTextSetSpinValueAttrib(Ihandle* ih, const char* value)
{
  Widget spinbox = (Widget)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  if (spinbox && XmIsSpinBox(spinbox))
  {
    int pos;
    if (iupStrToInt(value, &pos))
    {
      char* text = NULL;
      int min, max;
      ih->data->disable_callbacks = 1;
      XtVaGetValues(ih->handle, XmNminimumValue, &min, 
                                XmNmaximumValue, &max, NULL);
      if (pos < min) pos = min;
      if (pos > max) pos = max;
      if (iupAttribGet(ih, "_IUPMOT_SPIN_NOAUTO"))
        text = XmTextGetString(ih->handle);

      XtVaSetValues(ih->handle, XmNposition, pos, NULL);

      if (text)
      {
        iupmotTextSetString(ih->handle, text);
        XtFree(text);
      }
      ih->data->disable_callbacks = 0;
      return 1;
    }
  }
  return 0;
}

static char* motTextGetSpinValueAttrib(Ihandle* ih)
{
  Widget spinbox = (Widget)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  if (spinbox && XmIsSpinBox(spinbox))
  {
    int pos;
    XtVaGetValues(ih->handle, XmNposition, &pos, NULL);
    return iupStrReturnInt(pos);
  }
  return NULL;
}

static int motTextSetValueAttrib(Ihandle* ih, const char* value)
{
  if (!value) value = "";
  motTextSetSpinValueAttrib(ih, value);
  /* disable callbacks */
  ih->data->disable_callbacks = 1;
  iupmotTextSetString(ih->handle, value);
  ih->data->disable_callbacks = 0;
  return 0;
}

static char* motTextGetValueAttrib(Ihandle* ih)
{
  char* value = XmTextGetString(ih->handle);
  char* str = iupStrReturnStr(value);
  XtFree(value);
  return str;
}
                       
static char* motTextGetLineValueAttrib(Ihandle* ih)
{
  if (ih->data->is_multiline)
  {
    int lin, col, start, end;
    char* str = iupStrGetMemory(200);
    char *value = XmTextGetString(ih->handle);
    XmTextPosition pos = XmTextGetInsertionPosition(ih->handle);
    motTextGetLinColFromPosition(value, pos, &lin, &col);
    start = motTextSetLinColToPosition(value, lin, 1);
    end = motTextSetLinColToPosition(value, lin, 20000);
    XtFree(value);
    XmTextGetSubstring(ih->handle, start, end-start, 200, str);  /* do not include the EOL */
    return str;
  }
  else
    return motTextGetValueAttrib(ih);
}


/******************************************************************************/


static void motTextSpinModifyVerifyCallback(Widget w, Ihandle* ih, XmSpinBoxCallbackStruct *cbs)
{
  IFni cb = (IFni) IupGetCallback(ih, "SPIN_CB");
  if (cb)
  {
    int ret = cb(ih, cbs->position);
    if (ret == IUP_IGNORE)
    {
      cbs->doit = 1;
      return;
    }
  }
  (void)w;

  iupAttribSet(ih, "_IUPMOT_SPIN_DISABLE_TEXT_CB", "1");
}

static void motTextModifyVerifyCallback(Widget w, Ihandle *ih, XmTextVerifyPtr text)
{
  int start, end, remove_dir = 0, ret;
  char *insert_value;
  KeySym motcode = 0;
  IFnis cb;

  if (ih->data->disable_callbacks)
    return;

  if (iupAttribGet(ih, "_IUPMOT_SPIN_DISABLE_TEXT_CB"))
  {
    if (iupAttribGet(ih, "_IUPMOT_SPIN_NOAUTO"))
      text->doit = False;

    iupAttribSet(ih, "_IUPMOT_SPIN_DISABLE_TEXT_CB", NULL);
    return;
  }

  if (text->event && text->event->type == KeyPress)
  {
    unsigned int state = ((XKeyEvent*)text->event)->state;
    int has_ctrl = state & ControlMask;  /* Ctrl */
    int has_alt = state & Mod1Mask || state & Mod5Mask;  /* Alt */
    int has_sys = state & Mod4Mask; /* Apple/Win */

    /* only process when no modifiers are used */        
    /* except when Ctrl and Alt are pressed at the same time */
    if (has_sys ||
        (!has_ctrl && has_alt) ||
        (has_ctrl && !has_alt))
    {
      text->doit = False;     /* abort processing */
      return;
    }

    motcode = iupmotKeycodeToKeysym((XKeyEvent*)(text->event));
  }

  cb = (IFnis)IupGetCallback(ih, "ACTION");

  start = text->startPos;
  end = text->endPos;
  insert_value = text->text->ptr;

  if (motcode == XK_Delete)
  {
    insert_value = NULL;
    remove_dir = 1;
  }
  else if (motcode == XK_BackSpace)
  {
    insert_value = NULL;
    remove_dir = -1;
  }

  ret = iupEditCallActionCb(ih, cb, insert_value, start, end, ih->data->mask, ih->data->nc, remove_dir, 0);  /* TODO: UTF8 support */
  if (ret == 0)
  {
    text->doit = False;     /* abort processing */
    return;
  }

  if (ret != -1)
  {
    insert_value = text->text->ptr;
    insert_value[0] = (char)ret;  /* replace key */
  }

  if (text->doit)
  {
    /* Spin is not automatically updated when you directly edit the text */
    Widget spinbox = (Widget)iupAttribGet(ih, "_IUP_EXTRAPARENT");
    if (spinbox && XmIsSpinBox(spinbox) && !iupAttribGet(ih, "_IUPMOT_SPIN_NOAUTO"))
    {
      XmTextPosition caret_pos = text->currInsert;
      /* do not handle all situations, but handle the basic ones */
      if (text->startPos == text->endPos) /* insert */
        caret_pos++;
      else if (text->startPos < text->endPos && text->startPos < text->currInsert)  /* backspace */
        caret_pos--;
      XmTextSetInsertionPosition(ih->handle, caret_pos);
      text->doit = False;

      iupAttribSet(ih, "_IUPMOT_UPDATESPIN", "1");
    }
  }

  (void)w;
}

static void motTextMotionVerifyCallback(Widget w, Ihandle* ih, XmTextVerifyCallbackStruct* textverify)
{
  int col, lin, pos;

  IFniii cb = (IFniii)IupGetCallback(ih, "CARET_CB");
  if (!cb) return;

  pos = textverify->newInsert;

  if (ih->data->is_multiline)
  {
    char *value = XmTextGetString(ih->handle);
    motTextGetLinColFromPosition(value, pos, &lin, &col);
    XtFree(value);
  }
  else
  {
    col = pos;
    col++; /* IUP starts at 1 */
    lin = 1;
  }

  if (pos != ih->data->last_caret_pos)
  {
    ih->data->last_caret_pos = pos;
    cb(ih, lin, col, pos);
  }

  (void)w;
}

static void motTextUpdateSpin(Ihandle* ih)
{
  int pos = IupGetInt(ih, "VALUE");
  ih->data->disable_callbacks = 1;
  XtVaSetValues(ih->handle, XmNposition, pos, NULL);
  ih->data->disable_callbacks = 0;
  iupAttribSet(ih, "_IUPMOT_UPDATESPIN", NULL);
}

static void motTextValueChangedCallback(Widget w, Ihandle* ih, XmAnyCallbackStruct* valuechanged)
{
  if (ih->data->disable_callbacks)
    return;

  if (iupAttribGet(ih, "_IUPMOT_UPDATESPIN"))
    motTextUpdateSpin(ih);

  iupBaseCallValueChangedCb(ih);

  (void)valuechanged;
  (void)w;
}

static void motTextKeyPressEvent(Widget w, Ihandle *ih, XKeyEvent *evt, Boolean *cont)
{
  Widget spinbox;

  *cont = True;
  iupmotKeyPressEvent(w, ih, (XEvent*)evt, cont);
  if (*cont == False)
    return;

  if (evt->state & ControlMask && !(evt->state & Mod1Mask || evt->state & Mod5Mask))   /* Ctrl but NOT Alt */
  {
    KeySym motcode = iupmotKeycodeToKeysym(evt);
    if (motcode == XK_c || motcode == XK_x || motcode == XK_v || motcode == XK_a)
    {
      ih->data->disable_callbacks = -1; /* let callbacks be processed in motTextSetClipboardAttrib */

      if (motcode == XK_c)
        motTextSetClipboardAttrib(ih, "COPY");
      else if (motcode == XK_x)
        motTextSetClipboardAttrib(ih, "CUT");
      else if (motcode == XK_v)
        motTextSetClipboardAttrib(ih, "PASTE");
      else if (motcode == XK_a)
        XmTextSetSelection(ih->handle, 0, XmTextGetLastPosition(ih->handle), CurrentTime);

      ih->data->disable_callbacks = 0;

      *cont = False; 
      return;
    }
  }

  spinbox = (Widget)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  if (spinbox && XmIsSpinBox(spinbox))
  {
    KeySym motcode = iupmotKeycodeToKeysym(evt);
    if (motcode == XK_Left || motcode == XK_Right)
    {
      /* avoid spin increment using Left/Right arrows, 
         but must manually handle the new cursor position */
      XmTextPosition caret_pos = XmTextGetInsertionPosition(ih->handle);
      XmTextSetInsertionPosition(ih->handle, (motcode == XK_Left)? caret_pos-1: caret_pos+1);
      *cont = False; 
    }
  }
}


/******************************************************************************/

static void motDummyXtErrorHandler(String msg)
{
  /* does nothing */
  (void)msg;
}


static void motTextLayoutUpdateMethod(Ihandle* ih)
{
  Widget spinbox = (Widget)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  if (spinbox && XmIsSpinBox(spinbox))
  {
    /* avoid abort in X */
    if (ih->currentwidth == 0) ih->currentwidth = 2;
    if (ih->currentheight == 0) ih->currentheight = 1;

    XtVaSetValues(spinbox,
      XmNwidth, (XtArgVal)ih->currentwidth,
      XmNheight, (XtArgVal)ih->currentheight,
      XmNarrowSize, ih->currentheight/2,
      NULL);

    XtVaSetValues(ih->handle,
      XmNwidth, (XtArgVal)ih->currentwidth-ih->currentheight/2,
      XmNheight, (XtArgVal)ih->currentheight,
      NULL);

    iupmotSetPosition(spinbox, ih->x, ih->y);
  }
  else
  {
    /* to avoid the Scrollbar warning */
    XtAppSetWarningHandler(iupmot_appcontext, motDummyXtErrorHandler);
    iupdrvBaseLayoutUpdateMethod(ih);
    XtAppSetWarningHandler(iupmot_appcontext, NULL);
  }
}

static int motTextMapMethod(Ihandle* ih)
{
  int num_args = 0;
  Arg args[30];
  Widget parent = iupChildTreeGetNativeParentHandle(ih);
  char* child_id = iupDialogGetChildIdStr(ih);
  int spin = 0;
  WidgetClass widget_class = xmTextWidgetClass;

  if (ih->data->is_multiline)
  {
    Widget sb_win;
    int wordwrap = 0;

    if (iupAttribGetBoolean(ih, "WORDWRAP"))
    {
      wordwrap = 1;
      ih->data->sb &= ~IUP_SB_HORIZ;  /* must remove the horizontal scrollbar */
    }

    /******************************/
    /* Create the scrolled window */
    /******************************/

    iupMOT_SETARG(args, num_args, XmNmappedWhenManaged, False);  /* not visible when managed */
    iupMOT_SETARG(args, num_args, XmNscrollingPolicy, XmAPPLICATION_DEFINED);
    iupMOT_SETARG(args, num_args, XmNvisualPolicy, XmVARIABLE);
    iupMOT_SETARG(args, num_args, XmNscrollBarDisplayPolicy, XmSTATIC);   /* can NOT be XmAS_NEEDED because XmAPPLICATION_DEFINED */
    iupMOT_SETARG(args, num_args, XmNspacing, 0); /* no space between scrollbars and text */
    iupMOT_SETARG(args, num_args, XmNborderWidth, 0);
    iupMOT_SETARG(args, num_args, XmNshadowThickness, 0);
    
    sb_win = XtCreateManagedWidget(
      child_id,  /* child identifier */
      xmScrolledWindowWidgetClass, /* widget class */
      parent,                      /* widget parent */
      args, num_args);

    if (!sb_win)
      return IUP_ERROR;

    parent = sb_win;
    child_id = "text";

    num_args = 0;
    iupMOT_SETARG(args, num_args, XmNeditMode, XmMULTI_LINE_EDIT);
    if (wordwrap)
      iupMOT_SETARG(args, num_args, XmNwordWrap, True);
  }
  else
  {
    widget_class = xmTextFieldWidgetClass;

    if (iupAttribGetBoolean(ih, "SPIN"))
    {
      Widget spinbox;

      num_args = 0;
      iupMOT_SETARG(args, num_args, XmNmappedWhenManaged, False);  /* not visible when managed */
      iupMOT_SETARG(args, num_args, XmNspacing, 0); /* no space between spin and text */
      iupMOT_SETARG(args, num_args, XmNborderWidth, 0);
      iupMOT_SETARG(args, num_args, XmNshadowThickness, 0);
      iupMOT_SETARG(args, num_args, XmNmarginHeight, 0);
      iupMOT_SETARG(args, num_args, XmNmarginWidth, 0);
      iupMOT_SETARG(args, num_args, XmNarrowSize, 8);

      if (iupStrEqualNoCase(iupAttribGetStr(ih, "SPINALIGN"), "LEFT"))
        iupMOT_SETARG(args, num_args, XmNarrowLayout, XmARROWS_BEGINNING);
      else
        iupMOT_SETARG(args, num_args, XmNarrowLayout, XmARROWS_END);

      spinbox = XtCreateManagedWidget(
        child_id,  /* child identifier */
        xmSpinBoxWidgetClass, /* widget class */
        parent,                      /* widget parent */
        args, num_args);

      if (!spinbox)
        return IUP_ERROR;

      XtAddCallback(spinbox, XmNmodifyVerifyCallback, (XtCallbackProc)motTextSpinModifyVerifyCallback, (XtPointer)ih);

      parent = spinbox;
      child_id = "text";
      spin = 1;

      if (!iupAttribGetBoolean(ih, "SPINAUTO"))
        iupAttribSet(ih, "_IUPMOT_SPIN_NOAUTO", "1");
    }

    num_args = 0;
    iupMOT_SETARG(args, num_args, XmNeditMode, XmSINGLE_LINE_EDIT);

    if (spin)
    {
      /* Spin Constraints */
      iupMOT_SETARG(args, num_args, XmNspinBoxChildType, XmNUMERIC);
      iupMOT_SETARG(args, num_args, XmNminimumValue, iupAttribGetInt(ih, "SPINMIN"));
      iupMOT_SETARG(args, num_args, XmNmaximumValue, iupAttribGetInt(ih, "SPINMAX"));
      iupMOT_SETARG(args, num_args, XmNincrementValue, iupAttribGetInt(ih, "SPININC"));
      iupMOT_SETARG(args, num_args, XmNposition, iupAttribGetInt(ih, "SPINMIN"));

      if (iupAttribGetBoolean(ih, "SPINWRAP"))
        iupMOT_SETARG(args, num_args, XmNwrap, TRUE);
      else
        iupMOT_SETARG(args, num_args, XmNwrap, FALSE);
    }
    else
    {
      iupMOT_SETARG(args, num_args, XmNmappedWhenManaged, False);  /* not visible when managed */
    }
  }

  iupMOT_SETARG(args, num_args, XmNx, 0);  /* x-position */
  iupMOT_SETARG(args, num_args, XmNy, 0);  /* y-position */
  iupMOT_SETARG(args, num_args, XmNwidth, 10);  /* default width to avoid 0 */
  iupMOT_SETARG(args, num_args, XmNheight, 10); /* default height to avoid 0 */

  iupMOT_SETARG(args, num_args, XmNmarginHeight, 0);  /* default padding */
  iupMOT_SETARG(args, num_args, XmNmarginWidth, 0);

  if (iupAttribGetBoolean(ih, "CANFOCUS"))
    iupMOT_SETARG(args, num_args, XmNtraversalOn, True);
  else
    iupMOT_SETARG(args, num_args, XmNtraversalOn, False);

  iupMOT_SETARG(args, num_args, XmNnavigationType, XmTAB_GROUP);
  iupMOT_SETARG(args, num_args, XmNhighlightThickness, 2);
  iupMOT_SETARG(args, num_args, XmNverifyBell, False);
  iupMOT_SETARG(args, num_args, XmNspacing, 0);

  if (iupAttribGetBoolean(ih, "BORDER"))
    iupMOT_SETARG(args, num_args, XmNshadowThickness, 2);
  else
    iupMOT_SETARG(args, num_args, XmNshadowThickness, 0);

  if (ih->data->is_multiline)
  {
    if (ih->data->sb & IUP_SB_HORIZ)
      iupMOT_SETARG(args, num_args, XmNscrollHorizontal, True);
    else
      iupMOT_SETARG(args, num_args, XmNscrollHorizontal, False);

    if (ih->data->sb & IUP_SB_VERT)
      iupMOT_SETARG(args, num_args, XmNscrollVertical, True);
    else
      iupMOT_SETARG(args, num_args, XmNscrollVertical, False);
  }

  ih->handle = XtCreateManagedWidget(
    child_id,       /* child identifier */
    widget_class,   /* widget class */
    parent,         /* widget parent */
    args, num_args);

  if (!ih->handle)
    return IUP_ERROR;

  ih->serial = iupDialogGetChildId(ih); /* must be after using the string */

  if (ih->data->is_multiline)
  {
    iupAttribSet(ih, "_IUP_EXTRAPARENT", (char*)parent);
    XtVaSetValues(parent, XmNworkWindow, ih->handle, NULL);
  } 
  else if (spin)
    iupAttribSet(ih, "_IUP_EXTRAPARENT", (char*)parent);

  XtAddCallback(ih->handle, XmNhelpCallback, (XtCallbackProc)iupmotHelpCallback, (XtPointer)ih);
  XtAddEventHandler(ih->handle, EnterWindowMask, False, (XtEventHandler)iupmotEnterLeaveWindowEvent, (XtPointer)ih);
  XtAddEventHandler(ih->handle, LeaveWindowMask, False, (XtEventHandler)iupmotEnterLeaveWindowEvent, (XtPointer)ih);
  XtAddEventHandler(ih->handle, FocusChangeMask, False, (XtEventHandler)iupmotFocusChangeEvent, (XtPointer)ih);
  XtAddEventHandler(ih->handle, KeyPressMask, False, (XtEventHandler)motTextKeyPressEvent, (XtPointer)ih);
  XtAddEventHandler(ih->handle, PointerMotionMask, False, (XtEventHandler)iupmotPointerMotionEvent, (XtPointer)ih);
  XtAddEventHandler(ih->handle, ButtonPressMask|ButtonReleaseMask, False, (XtEventHandler)iupmotButtonPressReleaseEvent, (XtPointer)ih);

  XtAddCallback(ih->handle, XmNmodifyVerifyCallback, (XtCallbackProc)motTextModifyVerifyCallback, (XtPointer)ih);
  XtAddCallback(ih->handle, XmNmotionVerifyCallback, (XtCallbackProc)motTextMotionVerifyCallback, (XtPointer)ih);
  XtAddCallback(ih->handle, XmNvalueChangedCallback, (XtCallbackProc)motTextValueChangedCallback, (XtPointer)ih);

  /* initialize the widget */
  if (ih->data->is_multiline || spin)
    XtRealizeWidget(parent);
  else
    XtRealizeWidget(ih->handle);

  if (IupGetGlobal("_IUP_RESET_TXTCOLORS"))
  {
    iupmotSetGlobalColorAttrib(ih->handle, XmNbackground, "TXTBGCOLOR");
    iupmotSetGlobalColorAttrib(ih->handle, XmNforeground, "TXTFGCOLOR");
    iupmotSetGlobalColorAttrib(ih->handle, XmNhighlightColor, "TXTHLCOLOR");
    IupSetGlobal("_IUP_RESET_TXTCOLORS", NULL);
  }

  IupSetCallback(ih, "_IUP_XY2POS_CB", (Icallback)motTextConvertXYToPos);

  return IUP_NOERROR;
}

void iupdrvTextInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = motTextMapMethod;
  ic->LayoutUpdate = motTextLayoutUpdateMethod;

  /* Driver Dependent Attribute functions */

  /* Overwrite Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, motTextSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTBGCOLOR", IUPAF_DEFAULT);

  /* Special */
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, iupdrvBaseSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTFGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "AUTOREDRAW", NULL, motTextSetAutoRedrawAttrib, IUPAF_SAMEASSYSTEM, "Yes", IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

  /* IupText only */
  iupClassRegisterAttribute(ic, "PADDING", iupTextGetPaddingAttrib, motTextSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "VALUE", motTextGetValueAttrib, motTextSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LINEVALUE", motTextGetLineValueAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTEDTEXT", motTextGetSelectedTextAttrib, motTextSetSelectedTextAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTION", motTextGetSelectionAttrib, motTextSetSelectionAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTIONPOS", motTextGetSelectionPosAttrib, motTextSetSelectionPosAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARET", motTextGetCaretAttrib, motTextSetCaretAttrib, NULL, NULL, IUPAF_NO_SAVE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARETPOS", motTextGetCaretPosAttrib, motTextSetCaretPosAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_SAVE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERT", NULL, motTextSetInsertAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "APPEND", NULL, motTextSetAppendAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "READONLY", motTextGetReadOnlyAttrib, motTextSetReadOnlyAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "NC", iupTextGetNCAttrib, motTextSetNCAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "CLIPBOARD", NULL, motTextSetClipboardAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATTING", NULL, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);  /* formatting not supported in Motif */
  iupClassRegisterAttribute(ic, "SCROLLTO", NULL, motTextSetScrollToAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTOPOS", NULL, motTextSetScrollToPosAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINMIN", NULL, motTextSetSpinMinAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINMAX", NULL, motTextSetSpinMaxAttrib, IUPAF_SAMEASSYSTEM, "100", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPININC", NULL, motTextSetSpinIncAttrib, IUPAF_SAMEASSYSTEM, "1", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINVALUE", motTextGetSpinValueAttrib, motTextSetSpinValueAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COUNT", motTextGetCountAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LINECOUNT", motTextGetLineCountAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);

  /* Not Supported */
  iupClassRegisterAttribute(ic, "ALIGNMENT", NULL, NULL, IUPAF_SAMEASSYSTEM, "ALEFT", IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CUEBANNER", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FILTER", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATTING", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "OVERWRITE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABSIZE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
}
