/** \file
 * \brief List Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <Xm/Xm.h>
#include <Xm/List.h>
#include <Xm/ComboBox.h>
#include <Xm/ScrolledW.h>
#include <Xm/TextF.h>
#include <Xm/Transfer.h>
#include <Xm/DragDrop.h>
#include <X11/keysym.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdarg.h>
#include <time.h>
#include <limits.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_childtree.h"
#include "iup_attrib.h"
#include "iup_dialog.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_mask.h"
#include "iup_key.h"
#include "iup_list.h"

#include "iupmot_drv.h"
#include "iupmot_color.h"


static void motListComboBoxSelectionCallback(Widget w, Ihandle* ih, XmComboBoxCallbackStruct* call_data);


void* iupdrvListGetImageHandle(Ihandle* ih, int id)
{
  (void)ih;
  (void)id;
  return NULL;
}

int iupdrvListSetImageHandle(Ihandle* ih, int id, void* hImage)
{
  (void)ih;
  (void)id;
  (void)hImage;
  return 0;
}

void iupdrvListAddItemSpace(Ihandle* ih, int *h)
{
  if (ih->data->has_editbox)
    *h += 1;
  else
    *h += 3;
}

void iupdrvListAddBorders(Ihandle* ih, int *x, int *y)
{
  int border_size = 2*4;
  (*x) += border_size;
  (*y) += border_size;

  if (ih->data->is_dropdown)
  {
    if (ih->data->has_editbox)
    {
      /* extra border for the editbox */
      int internal_border_size = 2*2;
      (*x) += internal_border_size; 
      (*y) += internal_border_size;
    }
  }
  else
  {
    if (ih->data->has_editbox)
      (*y) += 2*2; /* internal border between editbox and list */
    else
      (*x) += 2; /* extra border for the simple list */
  }
}

static int motListConvertXYToPos(Ihandle* ih, int x, int y)
{
  int count, pos;
  (void)x;

  if (ih->data->is_dropdown)
    return -1;

  if (ih->data->has_editbox)
  {
    Widget cblist;
    XtVaGetValues(ih->handle, XmNlist, &cblist, NULL);
    pos = XmListYToPos(cblist, (Position)y);   /* XmListYToPos returns start at 1 */
  }
  else
    pos = XmListYToPos(ih->handle, (Position)y);

  XtVaGetValues(ih->handle, XmNitemCount, &count, NULL);
  if (pos <= 0 || pos >= count)
    return -1;
  else
    return pos;
}

int iupdrvListGetCount(Ihandle* ih)
{
  int count;
  XtVaGetValues(ih->handle, XmNitemCount, &count, NULL);
  return count;
}

static void motListAddItem(Ihandle* ih, int pos, const char* value)
{
  XmString str = iupmotStringCreate(value);
  /* The utility functions use 0=last 1=first */
  if (ih->data->is_dropdown || ih->data->has_editbox)
    XmComboBoxAddItem(ih->handle, str, pos+1, False);
  else
    XmListAddItem(ih->handle, str, pos+1);
  XmStringFree(str);
}

static void motListAddSortedItem(Ihandle* ih, const char *value)
{
  char *text;
  XmString *strlist;
  int u_bound, l_bound = 0;

  XtVaGetValues(ih->handle, XmNitemCount, &u_bound, XmNitems, &strlist, NULL);

  u_bound--;
  /* perform binary search */
  while (u_bound >= l_bound) 
  {
    int i = l_bound + (u_bound - l_bound)/2;
    text = (char*)XmStringUnparse(strlist[i], NULL, XmCHARSET_TEXT, XmCHARSET_TEXT, NULL, 0, XmOUTPUT_ALL);
    if (!text)
      break;
    if (strcmp (text, value) > 0)
      u_bound = i-1; /* newtext comes before item */
    else
      l_bound = i+1; /* newtext comes after item */
    XtFree(text);
  }

  motListAddItem(ih, l_bound, value);
}

void iupdrvListAppendItem(Ihandle* ih, const char* value)
{
  if (iupAttribGetBoolean(ih, "SORT"))
    motListAddSortedItem(ih, value);
  else
    motListAddItem(ih, -1, value);
}

void iupdrvListInsertItem(Ihandle* ih, int pos, const char* value)
{
  if (iupAttribGetBoolean(ih, "SORT"))
    motListAddSortedItem(ih, value);
  else
    motListAddItem(ih, pos, value);

  iupListUpdateOldValue(ih, pos, 0);
}

void iupdrvListRemoveItem(Ihandle* ih, int pos)
{
  /* The utility functions use 0=last 1=first */
  if (ih->data->is_dropdown || ih->data->has_editbox)
  {
    if (ih->data->is_dropdown && !ih->data->has_editbox)
    {
      /* must check if removing the current item */
      int curpos;
      XtVaGetValues(ih->handle, XmNselectedPosition, &curpos, NULL);
      if (pos == curpos && iupdrvListGetCount(ih)>1)
      {
        if (curpos > 0) curpos--;
        else curpos++;

        XtRemoveCallback(ih->handle, XmNselectionCallback, (XtCallbackProc)motListComboBoxSelectionCallback, (XtPointer)ih);
        XtVaSetValues(ih->handle, XmNselectedPosition, curpos, NULL);  
        XtAddCallback(ih->handle, XmNselectionCallback, (XtCallbackProc)motListComboBoxSelectionCallback, (XtPointer)ih);
      }
    }
    XmComboBoxDeletePos(ih->handle, pos+1);
  }
  else
    XmListDeletePos(ih->handle, pos+1);

  iupListUpdateOldValue(ih, pos, 1);
}

void iupdrvListRemoveAllItems(Ihandle* ih)
{
  if (ih->data->is_dropdown || ih->data->has_editbox)
  {
    Widget cblist;
    XtVaGetValues(ih->handle, XmNlist, &cblist, NULL);
    XmListDeleteAllItems(cblist);
    XmComboBoxUpdate(ih->handle);
  }
  else
    XmListDeleteAllItems(ih->handle);
}


/*********************************************************************************/


static char* motListGetIdValueAttrib(Ihandle* ih, int id)
{
  int pos = iupListGetPosAttrib(ih, id);
  if (pos >= 0)
  {
    XmString* items;
    XtVaGetValues(ih->handle, XmNitems, &items, NULL);  /* returns the actual list, not a copy */
    return iupmotReturnXmString(items[pos]);
  }
  return NULL;
}

