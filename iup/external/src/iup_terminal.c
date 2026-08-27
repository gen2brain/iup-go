/** \file
 * \brief Terminal Control.
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iup.h"
#include "iupcbs.h"
#include "iupkey.h"

#include "iup_drvdraw.h"
#include "iup_draw.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_drvinfo.h"
#include "iup_stdcontrols.h"
#include "iup_register.h"

#include "iup_terminal.h"

#define ITERM_PADDING 2
#define ITERM_PTY_RATE 25
#define ITERM_PTY_CHUNK 16384
#define ITERM_PTY_MAXREAD (256*1024)
#define ITERM_MEASURE_RUN 16
#define ITERM_DEF_FGCOLOR "229 229 229"
#define ITERM_DEF_BGCOLOR "0 0 0"

struct _IcontrolData
{
  iupCanvas canvas;  /* from IupCanvas (must reserve it) */

  Iterm term;
  int ch_w, ch_h;
  int view_offset;
  int has_focus;
  int blink_on;
  Ihandle* blink_timer;

  /* selection anchored to monotonic line ids so it survives scrolling */
  int sel_valid, sel_dragging, sel_mode;
  int sel_anchor_line, sel_anchor_col;
  int sel_start_line, sel_start_col;
  int sel_end_line, sel_end_col;
  int last_click_count;
  int drag_x, drag_y;

  char font_face[64];
  int font_size;

  ItermPty* pty;
  Ihandle* pty_timer;
  int pty_eof;
};

static unsigned int itermPackColor(const char* value, unsigned int def)
{
  unsigned char r, g, b;
  if (value && iupStrToRGB(value, &r, &g, &b))
    return ((unsigned int)r << 16) | ((unsigned int)g << 8) | (unsigned int)b;
  return def;
}

static void itermUpdateScrollbar(Ihandle* ih)
{
  Iterm* t = &ih->data->term;
  int total, pos;

  if (t->modes & ITERM_MODE_ALTSCREEN)
  {
    ih->data->view_offset = 0;
    total = t->rows;
    pos = 0;
  }
  else
  {
    total = t->sb_count + t->rows;
    if (ih->data->view_offset > t->sb_count)
      ih->data->view_offset = t->sb_count;
    pos = t->sb_count - ih->data->view_offset;
  }

  IupSetInt(ih, "YMIN", 0);
  IupSetInt(ih, "YMAX", total);
  IupSetInt(ih, "DY", t->rows);
  IupSetInt(ih, "LINEY", 1);
  IupSetInt(ih, "POSY", pos);
}

static void itermRedraw(Ihandle* ih)
{
  if (ih->handle)
    IupUpdate(ih);
}

static void itermCellColors(Ihandle* ih, ItermCell* cell, unsigned int* fg, unsigned int* bg)
{
  Iterm* t = &ih->data->term;
  unsigned int f = (cell->flags & ITERM_FL_FG_DEFAULT) ? t->def_fg : cell->fg;
  unsigned int b = (cell->flags & ITERM_FL_BG_DEFAULT) ? t->def_bg : cell->bg;

  if (cell->flags & ITERM_FL_INVERSE)
  {
    unsigned int tmp = f;
    f = b;
    b = tmp;
  }
  if (cell->flags & ITERM_FL_HIDDEN)
    f = b;
  if (cell->flags & ITERM_FL_DIM)
    f = ((f >> 1) & 0x7F7F7F);

  *fg = f;
  *bg = b;
}

static long itermDrawColor(unsigned int rgb)
{
  return iupDrawColor((unsigned char)(rgb >> 16), (unsigned char)(rgb >> 8), (unsigned char)rgb, 255);
}

