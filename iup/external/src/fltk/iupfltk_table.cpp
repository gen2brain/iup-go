/** \file
 * \brief Table Control - FLTK implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Table_Row.H>
#include <FL/Fl_Input.H>
#include <FL/fl_draw.H>

#include <vector>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <cstring>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_image.h"
#include "iup_table.h"
}

#include "iupfltk_drv.h"


static Fl_Align fltkTableGetColumnAlignment(Ihandle* ih, int col)
{
  char name[50];
  snprintf(name, sizeof(name), "ALIGNMENT%d", col);
  char* align_str = iupAttribGet(ih, name);

  if (!align_str)
    return FL_ALIGN_LEFT | FL_ALIGN_INSIDE;

  if (iupStrEqualNoCase(align_str, "ARIGHT") || iupStrEqualNoCase(align_str, "RIGHT"))
    return FL_ALIGN_RIGHT | FL_ALIGN_INSIDE;
  else if (iupStrEqualNoCase(align_str, "ACENTER") || iupStrEqualNoCase(align_str, "CENTER"))
    return FL_ALIGN_CENTER | FL_ALIGN_INSIDE;

  return FL_ALIGN_LEFT | FL_ALIGN_INSIDE;
}

static void fltkTableGetCellBgColor(Ihandle* ih, int lin, int col, unsigned char* r, unsigned char* g, unsigned char* b)
{
  char* bgcolor = iupAttribGetId2(ih, "BGCOLOR", lin, col);
  if (!bgcolor)
    bgcolor = iupAttribGetId2(ih, "BGCOLOR", lin, 0);
  if (!bgcolor)
    bgcolor = iupAttribGetId2(ih, "BGCOLOR", 0, col);

  if (!bgcolor)
  {
    char* alternate = iupAttribGet(ih, "ALTERNATECOLOR");
    if (iupStrBoolean(alternate))
    {
      if (lin % 2 == 0)
        bgcolor = iupAttribGet(ih, "EVENROWCOLOR");
      else
        bgcolor = iupAttribGet(ih, "ODDROWCOLOR");
    }
  }

  if (!bgcolor)
    bgcolor = iupAttribGet(ih, "BGCOLOR");

  if (bgcolor && iupStrToRGB(bgcolor, r, g, b))
    return;

  *r = 255; *g = 255; *b = 255;
}

static void fltkTableGetCellFgColor(Ihandle* ih, int lin, int col, unsigned char* r, unsigned char* g, unsigned char* b)
{
  char* fgcolor = iupAttribGetId2(ih, "FGCOLOR", lin, col);
  if (!fgcolor)
    fgcolor = iupAttribGetId2(ih, "FGCOLOR", lin, 0);
  if (!fgcolor)
    fgcolor = iupAttribGetId2(ih, "FGCOLOR", 0, col);

  if (!fgcolor)
    fgcolor = iupAttribGet(ih, "FGCOLOR");

  if (fgcolor && iupStrToRGB(fgcolor, r, g, b))
    return;

  *r = 0; *g = 0; *b = 0;
}

static void fltkTableSetCellFont(Ihandle* ih, int lin, int col)
{
  char* font = iupAttribGetId2(ih, "FONT", lin, col);
  if (!font)
    font = iupAttribGetId2(ih, "FONT", 0, col);
  if (!font)
    font = iupAttribGetId2(ih, "FONT", lin, 0);

  if (font)
  {
    int fl_font_face, fl_font_size;
    if (iupfltkGetFontFromString(font, &fl_font_face, &fl_font_size))
      fl_font(fl_font_face, fl_font_size);
  }
  else
  {
    int fl_font_face, fl_font_size;
    if (iupfltkGetFont(ih, &fl_font_face, &fl_font_size))
      fl_font(fl_font_face, fl_font_size);
  }
}

class IupFltkTable;
static void fltkTableEndCellEdit(Ihandle* ih, int apply);
static void fltkTableStartCellEdit(Ihandle* ih, int lin, int col);
static void fltkTableDeferredEdit(void* data);
static void fltkTableSortRows(Ihandle* ih, int col, int ascending);
static void fltkTableHandleHeaderClick(Ihandle* ih, int col);
static void fltkTableSwapColumns(Ihandle* ih, int src, int dst);
static int fltkTableFindTargetCol(IupFltkTable* table, int mx);

static int fltkTableIsCellEditable(Ihandle* ih, int lin, int col)
{
  char name[32];
  snprintf(name, sizeof(name), "EDITABLE%d", col);
  char* editable = iupAttribGet(ih, name);
  if (!editable)
    editable = iupAttribGet(ih, "EDITABLE");
  return editable ? iupStrBoolean(editable) : 0;
}

class IupFltkTableInput : public Fl_Input
{
public:
  Ihandle* iup_handle;
  int active;

  IupFltkTableInput(int x, int y, int w, int h, Ihandle* ih)
    : Fl_Input(x, y, w, h), iup_handle(ih), active(0) {}

  int handle(int event) override
  {
    if (event == FL_KEYBOARD)
    {
      if (Fl::event_key() == FL_Escape)
      {
        fltkTableEndCellEdit(iup_handle, 0);
        return 1;
      }
      if (Fl::event_key() == FL_Enter || Fl::event_key() == FL_KP_Enter)
      {
        if (active)
        {
          fltkTableEndCellEdit(iup_handle, 1);
          return 1;
        }
      }
    }
    else if (event == FL_UNFOCUS)
    {
      if (active)
      {
        fltkTableEndCellEdit(iup_handle, 1);
        return 1;
      }
    }
    return Fl_Input::handle(event);
  }
};

static void fltkTableEndCellEdit(Ihandle* ih, int apply)
{
  Fl_Input* edit = (Fl_Input*)iupAttribGet(ih, "_IUPFLTK_TABLE_EDIT");
  if (!edit)
    return;

  iupAttribSet(ih, "_IUPFLTK_TABLE_EDIT", NULL);

  int lin = iupAttribGetInt(ih, "_IUPFLTK_TABLE_EDIT_LIN");
  int col = iupAttribGetInt(ih, "_IUPFLTK_TABLE_EDIT_COL");

  if (apply)
  {
    const char* new_text = edit->value();
    const char* old_text = IupGetAttributeId2(ih, "", lin, col);
    if (!old_text) old_text = "";

    IFniisi editend_cb = (IFniisi)IupGetCallback(ih, "EDITEND_CB");
    if (editend_cb)
    {
      if (editend_cb(ih, lin, col, (char*)new_text, 1) == IUP_IGNORE)
        apply = 0;
    }

    if (apply && strcmp(old_text, new_text) != 0)
    {
      IupSetAttributeId2(ih, "", lin, col, new_text);

      IFnii vcb = (IFnii)IupGetCallback(ih, "VALUECHANGED_CB");
      if (vcb)
        vcb(ih, lin, col);
    }
  }
  else
  {
    IFniisi editend_cb = (IFniisi)IupGetCallback(ih, "EDITEND_CB");
    if (editend_cb)
      editend_cb(ih, lin, col, (char*)"", 0);
  }

  Fl_Window* win = edit->window();
  if (win)
  {
    win->remove(edit);
    win->cursor(FL_CURSOR_DEFAULT);
    win->redraw();
  }

  Fl::delete_widget(edit);
}

class IupFltkTable : public Fl_Table_Row
{
public:
  Ihandle* iup_handle;
  std::vector<std::vector<std::string>> cells;
  std::vector<std::string> col_titles;
  int focus_lin;
  int focus_col;
  int show_grid;
  int is_virtual;
  int has_dummy_col;
  int auto_widths_done;
  int sort_column;
  int sort_ascending;
  int drag_source_col;
  int drag_target_col;
  int drag_start_x, drag_start_y;
  int dragging;

  IupFltkTable(int X, int Y, int W, int H, Ihandle* ih)
    : Fl_Table_Row(X, Y, W, H), iup_handle(ih),
      focus_lin(0), focus_col(0), show_grid(1), is_virtual(0), has_dummy_col(0), auto_widths_done(0),
      sort_column(0), sort_ascending(1),
      drag_source_col(-1), drag_target_col(-1), drag_start_x(0), drag_start_y(0), dragging(0)
  {
    selection_color(FL_SELECTION_COLOR);
  }

  int getCellRect(int R, int C, int &X, int &Y, int &W, int &H)
  {
    return find_cell(CONTEXT_CELL, R, C, X, Y, W, H);
  }


  void resize_storage(int nlin, int ncol)
  {
    cells.resize(nlin);
    for (auto& row : cells)
      row.resize(ncol);
    col_titles.resize(ncol);
  }

protected:
  void draw_cell(TableContext context, int R, int C, int X, int Y, int W, int H) override
  {
    switch (context)
    {
      case CONTEXT_STARTPAGE:
      case CONTEXT_ENDPAGE:
        break;

      case CONTEXT_COL_HEADER:
      {
        fl_push_clip(X, Y, W, H);

        fl_color(FL_BACKGROUND_COLOR);
        fl_rectf(X, Y, W, H);

        if (!has_dummy_col || C < iup_handle->data->num_col)
        {
          fl_color(FL_BLACK);
          int fl_font_face, fl_font_size;
          if (iupfltkGetFont(iup_handle, &fl_font_face, &fl_font_size))
            fl_font(fl_font_face, fl_font_size);

          const char* title = "";
          if (C >= 0 && C < (int)col_titles.size() && !col_titles[C].empty())
            title = col_titles[C].c_str();

          int arrow_space = (sort_column == C + 1) ? 14 : 0;
          Fl_Align align = fltkTableGetColumnAlignment(iup_handle, C + 1);
          fl_draw(title, X + 2, Y, W - 4 - arrow_space, H, align);

          if (sort_column == C + 1)
          {
            int ax = X + W - 12;
            int ay = Y + H / 2;
            fl_color(FL_DARK3);
            if (sort_ascending)
            {
              fl_polygon(ax, ay + 3, ax + 4, ay - 3, ax + 8, ay + 3);
            }
            else
            {
              fl_polygon(ax, ay - 3, ax + 4, ay + 3, ax + 8, ay - 3);
            }
          }
        }

        fl_color(FL_DARK2);
        fl_line(X, Y + H - 1, X + W - 1, Y + H - 1);

        if (!has_dummy_col || C < iup_handle->data->num_col)
          fl_line(X + W - 1, Y, X + W - 1, Y + H - 1);

        if (dragging && C == drag_target_col)
        {
          fl_color(FL_SELECTION_COLOR);
          fl_line_style(FL_SOLID, 2);
          fl_line(X, Y, X, Y + H);
          fl_line_style(FL_SOLID, 1);
        }

        fl_pop_clip();
        break;
      }

      case CONTEXT_ROW_HEADER:
      {
        fl_push_clip(X, Y, W, H);

        fl_color(FL_BACKGROUND_COLOR);
        fl_rectf(X, Y, W, H);

        fl_color(FL_BLACK);
        int fl_font_face, fl_font_size;
        if (iupfltkGetFont(iup_handle, &fl_font_face, &fl_font_size))
          fl_font(fl_font_face, fl_font_size);

        char buf[32];
        snprintf(buf, sizeof(buf), "%d", R + 1);
        fl_draw(buf, X + 2, Y, W - 4, H, FL_ALIGN_CENTER | FL_ALIGN_INSIDE);

        fl_color(FL_DARK2);
        fl_line(X, Y + H - 1, X + W - 1, Y + H - 1);
        fl_line(X + W - 1, Y, X + W - 1, Y + H - 1);

        fl_pop_clip();
        break;
      }

      case CONTEXT_CELL:
      {
        fl_push_clip(X, Y, W, H);

        if (has_dummy_col && C >= iup_handle->data->num_col)
        {
          unsigned char r, g, b;
          char* bgcolor = iupAttribGet(iup_handle, "BGCOLOR");
          if (bgcolor && iupStrToRGB(bgcolor, &r, &g, &b))
            fl_color(r, g, b);
          else
            fl_color(FL_BACKGROUND_COLOR);
          fl_rectf(X, Y, W, H);
          fl_pop_clip();
          break;
        }

        int iup_lin = R + 1;
        int iup_col = C + 1;

        const char* text = "";
        if (is_virtual)
        {
          sIFnii value_cb = (sIFnii)IupGetCallback(iup_handle, "VALUE_CB");
          if (value_cb)
          {
            char* val = value_cb(iup_handle, iup_lin, iup_col);
            if (val)
              text = val;
          }
        }
        else
        {
          if (R >= 0 && R < (int)cells.size() && C >= 0 && C < (int)cells[R].size())
            text = cells[R][C].c_str();
        }

        unsigned char bg_r, bg_g, bg_b;
        fltkTableGetCellBgColor(iup_handle, iup_lin, iup_col, &bg_r, &bg_g, &bg_b);

        if (row_selected(R))
          fl_color(FL_SELECTION_COLOR);
        else
          fl_color(bg_r, bg_g, bg_b);
        fl_rectf(X, Y, W, H);

        unsigned char fg_r, fg_g, fg_b;
        fltkTableGetCellFgColor(iup_handle, iup_lin, iup_col, &fg_r, &fg_g, &fg_b);

        fltkTableSetCellFont(iup_handle, iup_lin, iup_col);

        if (row_selected(R))
          fl_color(fl_contrast(fl_rgb_color(fg_r, fg_g, fg_b), FL_SELECTION_COLOR));
        else
          fl_color(fg_r, fg_g, fg_b);

        {
          int img_w = 0;

          if (iup_handle->data->show_image)
          {
            char* image_name = NULL;
            char img_key[50];
            snprintf(img_key, sizeof(img_key), "_CELLIMAGE%d:%d", iup_lin, iup_col);
            image_name = iupAttribGet(iup_handle, img_key);

            if (!image_name)
              image_name = iupTableGetCellImageCb(iup_handle, iup_lin, iup_col);

            if (image_name)
            {
              Fl_Image* img = (Fl_Image*)iupImageGetImage(image_name, iup_handle, 0, NULL);
              if (img)
              {
                Fl_Image* draw_img = img;
                int avail_h = H - 2;
                if (img->h() > avail_h && avail_h > 0)
                {
                  int scaled_w = (img->w() * avail_h) / img->h();
                  draw_img = img->copy(scaled_w, avail_h);
                }

                int iy = Y + (H - draw_img->h()) / 2;
                draw_img->draw(X + 2, iy);
                img_w = draw_img->w() + 4;

                if (draw_img != img)
                  delete draw_img;
              }
            }
          }

          int tw = 0, th = 0;
          fl_measure(text, tw, th, 0);
          Fl_Align align = fltkTableGetColumnAlignment(iup_handle, iup_col);

          int cell_x = X + 2 + img_w;
          int cell_w = W - 4 - img_w;

          int tx = cell_x;
          if (align & FL_ALIGN_RIGHT)
            tx = cell_x + cell_w - tw;
          else if (!(align & FL_ALIGN_LEFT))
            tx = cell_x + (cell_w - tw) / 2;

          int ty = Y + (H + fl_height() - fl_descent()) / 2;
          fl_draw(text, (int)strlen(text), tx, ty);
        }

        if (show_grid)
        {
          fl_color(FL_DARK2);
          fl_rect(X, Y, W, H);
        }

        if (R == focus_lin && C == focus_col && Fl::focus() == this)
        {
          Fl_Color bg = row_selected(R) ? FL_SELECTION_COLOR : fl_rgb_color(bg_r, bg_g, bg_b);
          fl_color(fl_contrast(FL_BLACK, bg));
          fl_line_style(FL_DOT, 1);
          fl_rect(X + 1, Y + 1, W - 2, H - 2);
          fl_line_style(FL_SOLID, 1);
        }

        fl_pop_clip();
        break;
      }

      case CONTEXT_TABLE:
      {
        unsigned char r, g, b;
        char* bgcolor = iupAttribGet(iup_handle, "BGCOLOR");
        if (bgcolor && iupStrToRGB(bgcolor, &r, &g, &b))
          fl_color(r, g, b);
        else
          fl_color(FL_BACKGROUND_COLOR);
        fl_rectf(X, Y, W, H);
        break;
      }

      default:
        break;
    }
  }

  int handle(int event) override
  {
    switch (event)
    {
      case FL_PUSH:
      {
        take_focus();

        int R, C;
        ResizeFlag resizeflag;
        TableContext context = cursor2rowcol(R, C, resizeflag);

        if (context == CONTEXT_COL_HEADER && C >= 0 &&
            (!has_dummy_col || C < iup_handle->data->num_col))
        {
          if (iup_handle->data->allow_reorder)
          {
            drag_source_col = C;
            drag_target_col = C;
            drag_start_x = Fl::event_x();
            drag_start_y = Fl::event_y();
            dragging = 0;
          }
          else if (iup_handle->data->sortable)
          {
            fltkTableHandleHeaderClick(iup_handle, C + 1);
          }
        }

        if (context == CONTEXT_CELL && (!has_dummy_col || C < iup_handle->data->num_col))
        {
          int prev_lin = focus_lin;
          int prev_col = focus_col;
          focus_lin = R;
          focus_col = C;

          if (prev_lin != R || prev_col != C)
          {
            IFnii leave_cb = (IFnii)IupGetCallback(iup_handle, "LEAVEITEM_CB");
            if (leave_cb)
              leave_cb(iup_handle, prev_lin + 1, prev_col + 1);

            IFnii enter_cb = (IFnii)IupGetCallback(iup_handle, "ENTERITEM_CB");
            if (enter_cb)
              enter_cb(iup_handle, R + 1, C + 1);

            redraw();
          }

          if (Fl::event_clicks() > 0)
          {
            IFniis dblclick_cb = (IFniis)IupGetCallback(iup_handle, "DBLCLICK_CB");
            if (dblclick_cb)
            {
              const char* text = "";
              if (is_virtual)
              {
                sIFnii value_cb = (sIFnii)IupGetCallback(iup_handle, "VALUE_CB");
                if (value_cb)
                {
                  char* val = value_cb(iup_handle, R + 1, C + 1);
                  if (val) text = val;
                }
              }
              else if (R >= 0 && R < (int)cells.size() && C >= 0 && C < (int)cells[R].size())
              {
                text = cells[R][C].c_str();
              }
              dblclick_cb(iup_handle, R + 1, C + 1, (char*)text);
            }

            iupAttribSetInt(iup_handle, "_IUPFLTK_TABLE_PEND_LIN", R + 1);
            iupAttribSetInt(iup_handle, "_IUPFLTK_TABLE_PEND_COL", C + 1);
            Fl::add_timeout(0.01, fltkTableDeferredEdit, (void*)iup_handle);
          }
          else
          {
            IFniis click_cb = (IFniis)IupGetCallback(iup_handle, "CLICK_CB");
            if (click_cb)
            {
              char status[20] = "";
              int button = IUP_BUTTON1;
              if (Fl::event_button() == FL_MIDDLE_MOUSE) button = IUP_BUTTON2;
              else if (Fl::event_button() == FL_RIGHT_MOUSE) button = IUP_BUTTON3;
              iupfltkButtonKeySetStatus(Fl::event_state(), button, status, 0);
              click_cb(iup_handle, R + 1, C + 1, status);
            }
          }
        }
        break;
      }

      case FL_DRAG:
      {
        if (drag_source_col >= 0 && iup_handle->data->allow_reorder)
        {
          int dx = Fl::event_x() - drag_start_x;
          int dy = Fl::event_y() - drag_start_y;
          if (!dragging && (dx * dx + dy * dy) >= 25)
            dragging = 1;

          if (dragging)
          {
            int new_target = fltkTableFindTargetCol(this, Fl::event_x());
            if (new_target != drag_target_col)
            {
              drag_target_col = new_target;
              redraw();
            }
          }
        }
        break;
      }

      case FL_RELEASE:
      {
        if (drag_source_col >= 0 && iup_handle->data->allow_reorder)
        {
          int src = drag_source_col;
          int tgt = drag_target_col;
          int was_dragging = dragging;

          drag_source_col = -1;
          drag_target_col = -1;
          dragging = 0;

          if (was_dragging && src != tgt && tgt != src + 1)
          {
            int iup_src = src + 1;
            int iup_tgt = tgt < src ? tgt + 1 : tgt;

            IFnii reorder_cb = (IFnii)IupGetCallback(iup_handle, "REORDER_CB");
            int ret = IUP_DEFAULT;
            if (reorder_cb)
              ret = reorder_cb(iup_handle, iup_src, iup_tgt);

            if (ret != IUP_IGNORE)
              fltkTableSwapColumns(iup_handle, iup_src, iup_tgt);
          }
          else if (!was_dragging && iup_handle->data->sortable)
          {
            fltkTableHandleHeaderClick(iup_handle, src + 1);
          }

          redraw();
        }
        break;
      }

      case FL_FOCUS:
        iupfltkFocusInOutEvent((Fl_Widget*)this, iup_handle, FL_FOCUS);
        redraw();
        return 1;

      case FL_UNFOCUS:
        iupfltkFocusInOutEvent((Fl_Widget*)this, iup_handle, FL_UNFOCUS);
        redraw();
        return 1;

      case FL_ENTER:
      case FL_LEAVE:
        iupfltkEnterLeaveEvent((Fl_Widget*)this, iup_handle, event);
        break;

      case FL_KEYBOARD:
        if (iupfltkKeyPressEvent((Fl_Widget*)this, iup_handle))
          return 1;

        if ((Fl::event_key() == FL_F + 2 || Fl::event_key() == FL_Enter) &&
            focus_lin >= 0 && focus_col >= 0)
        {
          iupAttribSetInt(iup_handle, "_IUPFLTK_TABLE_PEND_LIN", focus_lin + 1);
          iupAttribSetInt(iup_handle, "_IUPFLTK_TABLE_PEND_COL", focus_col + 1);
          Fl::add_timeout(0.01, fltkTableDeferredEdit, (void*)iup_handle);
          return 1;
        }

        {
          int num_lin = iup_handle->data->num_lin;
          int num_col = iup_handle->data->num_col;
          int new_lin = focus_lin;
          int new_col = focus_col;
          int key = Fl::event_key();

          if (key == FL_Up && new_lin > 0) new_lin--;
          else if (key == FL_Down && new_lin < num_lin - 1) new_lin++;
          else if (key == FL_Left && new_col > 0) new_col--;
          else if (key == FL_Right && new_col < num_col - 1) new_col++;
          else if (key == FL_Home) { new_lin = 0; new_col = 0; }
          else if (key == FL_End) { new_lin = num_lin - 1; new_col = num_col - 1; }
          else if (key == FL_Page_Up) { new_lin -= rows() - 1; if (new_lin < 0) new_lin = 0; }
          else if (key == FL_Page_Down) { new_lin += rows() - 1; if (new_lin >= num_lin) new_lin = num_lin - 1; }
          else break;

          if (new_lin != focus_lin || new_col != focus_col)
          {
            IFnii leave_cb = (IFnii)IupGetCallback(iup_handle, "LEAVEITEM_CB");
            if (leave_cb)
              leave_cb(iup_handle, focus_lin + 1, focus_col + 1);

            focus_lin = new_lin;
            focus_col = new_col;

            IFnii enter_cb = (IFnii)IupGetCallback(iup_handle, "ENTERITEM_CB");
            if (enter_cb)
              enter_cb(iup_handle, focus_lin + 1, focus_col + 1);

            select_all_rows(0);
            select_row(focus_lin, 1);

            int X, Y, W, H;
            if (getCellRect(focus_lin, focus_col, X, Y, W, H) == 0)
              row_position(focus_lin);

            redraw();
          }
          return 1;
        }

      case FL_KEYUP:
        if (iupfltkKeyReleaseEvent((Fl_Widget*)this, iup_handle))
          return 1;
        break;

      default:
        break;
    }

    return Fl_Table_Row::handle(event);
  }
};


static void fltkTableStartCellEdit(Ihandle* ih, int lin, int col)
{
  if (!fltkTableIsCellEditable(ih, lin, col))
    return;

  IFnii editbegin_cb = (IFnii)IupGetCallback(ih, "EDITBEGIN_CB");
  if (editbegin_cb && editbegin_cb(ih, lin, col) == IUP_IGNORE)
    return;

  if (iupAttribGet(ih, "_IUPFLTK_TABLE_EDIT"))
    fltkTableEndCellEdit(ih, 1);

  IupFltkTable* table = (IupFltkTable*)ih->handle;
  if (!table) return;

  Fl_Window* win = table->window();
  if (!win) return;

  int X, Y, W, H;
  table->getCellRect(lin - 1, col - 1, X, Y, W, H);

  if (W <= 0 || H <= 0)
    return;

  win->begin();
  IupFltkTableInput* edit = new IupFltkTableInput(X, Y, W, H, ih);
  win->end();

  const char* value = IupGetAttributeId2(ih, "", lin, col);
  edit->value(value ? value : "");

  iupAttribSet(ih, "_IUPFLTK_TABLE_EDIT", (char*)edit);
  iupAttribSetInt(ih, "_IUPFLTK_TABLE_EDIT_LIN", lin);
  iupAttribSetInt(ih, "_IUPFLTK_TABLE_EDIT_COL", col);

  edit->show();
  edit->take_focus();
  edit->insert_position(0, value ? (int)strlen(value) : 0);
  edit->active = 1;
}

static void fltkTableDeferredEdit(void* data)
{
  Ihandle* ih = (Ihandle*)data;
  if (!iupObjectCheck(ih)) return;
  if (!ih->handle) return;

  int lin = iupAttribGetInt(ih, "_IUPFLTK_TABLE_PEND_LIN");
  int col = iupAttribGetInt(ih, "_IUPFLTK_TABLE_PEND_COL");
  fltkTableStartCellEdit(ih, lin, col);
}


static void fltkTableSortRows(Ihandle* ih, int col, int ascending)
{
  IupFltkTable* table = (IupFltkTable*)ih->handle;
  if (!table || table->is_virtual)
    return;

  int num_lin = ih->data->num_lin;
  int c = col - 1;

  if (c < 0 || num_lin <= 1)
    return;

  for (int i = 0; i < num_lin - 1; i++)
  {
    for (int j = i + 1; j < num_lin; j++)
    {
      const char* a = "";
      const char* b = "";
      if (i < (int)table->cells.size() && c < (int)table->cells[i].size())
        a = table->cells[i][c].c_str();
      if (j < (int)table->cells.size() && c < (int)table->cells[j].size())
        b = table->cells[j][c].c_str();

      int cmp = strcmp(a, b);
      if (ascending ? (cmp > 0) : (cmp < 0))
        std::swap(table->cells[i], table->cells[j]);
    }
  }
}

static void fltkTableHandleHeaderClick(Ihandle* ih, int col)
{
  IupFltkTable* table = (IupFltkTable*)ih->handle;
  if (!table || !ih->data->sortable)
    return;

  if (table->sort_column == col)
    table->sort_ascending = !table->sort_ascending;
  else
  {
    table->sort_column = col;
    table->sort_ascending = 1;
  }

  IFni sort_cb = (IFni)IupGetCallback(ih, "SORT_CB");
  int ret = IUP_DEFAULT;
  if (sort_cb)
    ret = sort_cb(ih, col);

  if (ret == IUP_DEFAULT && !table->is_virtual)
    fltkTableSortRows(ih, col, table->sort_ascending);

  table->redraw();
}

static void fltkTableSwapColumns(Ihandle* ih, int src, int dst)
{
  IupFltkTable* table = (IupFltkTable*)ih->handle;
  if (!table) return;

  int s = src - 1;
  int d = dst - 1;
  int num_col = ih->data->num_col;

  if (s < 0 || s >= num_col || d < 0 || d >= num_col || s == d)
    return;

  if (!table->is_virtual)
  {
    for (int r = 0; r < (int)table->cells.size(); r++)
    {
      if (s < (int)table->cells[r].size() && d < (int)table->cells[r].size())
        std::swap(table->cells[r][s], table->cells[r][d]);
    }
  }

  if (s < (int)table->col_titles.size() && d < (int)table->col_titles.size())
    std::swap(table->col_titles[s], table->col_titles[d]);

  int w_s = table->col_width(s);
  int w_d = table->col_width(d);
  table->col_width(s, w_d);
  table->col_width(d, w_s);

  char name_s[50], name_d[50];
  snprintf(name_s, sizeof(name_s), "ALIGNMENT%d", src);
  snprintf(name_d, sizeof(name_d), "ALIGNMENT%d", dst);
  char* align_s = iupStrDup(iupAttribGet(ih, name_s));
  char* align_d = iupStrDup(iupAttribGet(ih, name_d));
  if (align_s) iupAttribSetStr(ih, name_d, align_s);
  else iupAttribSet(ih, name_d, NULL);
  if (align_d) iupAttribSetStr(ih, name_s, align_d);
  else iupAttribSet(ih, name_s, NULL);
  if (align_s) free(align_s);
  if (align_d) free(align_d);

  const char* swap_attrs[] = { "FONT", "FGCOLOR", "BGCOLOR", "EDITABLE", NULL };
  for (int a = 0; swap_attrs[a]; a++)
  {
    snprintf(name_s, sizeof(name_s), "%s%d:%d", swap_attrs[a], 0, src);
    snprintf(name_d, sizeof(name_d), "%s%d:%d", swap_attrs[a], 0, dst);
    char* val_s = iupStrDup(iupAttribGet(ih, name_s));
    char* val_d = iupStrDup(iupAttribGet(ih, name_d));
    if (val_s) iupAttribSetStr(ih, name_d, val_s);
    else iupAttribSet(ih, name_d, NULL);
    if (val_d) iupAttribSetStr(ih, name_s, val_d);
    else iupAttribSet(ih, name_s, NULL);
    if (val_s) free(val_s);
    if (val_d) free(val_d);
  }

  if (table->sort_column == src)
    table->sort_column = dst;
  else if (table->sort_column == dst)
    table->sort_column = src;

  if (table->focus_col == s)
    table->focus_col = d;
  else if (table->focus_col == d)
    table->focus_col = s;
}

static int fltkTableFindTargetCol(IupFltkTable* table, int mx)
{
  int num_col = table->iup_handle->data->num_col;
  int cx = table->x() + (table->row_header() ? table->row_header_width() : 0);
  cx += Fl::box_dx(table->box());

  for (int c = 0; c < num_col; c++)
  {
    int cw = table->col_width(c);
    int mid = cx + cw / 2;
    if (mx < mid)
      return c;
    cx += cw;
  }
  return num_col;
}

static IupFltkTable* fltkTableGetWidget(Ihandle* ih)
{
  return (IupFltkTable*)ih->handle;
}

static int fltkTableColHasExplicitWidth(Ihandle* ih, int col)
{
  char name[50];
  int width = 0;
  char* width_str = NULL;

  snprintf(name, sizeof(name), "RASTERWIDTH%d", col);
  width_str = iupAttribGet(ih, name);
  if (!width_str)
  {
    snprintf(name, sizeof(name), "WIDTH%d", col);
    width_str = iupAttribGet(ih, name);
  }

  return (width_str && iupStrToInt(width_str, &width) && width > 0);
}

static void fltkTableAutoColumnWidths(Ihandle* ih, IupFltkTable* table)
{
  int num_col = ih->data->num_col;
  int num_lin = ih->data->num_lin;

  int fl_font_face, fl_font_size;
  if (iupfltkGetFont(ih, &fl_font_face, &fl_font_size))
    fl_font(fl_font_face, fl_font_size);

  for (int c = 0; c < num_col; c++)
  {
    if (fltkTableColHasExplicitWidth(ih, c + 1))
      continue;

    int iup_col = c + 1;
    int max_w = 0;
    int check_rows = num_lin < 50 ? num_lin : 50;
    int image_extra = 0;

    if (ih->data->show_image)
    {
      int row_h = table->row_height(0);
      int avail_h = row_h > 2 ? row_h - 2 : row_h;

      for (int r = 0; r < check_rows; r++)
      {
        char* image_name = NULL;
        char img_key[50];
        snprintf(img_key, sizeof(img_key), "_CELLIMAGE%d:%d", r + 1, iup_col);
        image_name = iupAttribGet(ih, img_key);
        if (!image_name)
          image_name = iupTableGetCellImageCb(ih, r + 1, iup_col);
        if (image_name)
        {
          int img_w = 0, img_h = 0;
          iupImageGetInfo(image_name, &img_w, &img_h, NULL);
          if (img_h > avail_h && avail_h > 0)
            img_w = (img_w * avail_h) / img_h;
          if (img_w + 6 > image_extra)
            image_extra = img_w + 6;
        }
      }
    }

    const char* title = NULL;
    if (c < (int)table->col_titles.size() && !table->col_titles[c].empty())
      title = table->col_titles[c].c_str();
    if (!title)
      title = iupAttribGetId(ih, "TITLE", iup_col);
    if (title && title[0])
    {
      int tw = 0, th = 0;
      fl_measure(title, tw, th, 0);
      if (tw + 16 > max_w)
        max_w = tw + 16;
    }

    for (int r = 0; r < check_rows; r++)
    {
      const char* text = NULL;

      if (table->is_virtual)
      {
        sIFnii value_cb = (sIFnii)IupGetCallback(ih, "VALUE_CB");
        if (value_cb)
          text = value_cb(ih, r + 1, iup_col);
      }
      else if (r < (int)table->cells.size() && c < (int)table->cells[r].size())
        text = table->cells[r][c].c_str();

      if (text && text[0])
      {
        char* cell_font = iupAttribGetId2(ih, "FONT", 0, iup_col);
        if (cell_font)
        {
          int cf, cs;
          if (iupfltkGetFontFromString(cell_font, &cf, &cs))
            fl_font(cf, cs);
        }

        int tw = 0, th = 0;
        fl_measure(text, tw, th, 0);
        int cell_w = tw + 16;

        if (cell_w > max_w)
          max_w = cell_w;

        if (cell_font && iupfltkGetFont(ih, &fl_font_face, &fl_font_size))
          fl_font(fl_font_face, fl_font_size);
      }
    }

    max_w += image_extra;

    if (max_w < 30)
      max_w = 30;

    table->col_width(c, max_w);
  }
}

static void fltkTableLayoutUpdateMethod(Ihandle* ih)
{
  IupFltkTable* table = fltkTableGetWidget(ih);
  if (!table)
    return;

  if (!table->auto_widths_done)
  {
    table->auto_widths_done = 1;
    fltkTableAutoColumnWidths(ih, table);
  }

  int width = ih->currentwidth;
  int height = ih->currentheight;

  int target_height = iupAttribGetInt(ih, "_IUPFLTK_TABLE_TARGET_HEIGHT");
  if (target_height > 0 && height > target_height)
    height = target_height;

  int visible_columns = iupAttribGetInt(ih, "_IUPFLTK_TABLE_VISIBLE_COLUMNS");
  if (visible_columns > 0)
  {
    int cols_width = 0;
    int num_cols = visible_columns;
    if (num_cols > ih->data->num_col)
      num_cols = ih->data->num_col;

    for (int c = 0; c < num_cols; c++)
      cols_width += table->col_width(c);

    int sb_size = iupdrvGetScrollbarSize();
    int border = iupdrvTableGetBorderWidth(ih);

    int visiblelines = iupAttribGetInt(ih, "VISIBLELINES");
    int need_vert_sb = (visiblelines > 0 && ih->data->num_lin > visiblelines);
    int vert_sb_width = need_vert_sb ? sb_size : 0;

    int target_width = cols_width + vert_sb_width + 2 * border;
    if (width > target_width)
      width = target_width;
  }

  ih->currentwidth = width;
  ih->currentheight = height;

  iupdrvBaseLayoutUpdateMethod(ih);

  if (ih->data->num_col > 0)
  {
    int last_col = ih->data->num_col - 1;
    int last_col_has_width = fltkTableColHasExplicitWidth(ih, ih->data->num_col);

    if (ih->data->stretch_last && !last_col_has_width)
    {
      int available = width - Fl::box_dw(table->box());

      if (table->row_header())
        available -= table->row_header_width();

      int scrollsize = Fl::scrollbar_size();
      int table_h = ih->data->num_lin * table->row_height(0);
      int visible_h = height - Fl::box_dh(table->box());
      if (table->col_header())
        visible_h -= table->col_header_height();
      if (table_h > visible_h)
        available -= scrollsize;

      int other_cols_width = 0;
      for (int c = 0; c < last_col; c++)
        other_cols_width += table->col_width(c);

      int last_width = available - other_cols_width;
      if (last_width < 1)
        last_width = 1;
      table->col_width(last_col, last_width);
    }
    else if (!ih->data->stretch_last || last_col_has_width)
    {
      int total_col_width = 0;
      for (int c = 0; c < ih->data->num_col; c++)
        total_col_width += table->col_width(c);

      int available = width - Fl::box_dw(table->box());
      if (table->row_header())
        available -= table->row_header_width();

      int scrollsize = Fl::scrollbar_size();
      int table_h = ih->data->num_lin * table->row_height(0);
      int visible_h = height - Fl::box_dh(table->box());
      if (table->col_header())
        visible_h -= table->col_header_height();
      if (table_h > visible_h)
        available -= scrollsize;

      if (total_col_width < available)
      {
        int dummy_width = available - total_col_width;
        if (!table->has_dummy_col)
        {
          table->has_dummy_col = 1;
          table->cols(ih->data->num_col + 1);
        }
        table->col_width(ih->data->num_col, dummy_width);
      }
      else if (table->has_dummy_col)
      {
        table->has_dummy_col = 0;
        table->cols(ih->data->num_col);
      }
    }
  }
}

static int fltkTableMapMethod(Ihandle* ih)
{
  if (!ih->parent)
    return IUP_ERROR;

  int num_lin = ih->data->num_lin;
  int num_col = ih->data->num_col;

  IupFltkTable* table = new IupFltkTable(0, 0, 10, 10, ih);
  table->end();

  ih->handle = (InativeHandle*)table;

  table->is_virtual = iupAttribGetBoolean(ih, "VIRTUALMODE");

  int charheight;
  iupdrvFontGetCharSize(ih, NULL, &charheight);
  int row_height = charheight + 6;

  table->rows(1);
  table->row_height(0, row_height);
  table->rows(num_lin);
  table->cols(num_col);

  if (table->is_virtual)
    table->col_titles.resize(num_col);
  else
    table->resize_storage(num_lin, num_col);

  table->col_header(1);
  table->row_header(0);

  table->col_resize(ih->data->user_resize ? 1 : 0);
  table->row_resize(0);

  table->col_header_height(row_height + 2);

  for (int c = 0; c < num_col; c++)
  {
    char name[50];
    int width = 0;
    char* width_str = NULL;

    snprintf(name, sizeof(name), "RASTERWIDTH%d", c + 1);
    width_str = iupAttribGet(ih, name);
    if (!width_str)
    {
      snprintf(name, sizeof(name), "WIDTH%d", c + 1);
      width_str = iupAttribGet(ih, name);
    }

    if (width_str && iupStrToInt(width_str, &width) && width > 0)
      table->col_width(c, width);
    else
      table->col_width(c, 100);
  }

  for (int c = 0; c < num_col; c++)
  {
    char name[50];
    snprintf(name, sizeof(name), "TITLE%d", c + 1);
    char* title = iupAttribGet(ih, name);
    if (title)
      table->col_titles[c] = title;
  }

  table->type(Fl_Table_Row::SELECT_SINGLE);

  char* sel_mode = iupAttribGetStr(ih, "SELECTIONMODE");
  if (sel_mode)
  {
    if (iupStrEqualNoCase(sel_mode, "MULTIPLE") || iupStrEqualNoCase(sel_mode, "EXTENDED"))
      table->type(Fl_Table_Row::SELECT_MULTI);
    else if (iupStrEqualNoCase(sel_mode, "NONE"))
      table->type(Fl_Table_Row::SELECT_NONE);
  }

  table->show_grid = iupAttribGetBoolean(ih, "SHOWGRID");

  iupfltkUpdateWidgetFont(ih, table);
  iupfltkAddToParent(ih);

  if (!iupAttribGetBoolean(ih, "CANFOCUS"))
    iupfltkSetCanFocus(table, 0);

  int visiblelines = iupAttribGetInt(ih, "VISIBLELINES");
  if (visiblelines > 0)
  {
    int row_h = iupdrvTableGetRowHeight(ih);
    int header_h = iupdrvTableGetHeaderHeight(ih);
    int sb_size = iupdrvGetScrollbarSize();
    int border = iupdrvTableGetBorderWidth(ih);

    int visiblecolumns = iupAttribGetInt(ih, "VISIBLECOLUMNS");
    int need_horiz_sb = (visiblecolumns > 0 && num_col > visiblecolumns);
    int horiz_sb_height = need_horiz_sb ? sb_size : 0;

    int target_height = header_h + (row_h * visiblelines) + horiz_sb_height + 2 * border;
    iupAttribSetInt(ih, "_IUPFLTK_TABLE_TARGET_HEIGHT", target_height);
  }

  int visiblecolumns = iupAttribGetInt(ih, "VISIBLECOLUMNS");
  if (visiblecolumns > 0)
    iupAttribSetInt(ih, "_IUPFLTK_TABLE_VISIBLE_COLUMNS", visiblecolumns);

  return IUP_NOERROR;
}

static void fltkTableUnMapMethod(Ihandle* ih)
{
  IupFltkTable* table = fltkTableGetWidget(ih);
  if (table)
    table->iup_handle = NULL;

  iupdrvBaseUnMapMethod(ih);
}

static int fltkTableSetSortableAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    ih->data->sortable = 1;
  else
  {
    ih->data->sortable = 0;

    IupFltkTable* table = fltkTableGetWidget(ih);
    if (table)
    {
      table->sort_column = 0;
      table->sort_ascending = 1;
      table->redraw();
    }
  }
  return 0;
}

static int fltkTableSetAllowReorderAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    ih->data->allow_reorder = 1;
  else
    ih->data->allow_reorder = 0;
  return 0;
}

static int fltkTableSetUserResizeAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    ih->data->user_resize = 1;
  else
    ih->data->user_resize = 0;

  if (ih->handle)
  {
    IupFltkTable* table = fltkTableGetWidget(ih);
    table->col_resize(ih->data->user_resize ? 1 : 0);
  }
  return 0;
}

extern "C" IUP_SDK_API void iupdrvTableSetNumLin(Ihandle* ih, int num_lin)
{
  if (num_lin < 0)
    num_lin = 0;

  ih->data->num_lin = num_lin;

  if (ih->handle)
  {
    IupFltkTable* table = fltkTableGetWidget(ih);
    if (!table->is_virtual)
      table->resize_storage(num_lin, ih->data->num_col);
    table->rows(num_lin);
    table->redraw();
  }
}

extern "C" IUP_SDK_API void iupdrvTableSetNumCol(Ihandle* ih, int num_col)
{
  if (num_col < 0)
    num_col = 0;

  ih->data->num_col = num_col;

  if (ih->handle)
  {
    IupFltkTable* table = fltkTableGetWidget(ih);
    if (table->is_virtual)
      table->col_titles.resize(num_col);
    else
      table->resize_storage(ih->data->num_lin, num_col);
    table->cols(num_col);
    table->redraw();
  }
}

extern "C" IUP_SDK_API void iupdrvTableAddLin(Ihandle* ih, int pos)
{
  IupFltkTable* table = fltkTableGetWidget(ih);
  if (!table)
    return;

  if (pos == 0)
    pos = ih->data->num_lin + 1;

  if (pos < 1 || pos > ih->data->num_lin + 1)
    return;

  int idx = pos - 1;
  std::vector<std::string> new_row(ih->data->num_col);
  table->cells.insert(table->cells.begin() + idx, new_row);

  ih->data->num_lin++;
  table->rows(ih->data->num_lin);
  table->redraw();
}

extern "C" IUP_SDK_API void iupdrvTableDelLin(Ihandle* ih, int pos)
{
  IupFltkTable* table = fltkTableGetWidget(ih);
  if (!table)
    return;

  if (pos < 1 || pos > ih->data->num_lin)
    return;

  int idx = pos - 1;
  table->cells.erase(table->cells.begin() + idx);

  ih->data->num_lin--;
  table->rows(ih->data->num_lin);
  table->redraw();
}

extern "C" IUP_SDK_API void iupdrvTableAddCol(Ihandle* ih, int pos)
{
  IupFltkTable* table = fltkTableGetWidget(ih);
  if (!table)
    return;

  if (pos == 0)
    pos = ih->data->num_col + 1;

  if (pos < 1 || pos > ih->data->num_col + 1)
    return;

  int idx = pos - 1;
  for (auto& row : table->cells)
    row.insert(row.begin() + idx, std::string());

  table->col_titles.insert(table->col_titles.begin() + idx, std::string());

  ih->data->num_col++;
  table->cols(ih->data->num_col);
  table->col_width(idx, 100);
  table->redraw();
}

extern "C" IUP_SDK_API void iupdrvTableDelCol(Ihandle* ih, int pos)
{
  IupFltkTable* table = fltkTableGetWidget(ih);
  if (!table)
    return;

  if (pos < 1 || pos > ih->data->num_col)
    return;

  int idx = pos - 1;
  for (auto& row : table->cells)
  {
    if (idx < (int)row.size())
      row.erase(row.begin() + idx);
  }

  if (idx < (int)table->col_titles.size())
    table->col_titles.erase(table->col_titles.begin() + idx);

  ih->data->num_col--;
  table->cols(ih->data->num_col);
  table->redraw();
}

extern "C" IUP_SDK_API void iupdrvTableSetCellValue(Ihandle* ih, int lin, int col, const char* value)
{
  IupFltkTable* table = fltkTableGetWidget(ih);
  if (!table)
    return;

  int r = lin - 1;
  int c = col - 1;

  if (r < 0 || r >= (int)table->cells.size() ||
      c < 0 || c >= (int)table->cells[r].size())
    return;

  table->cells[r][c] = value ? value : "";
  table->redraw();
}

extern "C" IUP_SDK_API char* iupdrvTableGetCellValue(Ihandle* ih, int lin, int col)
{
  IupFltkTable* table = fltkTableGetWidget(ih);
  if (!table)
    return NULL;

  if (table->is_virtual)
  {
    sIFnii value_cb = (sIFnii)IupGetCallback(ih, "VALUE_CB");
    if (value_cb)
    {
      char* val = value_cb(ih, lin, col);
      if (val)
        return iupStrReturnStr(val);
    }
    return NULL;
  }

  int r = lin - 1;
  int c = col - 1;

  if (r < 0 || r >= (int)table->cells.size() ||
      c < 0 || c >= (int)table->cells[r].size())
    return NULL;

  if (table->cells[r][c].empty())
    return NULL;

  return iupStrReturnStr(table->cells[r][c].c_str());
}

extern "C" IUP_SDK_API void iupdrvTableSetCellImage(Ihandle* ih, int lin, int col, const char* image)
{
  IupFltkTable* table = fltkTableGetWidget(ih);
  if (!table)
    return;

  char name[50];
  snprintf(name, sizeof(name), "_CELLIMAGE%d:%d", lin, col);
  iupAttribSetStr(ih, name, image);

  table->redraw();
}

extern "C" IUP_SDK_API void iupdrvTableSetColTitle(Ihandle* ih, int col, const char* title)
{
  IupFltkTable* table = fltkTableGetWidget(ih);
  if (!table)
    return;

  int c = col - 1;
  if (c < 0 || c >= (int)table->col_titles.size())
    return;

  table->col_titles[c] = title ? title : "";
  table->redraw();
}

extern "C" IUP_SDK_API char* iupdrvTableGetColTitle(Ihandle* ih, int col)
{
  IupFltkTable* table = fltkTableGetWidget(ih);
  if (!table)
    return NULL;

  int c = col - 1;
  if (c < 0 || c >= (int)table->col_titles.size())
    return NULL;

  if (table->col_titles[c].empty())
    return NULL;

  return iupStrReturnStr(table->col_titles[c].c_str());
}

extern "C" IUP_SDK_API void iupdrvTableSetColWidth(Ihandle* ih, int col, int width)
{
  IupFltkTable* table = fltkTableGetWidget(ih);
  if (!table)
    return;

  int c = col - 1;
  if (c < 0 || c >= table->cols())
    return;

  table->col_width(c, width);
  table->redraw();
}

extern "C" IUP_SDK_API int iupdrvTableGetColWidth(Ihandle* ih, int col)
{
  IupFltkTable* table = fltkTableGetWidget(ih);
  if (!table)
    return 0;

  int c = col - 1;
  if (c < 0 || c >= table->cols())
    return 0;

  return table->col_width(c);
}

extern "C" IUP_SDK_API void iupdrvTableSetFocusCell(Ihandle* ih, int lin, int col)
{
  IupFltkTable* table = fltkTableGetWidget(ih);
  if (!table)
    return;

  int r = lin - 1;
  int c = col - 1;

  if (r < 0 || r >= table->rows() || c < 0 || c >= table->cols())
    return;

  table->focus_lin = r;
  table->focus_col = c;

  table->select_row(r, 1);
  table->redraw();
}

extern "C" IUP_SDK_API void iupdrvTableGetFocusCell(Ihandle* ih, int* lin, int* col)
{
  IupFltkTable* table = fltkTableGetWidget(ih);
  if (!table)
  {
    *lin = 0;
    *col = 0;
    return;
  }

  *lin = table->focus_lin + 1;
  *col = table->focus_col + 1;
}

extern "C" IUP_SDK_API void iupdrvTableScrollToCell(Ihandle* ih, int lin, int col)
{
  IupFltkTable* table = fltkTableGetWidget(ih);
  if (!table)
    return;

  int r = lin - 1;
  int c = col - 1;

  if (r >= 0 && r < table->rows())
    table->row_position(r);
  if (c >= 0 && c < table->cols())
    table->col_position(c);
}

extern "C" IUP_SDK_API void iupdrvTableRedraw(Ihandle* ih)
{
  IupFltkTable* table = fltkTableGetWidget(ih);
  if (table)
    table->redraw();
}

extern "C" IUP_SDK_API void iupdrvTableSetShowGrid(Ihandle* ih, int show)
{
  IupFltkTable* table = fltkTableGetWidget(ih);
  if (table)
  {
    table->show_grid = show;
    table->redraw();
  }
}

extern "C" IUP_SDK_API int iupdrvTableGetBorderWidth(Ihandle* ih)
{
  IupFltkTable* table = fltkTableGetWidget(ih);
  if (table)
    return Fl::box_dw(table->box()) / 2;
  return 2;
}

extern "C" IUP_SDK_API int iupdrvTableGetRowHeight(Ihandle* ih)
{
  IupFltkTable* table = fltkTableGetWidget(ih);
  if (table && table->rows() > 0)
  {
    int h = table->row_height(0);
    if (h > 0)
      return h;
  }

  int charheight;
  iupdrvFontGetCharSize(ih, NULL, &charheight);
  return charheight + 6;
}

extern "C" IUP_SDK_API int iupdrvTableGetHeaderHeight(Ihandle* ih)
{
  IupFltkTable* table = fltkTableGetWidget(ih);
  if (table)
    return table->col_header_height();

  int charheight;
  iupdrvFontGetCharSize(ih, NULL, &charheight);
  return charheight + 8;
}

extern "C" IUP_SDK_API void iupdrvTableAddBorders(Ihandle* ih, int* w, int* h)
{
  int sb_size = iupdrvGetScrollbarSize();
  int border = iupdrvTableGetBorderWidth(ih);

  *w += sb_size + 2 * border;
  *h += 2 * border;

  int visiblecolumns = iupAttribGetInt(ih, "VISIBLECOLUMNS");
  if (visiblecolumns > 0 && ih->data->num_col > visiblecolumns)
    *h += sb_size;
}

extern "C" IUP_SDK_API void iupdrvTableInitClass(Iclass* ic)
{
  ic->Map = fltkTableMapMethod;
  ic->UnMap = fltkTableUnMapMethod;
  ic->LayoutUpdate = fltkTableLayoutUpdateMethod;

  iupClassRegisterReplaceAttribFunc(ic, "SORTABLE", NULL, fltkTableSetSortableAttrib);
  iupClassRegisterReplaceAttribFunc(ic, "ALLOWREORDER", NULL, fltkTableSetAllowReorderAttrib);
  iupClassRegisterReplaceAttribFunc(ic, "USERRESIZE", NULL, fltkTableSetUserResizeAttrib);
}