static int motListSetBgColorAttrib(Ihandle* ih, const char* value)
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

    return iupdrvBaseSetBgColorAttrib(ih, value);    /* use given value for contents */
  }
  else
  {
    char* parent_value;

    /* use given value for Edit and List also */
    Pixel color = iupmotColorGetPixelStr(value);
    if (color != (Pixel)-1)
    {
      Widget cbedit, cblist, sb;

      iupmotSetBgColor(ih->handle, color);

      XtVaGetValues(ih->handle, XmNtextField, &cbedit, NULL);
      if (cbedit) iupmotSetBgColor(cbedit, color);

      XtVaGetValues(ih->handle, XmNlist, &cblist, NULL);
      if (cblist) iupmotSetBgColor(cblist, color);

      if (iupStrBoolean(IupGetGlobal("SB_BGCOLOR")))
      {
        XtVaGetValues(cblist, XmNverticalScrollBar, &sb, NULL);
        if (sb) iupmotSetBgColor(sb, color);

        XtVaGetValues(cblist, XmNhorizontalScrollBar, &sb, NULL);
        if (sb) iupmotSetBgColor(sb, color);
      }
    }

    /* but reset just the background, so the combobox will look like a button */
    parent_value = iupBaseNativeParentGetBgColor(ih);

    color = iupmotColorGetPixelStr(parent_value);
    if (color != (Pixel)-1)
      XtVaSetValues(ih->handle, XmNbackground, color, NULL);

    return 1;
  }
}

static int motListSetFgColorAttrib(Ihandle* ih, const char* value)
{
  Pixel color = iupmotColorGetPixelStr(value);
  if (color != (Pixel)-1)
  {
    XtVaSetValues(ih->handle, XmNforeground, color, NULL);

    if (ih->data->is_dropdown || ih->data->has_editbox)
    {
      Widget w;
      XtVaGetValues(ih->handle, XmNtextField, &w, NULL);
      XtVaSetValues(w, XmNforeground, color, NULL);

      XtVaGetValues(ih->handle, XmNlist, &w, NULL);
      XtVaSetValues(w, XmNforeground, color, NULL);
    }
  }

  return 1;
}

static char* motListGetValueAttrib(Ihandle* ih)
{
  if (ih->data->has_editbox)
  {
    char *str, *xstr;
    Widget cbedit;
    XtVaGetValues(ih->handle, XmNtextField, &cbedit, NULL);
    xstr = XmTextFieldGetString(cbedit);
    str = iupStrReturnStr(xstr);
    XtFree(xstr);
    return str;
  }
  else 
  {
    if (ih->data->is_dropdown)
    {
      int pos;
      XtVaGetValues(ih->handle, XmNselectedPosition, &pos, NULL);
      return iupStrReturnInt(pos+1);  /* IUP starts at 1 */
    }
    else
    {
      int *pos, sel_count;
      if (XmListGetSelectedPos(ih->handle, &pos, &sel_count))  /* XmListGetSelectedPos starts at 1 */
      {
        if (!ih->data->is_multiple)
        {
          int ret = pos[0];  
          XtFree((char*)pos);
          return iupStrReturnInt(ret);
        }
        else
        {
          int i, count;
          char* str;
          XtVaGetValues(ih->handle, XmNitemCount, &count, NULL);
          str = iupStrGetMemory(count+1);
          memset(str, '-', count);
          str[count]=0;
          for (i=0; i<sel_count; i++)
            str[pos[i]-1] = '+';
          XtFree((char*)pos);
          return str;
        }
      }
    }
  }

  return NULL;
}

static int motListSetValueAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->has_editbox)
  {
    Widget cbedit;
    XtVaGetValues(ih->handle, XmNtextField, &cbedit, NULL);
    if (!value) value = "";

    iupAttribSet(ih, "_IUPMOT_DISABLE_TEXT_CB", "1"); /* disable callbacks */

    iupmotTextSetString(cbedit, value);

    iupAttribSet(ih, "_IUPMOT_DISABLE_TEXT_CB", NULL);
  }
  else 
  {
    if (ih->data->is_dropdown)
    {
      int pos;
      if (iupStrToInt(value, &pos)==1)
      {
        XtRemoveCallback(ih->handle, XmNselectionCallback, (XtCallbackProc)motListComboBoxSelectionCallback, (XtPointer)ih);

        XtVaSetValues(ih->handle, XmNselectedPosition, pos-1, NULL);  /* IUP starts at 1 */
        iupAttribSetInt(ih, "_IUPLIST_OLDVALUE", pos);

        XtAddCallback(ih->handle, XmNselectionCallback, (XtCallbackProc)motListComboBoxSelectionCallback, (XtPointer)ih);
      }
    }
    else
    {
      if (!ih->data->is_multiple)
      {
        int pos;
        if (iupStrToInt(value, &pos)==1)
        {
          XmListSelectPos(ih->handle, pos, FALSE);   /* XmListSelectPos starts at 1 */
          iupAttribSetInt(ih, "_IUPLIST_OLDVALUE", pos);
        }
        else
        {
          XmListDeselectAllItems(ih->handle);
          iupAttribSet(ih, "_IUPLIST_OLDVALUE", NULL);
        }
      }
      else
      {
        /* User has changed a multiple selection on a simple list. */
	      int i, count, len;

        /* Clear all selections */
        XmListDeselectAllItems(ih->handle);

        if (!value)
        {
          iupAttribSet(ih, "_IUPLIST_OLDVALUE", NULL);
          return 0;
        }

        XtVaGetValues(ih->handle, XmNitemCount, &count, NULL);
        len = strlen(value);
        if (len < count) 
          count = len;

        XtVaSetValues(ih->handle, XmNselectionPolicy, XmMULTIPLE_SELECT, NULL);

        /* update selection list */
        for (i = 0; i<count; i++)
        {
          if (value[i]=='+')
            XmListSelectPos(ih->handle, i+1, False);  /* XmListSelectPos starts at 1 */
        }

        XtVaSetValues(ih->handle, XmNselectionPolicy, XmEXTENDED_SELECT, 
                                  XmNselectionMode, XmNORMAL_MODE, NULL);  /* must also restore this */
        iupAttribSetStr(ih, "_IUPLIST_OLDVALUE", value);
      }
    }
  }

  return 0;
}

static int motListSetVisibleItemsAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->is_dropdown)
  {
    int count;
    if (iupStrToInt(value, &count)==1)
      XtVaSetValues(ih->handle, XmNvisibleItemCount, count, NULL);
  }
  return 1;
}

static int motListSetShowDropdownAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->is_dropdown)
  {
    if (iupStrBoolean(value))
    {
      XButtonEvent ev;
      memset(&ev, 0, sizeof(XButtonEvent));
      ev.type = ButtonPress;
      ev.display = XtDisplay(ih->handle);
      ev.send_event = True;
      ev.root = RootWindow(iupmot_display, iupmot_screen);
      ev.time = clock()*CLOCKS_PER_SEC;
      ev.window = XtWindow(ih->handle);
      ev.state = Button1Mask;
      ev.button = Button1;
      ev.same_screen = True;
      XtCallActionProc(ih->handle, "CBDropDownList", (XEvent*)&ev, 0, 0 ); 
    }
    else
      XtCallActionProc(ih->handle, "CBDisarm", 0, 0, 0 );
  }
  return 0;
}