static int itermUtf8Encode(unsigned int cp, char* out)
{
  if (cp == 0) { out[0] = ' '; return 1; }
  if (cp < 0x80) { out[0] = (char)cp; return 1; }
  if (cp < 0x800)
  {
    out[0] = (char)(0xC0 | (cp >> 6));
    out[1] = (char)(0x80 | (cp & 0x3F));
    return 2;
  }
  if (cp < 0x10000)
  {
    out[0] = (char)(0xE0 | (cp >> 12));
    out[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
    out[2] = (char)(0x80 | (cp & 0x3F));
    return 3;
  }
  out[0] = (char)(0xF0 | (cp >> 18));
  out[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
  out[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
  out[3] = (char)(0x80 | (cp & 0x3F));
  return 4;
}

#define ITERM_SEL_CHAR 0
#define ITERM_SEL_WORD 1
#define ITERM_SEL_LINE 2

/* monotonic id of the line shown at visible row vr */
static int itermVisibleLineId(Ihandle* ih, int vr)
{
  Iterm* t = &ih->data->term;
  int g = t->sb_count - ih->data->view_offset + vr;
  return t->sb_pushed - t->sb_count + g;
}

static ItermLine* itermLineById(Ihandle* ih, int line_id)
{
  Iterm* t = &ih->data->term;
  int rel = line_id - (t->sb_pushed - t->sb_count);
  if (rel < 0)
    return NULL;
  if (rel < t->sb_count)
    return iupTermScreenHistoryLine(t, rel);
  return iupTermScreenLine(t, rel - t->sb_count);
}

static int itermSelOrder(Ihandle* ih)
{
  if (ih->data->sel_start_line > ih->data->sel_end_line ||
      (ih->data->sel_start_line == ih->data->sel_end_line &&
       ih->data->sel_start_col > ih->data->sel_end_col))
    return 1;
  return 0;
}

static void itermSelBounds(Ihandle* ih, int* l0, int* c0, int* l1, int* c1)
{
  if (itermSelOrder(ih))
  {
    *l0 = ih->data->sel_end_line;   *c0 = ih->data->sel_end_col;
    *l1 = ih->data->sel_start_line; *c1 = ih->data->sel_start_col;
  }
  else
  {
    *l0 = ih->data->sel_start_line; *c0 = ih->data->sel_start_col;
    *l1 = ih->data->sel_end_line;   *c1 = ih->data->sel_end_col;
  }
}

static int itermCellSelected(Ihandle* ih, int line_id, int col)
{
  int l0, c0, l1, c1;
  if (!ih->data->sel_valid)
    return 0;
  itermSelBounds(ih, &l0, &c0, &l1, &c1);
  if (line_id < l0 || line_id > l1)
    return 0;
  if (line_id == l0 && col < c0)
    return 0;
  if (line_id == l1 && col >= c1)
    return 0;
  return 1;
}

static int itermIsWordChar(unsigned int cp)
{
  if (cp == 0)
    return 0;
  if (cp >= 0x80)
    return 1;
  return (cp >= '0' && cp <= '9') || (cp >= 'A' && cp <= 'Z') ||
         (cp >= 'a' && cp <= 'z') || cp == '_';
}

static void itermSelectWord(Ihandle* ih, int line_id, int col)
{
  ItermLine* line = itermLineById(ih, line_id);
  int a = col, b = col;
  if (!line || col >= line->ncells)
    return;
  if (!itermIsWordChar(line->cells[col].cp))
  {
    ih->data->sel_start_line = ih->data->sel_end_line = line_id;
    ih->data->sel_start_col = col;
    ih->data->sel_end_col = col + 1;
    return;
  }
  while (a > 0 && itermIsWordChar(line->cells[a - 1].cp)) a--;
  while (b + 1 < line->ncells && itermIsWordChar(line->cells[b + 1].cp)) b++;
  ih->data->sel_start_line = ih->data->sel_end_line = line_id;
  ih->data->sel_start_col = a;
  ih->data->sel_end_col = b + 1;
}

static void itermSelectLine(Ihandle* ih, int line_id)
{
  ih->data->sel_start_line = ih->data->sel_end_line = line_id;
  ih->data->sel_start_col = 0;
  ih->data->sel_end_col = ih->data->term.cols;
}

static char* itermSelectionText(Ihandle* ih)
{
  Iterm* t = &ih->data->term;
  int l0, c0, l1, c1, line_id;
  int cap, len = 0;
  char* out;

  if (!ih->data->sel_valid)
    return NULL;

  itermSelBounds(ih, &l0, &c0, &l1, &c1);
  cap = (l1 - l0 + 1) * (t->cols * 4 + 2) + 1;
  out = (char*)malloc(cap);
  if (!out)
    return NULL;

  for (line_id = l0; line_id <= l1; line_id++)
  {
    ItermLine* line = itermLineById(ih, line_id);
    int from = (line_id == l0) ? c0 : 0;
    int to = (line_id == l1) ? c1 : t->cols;
    int c, last = from - 1;

    if (!line)
      continue;
    if (to > line->ncells) to = line->ncells;

    for (c = from; c < to; c++)
      if (line->cells[c].cp != 0)
        last = c;

    for (c = from; c <= last; c++)
    {
      if (line->cells[c].flags & ITERM_FL_WIDECONT)
        continue;
      len += itermUtf8Encode(line->cells[c].cp, out + len);
    }

    if (line_id != l1 && !line->wrapped)
      out[len++] = '\n';
  }

  out[len] = 0;
  return out;
}

static void itermSetClipboard(Ihandle* ih, const char* text)
{
  Ihandle* clip;
  if (!text)
    return;
  clip = IupClipboard();
  if (!clip)
    return;
  IupSetStrAttribute(clip, "TEXT", text);
  IupDestroy(clip);
}

static char* itermGetClipboard(void)
{
  Ihandle* clip = IupClipboard();
  char* text;
  char* copy = NULL;
  if (!clip)
    return NULL;
  text = IupGetAttribute(clip, "TEXT");
  if (text)
    copy = iupStrDup(text);
  IupDestroy(clip);
  return copy;
}

static void itermCopySelection(Ihandle* ih)
{
  char* text = itermSelectionText(ih);
  if (text)
  {
    itermSetClipboard(ih, text);
    free(text);
  }
}

static ItermLine* itermVisibleLine(Ihandle* ih, int vr)
{
  Iterm* t = &ih->data->term;
  int g = t->sb_count - ih->data->view_offset + vr;
  if (g < t->sb_count)
    return iupTermScreenHistoryLine(t, g);
  return iupTermScreenLine(t, g - t->sb_count);
}


static void itermRunFont(Ihandle* ih, unsigned short flags, char* font)
{
  sprintf(font, "%s, %s%s%s%s%d", ih->data->font_face,
          (flags & ITERM_FL_BOLD) ? "Bold " : "",
          (flags & ITERM_FL_ITALIC) ? "Italic " : "",
          (flags & ITERM_FL_UNDERLINE) ? "Underline " : "",
          (flags & ITERM_FL_STRIKE) ? "Strikeout " : "",
          ih->data->font_size);
}

static void itermDrawCursor(Ihandle* ih, IdrawCanvas* dc)
{
  Iterm* t = &ih->data->term;
  int vr, x, y;
  ItermLine* line;
  ItermCell* cell;
  unsigned int fg, bg;
  long color;

  if (!(t->modes & ITERM_MODE_CURSORVIS) || ih->data->view_offset != 0)
    return;

  vr = t->cy;
  x = ITERM_PADDING + t->cx * ih->data->ch_w;
  y = ITERM_PADDING + vr * ih->data->ch_h;

  line = iupTermScreenLine(t, t->cy);
  cell = &line->cells[t->cx];
  itermCellColors(ih, cell, &fg, &bg);
  color = itermDrawColor((cell->flags & ITERM_FL_FG_DEFAULT) ? t->def_fg : fg);

  if (!ih->data->has_focus)
  {
    iupdrvDrawRectangle(dc, x, y, x + ih->data->ch_w - 1, y + ih->data->ch_h - 1,
                        color, IUP_DRAW_STROKE, 1);
    return;
  }

  if ((t->modes & ITERM_MODE_CURSORBLK) && !ih->data->blink_on)
    return;

  switch (t->cursor_style)
  {
  case ITERM_CURSOR_BAR:
    iupdrvDrawRectangle(dc, x, y, x + 1, y + ih->data->ch_h - 1, color, IUP_DRAW_FILL, 1);
    break;
  case ITERM_CURSOR_UNDERLINE:
    iupdrvDrawRectangle(dc, x, y + ih->data->ch_h - 2, x + ih->data->ch_w - 1,
                        y + ih->data->ch_h - 1, color, IUP_DRAW_FILL, 1);
    break;
  default:
    {
      int w = (cell->flags & ITERM_FL_WIDE) ? 2 * ih->data->ch_w : ih->data->ch_w;
      char text[8], font[128];
      int len;
      iupdrvDrawRectangle(dc, x, y, x + w - 1, y + ih->data->ch_h - 1, color, IUP_DRAW_FILL, 1);
      if (cell->cp)
      {
        len = itermUtf8Encode(cell->cp, text);
        text[len] = 0;
        itermRunFont(ih, cell->flags, font);
        iupdrvDrawText(dc, text, len, x, y, w, ih->data->ch_h,
                       itermDrawColor(bg), font, IUP_DRAW_LEFT, 0);
      }
    }
    break;
  }
}

static int itermRedraw_CB(Ihandle* ih)
{
  Iterm* t = &ih->data->term;
  IdrawCanvas* dc = iupdrvDrawCreateCanvas(ih);
  int w, h, vr;
  char* text;
  char font[128];
  int restore_utf8mode = 0;

  /* cell text is always UTF-8 */
  if (!IupGetInt(NULL, "UTF8MODE"))
  {
    IupSetGlobal("UTF8MODE", "YES");
    restore_utf8mode = 1;
  }

  iupdrvDrawGetSize(dc, &w, &h);
  iupdrvDrawRectangle(dc, 0, 0, w - 1, h - 1, itermDrawColor(t->def_bg), IUP_DRAW_FILL, 1);

  text = (char*)malloc(t->cols * 4 + 1);
  if (!text)
  {
    iupdrvDrawKillCanvas(dc);
    return IUP_DEFAULT;
  }

  for (vr = 0; vr < t->rows; vr++)
  {
    ItermLine* line = itermVisibleLine(ih, vr);
    int line_id = itermVisibleLineId(ih, vr);
    int y = ITERM_PADDING + vr * ih->data->ch_h;
    int c = 0, ncells;

    if (!line)
      continue;
    ncells = line->ncells < t->cols ? line->ncells : t->cols;

    while (c < ncells)
    {
      ItermCell* cell = &line->cells[c];
      unsigned int fg, bg;
      unsigned short style = (unsigned short)(cell->flags & ITERM_FL_STYLEMASK);
      int start = c, len = 0, run_cells, has_glyph = 0;
      int x = ITERM_PADDING + c * ih->data->ch_w;

      itermCellColors(ih, cell, &fg, &bg);
      if (itermCellSelected(ih, line_id, c))
      {
        unsigned int sw = fg; fg = bg; bg = sw;
      }

      while (c < ncells)
      {
        ItermCell* rc = &line->cells[c];
        unsigned int rfg, rbg;
        if ((unsigned short)(rc->flags & ITERM_FL_STYLEMASK) != style)
          break;
        itermCellColors(ih, rc, &rfg, &rbg);
        if (itermCellSelected(ih, line_id, c))
        {
          unsigned int sw = rfg; rfg = rbg; rbg = sw;
        }
        if (rfg != fg || rbg != bg)
          break;
        if (!(rc->flags & ITERM_FL_WIDECONT))
        {
          len += itermUtf8Encode(rc->cp, text + len);
          if (rc->cp)
            has_glyph = 1;
        }
        c++;
      }
      run_cells = c - start;

      if (bg != t->def_bg)
        iupdrvDrawRectangle(dc, x, y, x + run_cells * ih->data->ch_w - 1,
                            y + ih->data->ch_h - 1, itermDrawColor(bg), IUP_DRAW_FILL, 1);

      if (has_glyph || (style & (ITERM_FL_UNDERLINE | ITERM_FL_STRIKE)))
      {
        text[len] = 0;
        itermRunFont(ih, style, font);
        iupdrvDrawText(dc, text, len, x, y, run_cells * ih->data->ch_w, ih->data->ch_h,
                       itermDrawColor(fg), font, IUP_DRAW_LEFT, 0);
      }
    }
  }

  free(text);

  itermDrawCursor(ih, dc);

  if (restore_utf8mode)
    IupSetGlobal("UTF8MODE", "NO");

  memset(t->dirty, 0, t->rows);
  t->all_dirty = 0;

  iupdrvDrawFlush(dc);
  iupdrvDrawKillCanvas(dc);
  return IUP_DEFAULT;
}

static void itermSendInput(Ihandle* ih, const char* bytes, int len)
{
  IFnsi cb;

  if (ih->data->pty)
  {
    iupTermPtyWrite(ih->data->pty, bytes, len);
    return;
  }

  cb = (IFnsi)IupGetCallback(ih, "INPUT_CB");
  if (cb)
    cb(ih, (char*)bytes, len);
}

static void itermXY2Cell(Ihandle* ih, int x, int y, int* col, int* vr)
{
  int c = (x - ITERM_PADDING) / ih->data->ch_w;
  int r = (y - ITERM_PADDING) / ih->data->ch_h;
  if (c < 0) c = 0;
  if (c > ih->data->term.cols) c = ih->data->term.cols;
  if (r < 0) r = 0;
  if (r > ih->data->term.rows - 1) r = ih->data->term.rows - 1;
  *col = c;
  *vr = r;
}

static int itermMouseReporting(Ihandle* ih)
{
  return (ih->data->term.modes & ITERM_MODE_MOUSE_CLK) != 0;
}

/* xterm mouse report; SGR (1006) when the app asked for it, else the legacy form */
static void itermSendMouse(Ihandle* ih, int btn, int press, int col, int vr)
{
  Iterm* t = &ih->data->term;
  char buf[40];
  int len;

  if (col >= t->cols) col = t->cols - 1;

  if (t->modes & ITERM_MODE_MOUSE_SGR)
    len = sprintf(buf, "\033[<%d;%d;%d%c", btn, col + 1, vr + 1, press ? 'M' : 'm');
  else
  {
    if (!press)
      btn = 3;
    if (col > 222 || vr > 222)
      return;
    len = sprintf(buf, "\033[M%c%c%c", (char)(32 + btn), (char)(33 + col), (char)(33 + vr));
  }
  itermSendInput(ih, buf, len);
}

static void itermModelResponse(void* user, const char* bytes, int len)
{
  itermSendInput((Ihandle*)user, bytes, len);
}

static void itermModelBell(void* user)
{
  Ihandle* ih = (Ihandle*)user;
  Icallback cb = IupGetCallback(ih, "BELL_CB");
  if (cb)
    cb(ih);
}

/* OSC 52 lets whatever is running in the terminal write the system clipboard, so it stays opt-in */
static void itermModelClipboard(void* user, const char* text)
{
  Ihandle* ih = (Ihandle*)user;
  Ihandle* clip;

  if (!iupAttribGetBoolean(ih, "ALLOWOSC52"))
    return;

  clip = IupClipboard();
  if (!clip)
    return;

  IupSetStrAttribute(clip, "TEXT", text);
  IupDestroy(clip);
}

static void itermModelTitle(void* user, const char* title)
{
  Ihandle* ih = (Ihandle*)user;
  IFns cb = (IFns)IupGetCallback(ih, "TITLE_CB");
  if (cb)
    cb(ih, (char*)title);
}

static void itermAfterOutput(Ihandle* ih)
{
  Iterm* t = &ih->data->term;

  if (ih->data->view_offset != 0 && iupAttribGetBoolean(ih, "SCROLLONOUTPUT"))
    ih->data->view_offset = 0;

  itermUpdateScrollbar(ih);
  itermRedraw(ih);

  if (t->sb_count == 0 && ih->data->view_offset > 0)
    ih->data->view_offset = 0;
}

static void itermUpdateGrid(Ihandle* ih)
{
  Iterm* t = &ih->data->term;
  int width = 0, height = 0, cols, rows;

  /* DRAWSIZE is in canvas coordinates, RESIZE_CB reports HW pixels */
  IupGetIntInt(ih, "DRAWSIZE", &width, &height);
  if (width <= 0 || height <= 0)
    return;

  cols = (width - 2 * ITERM_PADDING) / ih->data->ch_w;
  rows = (height - 2 * ITERM_PADDING) / ih->data->ch_h;

  /* before the first layout there is no room for a grid, resizing to it would drop the contents */
  if (cols < 2 || rows < 1)
    return;

  if (cols != t->cols || rows != t->rows)
  {
    IFnii cb;
    iupTermScreenResize(t, cols, rows);
    itermUpdateScrollbar(ih);
    if (ih->data->pty)
      iupTermPtyResize(ih->data->pty, cols, rows);
    cb = (IFnii)IupGetCallback(ih, "TERMSIZE_CB");
    if (cb)
      cb(ih, cols, rows);
  }
}

static int itermResize_CB(Ihandle* ih, int width, int height)
{
  (void)width;
  (void)height;
  itermUpdateGrid(ih);
  return IUP_DEFAULT;
}

static int itermScroll_CB(Ihandle* ih, int op, float posx, float posy)
{
  Iterm* t = &ih->data->term;
  (void)op;
  (void)posx;
  if (!(t->modes & ITERM_MODE_ALTSCREEN))
  {
    int pos = (int)(posy + 0.5f);
    ih->data->view_offset = t->sb_count - pos;
    if (ih->data->view_offset < 0) ih->data->view_offset = 0;
    if (ih->data->view_offset > t->sb_count) ih->data->view_offset = t->sb_count;
  }
  itermRedraw(ih);
  return IUP_DEFAULT;
}

static int itermFocus_CB(Ihandle* ih, int focus)
{
  ih->data->has_focus = focus;
  ih->data->blink_on = 1;
  if (ih->data->blink_timer)
    IupSetAttribute(ih->data->blink_timer, "RUN", focus ? "YES" : "NO");
  itermRedraw(ih);
  return IUP_DEFAULT;
}

static int itermBlinkTimer_CB(Ihandle* timer)
{
  Ihandle* ih = (Ihandle*)iupAttribGet(timer, "_IUPTERM_IH");
  Iterm* t = &ih->data->term;
  if ((t->modes & ITERM_MODE_CURSORBLK) && (t->modes & ITERM_MODE_CURSORVIS))
  {
    ih->data->blink_on = !ih->data->blink_on;
    itermRedraw(ih);
  }
  else
    ih->data->blink_on = 1;
  return IUP_DEFAULT;
}

static void itermScrollOnKey(Ihandle* ih)
{
  if (ih->data->view_offset != 0 && iupAttribGetBoolean(ih, "SCROLLONKEY"))
  {
    ih->data->view_offset = 0;
    itermUpdateScrollbar(ih);
    itermRedraw(ih);
  }
}

static void itermPasteText(Ihandle* ih, const char* text)
{
  Iterm* t = &ih->data->term;
  int len;
  if (!text || !text[0])
    return;
  len = (int)strlen(text);
  if (t->modes & ITERM_MODE_BRACKPASTE)
  {
    itermSendInput(ih, "\033[200~", 6);
    itermSendInput(ih, text, len);
    itermSendInput(ih, "\033[201~", 6);
  }
  else
    itermSendInput(ih, text, len);
  itermScrollOnKey(ih);
}

static int itermButton_CB(Ihandle* ih, int button, int pressed, int x, int y, char* status)
{
  int col, vr, line_id;
  int shift = iup_isshift(status);

  if (!ih->data->has_focus)
    IupSetFocus(ih);

  itermXY2Cell(ih, x, y, &col, &vr);
  line_id = itermVisibleLineId(ih, vr);

  if (button == IUP_BUTTON2 && pressed && !itermMouseReporting(ih))
  {
    char* text = itermGetClipboard();
    if (text)
    {
      itermPasteText(ih, text);
      free(text);
    }
    return IUP_DEFAULT;
  }

  /* the app owns the mouse unless Shift is held, matching xterm */
  if (itermMouseReporting(ih) && !shift)
  {
    if (button >= IUP_BUTTON1 && button <= IUP_BUTTON3)
      itermSendMouse(ih, button - IUP_BUTTON1, pressed, col, vr);
    return IUP_DEFAULT;
  }

  if (button != IUP_BUTTON1)
    return IUP_DEFAULT;

  if (pressed)
  {
    int dbl = iup_isdouble(status);

    /* a second double-click inside the current word selection extends it to the line */
    if (!dbl)
      ih->data->last_click_count = 1;
    else if (ih->data->last_click_count >= 2 && ih->data->sel_mode == ITERM_SEL_WORD &&
             ih->data->sel_start_line == line_id)
      ih->data->last_click_count = 3;
    else
      ih->data->last_click_count = 2;

    ih->data->sel_valid = 1;
    if (ih->data->last_click_count == 3)
    {
      ih->data->sel_mode = ITERM_SEL_LINE;
      itermSelectLine(ih, line_id);
      ih->data->sel_dragging = 0;
    }
    else if (ih->data->last_click_count == 2)
    {
      ih->data->sel_mode = ITERM_SEL_WORD;
      itermSelectWord(ih, line_id, col < ih->data->term.cols ? col : ih->data->term.cols - 1);
      ih->data->sel_dragging = 0;
    }
    else if (shift && ih->data->sel_valid)
    {
      ih->data->sel_end_line = line_id;
      ih->data->sel_end_col = col;
      ih->data->sel_dragging = 1;
    }
    else
    {
      ih->data->sel_anchor_line = ih->data->sel_start_line = ih->data->sel_end_line = line_id;
      ih->data->sel_anchor_col = ih->data->sel_start_col = ih->data->sel_end_col = col;
      ih->data->sel_valid = 0;
      ih->data->sel_dragging = 1;
    }
    itermRedraw(ih);
  }
  else
  {
    ih->data->sel_dragging = 0;
    if (ih->data->sel_valid && iupAttribGetBoolean(ih, "AUTOCOPY"))
      itermCopySelection(ih);
  }

  return IUP_DEFAULT;
}

static int itermMotion_CB(Ihandle* ih, int x, int y, char* status)
{
  int col, vr;

  itermXY2Cell(ih, x, y, &col, &vr);

  if (itermMouseReporting(ih) && !iup_isshift(status))
  {
    if ((ih->data->term.modes & ITERM_MODE_MOUSE_DRAG) && iup_isbutton1(status))
      itermSendMouse(ih, 32, 1, col, vr);
    return IUP_DEFAULT;
  }

  if (!ih->data->sel_dragging || !iup_isbutton1(status))
    return IUP_DEFAULT;

  ih->data->sel_end_line = itermVisibleLineId(ih, vr);
  ih->data->sel_end_col = col;
  ih->data->sel_valid = (ih->data->sel_end_line != ih->data->sel_anchor_line ||
                         ih->data->sel_end_col != ih->data->sel_anchor_col);
  ih->data->sel_start_line = ih->data->sel_anchor_line;
  ih->data->sel_start_col = ih->data->sel_anchor_col;
  itermRedraw(ih);
  return IUP_DEFAULT;
}

static int itermWheel_CB(Ihandle* ih, float delta, int x, int y, char* status)
{
  Iterm* t = &ih->data->term;
  int lines = (int)(delta > 0 ? delta : -delta) * 3;
  int i;
  (void)x; (void)y; (void)status;

  if (lines < 1) lines = 1;

  /* on the alternate screen there is no history: feed arrows like xterm does */
  if (t->modes & ITERM_MODE_ALTSCREEN)
  {
    const char* seq = (t->modes & ITERM_MODE_CKM)
                      ? (delta > 0 ? "\033OA" : "\033OB")
                      : (delta > 0 ? "\033[A" : "\033[B");
    for (i = 0; i < lines; i++)
      itermSendInput(ih, seq, 3);
    return IUP_DEFAULT;
  }

  ih->data->view_offset += (delta > 0) ? lines : -lines;
  if (ih->data->view_offset < 0) ih->data->view_offset = 0;
  if (ih->data->view_offset > t->sb_count) ih->data->view_offset = t->sb_count;
  itermUpdateScrollbar(ih);
  itermRedraw(ih);
  return IUP_DEFAULT;
}

static int itermTextInput_CB(Ihandle* ih, char* value)
{
  itermSendInput(ih, value, (int)strlen(value));
  itermScrollOnKey(ih);
  return IUP_IGNORE;
}

static int itermKAny_CB(Ihandle* ih, int c)
{
  Iterm* t = &ih->data->term;
  char buf[16];
  int len = 0;
  int base = c & 0x0FFFFFFF;
  int ctrl = iup_isCtrlXkey(c);
  int alt = iup_isAltXkey(c);
  int shift = iup_isShiftXkey(c);
  int mod = 1 + (shift ? 1 : 0) + (alt ? 2 : 0) + (ctrl ? 4 : 0);
  const char* seq = NULL;

  switch (base)
  {
  case K_UP:    seq = "A"; break;
  case K_DOWN:  seq = "B"; break;
  case K_RIGHT: seq = "C"; break;
  case K_LEFT:  seq = "D"; break;
  case K_HOME:  seq = "H"; break;
  case K_END:   seq = "F"; break;
  }
  if (seq)
  {
    if (mod > 1)
      len = sprintf(buf, "\033[1;%d%s", mod, seq);
    else if (t->modes & ITERM_MODE_CKM)
      len = sprintf(buf, "\033O%s", seq);
    else
      len = sprintf(buf, "\033[%s", seq);
    itermSendInput(ih, buf, len);
    return IUP_IGNORE;
  }

  {
    int num = 0;
    switch (base)
    {
    case K_INS:  num = 2; break;
    case K_DEL:  num = 3; break;
    case K_PGUP: num = 5; break;
    case K_PGDN: num = 6; break;
    case K_F1: case K_F2: case K_F3: case K_F4:
      if (mod > 1)
        len = sprintf(buf, "\033[1;%d%c", mod, 'P' + (base - K_F1));
      else
        len = sprintf(buf, "\033O%c", 'P' + (base - K_F1));
      itermSendInput(ih, buf, len);
      return IUP_IGNORE;
    case K_F5:  num = 15; break;
    case K_F6:  num = 17; break;
    case K_F7:  num = 18; break;
    case K_F8:  num = 19; break;
    case K_F9:  num = 20; break;
    case K_F10: num = 21; break;
    case K_F11: num = 23; break;
    case K_F12: num = 24; break;
    }
    if (num)
    {
      if (mod > 1)
        len = sprintf(buf, "\033[%d;%d~", num, mod);
      else
        len = sprintf(buf, "\033[%d~", num);
      itermSendInput(ih, buf, len);
      return IUP_IGNORE;
    }
  }

  if (ctrl && shift && (base == 'C' || base == 'c'))
  {
    itermCopySelection(ih);
    return IUP_IGNORE;
  }
  if (ctrl && shift && (base == 'V' || base == 'v'))
  {
    char* text = itermGetClipboard();
    if (text)
    {
      itermPasteText(ih, text);
      free(text);
    }
    return IUP_IGNORE;
  }

  switch (base)
  {
  case K_CR:  buf[len++] = '\r'; break;
  case K_BS:  buf[len++] = 0x7F; break;
  case K_TAB:
    if (shift)
      len = sprintf(buf, "\033[Z");
    else
      buf[len++] = '\t';
    break;
  case K_ESC: buf[len++] = 0x1B; break;
  case K_SP:
    if (ctrl) buf[len++] = 0;
    else buf[len++] = ' ';
    break;
  default:
    if (ctrl && base >= 'a' && base <= 'z')
      buf[len++] = (char)(base - 'a' + 1);
    else if (ctrl && base >= 'A' && base <= 'Z')
      buf[len++] = (char)(base - 'A' + 1);
    else if (ctrl && base >= 1 && base <= 26)
      buf[len++] = (char)base;
    else if (base >= 32 && base < 127 && !ctrl)
    {
      if (alt)
      {
        buf[len++] = 0x1B;
        if (base >= 'A' && base <= 'Z' && !shift)
          base += 'a' - 'A';
      }
      buf[len++] = (char)base;
    }
    break;
  }

  if (len > 0)
  {
    itermSendInput(ih, buf, len);
    itermScrollOnKey(ih);
    return IUP_IGNORE;
  }

  return IUP_CONTINUE;
}

/* a run minus a single character recovers the fractional advance that per-string rounding loses */
static int itermMeasureAdvance(Ihandle* ih, char c, int* height)
{
  char run_font[128];
  char probe[ITERM_MEASURE_RUN + 1];
  char single[2];
  int w1 = 0, wn = 0, h = 0;

  itermRunFont(ih, 0, run_font);
  memset(probe, c, ITERM_MEASURE_RUN);
  probe[ITERM_MEASURE_RUN] = 0;
  single[0] = c;
  single[1] = 0;

  iupdrvFontGetTextSize(run_font, single, 1, &w1, &h);
  iupdrvFontGetTextSize(run_font, probe, ITERM_MEASURE_RUN, &wn, NULL);

  if (height)
    *height = h;

  return (wn > w1) ? (wn - w1 + (ITERM_MEASURE_RUN - 2) / 2) / (ITERM_MEASURE_RUN - 1) : w1;
}

static int itermFontIsMonospace(Ihandle* ih, int* advance, int* height)
{
  int wide = itermMeasureAdvance(ih, 'W', height);
  int narrow = itermMeasureAdvance(ih, 'i', NULL);

  *advance = wide;
  return wide > 0 && wide == narrow;
}

static void itermUpdateFontInfo(Ihandle* ih, const char* font_value)
{
  char typeface[64] = "Courier";
  int size = 10, b, i, u, s;
  int advance = 0, height = 0;
  const char* font = font_value ? font_value : IupGetAttribute(ih, "FONT");

  if (font && iupGetFontInfo(font, typeface, &size, &b, &i, &u, &s))
  {
    strcpy(ih->data->font_face, typeface);
    ih->data->font_size = size;
  }
  else
  {
    strcpy(ih->data->font_face, "Courier");
    ih->data->font_size = 10;
  }

  /* a proportional face would put every glyph out of its cell */
  if (!itermFontIsMonospace(ih, &advance, &height))
  {
    char requested[64];

    strcpy(requested, ih->data->font_face);
    strcpy(ih->data->font_face, "Monospace");

    if (!itermFontIsMonospace(ih, &advance, &height))
    {
      strcpy(ih->data->font_face, requested);
      advance = itermMeasureAdvance(ih, 'W', &height);
    }
  }

  ih->data->ch_w = advance;
  ih->data->ch_h = height;
  if (ih->data->ch_w <= 0) ih->data->ch_w = 8;
  if (ih->data->ch_h <= 0) ih->data->ch_h = 16;
}


/*****************************************************************************/


static int itermSetWriteAttrib(Ihandle* ih, const char* value)
{
  if (value)
  {
    iupTermParseBytes(&ih->data->term, value, (int)strlen(value));
    itermAfterOutput(ih);
  }
  return 0;
}

static int itermSetAppendAttrib(Ihandle* ih, const char* value)
{
  if (value)
  {
    iupTermParseBytes(&ih->data->term, value, (int)strlen(value));
    iupTermParseBytes(&ih->data->term, "\r\n", 2);
    itermAfterOutput(ih);
  }
  return 0;
}

static char* itermGetColumnsAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->term.cols);
}

static char* itermGetLinesAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->term.rows);
}

