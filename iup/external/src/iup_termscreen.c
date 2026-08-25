/** \file
 * \brief Terminal Control screen model.
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <string.h>

#include "iup_terminal.h"

static const unsigned int iterm_def_palette[16] = {
  0x000000, 0xCD0000, 0x00CD00, 0xCDCD00, 0x0000EE, 0xCD00CD, 0x00CDCD, 0xE5E5E5,
  0x7F7F7F, 0xFF0000, 0x00FF00, 0xFFFF00, 0x5C5CFF, 0xFF00FF, 0x00FFFF, 0xFFFFFF
};

static void itermBlankCell(Iterm* t, ItermCell* cell)
{
  cell->cp = 0;
  cell->fg = t->pen.fg;
  cell->bg = t->pen.bg;
  cell->flags = (unsigned short)(t->pen.flags & (ITERM_FL_FG_DEFAULT|ITERM_FL_BG_DEFAULT));
  if (!(t->pen.flags & ITERM_FL_BG_DEFAULT))
    cell->bg = t->pen.bg;
}

static void itermLineAlloc(Iterm* t, ItermLine* line, int cols)
{
  int i;
  line->cells = (ItermCell*)malloc(cols * sizeof(ItermCell));
  line->ncells = (short)cols;
  line->wrapped = 0;
  for (i = 0; i < cols; i++)
    itermBlankCell(t, &line->cells[i]);
}

static void itermLineClear(Iterm* t, ItermLine* line, int from, int to)
{
  int i;
  if (from < 0) from = 0;
  if (to >= line->ncells) to = line->ncells - 1;
  for (i = from; i <= to; i++)
    itermBlankCell(t, &line->cells[i]);
}

static ItermLine* itermActiveLines(Iterm* t)
{
  return (t->modes & ITERM_MODE_ALTSCREEN) ? t->alt_lines : t->lines;
}

ItermLine* iupTermScreenLine(Iterm* t, int row)
{
  if (row < 0 || row >= t->rows)
    return NULL;
  return &itermActiveLines(t)[row];
}

ItermLine* iupTermScreenHistoryLine(Iterm* t, int index)
{
  int oldest;
  if (index < 0 || index >= t->sb_count)
    return NULL;
  oldest = (t->sb_head - t->sb_count + t->sb_max) % t->sb_max;
  return &t->sb[(oldest + index) % t->sb_max];
}

static void itermDamageRow(Iterm* t, int row)
{
  if (row >= 0 && row < t->rows)
    t->dirty[row] = 1;
}

static void itermDamageRange(Iterm* t, int from, int to)
{
  int i;
  if (from < 0) from = 0;
  if (to >= t->rows) to = t->rows - 1;
  for (i = from; i <= to; i++)
    t->dirty[i] = 1;
}

void iupTermScreenDamageAll(Iterm* t)
{
  t->all_dirty = 1;
  memset(t->dirty, 1, t->rows);
}

static void itermPushScrollback(Iterm* t, ItermLine* line)
{
  if (t->sb_max <= 0 || t->sb_disabled || (t->modes & ITERM_MODE_ALTSCREEN))
  {
    free(line->cells);
    return;
  }

  if (t->sb_count == t->sb_max)
    free(t->sb[t->sb_head].cells);
  else
    t->sb_count++;

  t->sb[t->sb_head] = *line;
  t->sb_head = (t->sb_head + 1) % t->sb_max;
  t->sb_pushed++;
}

/* count > 0 scrolls up, < 0 down; content moves within the scroll region */
void iupTermScreenScroll(Iterm* t, int count)
{
  ItermLine* lines = itermActiveLines(t);
  int region = t->scroll_bot - t->scroll_top + 1;
  int i;

  if (count == 0)
    return;
  if (count > region) count = region;
  if (count < -region) count = -region;

  if (count > 0)
  {
    int full = (t->scroll_top == 0 && t->scroll_bot == t->rows - 1);
    for (i = 0; i < count; i++)
    {
      ItermLine top = lines[t->scroll_top];
      memmove(&lines[t->scroll_top], &lines[t->scroll_top + 1],
              (region - 1) * sizeof(ItermLine));
      if (full)
        itermPushScrollback(t, &top);
      else
        free(top.cells);
      itermLineAlloc(t, &lines[t->scroll_bot], t->cols);
    }
  }
  else
  {
    count = -count;
    for (i = 0; i < count; i++)
    {
      ItermLine bot = lines[t->scroll_bot];
      memmove(&lines[t->scroll_top + 1], &lines[t->scroll_top],
              (region - 1) * sizeof(ItermLine));
      free(bot.cells);
      itermLineAlloc(t, &lines[t->scroll_top], t->cols);
    }
  }

  itermDamageRange(t, t->scroll_top, t->scroll_bot);
}

/* clear leftover halves when a write lands on part of a wide pair */
static void itermFixWideOverlap(ItermLine* line, int col)
{
  ItermCell* cell = &line->cells[col];
  if ((cell->flags & ITERM_FL_WIDECONT) && col > 0)
  {
    line->cells[col - 1].cp = 0;
    line->cells[col - 1].flags &= (unsigned short)~ITERM_FL_WIDE;
  }
  if ((cell->flags & ITERM_FL_WIDE) && col + 1 < line->ncells)
  {
    line->cells[col + 1].cp = 0;
    line->cells[col + 1].flags &= (unsigned short)~ITERM_FL_WIDECONT;
  }
}