static int motListSetTopItemAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->is_dropdown)
  {
    int pos = 1;
    if (iupStrToInt(value, &pos))
    {
      if (ih->data->has_editbox)
      {
        Widget cblist;
        XtVaGetValues(ih->handle, XmNlist, &cblist, NULL);
        XtVaSetValues(cblist, XmNtopItemPosition, pos, NULL);
      }
      else
        XtVaSetValues(ih->handle, XmNtopItemPosition, pos, NULL);
    }
  }
  return 0;
}

static int motListSetSpacingAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->is_dropdown)
    return 0;

  if (!iupStrToInt(value, &ih->data->spacing))
    ih->data->spacing = 0;

  if (ih->handle)
  {
    Widget list = ih->handle;
    if (ih->data->has_editbox)
      XtVaGetValues(ih->handle, XmNlist, &list, NULL);

    XtVaSetValues(list, XmNlistSpacing, ih->data->spacing*2, 
                        XmNlistMarginWidth, ih->data->spacing, 
                        XmNlistMarginHeight, ih->data->spacing, 
                        NULL);
    return 0;
  }
  else
    return 1; /* store until not mapped, when mapped will be set again */
}

static int motListSetPaddingAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->has_editbox)
    return 0;

  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');
  if (ih->handle)
  {
    Widget cbedit;
    XtVaGetValues(ih->handle, XmNtextField, &cbedit, NULL);
    XtVaSetValues(cbedit, XmNmarginHeight, ih->data->vert_padding,
                          XmNmarginWidth, ih->data->horiz_padding, NULL);
    return 0;
  }
  else
    return 1; /* store until not mapped, when mapped will be set again */
}

static int motListSetReadOnlyAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->has_editbox)
  {
    Widget cbedit;
    XtVaGetValues(ih->handle, XmNtextField, &cbedit, NULL);
    XtVaSetValues(cbedit, XmNeditable, iupStrBoolean(value)? False: True, NULL);
  }
  return 0;
}

static char* motListGetReadOnlyAttrib(Ihandle* ih)
{
  if (ih->data->has_editbox)
  {
    Boolean editable;
    Widget cbedit;
    XtVaGetValues(ih->handle, XmNtextField, &cbedit, NULL);
    XtVaGetValues(cbedit, XmNeditable, &editable, NULL);
    return iupStrReturnBoolean(editable);
  }
  else
    return NULL;
}

static int motListSetInsertAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    return 0;

  if (ih->data->has_editbox)
  {
    Widget cbedit;
    XtVaGetValues(ih->handle, XmNtextField, &cbedit, NULL);
    iupAttribSet(ih, "_IUPMOT_DISABLE_TEXT_CB", "1"); /* disable callbacks */
    XmTextFieldRemove(cbedit);
    XmTextFieldInsert(cbedit, XmTextFieldGetInsertionPosition(cbedit), (char*)value);
    iupAttribSet(ih, "_IUPMOT_DISABLE_TEXT_CB", NULL);
  }

  return 0;
}

static int motListSetSelectedTextAttrib(Ihandle* ih, const char* value)
{
  XmTextPosition start, end;
  Widget cbedit;
  if (!ih->data->has_editbox)
    return 0;

  if (!value)
    return 0;

  XtVaGetValues(ih->handle, XmNtextField, &cbedit, NULL);
  if (XmTextFieldGetSelectionPosition(cbedit, &start, &end) && start!=end)
  {
    iupAttribSet(ih, "_IUPMOT_DISABLE_TEXT_CB", "1"); /* disable callbacks */
    XmTextFieldReplace(cbedit, start, end, (char*)value);
    iupAttribSet(ih, "_IUPMOT_DISABLE_TEXT_CB", NULL);
  }

  return 0;
}

static char* motListGetSelectedTextAttrib(Ihandle* ih)
{
  if (ih->data->has_editbox)
  {
    char* selectedtext, *str;
    Widget cbedit;
    XtVaGetValues(ih->handle, XmNtextField, &cbedit, NULL);
    selectedtext = XmTextFieldGetSelection(cbedit);
    str = iupStrReturnStr(selectedtext);
    XtFree(selectedtext);
    return str;
  }
  else
    return NULL;
}

static int motListSetAppendAttrib(Ihandle* ih, const char* value)
{
	if (value && ih->data->has_editbox)
  {
    XmTextPosition pos;
    Widget cbedit;
    XtVaGetValues(ih->handle, XmNtextField, &cbedit, NULL);
    pos = XmTextFieldGetLastPosition(cbedit);
    iupAttribSet(ih, "_IUPMOT_DISABLE_TEXT_CB", "1"); /* disable callbacks */
    XmTextFieldInsert(cbedit, pos+1, (char*)value);
    iupAttribSet(ih, "_IUPMOT_DISABLE_TEXT_CB", NULL);
  }
  return 0;
}

static int motListSetSelectionAttrib(Ihandle* ih, const char* value)
{
  int start=1, end=1;
  Widget cbedit;
  if (!ih->data->has_editbox)
    return 0;

  if (!value || iupStrEqualNoCase(value, "NONE"))
  {
    XtVaGetValues(ih->handle, XmNtextField, &cbedit, NULL);
    XmTextFieldClearSelection(cbedit, CurrentTime);
    return 0;
  }

  if (iupStrEqualNoCase(value, "ALL"))
  {
    XtVaGetValues(ih->handle, XmNtextField, &cbedit, NULL);
    XmTextFieldSetSelection(cbedit, (XmTextPosition)0, (XmTextPosition)XmTextFieldGetLastPosition(cbedit), CurrentTime);
    return 0;
  }

  if (iupStrToIntInt(value, &start, &end, ':')!=2) 
    return 0;

  if(start<1 || end<1) 
    return 0;

  start--; /* IUP starts at 1 */
  end--;

  /* end is inside the selection, in IUP is outside */
  end--;

  XtVaGetValues(ih->handle, XmNtextField, &cbedit, NULL);
  XmTextFieldSetSelection(cbedit, (XmTextPosition)start, (XmTextPosition)end, CurrentTime);

  return 0;
}

static char* motListGetSelectionAttrib(Ihandle* ih)
{
  XmTextPosition start = 0, end = 0;
  Widget cbedit;
  if (!ih->data->has_editbox)
    return NULL;

  XtVaGetValues(ih->handle, XmNtextField, &cbedit, NULL);
  if (!XmTextFieldGetSelectionPosition(cbedit, &start, &end) || start==end)
    return NULL;

  /* end is inside the selection, in IUP is outside */
  end++;

  start++; /* IUP starts at 1 */
  end++;
  return iupStrReturnIntInt((int)start, (int)end, ':');
}