static int itermSetFgColorAttrib(Ihandle* ih, const char* value)
{
  Iterm* t = &ih->data->term;
  t->def_fg = itermPackColor(value, t->def_fg);
  iupTermScreenDamageAll(t);
  itermRedraw(ih);
  return 1;
}

static int itermSetBgColorAttrib(Ihandle* ih, const char* value)
{
  Iterm* t = &ih->data->term;
  t->def_bg = itermPackColor(value, t->def_bg);
  iupTermScreenDamageAll(t);
  itermRedraw(ih);
  return 1;
}

static int itermSetColorAttrib(Ihandle* ih, int index, const char* value)
{
  Iterm* t = &ih->data->term;
  if (index >= 0 && index < 16)
  {
    t->palette[index] = itermPackColor(value, t->palette[index]);
    itermRedraw(ih);
  }
  return 1;
}

static char* itermGetColorAttrib(Ihandle* ih, int index)
{
  Iterm* t = &ih->data->term;
  if (index >= 0 && index < 16)
  {
    unsigned int c = t->palette[index];
    return iupStrReturnStrf("%d %d %d", (c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF);
  }
  return NULL;
}

static int itermSetCursorStyleAttrib(Ihandle* ih, const char* value)
{
  Iterm* t = &ih->data->term;
  if (iupStrEqualNoCase(value, "BAR"))
    t->cursor_style = ITERM_CURSOR_BAR;
  else if (iupStrEqualNoCase(value, "UNDERLINE"))
    t->cursor_style = ITERM_CURSOR_UNDERLINE;
  else
    t->cursor_style = ITERM_CURSOR_BLOCK;
  itermRedraw(ih);
  return 1;
}

static char* itermGetCursorStyleAttrib(Ihandle* ih)
{
  switch (ih->data->term.cursor_style)
  {
  case ITERM_CURSOR_BAR: return "BAR";
  case ITERM_CURSOR_UNDERLINE: return "UNDERLINE";
  }
  return "BLOCK";
}

static int itermSetCursorBlinkAttrib(Ihandle* ih, const char* value)
{
  Iterm* t = &ih->data->term;
  if (iupStrBoolean(value))
    t->modes |= ITERM_MODE_CURSORBLK;
  else
    t->modes &= ~ITERM_MODE_CURSORBLK;
  ih->data->blink_on = 1;
  itermRedraw(ih);
  return 1;
}

static char* itermGetCursorBlinkAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->term.modes & ITERM_MODE_CURSORBLK);
}

