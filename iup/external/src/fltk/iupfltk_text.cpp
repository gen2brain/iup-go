/** \file
 * \brief Text Control - FLTK Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Secret_Input.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Spinner.H>
#include <FL/fl_draw.H>

#include <cstdlib>
#include <cstdio>
#include <cstring>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_layout.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"
#include "iup_mask.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_key.h"
#include "iup_array.h"
#include "iup_text.h"
}

#include "iupfltk_drv.h"


IUP_DRV_API int iupfltkEditCheckMask(Ihandle* ih, Fl_Input_* input, int event, const char* cb_name, void* mask, int nc)
{
  if (event != FL_KEYBOARD)
    return 0;

  const char* text = Fl::event_text();
  if (!text || !text[0] || text[0] < 32)
    return 0;

  IFnis cb = (IFnis)IupGetCallback(ih, cb_name);
  if (!cb && !mask && nc == 0)
    return 0;

  int pos = input->insert_position();
  int ret = iupEditCallActionCb(ih, cb, text, pos, pos, mask, nc, 0, 1);

  if (ret == 0)
    return 1;

  if (ret != -1)
  {
    char replacement[2] = { (char)ret, '\0' };
    input->replace(pos, pos, replacement);
    return 1;
  }

  return 0;
}


class IupFltkInput : public Fl_Input
{
public:
  Ihandle* iup_handle;

  IupFltkInput(int x, int y, int w, int h, Ihandle* ih)
    : Fl_Input(x, y, w, h), iup_handle(ih) {}

protected:
  int handle(int event) override
  {
    if (event == FL_PASTE && iupfltkHandleDropFiles(iup_handle))
      return 1;

    switch (event)
    {
      case FL_FOCUS:
      case FL_UNFOCUS:
        iupfltkFocusInOutEvent(this, iup_handle, event);
        break;
      case FL_ENTER:
      case FL_LEAVE:
        iupfltkEnterLeaveEvent(this, iup_handle, event);
        break;
      case FL_KEYBOARD:
        if (iupfltkKeyPressEvent(this, iup_handle))
          return 1;
        if (iupfltkEditCheckMask(iup_handle, this, event, "ACTION", iup_handle->data->mask, iup_handle->data->nc))
          return 1;
        break;
    }
    return Fl_Input::handle(event);
  }
};

class IupFltkSecretInput : public Fl_Secret_Input
{
public:
  Ihandle* iup_handle;

  IupFltkSecretInput(int x, int y, int w, int h, Ihandle* ih)
    : Fl_Secret_Input(x, y, w, h), iup_handle(ih) {}

protected:
  int handle(int event) override
  {
    if (event == FL_PASTE && iupfltkHandleDropFiles(iup_handle))
      return 1;

    switch (event)
    {
      case FL_FOCUS:
      case FL_UNFOCUS:
        iupfltkFocusInOutEvent(this, iup_handle, event);
        break;
      case FL_ENTER:
      case FL_LEAVE:
        iupfltkEnterLeaveEvent(this, iup_handle, event);
        break;
      case FL_KEYBOARD:
        if (iupfltkKeyPressEvent(this, iup_handle))
          return 1;
        if (iupfltkEditCheckMask(iup_handle, this, event, "ACTION", iup_handle->data->mask, iup_handle->data->nc))
          return 1;
        break;
    }
    return Fl_Secret_Input::handle(event);
  }
};

class IupFltkTextEditor : public Fl_Text_Editor
{
public:
  Ihandle* iup_handle;
  Fl_Text_Buffer* text_buffer;
  int is_readonly;

  IupFltkTextEditor(int x, int y, int w, int h, Ihandle* ih)
    : Fl_Text_Editor(x, y, w, h), iup_handle(ih), is_readonly(0)
  {
    text_buffer = new Fl_Text_Buffer();
    buffer(text_buffer);
  }

  ~IupFltkTextEditor() override
  {
    buffer(NULL);
    delete text_buffer;
  }

protected:
  int handle(int event) override
  {
    if (event == FL_PASTE && iupfltkHandleDropFiles(iup_handle))
      return 1;

    switch (event)
    {
      case FL_FOCUS:
      case FL_UNFOCUS:
        iupfltkFocusInOutEvent(this, iup_handle, event);
        break;
      case FL_ENTER:
      case FL_LEAVE:
        iupfltkEnterLeaveEvent(this, iup_handle, event);
        break;
      case FL_RELEASE:
        if (Fl::event_button() == FL_LEFT_MOUSE && iup_handle->data->has_formatting)
        {
          int pos = xy_to_position(Fl::event_x(), Fl::event_y());
          const char* url = iupfltkFormatGetLinkAtPos(iup_handle, pos);
          if (url)
          {
            IFns cb = (IFns)IupGetCallback(iup_handle, "LINK_CB");
            if (cb)
            {
              int ret = cb(iup_handle, (char*)url);
              if (ret == IUP_CLOSE)
                IupExitLoop();
              else if (ret == IUP_DEFAULT)
                IupHelp(url);
            }
            else
              IupHelp(url);
          }
        }
        break;
      case FL_KEYBOARD:
        if (iupfltkKeyPressEvent(this, iup_handle))
          return 1;
        if (is_readonly)
          return Fl_Text_Display::handle(event);
        {
          const char* text = Fl::event_text();
          if (text && text[0] && text[0] >= 32 && !iup_handle->data->disable_callbacks)
          {
            IFnis cb = (IFnis)IupGetCallback(iup_handle, "ACTION");
            if (cb || iup_handle->data->mask)
            {
              int pos = insert_position();
              int ret = iupEditCallActionCb(iup_handle, cb, text, pos, pos,
                          iup_handle->data->mask, iup_handle->data->nc, 0, 1);
              if (ret == 0)
                return 1;
              if (ret != -1)
              {
                iup_handle->data->disable_callbacks = 1;
                char replacement[2] = { (char)ret, '\0' };
                text_buffer->insert(pos, replacement);
                insert_position(pos + 1);
                iup_handle->data->disable_callbacks = 0;
                return 1;
              }
            }
          }
        }
        break;
    }
    int ret = Fl_Text_Editor::handle(event);

    if (event == FL_MOVE && iup_handle->data->has_formatting &&
        text_area.w > 0 && text_area.h > 0 && buffer()->length() > 0)
    {
      int pos = xy_to_position(Fl::event_x(), Fl::event_y());
      const char* url = iupfltkFormatGetLinkAtPos(iup_handle, pos);
      if (url)
        window()->cursor(FL_CURSOR_HAND);
    }

    return ret;
  }
};

static void fltkTextInputCallback(Fl_Widget* w, void* data)
{
  Ihandle* ih = (Ihandle*)data;
  IFn cb = (IFn)IupGetCallback(ih, "VALUECHANGED_CB");
  if (cb)
  {
    if (cb(ih) == IUP_CLOSE)
      IupExitLoop();
  }
}

static void fltkTextEditorModifyCallback(int pos, int nInserted, int nDeleted, int nRestyled, const char* deletedText, void* data)
{
  Ihandle* ih = (Ihandle*)data;
  (void)pos;
  (void)nRestyled;
  (void)deletedText;

  if (nInserted == 0 && nDeleted == 0)
    return;

  IFn cb = (IFn)IupGetCallback(ih, "VALUECHANGED_CB");
  if (cb)
  {
    if (cb(ih) == IUP_CLOSE)
      IupExitLoop();
  }
}

class IupFltkSpinner : public Fl_Spinner
{
public:
  Ihandle* iup_handle;
  char saved_text[256];
  int noauto;

  IupFltkSpinner(int x, int y, int w, int h, Ihandle* ih)
    : Fl_Spinner(x, y, w, h), iup_handle(ih), noauto(0)
  {
    saved_text[0] = '\0';
  }

protected:
  int handle(int event) override
  {
    if (noauto && (event == FL_PUSH || event == FL_KEYBOARD))
    {
      Fl_Input* inp = (Fl_Input*)child(0);
      if (inp && inp->value())
        iupStrCopyN(saved_text, sizeof(saved_text), inp->value());
    }
    int ret = Fl_Spinner::handle(event);
    if (noauto && (event == FL_PUSH || event == FL_KEYBOARD))
    {
      Fl_Input* inp = (Fl_Input*)child(0);
      if (inp)
        inp->value(saved_text);
    }
    return ret;
  }
};

static void fltkSpinCallback(Fl_Widget* w, void* data)
{
  Ihandle* ih = (Ihandle*)data;
  IupFltkSpinner* spinner = (IupFltkSpinner*)w;
  int pos = (int)spinner->value();

  IFni cb = (IFni)IupGetCallback(ih, "SPIN_CB");
  if (cb)
  {
    int ret = cb(ih, pos);
    if (ret == IUP_CLOSE)
      IupExitLoop();
  }

  IFn vcb = (IFn)IupGetCallback(ih, "VALUECHANGED_CB");
  if (vcb)
  {
    if (vcb(ih) == IUP_CLOSE)
      IupExitLoop();
  }
}

static Fl_Input* fltkTextGetInputWidget(Ihandle* ih)
{
  if (ih->data->is_multiline)
    return NULL;

  if (iupAttribGetBoolean(ih, "SPIN"))
  {
    Fl_Spinner* spinner = (Fl_Spinner*)ih->handle;
    return (Fl_Input*)spinner->child(0);
  }

  return (Fl_Input*)ih->handle;
}

static Fl_Text_Buffer* fltkTextGetBuffer(Ihandle* ih)
{
  if (ih->data->is_multiline)
  {
    IupFltkTextEditor* editor = (IupFltkTextEditor*)ih->handle;
    return editor->text_buffer;
  }
  return NULL;
}

static int fltkTextSetValueAttrib(Ihandle* ih, const char* value)
{
  if (!value) value = "";

  if (ih->data->is_multiline)
  {
    Fl_Text_Buffer* buf = fltkTextGetBuffer(ih);
    if (buf)
      buf->text(value);
  }
  else
  {
    Fl_Input* input = fltkTextGetInputWidget(ih);
    if (input)
    {
      input->value(value);
    }
  }

  return 0;
}

static char* fltkTextGetValueAttrib(Ihandle* ih)
{
  if (ih->data->is_multiline)
  {
    Fl_Text_Buffer* buf = fltkTextGetBuffer(ih);
    if (buf)
      return iupStrReturnStr(buf->text());
  }
  else
  {
    Fl_Input* input = fltkTextGetInputWidget(ih);
    if (input)
      return iupStrReturnStr(input->value());
  }
  return NULL;
}

static int fltkTextSetReadOnlyAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->is_multiline)
  {
    IupFltkTextEditor* editor = (IupFltkTextEditor*)ih->handle;
    if (editor)
      editor->is_readonly = iupStrBoolean(value);
  }
  else
  {
    Fl_Input* input = fltkTextGetInputWidget(ih);
    if (input)
      input->readonly(iupStrBoolean(value));
  }
  return 0;
}

static char* fltkTextGetReadOnlyAttrib(Ihandle* ih)
{
  if (ih->data->is_multiline)
  {
    IupFltkTextEditor* editor = (IupFltkTextEditor*)ih->handle;
    if (editor)
      return iupStrReturnBoolean(editor->is_readonly);
  }
  else
  {
    Fl_Input* input = fltkTextGetInputWidget(ih);
    if (input)
      return iupStrReturnBoolean(input->readonly());
  }
  return NULL;
}

static int fltkTextSetCaretAttrib(Ihandle* ih, const char* value)
{
  int lin = 1, col = 1;
  iupStrToIntInt(value, &lin, &col, ',');

  if (ih->data->is_multiline)
  {
    Fl_Text_Buffer* buf = fltkTextGetBuffer(ih);
    IupFltkTextEditor* editor = (IupFltkTextEditor*)ih->handle;
    if (buf && editor)
    {
      int pos = buf->skip_lines(0, lin - 1);
      pos += col - 1;
      editor->insert_position(pos);
    }
  }
  else
  {
    Fl_Input* input = fltkTextGetInputWidget(ih);
    if (input)
      input->insert_position(col - 1);
  }

  return 0;
}

static char* fltkTextGetCaretAttrib(Ihandle* ih)
{
  if (ih->data->is_multiline)
  {
    Fl_Text_Buffer* buf = fltkTextGetBuffer(ih);
    IupFltkTextEditor* editor = (IupFltkTextEditor*)ih->handle;
    if (buf && editor)
    {
      int pos = editor->insert_position();
      int lin = buf->count_lines(0, pos) + 1;
      int line_start = buf->line_start(pos);
      int col = pos - line_start + 1;
      return iupStrReturnIntInt(lin, col, ',');
    }
  }
  else
  {
    Fl_Input* input = fltkTextGetInputWidget(ih);
    if (input)
    {
      int pos = input->insert_position();
      return iupStrReturnIntInt(1, pos + 1, ',');
    }
  }
  return NULL;
}

static int fltkTextSetSelectionAttrib(Ihandle* ih, const char* value)
{
  if (!value || !ih->handle)
    return 0;

  if (ih->data->is_multiline)
  {
    Fl_Text_Buffer* buf = fltkTextGetBuffer(ih);
    IupFltkTextEditor* editor = (IupFltkTextEditor*)ih->handle;
    if (!buf || !editor)
      return 0;

    if (iupStrEqualNoCase(value, "NONE"))
    {
      buf->unselect();
      return 0;
    }

    if (iupStrEqualNoCase(value, "ALL"))
    {
      buf->select(0, buf->length());
      return 0;
    }

    int lin_start = 1, col_start = 1, lin_end = 1, col_end = 1;
    if (sscanf(value, "%d,%d:%d,%d", &lin_start, &col_start, &lin_end, &col_end) != 4)
      return 0;

    if (lin_start < 1 || col_start < 1 || lin_end < 1 || col_end < 1)
      return 0;

    int start_pos = buf->skip_lines(0, lin_start - 1) + (col_start - 1);
    int end_pos = buf->skip_lines(0, lin_end - 1) + (col_end - 1);
    buf->select(start_pos, end_pos);
  }
  else
  {
    int start = 1, end = 1;

    if (iupStrEqualNoCase(value, "NONE"))
    {
      Fl_Input* input = fltkTextGetInputWidget(ih);
      if (input)
        input->insert_position(input->insert_position());
      return 0;
    }

    if (iupStrEqualNoCase(value, "ALL"))
    {
      Fl_Input* input = fltkTextGetInputWidget(ih);
      if (input)
        input->insert_position(0, (int)strlen(input->value()));
      return 0;
    }

    if (iupStrToIntInt(value, &start, &end, ':') != 2)
      return 0;

    if (start < 1 || end < 1)
      return 0;

    start--;
    end--;

    Fl_Input* input = fltkTextGetInputWidget(ih);
    if (input)
      input->insert_position(start, end);
  }

  return 0;
}

static char* fltkTextGetSelectionAttrib(Ihandle* ih)
{
  if (ih->data->is_multiline)
  {
    Fl_Text_Buffer* buf = fltkTextGetBuffer(ih);
    if (buf)
    {
      int start, end;
      if (buf->selection_position(&start, &end))
      {
        int start_lin = buf->count_lines(0, start) + 1;
        int start_line_start = buf->line_start(start);
        int start_col = start - start_line_start + 1;

        int end_lin = buf->count_lines(0, end) + 1;
        int end_line_start = buf->line_start(end);
        int end_col = end - end_line_start + 1;

        return iupStrReturnStrf("%d,%d:%d,%d", start_lin, start_col, end_lin, end_col);
      }
    }
  }
  else
  {
    Fl_Input* input = fltkTextGetInputWidget(ih);
    if (input)
    {
      int mark = input->mark();
      int pos = input->insert_position();
      if (mark != pos)
      {
        int start = mark < pos ? mark : pos;
        int end = mark > pos ? mark : pos;
        start++;
        end++;
        return iupStrReturnIntInt(start, end, ':');
      }
    }
  }
  return NULL;
}

static char* fltkTextGetSelectedTextAttrib(Ihandle* ih)
{
  if (ih->data->is_multiline)
  {
    Fl_Text_Buffer* buf = fltkTextGetBuffer(ih);
    IupFltkTextEditor* editor = (IupFltkTextEditor*)ih->handle;
    if (buf && editor)
    {
      int start, end;
      if (buf->selection_position(&start, &end))
      {
        char* text = buf->selection_text();
        char* ret = iupStrReturnStr(text);
        free(text);
        return ret;
      }
    }
  }
  else
  {
    Fl_Input* input = fltkTextGetInputWidget(ih);
    if (input)
    {
      int mark = input->mark();
      int pos = input->insert_position();
      if (mark != pos)
      {
        int start = mark < pos ? mark : pos;
        int end = mark > pos ? mark : pos;
        const char* val = input->value();
        int len = end - start;
        char* text = (char*)malloc(len + 1);
        memcpy(text, val + start, len);
        text[len] = 0;
        char* ret = iupStrReturnStr(text);
        free(text);
        return ret;
      }
    }
  }
  return NULL;
}

static int fltkTextSetSelectedTextAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    return 0;

  ih->data->disable_callbacks = 1;

  if (ih->data->is_multiline)
  {
    Fl_Text_Buffer* buf = fltkTextGetBuffer(ih);
    IupFltkTextEditor* editor = (IupFltkTextEditor*)ih->handle;
    if (buf && editor)
    {
      int start, end;
      if (buf->selection_position(&start, &end))
      {
        buf->replace(start, end, value);
        buf->select(start, start + (int)strlen(value));
      }
      else
      {
        int pos = editor->insert_position();
        buf->insert(pos, value);
      }
    }
  }
  else
  {
    Fl_Input* input = fltkTextGetInputWidget(ih);
    if (input)
    {
      int mark = input->mark();
      int pos = input->insert_position();
      if (mark != pos)
      {
        int start = mark < pos ? mark : pos;
        int end = mark > pos ? mark : pos;
        input->replace(start, end, value);
      }
      else
        input->insert(value);
    }
  }

  ih->data->disable_callbacks = 0;
  return 0;
}

static int fltkTextSetNCAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    ih->data->nc = 0;
  else
    iupStrToInt(value, &ih->data->nc);

  if (!ih->data->is_multiline)
  {
    Fl_Input* input = fltkTextGetInputWidget(ih);
    if (input && ih->data->nc > 0)
      input->maximum_size(ih->data->nc);
  }

  return 0;
}

static int fltkTextSetAppendAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle)
    return 0;

  if (!value) value = "";

  ih->data->disable_callbacks = 1;

  if (ih->data->is_multiline)
  {
    IupFltkTextEditor* editor = (IupFltkTextEditor*)ih->handle;
    Fl_Text_Buffer* buf = editor ? editor->text_buffer : NULL;
    if (buf)
    {
      int len = buf->length();
      if (ih->data->append_newline && len > 0)
        buf->append("\n");
      buf->append(value);

      int end = buf->length();
      editor->insert_position(end);
      editor->show_insert_position();
    }
  }
  else
  {
    Fl_Input* input = fltkTextGetInputWidget(ih);
    if (input)
    {
      const char* old_val = input->value();
      int old_len = (int)strlen(old_val);
      int new_len = (int)strlen(value);
      char* new_val = (char*)malloc(old_len + new_len + 1);
      strcpy(new_val, old_val);
      strcat(new_val, value);
      input->value(new_val);
      free(new_val);
    }
  }

  ih->data->disable_callbacks = 0;
  return 0;
}

static int fltkTextSetInsertAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    return 0;

  if (ih->data->is_multiline)
  {
    IupFltkTextEditor* editor = (IupFltkTextEditor*)ih->handle;
    if (editor)
      editor->insert(value);
  }
  else
  {
    Fl_Input* input = fltkTextGetInputWidget(ih);
    if (input)
    {
      int pos = input->insert_position();
      const char* old_val = input->value();
      int old_len = (int)strlen(old_val);
      int ins_len = (int)strlen(value);
      char* new_val = (char*)malloc(old_len + ins_len + 1);
      memcpy(new_val, old_val, pos);
      memcpy(new_val + pos, value, ins_len);
      memcpy(new_val + pos + ins_len, old_val + pos, old_len - pos + 1);
      input->value(new_val);
      input->insert_position(pos + ins_len);
      free(new_val);
    }
  }
  return 0;
}

static int fltkTextSetAlignmentAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  (void)value;
  return 1;
}

static int fltkTextSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  Fl_Widget* widget = (Fl_Widget*)ih->handle;
  if (widget)
    widget->color(fl_rgb_color(r, g, b));

  return 1;
}

static int fltkTextSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  Fl_Color color = fl_rgb_color(r, g, b);

  if (ih->data->is_multiline)
  {
    IupFltkTextEditor* editor = (IupFltkTextEditor*)ih->handle;
    if (editor)
      editor->textcolor(color);
  }
  else
  {
    Fl_Input* input = fltkTextGetInputWidget(ih);
    if (input)
      input->textcolor(color);
  }

  return 1;
}

static int fltkTextSetFontAttrib(Ihandle* ih, const char* value)
{
  if (!iupdrvSetFontAttrib(ih, value))
    return 0;

  if (ih->handle)
  {
    int fl_font, fl_size;
    if (iupfltkGetFont(ih, &fl_font, &fl_size))
    {
      if (ih->data->is_multiline)
      {
        Fl_Text_Editor* editor = (Fl_Text_Editor*)ih->handle;
        editor->textfont((Fl_Font)fl_font);
        editor->textsize((Fl_Fontsize)fl_size);
        editor->redraw();
      }
      else if (!iupAttribGetBoolean(ih, "SPIN"))
      {
        Fl_Input* input = (Fl_Input*)ih->handle;
        input->textfont((Fl_Font)fl_font);
        input->textsize((Fl_Fontsize)fl_size);
        input->redraw();
      }
    }
  }

  return 1;
}

static int fltkTextSetSpinMinAttrib(Ihandle* ih, const char* value)
{
  if (iupAttribGetBoolean(ih, "SPIN"))
  {
    Fl_Spinner* spinner = (Fl_Spinner*)ih->handle;
    int min_val = 0;
    iupStrToInt(value, &min_val);
    spinner->minimum(min_val);
  }
  return 1;
}

static int fltkTextSetSpinMaxAttrib(Ihandle* ih, const char* value)
{
  if (iupAttribGetBoolean(ih, "SPIN"))
  {
    Fl_Spinner* spinner = (Fl_Spinner*)ih->handle;
    int max_val = 100;
    iupStrToInt(value, &max_val);
    spinner->maximum(max_val);
  }
  return 1;
}

static int fltkTextSetSpinValueAttrib(Ihandle* ih, const char* value)
{
  if (iupAttribGetBoolean(ih, "SPIN"))
  {
    IupFltkSpinner* spinner = (IupFltkSpinner*)ih->handle;
    int val = 0;
    iupStrToInt(value, &val);

    if (spinner->noauto)
    {
      Fl_Input* input = (Fl_Input*)spinner->child(0);
      const char* saved = input ? input->value() : NULL;
      char saved_buf[256] = "";
      if (saved)
        iupStrCopyN(saved_buf, sizeof(saved_buf), saved);

      spinner->value(val);

      if (input && saved_buf[0])
        input->value(saved_buf);
    }
    else
      spinner->value(val);
  }
  return 1;
}

static char* fltkTextGetSpinValueAttrib(Ihandle* ih)
{
  if (iupAttribGetBoolean(ih, "SPIN"))
  {
    Fl_Spinner* spinner = (Fl_Spinner*)ih->handle;
    return iupStrReturnInt((int)spinner->value());
  }
  return NULL;
}

static int fltkTextSetSpinIncAttrib(Ihandle* ih, const char* value)
{
  if (iupAttribGetBoolean(ih, "SPIN"))
  {
    Fl_Spinner* spinner = (Fl_Spinner*)ih->handle;
    int inc = 1;
    iupStrToInt(value, &inc);
    spinner->step(inc);
  }
  return 1;
}

static int fltkTextSetCaretPosAttrib(Ihandle* ih, const char* value)
{
  int pos = 0;

  if (!value)
    return 0;

  iupStrToInt(value, &pos);
  if (pos < 0) pos = 0;

  if (ih->data->is_multiline)
  {
    IupFltkTextEditor* editor = (IupFltkTextEditor*)ih->handle;
    if (editor)
      editor->insert_position(pos);
  }
  else
  {
    Fl_Input* input = fltkTextGetInputWidget(ih);
    if (input)
      input->insert_position(pos);
  }

  return 0;
}

static char* fltkTextGetCaretPosAttrib(Ihandle* ih)
{
  int pos;

  if (ih->data->is_multiline)
  {
    IupFltkTextEditor* editor = (IupFltkTextEditor*)ih->handle;
    if (editor)
      pos = editor->insert_position();
    else
      return NULL;
  }
  else
  {
    Fl_Input* input = fltkTextGetInputWidget(ih);
    if (input)
      pos = input->insert_position();
    else
      return NULL;
  }

  return iupStrReturnInt(pos);
}

static int fltkTextSetSelectionPosAttrib(Ihandle* ih, const char* value)
{
  int start = 0, end = 0;

  if (!value || iupStrEqualNoCase(value, "NONE"))
  {
    if (ih->data->is_multiline)
    {
      Fl_Text_Buffer* buf = fltkTextGetBuffer(ih);
      if (buf)
        buf->unselect();
    }
    else
    {
      Fl_Input* input = fltkTextGetInputWidget(ih);
      if (input)
        input->insert_position(input->insert_position());
    }
    return 0;
  }

  if (iupStrEqualNoCase(value, "ALL"))
  {
    if (ih->data->is_multiline)
    {
      Fl_Text_Buffer* buf = fltkTextGetBuffer(ih);
      if (buf)
        buf->select(0, buf->length());
    }
    else
    {
      Fl_Input* input = fltkTextGetInputWidget(ih);
      if (input)
        input->insert_position(0, (int)strlen(input->value()));
    }
    return 0;
  }

  if (iupStrToIntInt(value, &start, &end, ':') != 2)
    return 0;

  if (start < 0) start = 0;
  if (end < 0) end = 0;

  if (ih->data->is_multiline)
  {
    Fl_Text_Buffer* buf = fltkTextGetBuffer(ih);
    if (buf)
      buf->select(start, end);
  }
  else
  {
    Fl_Input* input = fltkTextGetInputWidget(ih);
    if (input)
      input->insert_position(start, end);
  }

  return 0;
}

static char* fltkTextGetSelectionPosAttrib(Ihandle* ih)
{
  if (ih->data->is_multiline)
  {
    Fl_Text_Buffer* buf = fltkTextGetBuffer(ih);
    if (buf)
    {
      int start, end;
      if (buf->selection_position(&start, &end))
        return iupStrReturnStrf("%d:%d", start, end);
    }
  }
  else
  {
    Fl_Input* input = fltkTextGetInputWidget(ih);
    if (input)
    {
      int mark = input->mark();
      int pos = input->insert_position();
      if (mark != pos)
      {
        int start = mark < pos ? mark : pos;
        int end = mark > pos ? mark : pos;
        return iupStrReturnStrf("%d:%d", start, end);
      }
    }
  }
  return NULL;
}

static char* fltkTextGetCountAttrib(Ihandle* ih)
{
  if (ih->data->is_multiline)
  {
    Fl_Text_Buffer* buf = fltkTextGetBuffer(ih);
    if (buf)
      return iupStrReturnInt(buf->length());
  }
  else
  {
    Fl_Input* input = fltkTextGetInputWidget(ih);
    if (input)
      return iupStrReturnInt((int)strlen(input->value()));
  }
  return NULL;
}

static char* fltkTextGetLineCountAttrib(Ihandle* ih)
{
  if (ih->data->is_multiline)
  {
    Fl_Text_Buffer* buf = fltkTextGetBuffer(ih);
    if (buf)
      return iupStrReturnInt(buf->count_lines(0, buf->length()) + 1);
  }
  return NULL;
}

static int fltkTextSetClipboardAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    return 0;

  if (iupStrEqualNoCase(value, "COPY"))
  {
    if (ih->data->is_multiline)
    {
      Fl_Text_Buffer* buf = fltkTextGetBuffer(ih);
      IupFltkTextEditor* editor = (IupFltkTextEditor*)ih->handle;
      if (buf && editor)
      {
        int start, end;
        if (buf->selection_position(&start, &end))
        {
          char* text = buf->selection_text();
          Fl::copy(text, (int)strlen(text), 1);
          free(text);
        }
      }
    }
    else
    {
      Fl_Input* input = fltkTextGetInputWidget(ih);
      if (input)
        input->copy(1);
    }
  }
  else if (iupStrEqualNoCase(value, "CUT"))
  {
    if (ih->data->is_multiline)
    {
      Fl_Text_Buffer* buf = fltkTextGetBuffer(ih);
      IupFltkTextEditor* editor = (IupFltkTextEditor*)ih->handle;
      if (buf && editor)
      {
        int start, end;
        if (buf->selection_position(&start, &end))
        {
          char* text = buf->selection_text();
          Fl::copy(text, (int)strlen(text), 1);
          free(text);
          buf->remove_selection();
        }
      }
    }
    else
    {
      Fl_Input* input = fltkTextGetInputWidget(ih);
      if (input)
        input->cut(1);
    }
  }
  else if (iupStrEqualNoCase(value, "PASTE"))
  {
    if (ih->data->is_multiline)
    {
      IupFltkTextEditor* editor = (IupFltkTextEditor*)ih->handle;
      if (editor)
        Fl::paste(*editor, 1);
    }
    else
    {
      Fl_Input* input = fltkTextGetInputWidget(ih);
      if (input)
        Fl::paste(*input, 1);
    }
  }
  else if (iupStrEqualNoCase(value, "CLEAR"))
  {
    if (ih->data->is_multiline)
    {
      Fl_Text_Buffer* buf = fltkTextGetBuffer(ih);
      if (buf)
      {
        int start, end;
        if (buf->selection_position(&start, &end))
          buf->remove_selection();
      }
    }
    else
    {
      Fl_Input* input = fltkTextGetInputWidget(ih);
      if (input)
        input->cut(1);
    }
  }
  else if (iupStrEqualNoCase(value, "UNDO"))
  {
    if (ih->data->is_multiline)
    {
      Fl_Text_Buffer* buf = fltkTextGetBuffer(ih);
      if (buf)
        buf->undo();
    }
    else
    {
      Fl_Input* input = fltkTextGetInputWidget(ih);
      if (input)
        input->undo();
    }
  }
  else if (iupStrEqualNoCase(value, "REDO"))
  {
    if (ih->data->is_multiline)
    {
      Fl_Text_Buffer* buf = fltkTextGetBuffer(ih);
      if (buf)
        buf->redo();
    }
  }

  return 0;
}

static int fltkTextSetScrollToAttrib(Ihandle* ih, const char* value)
{
  int lin = 1, col = 1;

  if (!value)
    return 0;

  iupStrToIntInt(value, &lin, &col, ',');
  if (lin < 1) lin = 1;
  if (col < 1) col = 1;

  if (ih->data->is_multiline)
  {
    IupFltkTextEditor* editor = (IupFltkTextEditor*)ih->handle;
    if (editor)
      editor->scroll(lin - 1, 0);
  }
  else
  {
    Fl_Input* input = fltkTextGetInputWidget(ih);
    if (input)
      input->insert_position(col - 1);
  }

  return 0;
}

static int fltkTextSetScrollToPosAttrib(Ihandle* ih, const char* value)
{
  int pos = 0;

  if (!value)
    return 0;

  iupStrToInt(value, &pos);
  if (pos < 0) pos = 0;

  if (ih->data->is_multiline)
  {
    Fl_Text_Buffer* buf = fltkTextGetBuffer(ih);
    IupFltkTextEditor* editor = (IupFltkTextEditor*)ih->handle;
    if (buf && editor)
    {
      int lin = buf->count_lines(0, pos);
      editor->scroll(lin, 0);
    }
  }
  else
  {
    Fl_Input* input = fltkTextGetInputWidget(ih);
    if (input)
      input->insert_position(pos);
  }

  return 0;
}

static char* fltkTextGetLineValueAttrib(Ihandle* ih)
{
  if (ih->data->is_multiline)
  {
    Fl_Text_Buffer* buf = fltkTextGetBuffer(ih);
    IupFltkTextEditor* editor = (IupFltkTextEditor*)ih->handle;
    if (buf && editor)
    {
      int pos = editor->insert_position();
      int line_start = buf->line_start(pos);
      int line_end = buf->line_end(pos);
      char* text = buf->text_range(line_start, line_end);
      char* ret = iupStrReturnStr(text);
      free(text);
      return ret;
    }
  }
  else
    return fltkTextGetValueAttrib(ih);

  return NULL;
}

static int fltkTextSetTabSizeAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->is_multiline)
  {
    int tabsize = 8;
    iupStrToInt(value, &tabsize);

    IupFltkTextEditor* editor = (IupFltkTextEditor*)ih->handle;
    if (editor && editor->text_buffer)
      editor->text_buffer->tab_distance(tabsize);
  }

  return 1;
}

static int fltkTextSetPaddingAttrib(Ihandle* ih, const char* value)
{
  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');

  if (ih->handle)
    return 0;

  return 1;
}

extern "C" IUP_SDK_API void iupdrvTextAddBorders(Ihandle* ih, int *w, int *h)
{
  Fl_Boxtype box = FL_DOWN_BOX;

  if (ih->handle)
  {
    Fl_Widget* widget = (Fl_Widget*)ih->handle;
    box = widget->box();
  }

  *w += Fl::box_dw(box) + 6;   /* LEFT_MARGIN(3) + RIGHT_MARGIN(3) in Fl_Text_Display */
  *h += Fl::box_dh(box) + 2;   /* TOP_MARGIN(1) + BOTTOM_MARGIN(1) in Fl_Text_Display */
}