void iupTermScreenPutChar(Iterm* t, unsigned int cp)
{
  ItermLine* line;
  ItermCell* cell;
  int width = iupTermCharWidth(cp);

  if (width == 0)
    return;

  if (t->pending_wrap && (t->modes & ITERM_MODE_AUTOWRAP))
  {
    iupTermScreenLine(t, t->cy)->wrapped = 1;
    t->cx = 0;
    iupTermScreenLineFeed(t);
    t->pending_wrap = 0;
  }

  if (t->cx + width > t->cols)
  {
    if (t->modes & ITERM_MODE_AUTOWRAP)
    {
      iupTermScreenLine(t, t->cy)->wrapped = 1;
      t->cx = 0;
      iupTermScreenLineFeed(t);
    }
    else
      t->cx = t->cols - width;
  }

  line = iupTermScreenLine(t, t->cy);
  itermFixWideOverlap(line, t->cx);
  if (width == 2 && t->cx + 1 < t->cols)
    itermFixWideOverlap(line, t->cx + 1);

  cell = &line->cells[t->cx];
  cell->cp = cp;
  cell->fg = t->pen.fg;
  cell->bg = t->pen.bg;
  cell->flags = (unsigned short)(t->pen.flags & ITERM_FL_STYLEMASK);
  if (width == 2)
  {
    cell->flags |= ITERM_FL_WIDE;
    if (t->cx + 1 < t->cols)
    {
      ItermCell* cont = &line->cells[t->cx + 1];
      *cont = *cell;
      cont->cp = 0;
      cont->flags = (unsigned short)((cont->flags & ~ITERM_FL_WIDE) | ITERM_FL_WIDECONT);
    }
  }

  itermDamageRow(t, t->cy);

  t->cx += width;
  if (t->cx >= t->cols)
  {
    t->cx = t->cols - 1;
    t->pending_wrap = 1;
  }
}

void iupTermScreenLineFeed(Iterm* t)
{
  t->pending_wrap = 0;
  if (t->cy == t->scroll_bot)
    iupTermScreenScroll(t, 1);
  else if (t->cy < t->rows - 1)
    t->cy++;
}

void iupTermScreenReverseLineFeed(Iterm* t)
{
  t->pending_wrap = 0;
  if (t->cy == t->scroll_top)
    iupTermScreenScroll(t, -1);
  else if (t->cy > 0)
    t->cy--;
}

void iupTermScreenCarriageReturn(Iterm* t)
{
  t->cx = 0;
  t->pending_wrap = 0;
}

void iupTermScreenBackspace(Iterm* t)
{
  if (t->pending_wrap)
    t->pending_wrap = 0;
  else if (t->cx > 0)
    t->cx--;
}

void iupTermScreenTab(Iterm* t, int count, int backward)
{
  t->pending_wrap = 0;
  while (count > 0)
  {
    if (backward)
    {
      if (t->cx == 0) break;
      t->cx--;
      while (t->cx > 0 && !t->tabs[t->cx])
        t->cx--;
    }
    else
    {
      if (t->cx >= t->cols - 1) break;
      t->cx++;
      while (t->cx < t->cols - 1 && !t->tabs[t->cx])
        t->cx++;
    }
    count--;
  }
}

void iupTermScreenSetTab(Iterm* t)
{
  t->tabs[t->cx] = 1;
}

void iupTermScreenClearTab(Iterm* t, int all)
{
  if (all)
    memset(t->tabs, 0, t->cols);
  else
    t->tabs[t->cx] = 0;
}

static void itermClampCursor(Iterm* t)
{
  int top = 0, bot = t->rows - 1;
  if (t->modes & ITERM_MODE_ORIGIN)
  {
    top = t->scroll_top;
    bot = t->scroll_bot;
  }
  if (t->cx < 0) t->cx = 0;
  if (t->cx > t->cols - 1) t->cx = t->cols - 1;
  if (t->cy < top) t->cy = top;
  if (t->cy > bot) t->cy = bot;
}

void iupTermScreenMoveCursor(Iterm* t, int dx, int dy)
{
  t->pending_wrap = 0;
  t->cx += dx;
  t->cy += dy;
  itermClampCursor(t);
}

void iupTermScreenSetCursor(Iterm* t, int col, int row)
{
  t->pending_wrap = 0;
  if (t->modes & ITERM_MODE_ORIGIN)
    row += t->scroll_top;
  t->cx = col;
  t->cy = row;
  itermClampCursor(t);
}

void iupTermScreenSetCursorCol(Iterm* t, int col)
{
  t->pending_wrap = 0;
  t->cx = col;
  itermClampCursor(t);
}

void iupTermScreenSetCursorRow(Iterm* t, int row)
{
  t->pending_wrap = 0;
  if (t->modes & ITERM_MODE_ORIGIN)
    row += t->scroll_top;
  t->cy = row;
  itermClampCursor(t);
}

void iupTermScreenSaveCursor(Iterm* t)
{
  if (t->modes & ITERM_MODE_ALTSCREEN)
  {
    t->alt_saved_cx = t->cx;
    t->alt_saved_cy = t->cy;
    t->alt_saved_pen = t->pen;
  }
  else
  {
    t->saved_cx = t->cx;
    t->saved_cy = t->cy;
    t->saved_pen = t->pen;
    t->saved_charset = t->charset_gl;
  }
}