static int itermSetScrollbackLinesAttrib(Ihandle* ih, const char* value)
{
  Iterm* t = &ih->data->term;
  int lines;
  if (iupStrToInt(value, &lines) && lines >= 0 && lines != t->sb_max)
  {
    iupTermScreenClearScrollback(t);
    free(t->sb);
    t->sb = lines > 0 ? (ItermLine*)malloc(lines * sizeof(ItermLine)) : NULL;
    t->sb_max = lines;
    ih->data->view_offset = 0;
    itermUpdateScrollbar(ih);
    itermRedraw(ih);
  }
  return 0;
}

static char* itermGetScrollbackLinesAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->term.sb_max);
}

static char* itermGetSelectedTextAttrib(Ihandle* ih)
{
  char* text = itermSelectionText(ih);
  char* ret;
  if (!text)
    return NULL;
  ret = iupStrReturnStr(text);
  free(text);
  return ret;
}

static int itermSetCopySelectionAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  itermCopySelection(ih);
  return 0;
}

static int itermSetPasteAttrib(Ihandle* ih, const char* value)
{
  if (value && value[0])
    itermPasteText(ih, value);
  else
  {
    char* text = itermGetClipboard();
    if (text)
    {
      itermPasteText(ih, text);
      free(text);
    }
  }
  return 0;
}