static int motListSetSelectionPosAttrib(Ihandle* ih, const char* value)
{
  int start=0, end=0;
  Widget cbedit;
  if (!ih->data->has_editbox)
    return 0;

  if (!value || iupStrEqualNoCase(value, "NONE"))
  {
    XtVaGetValues(ih->handle, XmNtextField, &cbedit, NULL);
    XmTextFieldClearSelection(cbedit, CurrentTime);
    return 0;
  }

  if (iupStrEqualNoCase(value, "ALL"))
  {
    XtVaGetValues(ih->handle, XmNtextField, &cbedit, NULL);
    XmTextFieldSetSelection(cbedit, (XmTextPosition)0, (XmTextPosition)XmTextFieldGetLastPosition(cbedit), CurrentTime);
    return 0;
  }

  if (iupStrToIntInt(value, &start, &end, ':')!=2) 
    return 0;

  if(start<0 || end<0) 
    return 0;

  /* end is inside the selection, in IUP is outside */
  end--;

  XtVaGetValues(ih->handle, XmNtextField, &cbedit, NULL);
  XmTextFieldSetSelection(cbedit, (XmTextPosition)start, (XmTextPosition)end, CurrentTime);

  return 0;
}

static char* motListGetSelectionPosAttrib(Ihandle* ih)
{
  XmTextPosition start = 0, end = 0;
  Widget cbedit;
  if (!ih->data->has_editbox)
    return NULL;

  XtVaGetValues(ih->handle, XmNtextField, &cbedit, NULL);
  if (!XmTextFieldGetSelectionPosition(cbedit, &start, &end) || start==end)
    return NULL;

  /* end is inside the selection, in IUP is outside */
  end++;

  return iupStrReturnIntInt((int)start, (int)end, ':');
}

static int motListSetCaretAttrib(Ihandle* ih, const char* value)
{
  int pos = 1;
  Widget cbedit;
  if (!ih->data->has_editbox)
    return 0;

  if (!value)
    return 0;

  iupStrToInt(value, &pos);
  pos--; /* IUP starts at 1 */

  XtVaGetValues(ih->handle, XmNtextField, &cbedit, NULL);
  XmTextFieldSetInsertionPosition(cbedit, (XmTextPosition)pos);
  XmTextFieldShowPosition(cbedit, (XmTextPosition)pos);

  return 0;
}

static char* motListGetCaretAttrib(Ihandle* ih)
{
  if (ih->data->has_editbox)
  {
    XmTextPosition pos;
    Widget cbedit;
    XtVaGetValues(ih->handle, XmNtextField, &cbedit, NULL);
    pos = XmTextFieldGetInsertionPosition(cbedit);
    pos++; /* IUP starts at 1 */
    return iupStrReturnInt((int)pos);
  }
  else
    return NULL;
}

static int motListSetCaretPosAttrib(Ihandle* ih, const char* value)
{
  int pos = 0;
  Widget cbedit;
  if (!ih->data->has_editbox)
    return 0;

  if (!value)
    return 0;

  iupStrToInt(value, &pos);
  if (pos < 0) pos = 0;

  XtVaGetValues(ih->handle, XmNtextField, &cbedit, NULL);
  XmTextFieldSetInsertionPosition(cbedit, (XmTextPosition)pos);
  XmTextFieldShowPosition(cbedit, (XmTextPosition)pos);

  return 0;
}

static char* motListGetCaretPosAttrib(Ihandle* ih)
{
  if (ih->data->has_editbox)
  {
    XmTextPosition pos;
    Widget cbedit;
    XtVaGetValues(ih->handle, XmNtextField, &cbedit, NULL);
    pos = XmTextFieldGetInsertionPosition(cbedit);
    return iupStrReturnInt((int)pos);
  }
  else
    return NULL;
}

static int motListSetScrollToAttrib(Ihandle* ih, const char* value)
{
  int pos = 1;
  Widget cbedit;
  if (!ih->data->has_editbox)
    return 0;

  if (!value)
    return 0;

  iupStrToInt(value, &pos);
  if (pos < 1) pos = 1;
  pos--;  /* return to Motif referece */

  XtVaGetValues(ih->handle, XmNtextField, &cbedit, NULL);
  XmTextFieldShowPosition(cbedit, (XmTextPosition)pos);

  return 0;
}

static int motListSetScrollToPosAttrib(Ihandle* ih, const char* value)
{
  int pos = 0;
  Widget cbedit;
  if (!ih->data->has_editbox)
    return 0;

  if (!value)
    return 0;

  iupStrToInt(value, &pos);
  if (pos < 0) pos = 0;

  XtVaGetValues(ih->handle, XmNtextField, &cbedit, NULL);
  XmTextFieldShowPosition(cbedit, (XmTextPosition)pos);

  return 0;
}

static int motListSetNCAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->has_editbox)
    return 0;

  if (!iupStrToInt(value, &ih->data->nc))
    ih->data->nc = INT_MAX;

  if (ih->handle)
  {
    Widget cbedit;
    XtVaGetValues(ih->handle, XmNtextField, &cbedit, NULL);
    XtVaSetValues(cbedit, XmNmaxLength, ih->data->nc, NULL);
    return 0;
  }
  else
    return 1; /* store until not mapped, when mapped will be set again */
}

static int motListSetClipboardAttrib(Ihandle *ih, const char *value)
{
  Widget cbedit;
  if (!ih->data->has_editbox)
    return 0;

  XtVaGetValues(ih->handle, XmNtextField, &cbedit, NULL);

  if (iupStrEqualNoCase(value, "COPY"))
  {
    Ihandle* clipboard;
    char *str = XmTextFieldGetSelection(cbedit);
    if (!str) return 0;

    clipboard = IupClipboard();
    IupSetAttribute(clipboard, "TEXT", str);
    IupDestroy(clipboard);

    XtFree(str);
  }
  else if (iupStrEqualNoCase(value, "CUT"))
  {
    Ihandle* clipboard;
    char *str = XmTextFieldGetSelection(cbedit);
    if (!str) return 0;

    clipboard = IupClipboard();
    IupSetAttribute(clipboard, "TEXT", str);
    IupDestroy(clipboard);

    XtFree(str);

    /* disable callbacks */
    iupAttribSet(ih, "_IUPMOT_DISABLE_TEXT_CB", "1");
    XmTextFieldRemove(cbedit);
    iupAttribSet(ih, "_IUPMOT_DISABLE_TEXT_CB", NULL);
  }
  else if (iupStrEqualNoCase(value, "PASTE"))
  {
    Ihandle* clipboard;
    char *str;

    clipboard = IupClipboard();
    str = IupGetAttribute(clipboard, "TEXT");

    /* disable callbacks */
    iupAttribSet(ih, "_IUPMOT_DISABLE_TEXT_CB", "1");
    XmTextFieldRemove(cbedit);
    XmTextFieldInsert(cbedit, XmTextFieldGetInsertionPosition(cbedit), str);
    iupAttribSet(ih, "_IUPMOT_DISABLE_TEXT_CB", NULL);
  }
  else if (iupStrEqualNoCase(value, "CLEAR"))
  {
    /* disable callbacks */
    iupAttribSet(ih, "_IUPMOT_DISABLE_TEXT_CB", "1");
    XmTextFieldRemove(cbedit);
    iupAttribSet(ih, "_IUPMOT_DISABLE_TEXT_CB", NULL);
  }
  return 0;
}


/*********************************************************************************/