void iupTermScreenRestoreCursor(Iterm* t)
{
  t->pending_wrap = 0;
  if (t->modes & ITERM_MODE_ALTSCREEN)
  {
    t->cx = t->alt_saved_cx;
    t->cy = t->alt_saved_cy;
    t->pen = t->alt_saved_pen;
  }
  else
  {
    t->cx = t->saved_cx;
    t->cy = t->saved_cy;
    t->pen = t->saved_pen;
    t->charset_gl = t->saved_charset;
  }
  itermClampCursor(t);
}

void iupTermScreenEraseDisplay(Iterm* t, int mode)
{
  int i;
  t->pending_wrap = 0;
  switch (mode)
  {
  case 0:
    iupTermScreenEraseLine(t, 0);
    for (i = t->cy + 1; i < t->rows; i++)
      itermLineClear(t, iupTermScreenLine(t, i), 0, t->cols - 1);
    itermDamageRange(t, t->cy, t->rows - 1);
    break;
  case 1:
    iupTermScreenEraseLine(t, 1);
    for (i = 0; i < t->cy; i++)
      itermLineClear(t, iupTermScreenLine(t, i), 0, t->cols - 1);
    itermDamageRange(t, 0, t->cy);
    break;
  case 2:
    for (i = 0; i < t->rows; i++)
      itermLineClear(t, iupTermScreenLine(t, i), 0, t->cols - 1);
    iupTermScreenDamageAll(t);
    break;
  }
}

void iupTermScreenEraseLine(Iterm* t, int mode)
{
  ItermLine* line = iupTermScreenLine(t, t->cy);
  t->pending_wrap = 0;
  switch (mode)
  {
  case 0: itermLineClear(t, line, t->cx, t->cols - 1); break;
  case 1: itermLineClear(t, line, 0, t->cx); break;
  case 2: itermLineClear(t, line, 0, t->cols - 1); break;
  }
  line->wrapped = 0;
  itermDamageRow(t, t->cy);
}

void iupTermScreenEraseChars(Iterm* t, int count)
{
  ItermLine* line = iupTermScreenLine(t, t->cy);
  t->pending_wrap = 0;
  if (count < 1) count = 1;
  itermLineClear(t, line, t->cx, t->cx + count - 1);
  itermDamageRow(t, t->cy);
}

void iupTermScreenInsertChars(Iterm* t, int count)
{
  ItermLine* line = iupTermScreenLine(t, t->cy);
  int room = t->cols - t->cx;
  t->pending_wrap = 0;
  if (count < 1) count = 1;
  if (count > room) count = room;
  memmove(&line->cells[t->cx + count], &line->cells[t->cx],
          (room - count) * sizeof(ItermCell));
  itermLineClear(t, line, t->cx, t->cx + count - 1);
  itermDamageRow(t, t->cy);
}

void iupTermScreenDeleteChars(Iterm* t, int count)
{
  ItermLine* line = iupTermScreenLine(t, t->cy);
  int room = t->cols - t->cx;
  t->pending_wrap = 0;
  if (count < 1) count = 1;
  if (count > room) count = room;
  memmove(&line->cells[t->cx], &line->cells[t->cx + count],
          (room - count) * sizeof(ItermCell));
  itermLineClear(t, line, t->cols - count, t->cols - 1);
  itermDamageRow(t, t->cy);
}

void iupTermScreenInsertLines(Iterm* t, int count)
{
  int save_top;
  if (t->cy < t->scroll_top || t->cy > t->scroll_bot)
    return;
  if (count < 1) count = 1;
  save_top = t->scroll_top;
  t->scroll_top = t->cy;
  t->sb_disabled = 1;
  iupTermScreenScroll(t, -count);
  t->sb_disabled = 0;
  t->scroll_top = save_top;
  t->cx = 0;
  t->pending_wrap = 0;
}

void iupTermScreenDeleteLines(Iterm* t, int count)
{
  int save_top;
  if (t->cy < t->scroll_top || t->cy > t->scroll_bot)
    return;
  if (count < 1) count = 1;
  save_top = t->scroll_top;
  t->scroll_top = t->cy;
  t->sb_disabled = 1;
  iupTermScreenScroll(t, count);
  t->sb_disabled = 0;
  t->scroll_top = save_top;
  t->cx = 0;
  t->pending_wrap = 0;
}

void iupTermScreenSetScrollRegion(Iterm* t, int top, int bot)
{
  if (top < 0) top = 0;
  if (bot <= 0 || bot > t->rows - 1) bot = t->rows - 1;
  if (top >= bot)
  {
    top = 0;
    bot = t->rows - 1;
  }
  t->scroll_top = top;
  t->scroll_bot = bot;
  iupTermScreenSetCursor(t, 0, 0);
}

void iupTermScreenSetAltScreen(Iterm* t, int alt, int save_cursor, int clear)
{
  int i;
  int active = (t->modes & ITERM_MODE_ALTSCREEN) != 0;
  if (alt == active)
    return;

  if (alt)
  {
    if (save_cursor)
      iupTermScreenSaveCursor(t);
    if (!t->alt_lines)
    {
      t->alt_lines = (ItermLine*)malloc(t->rows * sizeof(ItermLine));
      for (i = 0; i < t->rows; i++)
        itermLineAlloc(t, &t->alt_lines[i], t->cols);
    }
    t->modes |= ITERM_MODE_ALTSCREEN;
    if (clear)
      for (i = 0; i < t->rows; i++)
        itermLineClear(t, &t->alt_lines[i], 0, t->cols - 1);
    iupTermScreenSetCursor(t, 0, 0);
  }
  else
  {
    t->modes &= ~ITERM_MODE_ALTSCREEN;
    if (save_cursor)
      iupTermScreenRestoreCursor(t);
  }

  t->scroll_top = 0;
  t->scroll_bot = t->rows - 1;
  t->pending_wrap = 0;
  iupTermScreenDamageAll(t);
}