static int itermSetSelectAllAttrib(Ihandle* ih, const char* value)
{
  Iterm* t = &ih->data->term;
  (void)value;
  ih->data->sel_valid = 1;
  ih->data->sel_mode = ITERM_SEL_LINE;
  ih->data->sel_start_line = t->sb_pushed - t->sb_count;
  ih->data->sel_start_col = 0;
  ih->data->sel_end_line = t->sb_pushed + t->rows - 1;
  ih->data->sel_end_col = t->cols;
  itermRedraw(ih);
  return 0;
}

static int itermSetSelectionAttrib(Ihandle* ih, const char* value)
{
  if (!value || !value[0])
  {
    ih->data->sel_valid = 0;
    itermRedraw(ih);
  }
  return 0;
}

static int itermSetResetAttrib(Ihandle* ih, const char* value)
{
  Iterm* t = &ih->data->term;
  (void)value;
  iupTermScreenClearScrollback(t);
  iupTermScreenReset(t);
  ih->data->sel_valid = 0;
  ih->data->view_offset = 0;
  itermUpdateScrollbar(ih);
  itermRedraw(ih);
  return 0;
}

static int itermSetClearScreenAttrib(Ihandle* ih, const char* value)
{
  Iterm* t = &ih->data->term;
  (void)value;
  iupTermScreenEraseDisplay(t, 2);
  iupTermScreenSetCursor(t, 0, 0);
  itermRedraw(ih);
  return 0;
}