extern "C" IUP_SDK_API void iupdrvTextAddSpin(Ihandle* ih, int *w, int h)
{
  (void)ih;
  (void)h;
  *w += 20;
}

extern "C" IUP_SDK_API void iupdrvTextConvertLinColToPos(Ihandle* ih, int lin, int col, int *pos)
{
  if (ih->data->is_multiline)
  {
    Fl_Text_Buffer* buf = fltkTextGetBuffer(ih);
    if (buf)
    {
      int p = buf->skip_lines(0, lin - 1);
      p += col - 1;
      *pos = p;
      return;
    }
  }
  *pos = col - 1;
}

extern "C" IUP_SDK_API void iupdrvTextConvertPosToLinCol(Ihandle* ih, int pos, int *lin, int *col)
{
  if (ih->data->is_multiline)
  {
    Fl_Text_Buffer* buf = fltkTextGetBuffer(ih);
    if (buf)
    {
      *lin = buf->count_lines(0, pos) + 1;
      int line_start = buf->line_start(pos);
      *col = pos - line_start + 1;
      return;
    }
  }
  *lin = 1;
  *col = pos + 1;
}

extern "C" IUP_SDK_API void iupdrvTextAddExtraPadding(Ihandle* ih, int *w, int *h)
{
  (void)ih;
  (void)w;
  (void)h;
}