void iupTermScreenResetPalette(Iterm* t, int index)
{
  if (index >= 0 && index < 16)
    t->palette[index] = iterm_def_palette[index];
  else
    memcpy(t->palette, iterm_def_palette, sizeof(t->palette));
}

void iupTermScreenClearScrollback(Iterm* t)
{
  int i;
  for (i = 0; i < t->sb_count; i++)
    free(iupTermScreenHistoryLine(t, i)->cells);
  t->sb_count = 0;
  t->sb_head = 0;
}

void iupTermScreenResize(Iterm* t, int cols, int rows)
{
  int i;
  ItermPen save_pen = t->pen;

  if (cols < 2) cols = 2;
  if (rows < 1) rows = 1;
  if (cols == t->cols && rows == t->rows)
    return;

  t->pen.flags = ITERM_FL_FG_DEFAULT | ITERM_FL_BG_DEFAULT;

  if (rows < t->rows && !(t->modes & ITERM_MODE_ALTSCREEN))
  {
    int excess = t->rows - rows;
    int below = t->rows - 1 - t->cy;
    int push = excess - below;
    if (push > 0)
    {
      for (i = 0; i < push; i++)
        itermPushScrollback(t, &t->lines[i]);
      memmove(&t->lines[0], &t->lines[push], (t->rows - push) * sizeof(ItermLine));
      /* the scrollback owns the pushed cells, the vacated slots need lines of their own */
      for (i = t->rows - push; i < t->rows; i++)
        itermLineAlloc(t, &t->lines[i], t->cols);
      t->cy -= push;
    }
  }

  {
    ItermLine* screens[2];
    screens[0] = t->lines;
    screens[1] = t->alt_lines;
    for (i = 0; i < 2; i++)
    {
      ItermLine* old = screens[i];
      ItermLine* new_lines;
      int r;
      if (!old)
        continue;
      new_lines = (ItermLine*)malloc(rows * sizeof(ItermLine));
      for (r = 0; r < rows; r++)
      {
        if (r < t->rows)
        {
          ItermLine* ol = &old[r];
          new_lines[r] = *ol;
          if (cols != ol->ncells)
          {
            int c;
            new_lines[r].cells = (ItermCell*)realloc(ol->cells, cols * sizeof(ItermCell));
            for (c = ol->ncells; c < cols; c++)
              itermBlankCell(t, &new_lines[r].cells[c]);
            new_lines[r].ncells = (short)cols;
          }
        }
        else
          itermLineAlloc(t, &new_lines[r], cols);
      }
      for (r = rows; r < t->rows; r++)
        free(old[r].cells);
      free(old);
      if (i == 0) t->lines = new_lines;
      else t->alt_lines = new_lines;
    }
  }

  t->cols = cols;
  t->rows = rows;
  t->scroll_top = 0;
  t->scroll_bot = rows - 1;
  t->pen = save_pen;

  t->tabs = (unsigned char*)realloc(t->tabs, cols);
  for (i = 0; i < cols; i++)
    t->tabs[i] = (i % 8) == 0;

  t->dirty = (unsigned char*)realloc(t->dirty, rows);
  iupTermScreenDamageAll(t);

  if (t->cx > cols - 1) t->cx = cols - 1;
  if (t->cy > rows - 1) t->cy = rows - 1;
  t->pending_wrap = 0;
}

void iupTermScreenReset(Iterm* t)
{
  int i;

  t->pen.fg = t->def_fg;
  t->pen.bg = t->def_bg;
  t->pen.flags = ITERM_FL_FG_DEFAULT | ITERM_FL_BG_DEFAULT;

  iupTermScreenSetAltScreen(t, 0, 0, 0);
  for (i = 0; i < t->rows; i++)
  {
    itermLineClear(t, &t->lines[i], 0, t->cols - 1);
    t->lines[i].wrapped = 0;
  }

  t->cx = 0;
  t->cy = 0;
  t->pending_wrap = 0;
  t->scroll_top = 0;
  t->scroll_bot = t->rows - 1;
  t->saved_cx = t->saved_cy = 0;
  t->saved_pen = t->pen;
  t->saved_charset = 0;
  t->alt_saved_cx = t->alt_saved_cy = 0;
  t->alt_saved_pen = t->pen;

  t->modes = ITERM_MODE_AUTOWRAP | ITERM_MODE_CURSORVIS;
  t->cursor_style = ITERM_CURSOR_BLOCK;
  t->charset_gl = 0;
  t->charset[0] = 0;
  t->charset[1] = 0;

  memcpy(t->palette, iterm_def_palette, sizeof(t->palette));

  for (i = 0; i < t->cols; i++)
    t->tabs[i] = (i % 8) == 0;

  iupTermParseInit(t);
  iupTermScreenDamageAll(t);
}