static int itermSetFontAttrib(Ihandle* ih, const char* value)
{
  if (!iupdrvSetFontAttrib(ih, value))
    return 0;
  if (ih->handle)
  {
    itermUpdateFontInfo(ih, value);
    itermUpdateGrid(ih);
    iupTermScreenDamageAll(&ih->data->term);
    itermRedraw(ih);
  }
  return 1;
}


/*****************************************************************************/


static void itermPtyDetach(Ihandle* ih)
{
  if (ih->data->pty_timer)
    IupSetAttribute(ih->data->pty_timer, "RUN", "NO");

  if (ih->data->pty)
  {
    iupTermPtyClose(ih->data->pty);
    ih->data->pty = NULL;
  }
  ih->data->pty_eof = 0;
}

static int itermPtyTimer_CB(Ihandle* timer)
{
  Ihandle* ih = (Ihandle*)iupAttribGet(timer, "_IUPTERM_IH");
  char buf[ITERM_PTY_CHUNK];
  int total = 0;

  if (!ih || !ih->data->pty)
    return IUP_DEFAULT;

  while (!ih->data->pty_eof)
  {
    int n = iupTermPtyRead(ih->data->pty, buf, sizeof(buf));
    if (n > 0)
    {
      iupTermParseBytes(&ih->data->term, buf, n);
      total += n;
      if (total >= ITERM_PTY_MAXREAD)
        break;
      continue;
    }
    if (n < 0)
      ih->data->pty_eof = 1;
    break;
  }

  if (total)
    itermAfterOutput(ih);

  /* the child can still be running after it closed the terminal, so keep polling for its status */
  if (ih->data->pty_eof)
  {
    int status = 0;
    if (iupTermPtyCheckExit(ih->data->pty, &status))
    {
      IFni cb;
      itermPtyDetach(ih);
      cb = (IFni)IupGetCallback(ih, "EXIT_CB");
      if (cb)
        cb(ih, status);
    }
  }

  return IUP_DEFAULT;
}