static int fltkTextMapMethod(Ihandle* ih)
{
  if (ih->data->is_multiline)
  {
    IupFltkTextEditor* editor = new IupFltkTextEditor(0, 0, 10, 10, ih);
    ih->handle = (InativeHandle*)editor;

    editor->text_buffer->add_modify_callback(fltkTextEditorModifyCallback, (void*)ih);

    if (iupAttribGetBoolean(ih, "WORDWRAP") || !(ih->data->sb & IUP_SB_HORIZ))
      editor->wrap_mode(Fl_Text_Display::WRAP_AT_BOUNDS, 0);

    char* value = iupAttribGet(ih, "VALUE");
    if (value)
      editor->text_buffer->text(value);

    iupfltkAddToParent(ih);

    if (!iupAttribGetBoolean(ih, "CANFOCUS"))
      editor->visible_focus(0);

    if (iupAttribGetBoolean(ih, "READONLY"))
      editor->is_readonly = 1;

    iupfltkFormatInit(ih);

    if (ih->data->formattags)
      iupTextUpdateFormatTags(ih);
  }
  else if (iupAttribGetBoolean(ih, "SPIN"))
  {
    IupFltkSpinner* spinner = new IupFltkSpinner(0, 0, 10, 10, ih);
    ih->handle = (InativeHandle*)spinner;

    spinner->minimum(iupAttribGetInt(ih, "SPINMIN"));
    spinner->maximum(iupAttribGetInt(ih, "SPINMAX"));
    spinner->step(iupAttribGetInt(ih, "SPININC"));
    spinner->value(iupAttribGetInt(ih, "SPINVALUE"));

    if (!iupAttribGetBoolean(ih, "SPINAUTO"))
      spinner->noauto = 1;

    spinner->callback(fltkSpinCallback, (void*)ih);

    iupfltkAddToParent(ih);
  }
  else if (iupAttribGetBoolean(ih, "PASSWORD"))
  {
    IupFltkSecretInput* input = new IupFltkSecretInput(0, 0, 10, 10, ih);
    ih->handle = (InativeHandle*)input;

    char* value = iupAttribGet(ih, "VALUE");
    if (value)
      input->value(value);

    input->when(FL_WHEN_CHANGED);
    input->callback(fltkTextInputCallback, (void*)ih);

    iupfltkAddToParent(ih);
  }
  else
  {
    IupFltkInput* input = new IupFltkInput(0, 0, 10, 10, ih);
    ih->handle = (InativeHandle*)input;

    char* value = iupAttribGet(ih, "VALUE");
    if (value)
      input->value(value);

    input->when(FL_WHEN_CHANGED);
    input->callback(fltkTextInputCallback, (void*)ih);

    if (ih->data->nc > 0)
      input->maximum_size(ih->data->nc);

    if (iupAttribGetBoolean(ih, "READONLY"))
      input->readonly(1);

    iupfltkAddToParent(ih);

    if (!iupAttribGetBoolean(ih, "CANFOCUS"))
      input->visible_focus(0);
  }

  return IUP_NOERROR;
}