int iupTermScreenInit(Iterm* t, int cols, int rows, int sb_max)
{
  int i;

  memset(t, 0, sizeof(Iterm));
  if (cols < 2) cols = 2;
  if (rows < 1) rows = 1;
  if (sb_max < 0) sb_max = 0;

  t->cols = cols;
  t->rows = rows;
  t->sb_max = sb_max;
  t->def_fg = 0xE5E5E5;
  t->def_bg = 0x000000;

  t->pen.flags = ITERM_FL_FG_DEFAULT | ITERM_FL_BG_DEFAULT;

  t->lines = (ItermLine*)malloc(rows * sizeof(ItermLine));
  for (i = 0; i < rows; i++)
    itermLineAlloc(t, &t->lines[i], cols);

  if (sb_max > 0)
    t->sb = (ItermLine*)malloc(sb_max * sizeof(ItermLine));

  t->tabs = (unsigned char*)malloc(cols);
  t->dirty = (unsigned char*)malloc(rows);

  iupTermScreenReset(t);
  return 1;
}

void iupTermScreenRelease(Iterm* t)
{
  int i;
  iupTermScreenClearScrollback(t);
  for (i = 0; i < t->rows; i++)
    free(t->lines[i].cells);
  free(t->lines);
  if (t->alt_lines)
  {
    for (i = 0; i < t->rows; i++)
      free(t->alt_lines[i].cells);
    free(t->alt_lines);
  }
  free(t->sb);
  free(t->tabs);
  free(t->dirty);
  memset(t, 0, sizeof(Iterm));
}

/* Unicode 16.0 interval tables generated from unicodedata:
   zerow = Mn+Me+Cf (minus U+00AD) + Hangul Jamo V/T; wide = East Asian W+F */