static int itermSetExecAttrib(Ihandle* ih, const char* value)
{
  char error[256] = "";
  ItermPty* pty;

  itermPtyDetach(ih);
  iupAttribSet(ih, "_IUPTERM_PTYERROR", NULL);

  pty = iupTermPtyStart(value, IupGetAttribute(ih, "TERMNAME"),
                        ih->data->term.cols, ih->data->term.rows, error, sizeof(error));
  if (!pty)
  {
    iupAttribSetStr(ih, "_IUPTERM_PTYERROR", error);
    return 0;
  }

  ih->data->pty = pty;

  if (!ih->data->pty_timer)
  {
    ih->data->pty_timer = IupTimer();
    IupSetInt(ih->data->pty_timer, "TIME", ITERM_PTY_RATE);
    iupAttribSet(ih->data->pty_timer, "_IUPTERM_IH", (char*)ih);
    IupSetCallback(ih->data->pty_timer, "ACTION_CB", (Icallback)itermPtyTimer_CB);
  }
  IupSetAttribute(ih->data->pty_timer, "RUN", "YES");

  return 0;
}

static int itermSetKillAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->pty)
    iupTermPtyKill(ih->data->pty, iupStrEqualNoCase(value, "FORCE"));
  return 0;
}

static char* itermGetPtyPidAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->pty ? (int)iupTermPtyGetPid(ih->data->pty) : -1);
}

static char* itermGetPtyErrorAttrib(Ihandle* ih)
{
  return iupAttribGet(ih, "_IUPTERM_PTYERROR");
}

static char* itermGetPtySupportAttrib(Ihandle* ih)
{
  (void)ih;
  return iupStrReturnBoolean(iupTermPtyAvailable());
}

static int itermMapMethod(Ihandle* ih)
{
  itermUpdateFontInfo(ih, NULL);

  ih->data->blink_timer = IupTimer();
  IupSetAttribute(ih->data->blink_timer, "TIME", "500");
  iupAttribSet(ih->data->blink_timer, "_IUPTERM_IH", (char*)ih);
  IupSetCallback(ih->data->blink_timer, "ACTION_CB", itermBlinkTimer_CB);

  itermUpdateScrollbar(ih);
  return IUP_NOERROR;
}