static void motListDragTransferProc(Widget drop_context, Ihandle* ih, Atom *seltype, Atom *type, XtPointer value, unsigned long *length, int format)
{
  Atom atomListItem = XInternAtom(iupmot_display, "LIST_ITEM", False);
  int idDrag = (int)value;  /* starts at 1 */
  int idDrop = iupAttribGetInt(ih, "_IUPLIST_DROPITEM");  /* starts at 1 */

  if (idDrop==0)
    return;

  /* shift to start at 0 */
  idDrag--;
  idDrop--;

  if (*type == atomListItem)
  {
    int is_ctrl;

    if (iupListCallDragDropCb(ih, idDrag, idDrop, &is_ctrl) == IUP_CONTINUE)  /* starts at 0 */
    {
      char* text = motListGetIdValueAttrib(ih, idDrag+1); /* starts at 1 */
      int count = iupdrvListGetCount(ih);
        
      /* Copy the item to the idDrop position */
      if (idDrop >= 0 && idDrop < count) /* starts at 0 */
      {
        iupdrvListInsertItem(ih, idDrop, text);   /* starts at 0, insert before */
        if (idDrag > idDrop)
          idDrag++;
      }
      else  /* idDrop = -1 */
      {
        iupdrvListAppendItem(ih, text);
        idDrop = count;  /* new item is the previous count */
      }

      /* Selects the drop item */
      XmListSelectPos(ih->handle, idDrop+1, FALSE);    /* starts at 1 */
      iupAttribSetInt(ih, "_IUPLIST_OLDVALUE", idDrop+1);  /* starts at 1 */

      /* Remove the Drag item if moving */
      if (!is_ctrl)
        iupdrvListRemoveItem(ih, idDrag);   /* starts at 0 */
    }
  }

  iupAttribSet(ih, "_IUPLIST_DROPITEM", NULL);

  (void)drop_context;
  (void)seltype;
  (void)format;
  (void)length;
}

static void motListDropProc(Widget w, XtPointer client_data, XmDropProcCallbackStruct* drop_data)
{
  Atom atomListItem = XInternAtom(iupmot_display, "LIST_ITEM", False);
  XmDropTransferEntryRec transferList[2];
  Arg args[10];
  int i, num_args = 0;
  Widget drop_context;
  Cardinal numExportTargets;
  Atom *exportTargets;
  Boolean found = False;
  Ihandle* ih = NULL;
  (void)client_data;

  drop_context = drop_data->dragContext;

  /* retrieve the data targets */
  XtVaGetValues(drop_context, XmNexportTargets, &exportTargets,
    XmNnumExportTargets, &numExportTargets, 
    NULL);

  for (i = 0; i < (int)numExportTargets; i++) 
  {
    if (exportTargets[i] == atomListItem) 
    {
      found = True;
      break;
    }
  }

  XtVaGetValues(w, XmNuserData, &ih, NULL);
  if(!ih->handle)
    found = False;

  num_args = 0;
  if ((!found) || (drop_data->dropAction != XmDROP) ||  (drop_data->operation != XmDROP_COPY && drop_data->operation != XmDROP_MOVE)) 
  {
    iupMOT_SETARG(args, num_args, XmNtransferStatus, XmTRANSFER_FAILURE);
    iupMOT_SETARG(args, num_args, XmNnumDropTransfers, 0);
  }
  else 
  {
    transferList[0].target = atomListItem;
    transferList[0].client_data = (XtPointer)ih;
    iupMOT_SETARG(args, num_args, XmNdropTransfers, transferList);
    iupMOT_SETARG(args, num_args, XmNnumDropTransfers, 1);
    iupMOT_SETARG(args, num_args, XmNtransferProc, motListDragTransferProc);
  }

  XmDropTransferStart(drop_context, args, num_args);
}

static void motListDragDropFinishCallback(Widget drop_context, XtPointer client_data, XtPointer call_data)
{
  Widget source_icon = NULL;
  XtVaGetValues(drop_context, XmNsourceCursorIcon, &source_icon, NULL);
  if (source_icon)
    XtDestroyWidget(source_icon);
  (void)call_data;
  (void)client_data;
}

static void motListDragMotionCallback(Widget drop_context, Ihandle *ih, XmDragMotionCallbackStruct* drag_motion)
{
  (void)drop_context;
  if (!iupAttribGet(ih, "NODRAGFEEDBACK"))
  {
    int idDrop;
    int x = drag_motion->x;
    int y = drag_motion->y;
    iupdrvScreenToClient(ih, &x, &y);
    
    idDrop = motListConvertXYToPos(ih, x, y);  /* starts at 1 */
    iupAttribSetInt(ih, "_IUPLIST_DROPITEM", idDrop);  /* starts at 1 */

    XmListSelectPos(ih->handle, idDrop, FALSE);  /* starts at 1 */
  }
}

static Boolean motListConvertProc(Widget drop_context, Atom *selection, Atom *target, Atom *type_return,
                                  XtPointer *value_return, unsigned long *length_return, int *format_return)
{
  Atom atomMotifDrop = XInternAtom(iupmot_display, "_MOTIF_DROP", False);
  Atom atomTreeItem = XInternAtom(iupmot_display, "LIST_ITEM", False);
  int idDrag = 0;

  /* check if we are dealing with a drop */
  if (*selection != atomMotifDrop || *target != atomTreeItem)
    return False;

  XtVaGetValues(drop_context, XmNclientData, &idDrag, NULL);  /* starts at 1 */
  if (!idDrag)
    return False;

  /* format the value for transfer */
  *type_return = atomTreeItem;
  *value_return = (XtPointer)idDrag;  /* starts at 1 */
  *length_return = 1;
  *format_return = 32;
  return True;
}

static void motListDragStart(Widget w, XButtonEvent* evt, String* params, Cardinal* num_params)
{
  Atom atomListItem = XInternAtom(iupmot_display, "LIST_ITEM", False);
  Atom exportList[1];
  Widget drop_context;
  int idDrag, num_args = 0;
  Arg args[40];
  Ihandle *ih = NULL;

  XtVaGetValues(w, XmNuserData, &ih, NULL);
  if(!ih->handle)
    return;

  exportList[0] = atomListItem;
  idDrag = motListConvertXYToPos(ih, evt->x, evt->y);  /* starts at 1 */

  /* specify resources for DragContext for the transfer */
  num_args = 0;
  iupMOT_SETARG(args, num_args, XmNexportTargets, exportList);
  iupMOT_SETARG(args, num_args, XmNnumExportTargets, 1);
  iupMOT_SETARG(args, num_args, XmNdragOperations, XmDROP_MOVE|XmDROP_COPY);
  iupMOT_SETARG(args, num_args, XmNconvertProc, motListConvertProc);
  iupMOT_SETARG(args, num_args, XmNclientData, idDrag);

  /* start the drag and register a callback to clean up when done */
  drop_context = XmDragStart(w, (XEvent*)evt, args, num_args);

  XtAddCallback(drop_context, XmNdragDropFinishCallback, (XtCallbackProc)motListDragDropFinishCallback, NULL);
  XtAddCallback(drop_context, XmNdragMotionCallback, (XtCallbackProc)motListDragMotionCallback, (XtPointer)ih);

  iupAttribSet(ih, "_IUPLIST_DROPITEM", NULL);

  (void)params;
  (void)num_params;
}