static const struct { unsigned int first, last; } iterm_zerow[] = {
  {0x0300, 0x036F}, {0x0483, 0x0489}, {0x0591, 0x05BD}, {0x05BF, 0x05BF},
  {0x05C1, 0x05C2}, {0x05C4, 0x05C5}, {0x05C7, 0x05C7}, {0x0600, 0x0605},
  {0x0610, 0x061A}, {0x061C, 0x061C}, {0x064B, 0x065F}, {0x0670, 0x0670},
  {0x06D6, 0x06DD}, {0x06DF, 0x06E4}, {0x06E7, 0x06E8}, {0x06EA, 0x06ED},
  {0x070F, 0x070F}, {0x0711, 0x0711}, {0x0730, 0x074A}, {0x07A6, 0x07B0},
  {0x07EB, 0x07F3}, {0x07FD, 0x07FD}, {0x0816, 0x0819}, {0x081B, 0x0823},
  {0x0825, 0x0827}, {0x0829, 0x082D}, {0x0859, 0x085B}, {0x0890, 0x0891},
  {0x0897, 0x089F}, {0x08CA, 0x0902}, {0x093A, 0x093A}, {0x093C, 0x093C},
  {0x0941, 0x0948}, {0x094D, 0x094D}, {0x0951, 0x0957}, {0x0962, 0x0963},
  {0x0981, 0x0981}, {0x09BC, 0x09BC}, {0x09C1, 0x09C4}, {0x09CD, 0x09CD},
  {0x09E2, 0x09E3}, {0x09FE, 0x09FE}, {0x0A01, 0x0A02}, {0x0A3C, 0x0A3C},
  {0x0A41, 0x0A42}, {0x0A47, 0x0A48}, {0x0A4B, 0x0A4D}, {0x0A51, 0x0A51},
  {0x0A70, 0x0A71}, {0x0A75, 0x0A75}, {0x0A81, 0x0A82}, {0x0ABC, 0x0ABC},
  {0x0AC1, 0x0AC5}, {0x0AC7, 0x0AC8}, {0x0ACD, 0x0ACD}, {0x0AE2, 0x0AE3},
  {0x0AFA, 0x0AFF}, {0x0B01, 0x0B01}, {0x0B3C, 0x0B3C}, {0x0B3F, 0x0B3F},
  {0x0B41, 0x0B44}, {0x0B4D, 0x0B4D}, {0x0B55, 0x0B56}, {0x0B62, 0x0B63},
  {0x0B82, 0x0B82}, {0x0BC0, 0x0BC0}, {0x0BCD, 0x0BCD}, {0x0C00, 0x0C00},
  {0x0C04, 0x0C04}, {0x0C3C, 0x0C3C}, {0x0C3E, 0x0C40}, {0x0C46, 0x0C48},
  {0x0C4A, 0x0C4D}, {0x0C55, 0x0C56}, {0x0C62, 0x0C63}, {0x0C81, 0x0C81},
  {0x0CBC, 0x0CBC}, {0x0CBF, 0x0CBF}, {0x0CC6, 0x0CC6}, {0x0CCC, 0x0CCD},
  {0x0CE2, 0x0CE3}, {0x0D00, 0x0D01}, {0x0D3B, 0x0D3C}, {0x0D41, 0x0D44},
  {0x0D4D, 0x0D4D}, {0x0D62, 0x0D63}, {0x0D81, 0x0D81}, {0x0DCA, 0x0DCA},
  {0x0DD2, 0x0DD4}, {0x0DD6, 0x0DD6}, {0x0E31, 0x0E31}, {0x0E34, 0x0E3A},
  {0x0E47, 0x0E4E}, {0x0EB1, 0x0EB1}, {0x0EB4, 0x0EBC}, {0x0EC8, 0x0ECE},
  {0x0F18, 0x0F19}, {0x0F35, 0x0F35}, {0x0F37, 0x0F37}, {0x0F39, 0x0F39},
  {0x0F71, 0x0F7E}, {0x0F80, 0x0F84}, {0x0F86, 0x0F87}, {0x0F8D, 0x0F97},
  {0x0F99, 0x0FBC}, {0x0FC6, 0x0FC6}, {0x102D, 0x1030}, {0x1032, 0x1037},
  {0x1039, 0x103A}, {0x103D, 0x103E}, {0x1058, 0x1059}, {0x105E, 0x1060},
  {0x1071, 0x1074}, {0x1082, 0x1082}, {0x1085, 0x1086}, {0x108D, 0x108D},
  {0x109D, 0x109D}, {0x1160, 0x11FF}, {0x135D, 0x135F}, {0x1712, 0x1714},
  {0x1732, 0x1733}, {0x1752, 0x1753}, {0x1772, 0x1773}, {0x17B4, 0x17B5},
  {0x17B7, 0x17BD}, {0x17C6, 0x17C6}, {0x17C9, 0x17D3}, {0x17DD, 0x17DD},
  {0x180B, 0x180F}, {0x1885, 0x1886}, {0x18A9, 0x18A9}, {0x1920, 0x1922},
  {0x1927, 0x1928}, {0x1932, 0x1932}, {0x1939, 0x193B}, {0x1A17, 0x1A18},
  {0x1A1B, 0x1A1B}, {0x1A56, 0x1A56}, {0x1A58, 0x1A5E}, {0x1A60, 0x1A60},
  {0x1A62, 0x1A62}, {0x1A65, 0x1A6C}, {0x1A73, 0x1A7C}, {0x1A7F, 0x1A7F},
  {0x1AB0, 0x1ACE}, {0x1B00, 0x1B03}, {0x1B34, 0x1B34}, {0x1B36, 0x1B3A},
  {0x1B3C, 0x1B3C}, {0x1B42, 0x1B42}, {0x1B6B, 0x1B73}, {0x1B80, 0x1B81},
  {0x1BA2, 0x1BA5}, {0x1BA8, 0x1BA9}, {0x1BAB, 0x1BAD}, {0x1BE6, 0x1BE6},
  {0x1BE8, 0x1BE9}, {0x1BED, 0x1BED}, {0x1BEF, 0x1BF1}, {0x1C2C, 0x1C33},
  {0x1C36, 0x1C37}, {0x1CD0, 0x1CD2}, {0x1CD4, 0x1CE0}, {0x1CE2, 0x1CE8},
  {0x1CED, 0x1CED}, {0x1CF4, 0x1CF4}, {0x1CF8, 0x1CF9}, {0x1DC0, 0x1DFF},
  {0x200B, 0x200F}, {0x202A, 0x202E}, {0x2060, 0x2064}, {0x2066, 0x206F},
  {0x20D0, 0x20F0}, {0x2CEF, 0x2CF1}, {0x2D7F, 0x2D7F}, {0x2DE0, 0x2DFF},
  {0x302A, 0x302D}, {0x3099, 0x309A}, {0xA66F, 0xA672}, {0xA674, 0xA67D},
  {0xA69E, 0xA69F}, {0xA6F0, 0xA6F1}, {0xA802, 0xA802}, {0xA806, 0xA806},
  {0xA80B, 0xA80B}, {0xA825, 0xA826}, {0xA82C, 0xA82C}, {0xA8C4, 0xA8C5},
  {0xA8E0, 0xA8F1}, {0xA8FF, 0xA8FF}, {0xA926, 0xA92D}, {0xA947, 0xA951},
  {0xA980, 0xA982}, {0xA9B3, 0xA9B3}, {0xA9B6, 0xA9B9}, {0xA9BC, 0xA9BD},
  {0xA9E5, 0xA9E5}, {0xAA29, 0xAA2E}, {0xAA31, 0xAA32}, {0xAA35, 0xAA36},
  {0xAA43, 0xAA43}, {0xAA4C, 0xAA4C}, {0xAA7C, 0xAA7C}, {0xAAB0, 0xAAB0},
  {0xAAB2, 0xAAB4}, {0xAAB7, 0xAAB8}, {0xAABE, 0xAABF}, {0xAAC1, 0xAAC1},
  {0xAAEC, 0xAAED}, {0xAAF6, 0xAAF6}, {0xABE5, 0xABE5}, {0xABE8, 0xABE8},
  {0xABED, 0xABED}, {0xFB1E, 0xFB1E}, {0xFE00, 0xFE0F}, {0xFE20, 0xFE2F},
  {0xFEFF, 0xFEFF}, {0xFFF9, 0xFFFB}, {0x101FD, 0x101FD}, {0x102E0, 0x102E0},
  {0x10376, 0x1037A}, {0x10A01, 0x10A03}, {0x10A05, 0x10A06}, {0x10A0C, 0x10A0F},
  {0x10A38, 0x10A3A}, {0x10A3F, 0x10A3F}, {0x10AE5, 0x10AE6}, {0x10D24, 0x10D27},
  {0x10D69, 0x10D6D}, {0x10EAB, 0x10EAC}, {0x10EFC, 0x10EFF}, {0x10F46, 0x10F50},
  {0x10F82, 0x10F85}, {0x11001, 0x11001}, {0x11038, 0x11046}, {0x11070, 0x11070},
  {0x11073, 0x11074}, {0x1107F, 0x11081}, {0x110B3, 0x110B6}, {0x110B9, 0x110BA},
  {0x110BD, 0x110BD}, {0x110C2, 0x110C2}, {0x110CD, 0x110CD}, {0x11100, 0x11102},
  {0x11127, 0x1112B}, {0x1112D, 0x11134}, {0x11173, 0x11173}, {0x11180, 0x11181},
  {0x111B6, 0x111BE}, {0x111C9, 0x111CC}, {0x111CF, 0x111CF}, {0x1122F, 0x11231},
  {0x11234, 0x11234}, {0x11236, 0x11237}, {0x1123E, 0x1123E}, {0x11241, 0x11241},
  {0x112DF, 0x112DF}, {0x112E3, 0x112EA}, {0x11300, 0x11301}, {0x1133B, 0x1133C},
  {0x11340, 0x11340}, {0x11366, 0x1136C}, {0x11370, 0x11374}, {0x113BB, 0x113C0},
  {0x113CE, 0x113CE}, {0x113D0, 0x113D0}, {0x113D2, 0x113D2}, {0x113E1, 0x113E2},
  {0x11438, 0x1143F}, {0x11442, 0x11444}, {0x11446, 0x11446}, {0x1145E, 0x1145E},
  {0x114B3, 0x114B8}, {0x114BA, 0x114BA}, {0x114BF, 0x114C0}, {0x114C2, 0x114C3},
  {0x115B2, 0x115B5}, {0x115BC, 0x115BD}, {0x115BF, 0x115C0}, {0x115DC, 0x115DD},
  {0x11633, 0x1163A}, {0x1163D, 0x1163D}, {0x1163F, 0x11640}, {0x116AB, 0x116AB},
  {0x116AD, 0x116AD}, {0x116B0, 0x116B5}, {0x116B7, 0x116B7}, {0x1171D, 0x1171D},
  {0x1171F, 0x1171F}, {0x11722, 0x11725}, {0x11727, 0x1172B}, {0x1182F, 0x11837},
  {0x11839, 0x1183A}, {0x1193B, 0x1193C}, {0x1193E, 0x1193E}, {0x11943, 0x11943},
  {0x119D4, 0x119D7}, {0x119DA, 0x119DB}, {0x119E0, 0x119E0}, {0x11A01, 0x11A0A},
  {0x11A33, 0x11A38}, {0x11A3B, 0x11A3E}, {0x11A47, 0x11A47}, {0x11A51, 0x11A56},
  {0x11A59, 0x11A5B}, {0x11A8A, 0x11A96}, {0x11A98, 0x11A99}, {0x11C30, 0x11C36},
  {0x11C38, 0x11C3D}, {0x11C3F, 0x11C3F}, {0x11C92, 0x11CA7}, {0x11CAA, 0x11CB0},
  {0x11CB2, 0x11CB3}, {0x11CB5, 0x11CB6}, {0x11D31, 0x11D36}, {0x11D3A, 0x11D3A},
  {0x11D3C, 0x11D3D}, {0x11D3F, 0x11D45}, {0x11D47, 0x11D47}, {0x11D90, 0x11D91},
  {0x11D95, 0x11D95}, {0x11D97, 0x11D97}, {0x11EF3, 0x11EF4}, {0x11F00, 0x11F01},
  {0x11F36, 0x11F3A}, {0x11F40, 0x11F40}, {0x11F42, 0x11F42}, {0x11F5A, 0x11F5A},
  {0x13430, 0x13440}, {0x13447, 0x13455}, {0x1611E, 0x16129}, {0x1612D, 0x1612F},
  {0x16AF0, 0x16AF4}, {0x16B30, 0x16B36}, {0x16F4F, 0x16F4F}, {0x16F8F, 0x16F92},
  {0x16FE4, 0x16FE4}, {0x1BC9D, 0x1BC9E}, {0x1BCA0, 0x1BCA3}, {0x1CF00, 0x1CF2D},
  {0x1CF30, 0x1CF46}, {0x1D167, 0x1D169}, {0x1D173, 0x1D182}, {0x1D185, 0x1D18B},
  {0x1D1AA, 0x1D1AD}, {0x1D242, 0x1D244}, {0x1DA00, 0x1DA36}, {0x1DA3B, 0x1DA6C},
  {0x1DA75, 0x1DA75}, {0x1DA84, 0x1DA84}, {0x1DA9B, 0x1DA9F}, {0x1DAA1, 0x1DAAF},
  {0x1E000, 0x1E006}, {0x1E008, 0x1E018}, {0x1E01B, 0x1E021}, {0x1E023, 0x1E024},
  {0x1E026, 0x1E02A}, {0x1E08F, 0x1E08F}, {0x1E130, 0x1E136}, {0x1E2AE, 0x1E2AE},
  {0x1E2EC, 0x1E2EF}, {0x1E4EC, 0x1E4EF}, {0x1E5EE, 0x1E5EF}, {0x1E8D0, 0x1E8D6},
  {0x1E944, 0x1E94A}, {0xE0001, 0xE0001}, {0xE0020, 0xE007F}, {0xE0100, 0xE01EF},
};