static void itermDestroyMethod(Ihandle* ih)
{
  itermPtyDetach(ih);
  if (ih->data->pty_timer)
    IupDestroy(ih->data->pty_timer);
  if (ih->data->blink_timer)
    IupDestroy(ih->data->blink_timer);
  iupTermScreenRelease(&ih->data->term);
}

static void itermComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
  int cols = iupAttribGetInt(ih, "VISIBLECOLUMNS");
  int rows = iupAttribGetInt(ih, "VISIBLELINES");
  int ch_w, ch_h;

  if (cols <= 0) cols = 80;
  if (rows <= 0) rows = 24;

  itermUpdateFontInfo(ih, NULL);
  ch_w = ih->data->ch_w;
  ch_h = ih->data->ch_h;

  /* natural size is in HW pixels; the measurement above is in canvas coordinates */
  *w = iupdrvScaleNaturalPx(cols * ch_w + 2 * ITERM_PADDING);
  *h = iupdrvScaleNaturalPx(rows * ch_h + 2 * ITERM_PADDING);
  (void)children_expand;
}

static int itermCreateMethod(Ihandle* ih, void **params)
{
  Iterm* t;
  (void)params;

  free(ih->data);
  ih->data = iupALLOCCTRLDATA();

  t = &ih->data->term;
  iupTermScreenInit(t, 80, 24, ITERM_DEF_SCROLLBACK);
  t->def_fg = itermPackColor(ITERM_DEF_FGCOLOR, t->def_fg);
  t->def_bg = itermPackColor(ITERM_DEF_BGCOLOR, t->def_bg);
  iupTermScreenReset(t);

  t->cb.response = itermModelResponse;
  t->cb.bell = itermModelBell;
  t->cb.title = itermModelTitle;
  t->cb.clipboard = itermModelClipboard;
  t->cb_user = ih;

  ih->data->blink_on = 1;
  strcpy(ih->data->font_face, "Courier");
  ih->data->font_size = 10;
  ih->data->ch_w = 8;
  ih->data->ch_h = 16;

  IupSetAttribute(ih, "SCROLLBAR", "VERTICAL");
  IupSetAttribute(ih, "FONT", "Courier, 10");

  IupSetCallback(ih, "ACTION", (Icallback)itermRedraw_CB);
  IupSetCallback(ih, "RESIZE_CB", (Icallback)itermResize_CB);
  IupSetCallback(ih, "SCROLL_CB", (Icallback)itermScroll_CB);
  IupSetCallback(ih, "FOCUS_CB", (Icallback)itermFocus_CB);
  IupSetCallback(ih, "BUTTON_CB", (Icallback)itermButton_CB);
  IupSetCallback(ih, "MOTION_CB", (Icallback)itermMotion_CB);
  IupSetCallback(ih, "WHEEL_CB", (Icallback)itermWheel_CB);
  IupSetCallback(ih, "K_ANY", (Icallback)itermKAny_CB);
  IupSetCallback(ih, "TEXTINPUT_CB", (Icallback)itermTextInput_CB);

  return IUP_NOERROR;
}

Iclass* iupTerminalNewClass(void)
{
  Iclass* ic = iupClassNew(iupRegisterFindClass("canvas"));

  ic->name = "terminal";
  ic->format = NULL;
  ic->nativetype = IUP_TYPECANVAS;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 1;

  ic->New = iupTerminalNewClass;
  ic->Create = itermCreateMethod;
  ic->Destroy = itermDestroyMethod;
  ic->Map = itermMapMethod;
  ic->ComputeNaturalSize = itermComputeNaturalSizeMethod;

  iupClassRegisterCallback(ic, "INPUT_CB", "si");
  iupClassRegisterCallback(ic, "TITLE_CB", "s");
  iupClassRegisterCallback(ic, "BELL_CB", "");
  iupClassRegisterCallback(ic, "TERMSIZE_CB", "ii");
  iupClassRegisterCallback(ic, "EXIT_CB", "i");

  iupClassRegisterReplaceAttribDef(ic, "BORDER", "NO", NULL);
  iupClassRegisterReplaceAttribFlags(ic, "BORDER", IUPAF_READONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "WRITE", NULL, itermSetWriteAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "APPEND", NULL, itermSetAppendAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COLUMNS", itermGetColumnsAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LINES", itermGetLinesAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VISIBLECOLUMNS", NULL, NULL, IUPAF_SAMEASSYSTEM, "80", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VISIBLELINES", NULL, NULL, IUPAF_SAMEASSYSTEM, "24", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLBACKLINES", itermGetScrollbackLinesAttrib, itermSetScrollbackLinesAttrib, IUPAF_SAMEASSYSTEM, "5000", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CURSORSTYLE", itermGetCursorStyleAttrib, itermSetCursorStyleAttrib, IUPAF_SAMEASSYSTEM, "BLOCK", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CURSORBLINK", itermGetCursorBlinkAttrib, itermSetCursorBlinkAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLONOUTPUT", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLONKEY", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "OPTIONASMETA", NULL, NULL, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ALLOWOSC52", NULL, NULL, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "RESET", NULL, itermSetResetAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLEARSCREEN", NULL, itermSetClearScreenAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "EXEC", NULL, itermSetExecAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "KILL", NULL, itermSetKillAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PTYPID", itermGetPtyPidAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PTYERROR", itermGetPtyErrorAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PTYSUPPORT", itermGetPtySupportAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TERMNAME", NULL, NULL, IUPAF_SAMEASSYSTEM, "xterm-256color", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTEDTEXT", itermGetSelectedTextAttrib, itermSetSelectionAttrib, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COPYSELECTION", NULL, itermSetCopySelectionAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PASTE", NULL, itermSetPasteAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTALL", NULL, itermSetSelectAllAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AUTOCOPY", NULL, NULL, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttributeId(ic, "COLOR", itermGetColorAttrib, itermSetColorAttrib, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, itermSetFgColorAttrib, IUPAF_SAMEASSYSTEM, ITERM_DEF_FGCOLOR, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, itermSetBgColorAttrib, IUPAF_SAMEASSYSTEM, ITERM_DEF_BGCOLOR, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FONT", NULL, itermSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NO_SAVE | IUPAF_NOT_MAPPED);

  return ic;
}

IUP_API Ihandle* IupTerminal(void)
{
  return IupCreate("terminal");
}