static void motListEnableDragDrop(Widget w)
{
  Atom atomListItem = XInternAtom(iupmot_display, "LIST_ITEM", False);
  Atom importList[1];
  Arg args[40];
  int num_args = 0;
  char dragTranslations[] = "#override <Btn2Down>: iupListStartDrag()";
  static int do_rec = 0;
  if (!do_rec)
  {
    XtActionsRec rec = {"iupListStartDrag", (XtActionProc)motListDragStart};
    XtAppAddActions(iupmot_appcontext, &rec, 1);
    do_rec = 1;
  }
  XtOverrideTranslations(w, XtParseTranslationTable(dragTranslations));

  importList[0] = atomListItem;
  iupMOT_SETARG(args, num_args, XmNimportTargets, importList);
  iupMOT_SETARG(args, num_args, XmNnumImportTargets, 1);
  iupMOT_SETARG(args, num_args, XmNdropSiteOperations, XmDROP_MOVE|XmDROP_COPY);
  iupMOT_SETARG(args, num_args, XmNdropProc, motListDropProc);
  XmDropSiteRegister(w, args, num_args);
}

/*********************************************************************************/


static void motListEditModifyVerifyCallback(Widget cbedit, Ihandle *ih, XmTextVerifyPtr text)
{
  int start, end, remove_dir = 0, ret;
  char *insert_value;
  KeySym motcode = 0;
  IFnis cb;

  if (iupAttribGet(ih, "_IUPMOT_DISABLE_TEXT_CB"))
    return;

  if (text->event && text->event->type == KeyPress)
  {
    unsigned int state = ((XKeyEvent*)text->event)->state;
    if (state & ControlMask ||  /* Ctrl */
        state & Mod1Mask || state & Mod5Mask ||  /* Alt */
        state & Mod4Mask) /* Apple/Win */
    {
      text->doit = False;     /* abort processing */
      return;
    }

    motcode = iupmotKeycodeToKeysym((XKeyEvent*)text->event);
  }

  cb = (IFnis)IupGetCallback(ih, "EDIT_CB");
  if (!cb && !ih->data->mask)
    return;

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

  (void)cbedit;
}

static void motListEditMotionVerifyCallback(Widget w, Ihandle* ih, XmTextVerifyCallbackStruct* textverify)
{
  int pos;

  IFniii cb = (IFniii)IupGetCallback(ih, "CARET_CB");
  if (!cb) return;

  pos = textverify->newInsert;

  if (pos != ih->data->last_caret_pos)
  {
    ih->data->last_caret_pos = pos;
    cb(ih, 1, pos+1, pos);
  }

  (void)w;
}

static void motListEditKeyPressEvent(Widget cbedit, Ihandle *ih, XKeyEvent *evt, Boolean *cont)
{
  *cont = True;
  iupmotKeyPressEvent(cbedit, ih, (XEvent*)evt, cont);
  if (*cont == False)
    return;

  if (evt->state & ControlMask)   /* Ctrl */
  {
    KeySym motcode = iupmotKeycodeToKeysym(evt);
    if (motcode == XK_c)
    {
      motListSetClipboardAttrib(ih, "COPY");
      *cont = False;
      return;
    }
    else if (motcode == XK_x)
    {
      motListSetClipboardAttrib(ih, "CUT");
      *cont = False;
      return;
    }
    else if (motcode == XK_v)
    {
      motListSetClipboardAttrib(ih, "PASTE");
      *cont = False;
      return;
    }
    else if (motcode == XK_a)
    {
      XmTextFieldSetSelection(cbedit, 0, XmTextFieldGetLastPosition(cbedit), CurrentTime);
      *cont = False;
      return;
    }
  }
}

static void motListEditValueChangedCallback(Widget w, Ihandle* ih, XmAnyCallbackStruct* valuechanged)
{
  if (iupAttribGet(ih, "_IUPMOT_DISABLE_TEXT_CB"))
    return;

  iupBaseCallValueChangedCb(ih);

  (void)valuechanged;
  (void)w;
}

static void motListDropDownPopupCallback(Widget w, Ihandle* ih, XtPointer call_data)
{
  IFni cb = (IFni)IupGetCallback(ih, "DROPDOWN_CB");
  if (cb)
    cb(ih, 1);
  (void)w;
  (void)call_data;
}

static void motListDropDownPopdownCallback(Widget w, Ihandle* ih, XtPointer call_data)
{
  IFni cb = (IFni)IupGetCallback(ih, "DROPDOWN_CB");
  if (cb)
    cb(ih, 0);
  (void)w;
  (void)call_data;
}

static void motListDefaultActionCallback(Widget w, Ihandle* ih, XmListCallbackStruct* call_data)
{         
  if (call_data->event->type == ButtonPress || call_data->event->type == ButtonRelease)
  {
    IFnis cb = (IFnis) IupGetCallback(ih, "DBLCLICK_CB");
    if (cb)
    {
      int pos = call_data->item_position;  /* Here Motif already starts at 1 */
      iupListSingleCallDblClickCb(ih, cb, pos);
    }
  }

  (void)w;
}

static void motListComboBoxSelectionCallback(Widget w, Ihandle* ih, XmComboBoxCallbackStruct* call_data)
{         
  IFnsii cb = (IFnsii) IupGetCallback(ih, "ACTION");
  if (cb)
  {
    int pos = call_data->item_position;
    if (pos==0)
    {
      /* must check if it is really checked or it is for the edit box */
      XmString* items;
      XtVaGetValues(ih->handle, XmNitems, &items, NULL);
      if (!XmStringCompare(call_data->item_or_text, items[0]))
       return;
    }
    pos++;  /* IUP starts at 1 */
    iupListSingleCallActionCb(ih, cb, pos);
  }

  if (!ih->data->has_editbox)
    iupBaseCallValueChangedCb(ih);

  (void)w;
}

static void motListBrowseSelectionCallback(Widget w, Ihandle* ih, XmListCallbackStruct* call_data)
{         
  IFnsii cb = (IFnsii) IupGetCallback(ih, "ACTION");
  if (cb)
  {
    int pos = call_data->item_position;  /* Here Motif already starts at 1 */
    iupListSingleCallActionCb(ih, cb, pos);
  }

  if (!ih->data->has_editbox)
    iupBaseCallValueChangedCb(ih);

  (void)w;
}