static const struct { unsigned int first, last; } iterm_wide[] = {
  {0x1100, 0x115F}, {0x231A, 0x231B}, {0x2329, 0x232A}, {0x23E9, 0x23EC},
  {0x23F0, 0x23F0}, {0x23F3, 0x23F3}, {0x25FD, 0x25FE}, {0x2614, 0x2615},
  {0x2630, 0x2637}, {0x2648, 0x2653}, {0x267F, 0x267F}, {0x268A, 0x268F},
  {0x2693, 0x2693}, {0x26A1, 0x26A1}, {0x26AA, 0x26AB}, {0x26BD, 0x26BE},
  {0x26C4, 0x26C5}, {0x26CE, 0x26CE}, {0x26D4, 0x26D4}, {0x26EA, 0x26EA},
  {0x26F2, 0x26F3}, {0x26F5, 0x26F5}, {0x26FA, 0x26FA}, {0x26FD, 0x26FD},
  {0x2705, 0x2705}, {0x270A, 0x270B}, {0x2728, 0x2728}, {0x274C, 0x274C},
  {0x274E, 0x274E}, {0x2753, 0x2755}, {0x2757, 0x2757}, {0x2795, 0x2797},
  {0x27B0, 0x27B0}, {0x27BF, 0x27BF}, {0x2B1B, 0x2B1C}, {0x2B50, 0x2B50},
  {0x2B55, 0x2B55}, {0x2E80, 0x2E99}, {0x2E9B, 0x2EF3}, {0x2F00, 0x2FD5},
  {0x2FF0, 0x303E}, {0x3041, 0x3096}, {0x3099, 0x30FF}, {0x3105, 0x312F},
  {0x3131, 0x318E}, {0x3190, 0x31E5}, {0x31EF, 0x321E}, {0x3220, 0x3247},
  {0x3250, 0xA48C}, {0xA490, 0xA4C6}, {0xA960, 0xA97C}, {0xAC00, 0xD7A3},
  {0xF900, 0xFAFF}, {0xFE10, 0xFE19}, {0xFE30, 0xFE52}, {0xFE54, 0xFE66},
  {0xFE68, 0xFE6B}, {0xFF01, 0xFF60}, {0xFFE0, 0xFFE6}, {0x16FE0, 0x16FE4},
  {0x16FF0, 0x16FF1}, {0x17000, 0x187F7}, {0x18800, 0x18CD5}, {0x18CFF, 0x18D08},
  {0x1AFF0, 0x1AFF3}, {0x1AFF5, 0x1AFFB}, {0x1AFFD, 0x1AFFE}, {0x1B000, 0x1B122},
  {0x1B132, 0x1B132}, {0x1B150, 0x1B152}, {0x1B155, 0x1B155}, {0x1B164, 0x1B167},
  {0x1B170, 0x1B2FB}, {0x1D300, 0x1D356}, {0x1D360, 0x1D376}, {0x1F004, 0x1F004},
  {0x1F0CF, 0x1F0CF}, {0x1F18E, 0x1F18E}, {0x1F191, 0x1F19A}, {0x1F200, 0x1F202},
  {0x1F210, 0x1F23B}, {0x1F240, 0x1F248}, {0x1F250, 0x1F251}, {0x1F260, 0x1F265},
  {0x1F300, 0x1F320}, {0x1F32D, 0x1F335}, {0x1F337, 0x1F37C}, {0x1F37E, 0x1F393},
  {0x1F3A0, 0x1F3CA}, {0x1F3CF, 0x1F3D3}, {0x1F3E0, 0x1F3F0}, {0x1F3F4, 0x1F3F4},
  {0x1F3F8, 0x1F43E}, {0x1F440, 0x1F440}, {0x1F442, 0x1F4FC}, {0x1F4FF, 0x1F53D},
  {0x1F54B, 0x1F54E}, {0x1F550, 0x1F567}, {0x1F57A, 0x1F57A}, {0x1F595, 0x1F596},
  {0x1F5A4, 0x1F5A4}, {0x1F5FB, 0x1F64F}, {0x1F680, 0x1F6C5}, {0x1F6CC, 0x1F6CC},
  {0x1F6D0, 0x1F6D2}, {0x1F6D5, 0x1F6D7}, {0x1F6DC, 0x1F6DF}, {0x1F6EB, 0x1F6EC},
  {0x1F6F4, 0x1F6FC}, {0x1F7E0, 0x1F7EB}, {0x1F7F0, 0x1F7F0}, {0x1F90C, 0x1F93A},
  {0x1F93C, 0x1F945}, {0x1F947, 0x1F9FF}, {0x1FA70, 0x1FA7C}, {0x1FA80, 0x1FA89},
  {0x1FA8F, 0x1FAC6}, {0x1FACE, 0x1FADC}, {0x1FADF, 0x1FAE9}, {0x1FAF0, 0x1FAF8},
  {0x20000, 0x2FFFD}, {0x30000, 0x3FFFD},
};

static int itermInTable(unsigned int cp, const void* table_v, int count)
{
  const struct { unsigned int first, last; } *table = table_v;
  int lo = 0, hi = count - 1;
  if (cp < table[0].first || cp > table[hi].last)
    return 0;
  while (lo <= hi)
  {
    int mid = (lo + hi) / 2;
    if (cp < table[mid].first) hi = mid - 1;
    else if (cp > table[mid].last) lo = mid + 1;
    else return 1;
  }
  return 0;
}

int iupTermCharWidth(unsigned int cp)
{
  if (cp < 0x20)
    return 0;
  if (cp < 0x300)
    return 1;
  if (itermInTable(cp, iterm_zerow, sizeof(iterm_zerow)/sizeof(iterm_zerow[0])))
    return 0;
  if (cp >= 0x1100 && itermInTable(cp, iterm_wide, sizeof(iterm_wide)/sizeof(iterm_wide[0])))
    return 2;
  return 1;
}