static void fltkTextUnMapMethod(Ihandle* ih)
{
  iupfltkFormatCleanup(ih);

  Fl_Widget* widget = (Fl_Widget*)ih->handle;
  if (widget)
  {
    delete widget;
    ih->handle = NULL;
  }
}

extern "C" IUP_SDK_API void iupdrvTextInitClass(Iclass* ic)
{
  ic->Map = fltkTextMapMethod;
  ic->UnMap = fltkTextUnMapMethod;

  /* Visual */
  iupClassRegisterAttribute(ic, "FONT", NULL, fltkTextSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, fltkTextSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, fltkTextSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTFGCOLOR", IUPAF_DEFAULT);

  /* IupText only */
  iupClassRegisterAttribute(ic, "PADDING", iupTextGetPaddingAttrib, fltkTextSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "VALUE", fltkTextGetValueAttrib, fltkTextSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LINEVALUE", fltkTextGetLineValueAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTEDTEXT", fltkTextGetSelectedTextAttrib, fltkTextSetSelectedTextAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTION", fltkTextGetSelectionAttrib, fltkTextSetSelectionAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTIONPOS", fltkTextGetSelectionPosAttrib, fltkTextSetSelectionPosAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARET", fltkTextGetCaretAttrib, fltkTextSetCaretAttrib, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARETPOS", fltkTextGetCaretPosAttrib, fltkTextSetCaretPosAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_SAVE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "READONLY", fltkTextGetReadOnlyAttrib, fltkTextSetReadOnlyAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "NC", iupTextGetNCAttrib, fltkTextSetNCAttrib, NULL, NULL, IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "CLIPBOARD", NULL, fltkTextSetClipboardAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTO", NULL, fltkTextSetScrollToAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTOPOS", NULL, fltkTextSetScrollToPosAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "APPEND", NULL, fltkTextSetAppendAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERT", NULL, fltkTextSetInsertAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ALIGNMENT", NULL, fltkTextSetAlignmentAttrib, IUPAF_SAMEASSYSTEM, "ALEFT", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COUNT", fltkTextGetCountAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LINECOUNT", fltkTextGetLineCountAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATTING", iupTextGetFormattingAttrib, iupTextSetFormattingAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ADDFORMATTAG", NULL, iupTextSetAddFormatTagAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ADDFORMATTAG_HANDLE", NULL, iupTextSetAddFormatTagHandleAttrib, NULL, NULL, IUPAF_IHANDLE | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "REMOVEFORMATTING", NULL, iupfltkFormatSetRemoveFormattingAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "OVERWRITE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABSIZE", NULL, fltkTextSetTabSizeAttrib, IUPAF_SAMEASSYSTEM, "8", IUPAF_DEFAULT);

  /* Spin */
  iupClassRegisterAttribute(ic, "SPINMIN", NULL, fltkTextSetSpinMinAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINMAX", NULL, fltkTextSetSpinMaxAttrib, IUPAF_SAMEASSYSTEM, "100", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINVALUE", fltkTextGetSpinValueAttrib, fltkTextSetSpinValueAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPININC", NULL, fltkTextSetSpinIncAttrib, IUPAF_SAMEASSYSTEM, "1", IUPAF_NO_INHERIT);
}