static void motListExtendedSelectionCallback(Widget w, Ihandle* ih, XmListCallbackStruct* call_data)
{         
  IFns multi_cb = (IFns)IupGetCallback(ih, "MULTISELECT_CB");
  IFnsii cb = (IFnsii) IupGetCallback(ih, "ACTION");
  if (multi_cb || cb)
  {
    int* pos = call_data->selected_item_positions;
    int sel_count = call_data->selected_item_count;
    int i;

    /* In Motif, the position of item is "plus one".
       "iupListMultipleCallActionCb" works with the list of selected items from the zero position.
       So, "minus one" here. */
    for (i = 0; i < sel_count; i++)
      pos[i] -= 1;

    iupListMultipleCallActionCb(ih, cb, multi_cb, pos, sel_count);
  }

  if (!ih->data->has_editbox)
    iupBaseCallValueChangedCb(ih);

  (void)w;
}


/*********************************************************************************/


static int motListMapMethod(Ihandle* ih)
{
  int num_args = 0;
  Arg args[30];
  Widget parent = iupChildTreeGetNativeParentHandle(ih);
  char* child_id = iupDialogGetChildIdStr(ih);

  if (ih->data->is_dropdown || ih->data->has_editbox)
  {
    /* could not set XmNmappedWhenManaged to False because the list and the edit box where not displayed */
    /* iupMOT_SETARG(args, num_args, XmNmappedWhenManaged, False); */
    iupMOT_SETARG(args, num_args, XmNx, 0);  /* x-position */
    iupMOT_SETARG(args, num_args, XmNy, 0);  /* y-position */
    iupMOT_SETARG(args, num_args, XmNwidth, 10);  /* default width to avoid 0 */
    iupMOT_SETARG(args, num_args, XmNheight, 10); /* default height to avoid 0 */
    iupMOT_SETARG(args, num_args, XmNmarginHeight, 0);
    iupMOT_SETARG(args, num_args, XmNmarginWidth, 0);

    if (iupAttribGetBoolean(ih, "CANFOCUS"))
      iupMOT_SETARG(args, num_args, XmNtraversalOn, True);
    else
      iupMOT_SETARG(args, num_args, XmNtraversalOn, False);

    iupMOT_SETARG(args, num_args, XmNnavigationType, XmTAB_GROUP);
    iupMOT_SETARG(args, num_args, XmNhighlightThickness, 2);
    iupMOT_SETARG(args, num_args, XmNshadowThickness, 2);

    if (ih->data->has_editbox)
    {
      if (ih->data->is_dropdown)
        iupMOT_SETARG(args, num_args, XmNcomboBoxType, XmDROP_DOWN_COMBO_BOX);  /* hidden-list+edit */
      else
        iupMOT_SETARG(args, num_args, XmNcomboBoxType, XmCOMBO_BOX);  /* visible-list+edit */
    }
    else
      iupMOT_SETARG(args, num_args, XmNcomboBoxType, XmDROP_DOWN_LIST);   /* hidden-list */

    /* XmComboBoxWidget inherits from XmManager, 
       so it is a container with the actual list inside */

    ih->handle = XtCreateManagedWidget(
      child_id,  /* child identifier */
      xmComboBoxWidgetClass, /* widget class */
      parent,                      /* widget parent */
      args, num_args);
  }
  else
  {
    Widget sb_win;

    /* Create the scrolled window */

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
    child_id = "list";

    /* Create the list */

    num_args = 0;
    iupMOT_SETARG(args, num_args, XmNx, 0);  /* x-position */
    iupMOT_SETARG(args, num_args, XmNy, 0);  /* y-position */
    iupMOT_SETARG(args, num_args, XmNwidth, 10);  /* default width to avoid 0 */
    iupMOT_SETARG(args, num_args, XmNheight, 10); /* default height to avoid 0 */

    if (iupAttribGetBoolean(ih, "CANFOCUS"))
      iupMOT_SETARG(args, num_args, XmNtraversalOn, True);
    else
      iupMOT_SETARG(args, num_args, XmNtraversalOn, False);

    iupMOT_SETARG(args, num_args, XmNnavigationType, XmTAB_GROUP);
    iupMOT_SETARG(args, num_args, XmNhighlightThickness, 2);
    iupMOT_SETARG(args, num_args, XmNshadowThickness, 2);

    iupMOT_SETARG(args, num_args, XmNlistMarginHeight, 0);  /* default padding */
    iupMOT_SETARG(args, num_args, XmNlistMarginWidth, 0);
    iupMOT_SETARG(args, num_args, XmNlistSpacing, 0);
    iupMOT_SETARG(args, num_args, XmNlistSizePolicy, XmCONSTANT);  /* don't grow to fit, add scrollbar */

    if (ih->data->is_multiple)
      iupMOT_SETARG(args, num_args, XmNselectionPolicy, XmEXTENDED_SELECT);
    else
      iupMOT_SETARG(args, num_args, XmNselectionPolicy, XmBROWSE_SELECT);

    if (iupAttribGetBoolean(ih, "AUTOHIDE"))
      iupMOT_SETARG(args, num_args, XmNscrollBarDisplayPolicy, XmAS_NEEDED);
    else
      iupMOT_SETARG(args, num_args, XmNscrollBarDisplayPolicy, XmSTATIC);

    ih->handle = XtCreateManagedWidget(
      child_id,          /* child identifier */
      xmListWidgetClass, /* widget class */
      parent,            /* widget parent */
      args, num_args);
  }

  if (!ih->handle)
    return IUP_ERROR;

  ih->serial = iupDialogGetChildId(ih); /* must be after using the string */

  if (ih->data->is_dropdown || ih->data->has_editbox)
  {
    Widget cbedit, cblist;
    XtVaGetValues(ih->handle, XmNtextField, &cbedit, NULL);
    XtVaGetValues(ih->handle, XmNlist, &cblist, NULL);

    XtAddEventHandler(cbedit, FocusChangeMask, False, (XtEventHandler)iupmotFocusChangeEvent, (XtPointer)ih);
    XtAddEventHandler(cbedit, EnterWindowMask, False, (XtEventHandler)iupmotEnterLeaveWindowEvent, (XtPointer)ih);
    XtAddEventHandler(cbedit, LeaveWindowMask, False, (XtEventHandler)iupmotEnterLeaveWindowEvent, (XtPointer)ih);
    XtAddCallback(cbedit, XmNhelpCallback, (XtCallbackProc)iupmotHelpCallback, (XtPointer)ih);

    XtAddCallback(ih->handle, XmNselectionCallback, (XtCallbackProc)motListComboBoxSelectionCallback, (XtPointer)ih);

    if (iupStrBoolean(IupGetGlobal("INPUTCALLBACKS")))
      XtAddEventHandler(cbedit, PointerMotionMask, False, (XtEventHandler)iupmotDummyPointerMotionEvent, NULL);

    if (ih->data->has_editbox)
    {
      XtAddEventHandler(cbedit, KeyPressMask, False, (XtEventHandler)motListEditKeyPressEvent, (XtPointer)ih);
      XtAddCallback(cbedit, XmNmodifyVerifyCallback, (XtCallbackProc)motListEditModifyVerifyCallback, (XtPointer)ih);
      XtAddCallback(cbedit, XmNmotionVerifyCallback, (XtCallbackProc)motListEditMotionVerifyCallback, (XtPointer)ih);
      XtAddCallback(cbedit, XmNvalueChangedCallback, (XtCallbackProc)motListEditValueChangedCallback, (XtPointer)ih);

      iupAttribSet(ih, "_IUPMOT_DND_WIDGET", (char*)cbedit);
    }
    else
      XtAddEventHandler(cbedit, KeyPressMask, False, (XtEventHandler)iupmotKeyPressEvent, (XtPointer)ih);

    if (ih->data->is_dropdown)
    {
      XtVaSetValues(ih->handle, XmNvisibleItemCount, 5, NULL);
      XtAddCallback(XtParent(XtParent(cblist)), XmNpopupCallback, (XtCallbackProc)motListDropDownPopupCallback, (XtPointer)ih);
      XtAddCallback(XtParent(XtParent(cblist)), XmNpopdownCallback, (XtCallbackProc)motListDropDownPopdownCallback, (XtPointer)ih);
    }
    else
    {
      XtAddCallback(cblist, XmNdefaultActionCallback, (XtCallbackProc)motListDefaultActionCallback, (XtPointer)ih);
      XtAddEventHandler(cblist, PointerMotionMask, False, (XtEventHandler)iupmotPointerMotionEvent, (XtPointer)ih);
      XtAddEventHandler(cblist, ButtonPressMask|ButtonReleaseMask, False, (XtEventHandler)iupmotButtonPressReleaseEvent, (XtPointer)ih);

      /* Disable Drag Source */
      iupmotDisableDragSource(cblist);
    }
  }
  else
  {
    iupAttribSet(ih, "_IUP_EXTRAPARENT", (char*)parent);
    XtVaSetValues(parent, XmNworkWindow, ih->handle, NULL);

    XtAddEventHandler(ih->handle, FocusChangeMask, False, (XtEventHandler)iupmotFocusChangeEvent, (XtPointer)ih);
    XtAddEventHandler(ih->handle, EnterWindowMask, False, (XtEventHandler)iupmotEnterLeaveWindowEvent, (XtPointer)ih);
    XtAddEventHandler(ih->handle, LeaveWindowMask, False, (XtEventHandler)iupmotEnterLeaveWindowEvent, (XtPointer)ih);
    XtAddEventHandler(ih->handle, KeyPressMask, False, (XtEventHandler)iupmotKeyPressEvent, (XtPointer)ih);
    XtAddEventHandler(ih->handle, PointerMotionMask, False, (XtEventHandler)iupmotPointerMotionEvent, (XtPointer)ih);
    XtAddEventHandler(ih->handle, ButtonPressMask|ButtonReleaseMask, False, (XtEventHandler)iupmotButtonPressReleaseEvent, (XtPointer)ih);

    XtAddCallback(ih->handle, XmNhelpCallback, (XtCallbackProc)iupmotHelpCallback, (XtPointer)ih);
    XtAddCallback (ih->handle, XmNbrowseSelectionCallback, (XtCallbackProc)motListBrowseSelectionCallback, (XtPointer)ih);
    XtAddCallback (ih->handle, XmNextendedSelectionCallback, (XtCallbackProc)motListExtendedSelectionCallback, (XtPointer)ih);
    XtAddCallback (ih->handle, XmNdefaultActionCallback, (XtCallbackProc)motListDefaultActionCallback, (XtPointer)ih);
  }

  /* initialize the widget */
  if (ih->data->is_dropdown || ih->data->has_editbox)
    XtRealizeWidget(ih->handle);
  else
    XtRealizeWidget(parent);

  /* Enable internal drag and drop support */
  if((ih->data->show_dragdrop && !ih->data->is_dropdown && !ih->data->is_multiple) ||
    (IupGetInt(ih, "DRAGDROPLIST")))  /* Enable drag and drop support between lists */
  {
    motListEnableDragDrop(ih->handle);
    XtVaSetValues(ih->handle, XmNuserData, ih, NULL);  /* to be used in motListDragStart and motListDragTransferProc */
  }
  else
    iupmotDisableDragSource(ih->handle);  /* Disable Drag Source */

  if (IupGetGlobal("_IUP_RESET_TXTCOLORS"))
  {
    iupmotSetGlobalColorAttrib(ih->handle, XmNbackground, "TXTBGCOLOR");
    iupmotSetGlobalColorAttrib(ih->handle, XmNforeground, "TXTFGCOLOR");
    iupmotSetGlobalColorAttrib(ih->handle, XmNhighlightColor, "TXTHLCOLOR");
    IupSetGlobal("_IUP_RESET_TXTCOLORS", NULL);
  }

  IupSetCallback(ih, "_IUP_XY2POS_CB", (Icallback)motListConvertXYToPos);

  iupListSetInitialItems(ih);

  return IUP_NOERROR;
}

void iupdrvListInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = motListMapMethod;

  /* Driver Dependent Attribute functions */

  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, motListSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTBGCOLOR", IUPAF_DEFAULT);

  /* Special */
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, motListSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTFGCOLOR", IUPAF_DEFAULT);

  /* IupList only */
  iupClassRegisterAttributeId(ic, "IDVALUE", motListGetIdValueAttrib, iupListSetIdValueAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUE", motListGetValueAttrib, motListSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VISIBLEITEMS", NULL, motListSetVisibleItemsAttrib, IUPAF_SAMEASSYSTEM, "5", IUPAF_DEFAULT);
  /*OLD*/iupClassRegisterAttribute(ic, "VISIBLE_ITEMS", NULL, motListSetVisibleItemsAttrib, IUPAF_SAMEASSYSTEM, "5", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "TOPITEM", NULL, motListSetTopItemAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWDROPDOWN", NULL, motListSetShowDropdownAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPACING", iupListGetSpacingAttrib, motListSetSpacingAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED);

  iupClassRegisterAttribute(ic, "PADDING", iupListGetPaddingAttrib, motListSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "SELECTEDTEXT", motListGetSelectedTextAttrib, motListSetSelectedTextAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTION", motListGetSelectionAttrib, motListSetSelectionAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTIONPOS", motListGetSelectionPosAttrib, motListSetSelectionPosAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARET", motListGetCaretAttrib, motListSetCaretAttrib, NULL, NULL, IUPAF_NO_SAVE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARETPOS", motListGetCaretPosAttrib, motListSetCaretPosAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_SAVE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERT", NULL, motListSetInsertAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "APPEND", NULL, motListSetAppendAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "READONLY", motListGetReadOnlyAttrib, motListSetReadOnlyAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "NC", iupListGetNCAttrib, motListSetNCAttrib, NULL, NULL, IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "CLIPBOARD", NULL, motListSetClipboardAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTO", NULL, motListSetScrollToAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTOPOS", NULL, motListSetScrollToPosAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

  /* Not Supported */
  iupClassRegisterAttributeId(ic, "IMAGE", NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWIMAGE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DROPEXPAND", NULL, NULL, IUPAF_SAMEASSYSTEM, "Yes", IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AUTOREDRAW", NULL, NULL, IUPAF_SAMEASSYSTEM, "Yes", IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
}
